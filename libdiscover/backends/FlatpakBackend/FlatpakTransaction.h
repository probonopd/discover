/***************************************************************************
 *   Copyright © 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *   Copyright © 2017 Jan Grulich <jgrulich@redhat.com>                    *
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

#ifndef FLATPAKTRANSACTION_H
#define FLATPAKTRANSACTION_H

#include <Transaction/Transaction.h>
#include <QPointer>

extern "C" {
#include <flatpak.h>
#include <gio/gio.h>
#include <glib.h>
}

class FlatpakResource;
class FlatpakTransactionJob;
class FlatpakTransaction : public Transaction
{
Q_OBJECT
public:
    FlatpakTransaction(FlatpakInstallation *installation, FlatpakResource *app, Role role, bool delayStart = false);
    FlatpakTransaction(FlatpakInstallation *installation, FlatpakResource *app, FlatpakResource *runtime, Role role, bool delayStart = false);
    // FIXME ignore addons, they are not used in flatpak world (yet)

    ~FlatpakTransaction();

    void cancel() override;
    void setRuntime(FlatpakResource *runtime);

public Q_SLOTS:
    void onAppJobFinished(bool success);
    void onAppJobProgressChanged(int progress);
    void onRuntimeJobFinished(bool success);
    void onRuntimeJobProgressChanged(int progress);
    void finishTransaction(bool success);
    void start();

private:
    void updateProgress();

    bool m_appJobFinished;
    bool m_runtimeJobFinished;
    int m_appJobProgress;
    int m_runtimeJobProgress;
    QPointer<FlatpakResource> m_app;
    QPointer<FlatpakResource> m_runtime;
    FlatpakInstallation *m_installation;
    QPointer<FlatpakTransactionJob> m_appJob;
    QPointer<FlatpakTransactionJob> m_runtimeJob;
};

#endif // FLATPAKTRANSACTION_H
