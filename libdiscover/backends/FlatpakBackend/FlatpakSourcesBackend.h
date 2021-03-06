/***************************************************************************
 *   Copyright © 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#ifndef FLATPAKSOURCESBACKEND_H
#define FLATPAKSOURCESBACKEND_H

#include <resources/AbstractSourcesBackend.h>
#include <QStandardItemModel>

extern "C" {
#include <flatpak.h>
}

class FlatpakResource;
class FlatpakSourcesBackend : public AbstractSourcesBackend
{
public:
    explicit FlatpakSourcesBackend(const QVector<FlatpakInstallation *>& installations, QObject *parent);

    QAbstractItemModel* sources() override;
    bool addSource(const QString &id) override;
    bool removeSource(const QString &id) override;
    QString name() const override { return QStringLiteral("Flatpak"); }
    QString idDescription() override { return QStringLiteral("Flatpak remote repositories"); }
    QList<QAction*> actions() const override;

    FlatpakRemote * installSource(FlatpakResource *resource);
private:
    bool listRepositories(FlatpakInstallation *installation);

    FlatpakInstallation *m_preferredInstallation;
    QStandardItemModel* m_sources;
};

#endif // FLATPAKSOURCESBACKEND_H
