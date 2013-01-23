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

#ifndef APPLICATIONUPDATES_H
#define APPLICATIONUPDATES_H

// Qt includes
#include <QtCore/QObject>
#include <QPointer>

// LibQApt includes
#include <LibQApt/Globals>

// Own includes
#include "resources/AbstractBackendUpdater.h"

namespace QApt {
    class Backend;
    class Transaction;
}

class ApplicationBackend;

class ApplicationUpdates : public AbstractBackendUpdater
{
    Q_OBJECT
public:
    explicit ApplicationUpdates(ApplicationBackend* parent);

    bool hasUpdates() const;
    qreal progress() const;
    void start();
    void setBackend(QApt::Backend* b);
    long unsigned int remainingTime() const;
    virtual void addResources(QList< AbstractResource* > apps);
    virtual void removeResources(QList< AbstractResource* > apps);
    virtual void cleanup();
    virtual void prepare();
    virtual QList<AbstractResource*> toUpdate() const;
    virtual bool isAllMarked() const;
    virtual QDateTime lastUpdate() const;

private:
    QApt::Backend* m_aptBackend;
    ApplicationBackend* m_appBackend;
    int m_lastRealProgress;
    long unsigned int m_eta;
    QApt::CacheState m_updatesCache;

private slots:
    void transactionStatusChanged(QApt::TransactionStatus status);
    void errorOccurred(QApt::ErrorCode error);
    void progressChanged(int progress);
    void etaChanged(quint64 eta);
    void installMessage(const QString& message);
    void setupTransaction(QApt::Transaction *trans);

signals:
    void errorSignal(QApt::ErrorCode error, const QString& details);
};

#endif // APPLICATIONUPDATES_H
