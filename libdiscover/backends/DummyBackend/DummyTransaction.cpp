/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#include "DummyTransaction.h"
#include "DummyBackend.h"
#include "DummyResource.h"
#include <Transaction/TransactionModel.h>
#include <QTimer>
#include <QDebug>
#include <KRandom>

DummyTransaction::DummyTransaction(DummyResource* app, Role role)
    : DummyTransaction(app, {}, role)
{
}

DummyTransaction::DummyTransaction(DummyResource* app, const AddonList& addons, Transaction::Role role)
    : Transaction(app->backend(), app, role, addons)
    , m_app(app)
{
    setCancellable(true);
    iterateTransaction();
}

void DummyTransaction::iterateTransaction()
{
    if (!m_iterate)
        return;

    setStatus(CommittingStatus);
    if(progress()<100) {
        setProgress(qBound(0, progress()+(KRandom::random()%30), 100));
        QTimer::singleShot(/*KRandom::random()%*/100, this, &DummyTransaction::iterateTransaction);
    } else
        finishTransaction();
}

void DummyTransaction::cancel()
{
    m_iterate = false;
    TransactionModel::global()->cancelTransaction(this);
}

void DummyTransaction::finishTransaction()
{
    setStatus(DoneStatus);
    AbstractResource::State newState;
    switch(role()) {
    case InstallRole:
    case ChangeAddonsRole:
        newState = AbstractResource::Installed;
        break;
    case RemoveRole:
        newState = AbstractResource::None;
        break;
    }
    m_app->setState(newState);
    m_app->setAddons(addons());
    TransactionModel::global()->removeTransaction(this);
    deleteLater();
}
