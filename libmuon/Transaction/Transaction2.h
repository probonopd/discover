/***************************************************************************
 *   Copyright © 2012 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#ifndef TRANSACTION2_H
#define TRANSACTION2_H

// Qt includes
#include <QtCore/QObject>

// Own includes
#include "AddonList.h"

#include "libmuonprivate_export.h"

class AbstractResource;

enum TransactionStatus {
    /// Not queued, newly created
    SetupStatus = 0,
    /// Queued, but not yet run
    QueuedStatus,
    /// Transaction is in the downloading phase
    DownloadingStatus,
    /// Transaction is doing an installation/removal
    CommittingStatus
};

enum TransactionRole {
    InstallRole = 0,
    RemoveRole,
    ChangeAddonsRole
};

class Transaction2 : public QObject
{
    Q_OBJECT

    Q_PROPERTY(AbstractResource* resource READ resource CONSTANT)
    Q_PROPERTY(TransactionRole role READ role CONSTANT)
    Q_PROPERTY(TransactionStatus status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool isCancellable READ isCancellable NOTIFY cancellableChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)

public:
    Transaction2(QObject *parent, AbstractResource *resource,
                 TransactionRole role);
    Transaction2(QObject *parent, AbstractResource *resource,
                 TransactionRole role, AddonList addons);

    AbstractResource *resource() const;
    TransactionRole role() const;
    TransactionStatus status() const;
    AddonList addons() const;
    bool isCancellable() const;
    int progress() const;

private:
    AbstractResource *m_resource;
    TransactionRole m_role;
    TransactionStatus m_status;
    AddonList m_addons;
    bool m_isCancellable;
    int m_progress;

    void setStatus(TransactionStatus status);
    void setCancellable(bool isCancellable);
    void setProgress(int progress);

signals:
    void statusChanged(TransactionStatus status);
    void cancellableChanged(bool cancellable);
    void progressChanged(int progress);
};

#endif // TRANSACTION2_H
