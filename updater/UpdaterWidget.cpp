/***************************************************************************
 *   Copyright © 2011 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#include "UpdaterWidget.h"

// Qt includes
#include <QApplication>
#include <QStandardItemModel>
#include <QtCore/QDir>
#include <QtGui/QHeaderView>
#include <QtGui/QTreeView>
#include <QtGui/QVBoxLayout>

// KDE includes
#include <KIcon>
#include <KLocale>
#include <KDebug>

// LibQApt includes
#include <LibQApt/Backend>

// Own includes
#include "../installer/Application.h"
#include "UpdateModel/UpdateModel.h"
#include "UpdateModel/UpdateItem.h"
#include "UpdateModel/UpdateDelegate.h"

UpdaterWidget::UpdaterWidget(QWidget *parent) :
    QWidget(parent)
{
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

    m_updateModel = new UpdateModel(this);

    connect(m_updateModel, SIGNAL(checkApps(QList<Application*>,bool)),
            this, SLOT(checkApps(QList<Application*>,bool)));
    connect(m_updateModel, SIGNAL(checkApp(Application*,bool)),
            this, SLOT(checkApp(Application*,bool)));

    m_updateView = new QTreeView(this);
    m_updateView->setAlternatingRowColors(true);
    m_updateView->header()->setResizeMode(0, QHeaderView::Stretch);
    m_updateView->setModel(m_updateModel);

    UpdateDelegate *delegate = new UpdateDelegate(m_updateView);
    m_updateView->setItemDelegate(delegate);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setMargin(0);

    mainLayout->addWidget(m_updateView);

    setLayout(mainLayout);
}

void UpdaterWidget::setBackend(QApt::Backend *backend)
{
    m_backend = backend;
    connect(m_backend, SIGNAL(packageChanged()),
            m_updateModel, SLOT(packageChanged()));

    populateUpdateModel();
}

void UpdaterWidget::reload()
{
    m_updateModel->clear();
    m_backend->reloadCache();

    populateUpdateModel();
}

void UpdaterWidget::populateUpdateModel()
{
    QApt::PackageList upgradeList = m_backend->upgradeablePackages();

    UpdateItem *securityItem = new UpdateItem(i18nc("@item:inlistbox", "Security Updates"),
                                              KIcon("security-medium"));

    UpdateItem *appItem = new UpdateItem(i18nc("@item:inlistbox", "Application Updates"),
                                          KIcon("applications-other"));

    UpdateItem *systemItem = new UpdateItem(i18nc("@item:inlistbox", "System Updates)",
                                             KIcon("applications-system"));

    QDir appDir("/usr/share/app-install/desktop/");
    QStringList fileList = appDir.entryList(QDir::Files);

    foreach(const QString &fileName, fileList) {
        Application *app = new Application("/usr/share/app-install/desktop/" + fileName, m_backend);
        QApt::Package *package = app->package();
        if (!package || !upgradeList.contains(package)) {
            continue;
        }
        int state = package->state();

        if (!(state & QApt::Package::Upgradeable)) {
            continue;
        }

        UpdateItem *updateItem = new UpdateItem(app);

        // Set update type
        if (package->component().contains(QLatin1String("security"))) {
            securityItem->appendChild(updateItem);
        } else {
            appItem->appendChild(updateItem);
        }

        m_upgradeableApps.append(app);

        upgradeList.removeAll(package);
    }

    // Remaining packages in the upgrade list aren't applications
    foreach (QApt::Package *package, upgradeList) {
        Application *app = new Application(package, m_backend);
        UpdateItem *updateItem = new UpdateItem(app);

        // Set update type
        if (package->component().contains(QLatin1String("security"))) {
            securityItem->appendChild(updateItem);
        } else {
            systemItem->appendChild(updateItem);
        }

        m_upgradeableApps.append(app);
    }

    if (securityItem->childCount()) {
        m_updateModel->addItem(securityItem);
    } else {
        delete securityItem;
    }

    if (appItem->childCount()) {
        m_updateModel->addItem(appItem);
    } else {
        delete appItem;
    }

    if (systemItem->childCount()) {
        m_updateModel->addItem(systemItem);
    } else {
        delete systemItem;
    }

    m_updateView->resizeColumnToContents(0);
    m_updateView->header()->setResizeMode(0, QHeaderView::Stretch);
    m_backend->saveCacheState();
    m_backend->markPackagesForDistUpgrade();
}

void UpdaterWidget::checkApp(Application *app, bool checked)
{
    m_backend->saveCacheState();
    checked ? app->package()->setInstall() : app->package()->setKeep();
}

void UpdaterWidget::checkApps(QList<Application *> apps, bool checked)
{
    QApt::PackageList list;
    foreach (Application *app, apps) {
        list << app->package();
    }

    QApt::Package::State action;
    if (checked) {
        action = QApt::Package::ToInstall;
    } else {
        action = QApt::Package::ToKeep;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_backend->saveCacheState();
    m_backend->markPackages(list, action);
    QApplication::restoreOverrideCursor();
}
