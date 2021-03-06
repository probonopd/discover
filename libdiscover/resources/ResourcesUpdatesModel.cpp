/***************************************************************************
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "ResourcesUpdatesModel.h"
#include <Transaction/Transaction.h>
#include <Transaction/TransactionModel.h>
#include "ResourcesModel.h"
#include "AbstractBackendUpdater.h"
#include "AbstractResource.h"
#include <QDateTime>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDebug>

#include <KLocalizedString>
#include <KFormat>

ResourcesUpdatesModel::ResourcesUpdatesModel(QObject* parent)
    : QStandardItemModel(parent)
    , m_lastIsProgressing(false)
    , m_transaction(nullptr)
{
    connect(ResourcesModel::global(), &ResourcesModel::backendsChanged, this, &ResourcesUpdatesModel::init);

    init();
}

void ResourcesUpdatesModel::init()
{
    const QVector<AbstractResourcesBackend*> backends = ResourcesModel::global()->backends();
    m_lastIsProgressing = false;
    foreach(AbstractResourcesBackend* b, backends) {
        AbstractBackendUpdater* updater = b->backendUpdater();
        if(updater && !m_updaters.contains(updater)) {
            connect(updater, &AbstractBackendUpdater::progressingChanged, this, &ResourcesUpdatesModel::slotProgressingChanged);
            connect(updater, &AbstractBackendUpdater::progressChanged, this, &ResourcesUpdatesModel::progressChanged);
            connect(updater, &AbstractBackendUpdater::statusMessageChanged, this, &ResourcesUpdatesModel::statusMessageChanged);
            connect(updater, &AbstractBackendUpdater::statusMessageChanged, this, &ResourcesUpdatesModel::message);
            connect(updater, &AbstractBackendUpdater::statusDetailChanged, this, &ResourcesUpdatesModel::message);
            connect(updater, &AbstractBackendUpdater::statusDetailChanged, this, &ResourcesUpdatesModel::statusDetailChanged);
            connect(updater, &AbstractBackendUpdater::remainingTimeChanged, this, &ResourcesUpdatesModel::etaChanged);
            connect(updater, &AbstractBackendUpdater::downloadSpeedChanged, this, &ResourcesUpdatesModel::downloadSpeedChanged);
            connect(updater, &AbstractBackendUpdater::cancelableChanged, this, &ResourcesUpdatesModel::cancelableChanged);
            connect(updater, &AbstractBackendUpdater::resourceProgressed, this, &ResourcesUpdatesModel::resourceProgressed);
            connect(updater, &AbstractBackendUpdater::destroyed, this, &ResourcesUpdatesModel::updaterDestroyed);
            m_updaters += updater;

            m_lastIsProgressing |= updater->isProgressing();
        }
    }
}

void ResourcesUpdatesModel::updaterDestroyed(QObject* obj)
{
    m_updaters.removeAll(static_cast<AbstractBackendUpdater*>(obj));
}

void ResourcesUpdatesModel::slotProgressingChanged()
{
    const bool newProgressing = isProgressing();
    if (newProgressing != m_lastIsProgressing) {
        m_lastIsProgressing = newProgressing;


        if (!newProgressing && m_transaction) {
            TransactionModel::global()->removeTransaction(m_transaction);
            m_transaction->deleteLater();
        }

        emit progressingChanged(newProgressing);

        if (!newProgressing) {
            Q_EMIT finished();
        }
    }
}

qreal ResourcesUpdatesModel::progress() const
{
    if (m_updaters.isEmpty())
        return -1;

    qreal total = 0;
    foreach(AbstractBackendUpdater* updater, m_updaters) {
        total += updater->progress();
    }
    return total / m_updaters.count();
}

void ResourcesUpdatesModel::message(const QString& msg)
{
    if(msg.isEmpty())
        return;

    appendRow(new QStandardItem(msg));
}

void ResourcesUpdatesModel::prepare()
{
    if(isProgressing()) {
        qWarning() << "trying to set up a running instance";
        return;
    }
    foreach(AbstractBackendUpdater* upd, m_updaters) {
        upd->prepare();
    }
}

class UpdateTransaction : public Transaction
{
public:
    UpdateTransaction(ResourcesUpdatesModel* parent)
        : Transaction(parent, nullptr, Transaction::InstallRole)
        , m_updater(parent)
    {
        setCancellable(m_updater->isCancelable());
        connect(m_updater, &ResourcesUpdatesModel::cancelableChanged, this, [this]() {
            setCancellable(m_updater->isCancelable());
        });

        foreach(auto updater, parent->updaters()) {
            connect(updater, &AbstractBackendUpdater::passiveMessage, this, &Transaction::passiveMessage);
            connect(updater, &AbstractBackendUpdater::proceedRequest, this, &UpdateTransaction::processProceedRequest);
        }
    }

    void processProceedRequest(const QString &title, const QString& message) {
        m_updatersWaitingForFeedback += qobject_cast<AbstractBackendUpdater*>(sender());
        Q_EMIT proceedRequest(title, message);
    }

    void cancel() override {
        foreach(auto updater, m_updatersWaitingForFeedback) {
            updater->cancel();
        }
        m_updater->cancel();
    }

    void proceed() override {
        m_updatersWaitingForFeedback.takeFirst()->proceed();
    }

private:
    ResourcesUpdatesModel* m_updater;
    QVector<AbstractBackendUpdater*> m_updatersWaitingForFeedback;
};

void ResourcesUpdatesModel::updateAll()
{
    if(m_updaters.isEmpty())
        slotProgressingChanged();
    else {
        delete m_transaction;
        m_transaction = new UpdateTransaction(this);
        TransactionModel::global()->addTransaction(m_transaction);
        Q_FOREACH (AbstractBackendUpdater* upd, m_updaters) {
            if (upd->hasUpdates())
                QMetaObject::invokeMethod(upd, "start", Qt::QueuedConnection);
        }
    }
}


QString ResourcesUpdatesModel::remainingTime() const
{
    long unsigned int maxEta = 0;
    foreach(AbstractBackendUpdater* upd, m_updaters) {
        maxEta = qMax(maxEta, upd->remainingTime());
    }

    // Ignore ETA if it's larger than 2 days.
    if(maxEta > 2 * 24 * 60 * 60)
        return QString();
    else if(maxEta==0)
        return i18nc("@item:intext Unknown remaining time", "Updating...");
    else
        return i18nc("@item:intext Remaining time", "%1 remaining", KFormat().formatDuration(maxEta));
}

quint64 ResourcesUpdatesModel::downloadSpeed() const
{
    quint64 ret = 0;
    foreach(AbstractBackendUpdater* upd, m_updaters) {
        ret += upd->downloadSpeed();
    }
    return ret;
}

bool ResourcesUpdatesModel::isCancelable() const
{
    bool cancelable = false;
    foreach(AbstractBackendUpdater* upd, m_updaters) {
        cancelable |= upd->isCancelable();
    }
    return cancelable;
}

bool ResourcesUpdatesModel::isProgressing() const
{
    if (m_updaters.isEmpty())
        return true;

    bool progressing = false;
    foreach(AbstractBackendUpdater* upd, m_updaters) {
        progressing |= upd->isProgressing();
    }
    return progressing;
}

QList<AbstractResource*> ResourcesUpdatesModel::toUpdate() const
{
    QList<AbstractResource*> ret;
    foreach(AbstractBackendUpdater* upd, m_updaters) {
        ret += upd->toUpdate();
    }
    return ret;
}

void ResourcesUpdatesModel::addResources(const QList<AbstractResource*>& resources)
{
    QHash<AbstractResourcesBackend*, QList<AbstractResource*> > sortedResources;
    foreach(AbstractResource* res, resources) {
        sortedResources[res->backend()] += res;
    }

    for(auto it=sortedResources.constBegin(), itEnd=sortedResources.constEnd(); it!=itEnd; ++it) {
        it.key()->backendUpdater()->addResources(*it);
    }
}

void ResourcesUpdatesModel::removeResources(const QList< AbstractResource* >& resources)
{
    QHash<AbstractResourcesBackend*, QList<AbstractResource*> > sortedResources;
    foreach(AbstractResource* res, resources) {
        sortedResources[res->backend()] += res;
    }

    for(auto it=sortedResources.constBegin(), itEnd=sortedResources.constEnd(); it!=itEnd; ++it) {
        it.key()->backendUpdater()->removeResources(*it);
    }
}

QDateTime ResourcesUpdatesModel::lastUpdate() const
{
    QDateTime ret;
    foreach(AbstractBackendUpdater* upd, m_updaters) {
        QDateTime current = upd->lastUpdate();
        if(!ret.isValid() || (current.isValid() && current>ret)) {
            ret = current;
        }
    }
    return ret;
}

void ResourcesUpdatesModel::cancel()
{
    foreach(AbstractBackendUpdater* upd, m_updaters) {
        if(upd->isCancelable())
            upd->cancel();
        else
            qWarning() << "tried to cancel " << upd->metaObject()->className() << "which is not cancelable";
    }
}

qint64 ResourcesUpdatesModel::secsToLastUpdate() const
{
    return lastUpdate().secsTo(QDateTime::currentDateTime());
}
