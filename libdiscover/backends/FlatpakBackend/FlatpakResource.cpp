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

#include "FlatpakResource.h"
#include "FlatpakBackend.h"

#include <Transaction/AddonList.h>

#include <AppStreamQt/icon.h>
#include <AppStreamQt/image.h>
#include <AppStreamQt/screenshot.h>

#include <KFormat>
#include <KLocalizedString>

#include <QDir>
#include <QDebug>
#include <QDesktopServices>
#include <QIcon>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringList>
#include <QTimer>

FlatpakResource::FlatpakResource(AppStream::Component *component, FlatpakInstallation* installation, FlatpakBackend *parent)
    : AbstractResource(parent)
    , m_appdata(component)
    , m_downloadSize(0)
    , m_installedSize(0)
    , m_propertyStates({{DownloadSize, NotKnownYet}, {InstalledSize, NotKnownYet},{RequiredRuntime, NotKnownYet}})
    , m_installation(installation)
    , m_state(AbstractResource::None)
    , m_type(FlatpakResource::DesktopApp)
{
    // Start fetching remote icons during initialization
    const auto icons = m_appdata->icons();
    if (!icons.isEmpty()) {
        foreach (const AppStream::Icon &icon, icons) {
            if (icon.kind() == AppStream::Icon::KindRemote) {
                const QString fileName = QStringLiteral("%1/icons/%2").arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation))
                                                                      .arg(icon.url().fileName());
                if (!QFileInfo::exists(fileName)) {
                    const QDir cacheDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
                    // Create $HOME/.cache/discover/icons folder
                    cacheDir.mkdir(QStringLiteral("icons"));
                    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
                    connect(manager, &QNetworkAccessManager::finished, [this, icon, fileName, manager] (QNetworkReply *reply) {
                        if (reply->error() == QNetworkReply::NoError) {
                            QByteArray iconData = reply->readAll();
                            QFile file(fileName);
                            if (file.open(QIODevice::WriteOnly)) {
                                file.write(iconData);
                            }
                            file.close();
                            Q_EMIT iconChanged();
                        }
                        manager->deleteLater();
                    });
                    manager->get(QNetworkRequest(icon.url()));
                }
            }
        }
    }
}

AppStream::Component *FlatpakResource::appstreamComponent() const
{
    return m_appdata;
}

QList<PackageState> FlatpakResource::addonsInformation()
{
    return m_addons;
}

QString FlatpakResource::availableVersion() const
{
    // TODO check if there is actually version available
    QString version = branch();
    if (version.isEmpty()) {
        version = i18n("Unknown");
    }

    return version;
}

QString FlatpakResource::appstreamId() const
{
    return m_appdata->id();
}

QString FlatpakResource::arch() const
{
    return m_arch;
}

QString FlatpakResource::branch() const
{
    return m_branch;
}

bool FlatpakResource::canExecute() const
{
    return (m_type == DesktopApp && (m_state == AbstractResource::Installed || m_state == AbstractResource::Upgradeable));
}

void FlatpakResource::updateFromRef(FlatpakRef* ref)
{
    setArch(QString::fromUtf8(flatpak_ref_get_arch(ref)));
    setBranch(QString::fromUtf8(flatpak_ref_get_branch(ref)));
    setCommit(QString::fromUtf8(flatpak_ref_get_commit(ref)));
    setFlatpakName(QString::fromUtf8(flatpak_ref_get_name(ref)));
    setType(flatpak_ref_get_kind(ref) == FLATPAK_REF_KIND_APP ? FlatpakResource::DesktopApp : FlatpakResource::Runtime);
}

QStringList FlatpakResource::categories()
{
    auto cats = m_appdata->categories();
    if (m_appdata->kind() != AppStream::Component::KindAddon)
        cats.append(QStringLiteral("Application"));
    return cats;
}

QString FlatpakResource::comment()
{
    const auto summary = m_appdata->summary();
    if (!summary.isEmpty()) {
        return summary;
    }

    return QString();
}

QString FlatpakResource::commit() const
{
    return m_commit;
}

int FlatpakResource::downloadSize() const
{
    return m_downloadSize;
}

QStringList FlatpakResource::executables() const
{
//     return m_appdata->provided(AppStream::Provided::KindBinary).items();
    return QStringList();
}

QVariant FlatpakResource::icon() const
{
    QIcon ret;
    const auto icons = m_appdata->icons();

    if (!m_bundledIcon.isNull()) {
        ret = QIcon(m_bundledIcon);
    } else if (icons.isEmpty()) {
        ret = QIcon::fromTheme(QStringLiteral("package-x-generic"));
    } else foreach(const AppStream::Icon &icon, icons) {
        QStringList stock;
        QString url = QString::fromUtf8("%1/icons/").arg(m_iconPath);

        switch (icon.kind()) {
            case AppStream::Icon::KindLocal:
            case AppStream::Icon::KindCached:
                url += icon.url().toLocalFile();
                if (QFileInfo::exists(url)) {
                    ret.addFile(url);
                } else {
                    ret = QIcon::fromTheme(QStringLiteral("package-x-generic"));
                }
                break;
            case AppStream::Icon::KindStock:
                stock += icon.name();
                break;
            case AppStream::Icon::KindRemote: {
                const QString fileName = QStringLiteral("%1/icons/%2").arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation))
                                                                .arg(icon.url().fileName());
                if (QFileInfo::exists(fileName)) {
                    ret.addFile(fileName);
                } else {
                    ret = QIcon::fromTheme(QStringLiteral("package-x-generic"));
                }
                break;
            }
            case AppStream::Icon::KindUnknown:
                break;
        }

        if (ret.isNull() && !stock.isEmpty()) {
            ret = QIcon::fromTheme(stock.first(), QIcon::fromTheme(QStringLiteral("package-x-generic")));
        }
    }
    return ret;
}

QString FlatpakResource::installedVersion() const
{
    // TODO check if there is actually version available
    QString version = branch();
    if (version.isEmpty()) {
        version = i18n("Unknown");
    }

    return version;
}

int FlatpakResource::installedSize() const
{
    return m_installedSize;
}

bool FlatpakResource::isTechnical() const
{
    return m_type == FlatpakResource::Runtime;
}

QUrl FlatpakResource::homepage()
{
    return m_appdata->url(AppStream::Component::UrlKindHomepage);
}

QString FlatpakResource::flatpakFileType() const
{
    return m_flatpakFileType;
}

QString FlatpakResource::flatpakName() const
{
    // If the flatpak name is not known (known only for installed apps), then use
    // appstream id instead;
    if (m_flatpakName.isEmpty()) {
        return m_appdata->id();
    }

    return m_flatpakName;
}

QString FlatpakResource::license()
{
    return m_appdata->projectLicense();
}

QString FlatpakResource::longDescription()
{
    return m_appdata->description();
}

QString FlatpakResource::name()
{
    QString name = m_appdata->name();
    if (name.isEmpty()) {
        name = m_appdata->id();
    }

    if (name.startsWith(QLatin1String("(Nightly) "))) {
        return name.mid(10);
    }

    return name;
}

QString FlatpakResource::origin() const
{
    return m_origin;
}

QString FlatpakResource::packageName() const
{
    return flatpakName();
}

FlatpakResource::PropertyState FlatpakResource::propertyState(FlatpakResource::PropertyKind kind) const
{
    return m_propertyStates[kind];
}

QUrl FlatpakResource::resourceFile() const
{
    return m_resourceFile;
}

QString FlatpakResource::runtime() const
{
    return m_runtime;
}

static QUrl imageOfKind(const QList<AppStream::Image> &images, AppStream::Image::Kind kind)
{
    QUrl ret;
    Q_FOREACH (const AppStream::Image &i, images) {
        if (i.kind() == kind) {
            ret = i.url();
            break;
        }
    }
    return ret;
}

static QUrl screenshot(AppStream::Component *comp, AppStream::Image::Kind kind)
{
    QUrl ret;
    Q_FOREACH (const AppStream::Screenshot &s, comp->screenshots()) {
        ret = imageOfKind(s.images(), kind);
        if (s.isDefault() && !ret.isEmpty())
            break;
    }
    return ret;
}

QUrl FlatpakResource::screenshotUrl()
{
    return screenshot(m_appdata, AppStream::Image::KindSource);
}

QString FlatpakResource::section()
{
    return QString();
}

int FlatpakResource::size()
{
    if (m_state == Installed) {
        return m_installedSize;
    } else {
        return m_downloadSize;
    }
}

QString FlatpakResource::sizeDescription()
{
    KFormat f;
    if (!isInstalled() || canUpgrade()) {
        if (propertyState(DownloadSize) == NotKnownYet || propertyState(InstalledSize) == NotKnownYet) {
            return i18n("Retrieving size information");
        } else if (propertyState(DownloadSize) == UnknownOrFailed || propertyState(InstalledSize) == UnknownOrFailed) {
            return i18n("Unknown size");
        } else {
            return i18nc("@info app size", "%1 to download, %2 on disk", f.formatByteSize(downloadSize()), f.formatByteSize(installedSize()));
        }
    } else {
        if (propertyState(InstalledSize) == NotKnownYet) {
            return i18n("Retrieving size information");
        } else if (propertyState(InstalledSize) == UnknownOrFailed) {
            return i18n("Unknown size");
        } else {
            return i18nc("@info app size", "%1 on disk", f.formatByteSize(installedSize()));
        }
    }
}

AbstractResource::State FlatpakResource::state()
{
    return m_state;
}

QUrl FlatpakResource::thumbnailUrl()
{
    return screenshot(m_appdata, AppStream::Image::KindThumbnail);
}

FlatpakResource::ResourceType FlatpakResource::type() const
{
    return m_type;
}

QString FlatpakResource::typeAsString() const
{
    switch (m_type) {
        case FlatpakResource::DesktopApp:
        case FlatpakResource::Source:
            return QLatin1String("app");
        case FlatpakResource::Runtime:
            return QLatin1String("runtime");
        default:
            return QLatin1String("app");
    }
}

QString FlatpakResource::uniqueId() const
{
    // Build uniqueId
    return QString::fromUtf8("%1/%2/%3/%4/%5/%6").arg(installationPath())
                                                 .arg(QLatin1String("flatpak"))
                                                 .arg(origin())
                                                 .arg(typeAsString())
                                                 .arg(m_appdata->id())
                                                 .arg(branch());
}

void FlatpakResource::invokeApplication() const
{
    g_autoptr(GCancellable) cancellable = g_cancellable_new();
    g_autoptr(GError) localError = nullptr;

    if (!flatpak_installation_launch(m_installation,
                                     flatpakName().toStdString().c_str(),
                                     arch().toStdString().c_str(),
                                     branch().toStdString().c_str(),
                                     nullptr,
                                     cancellable,
                                     &localError)) {
        qWarning() << "Failed to launch " << m_appdata->name() << ": " << localError->message;
    }
}

void FlatpakResource::fetchChangelog()
{
    QString log = longDescription();
    log.replace(QLatin1Char('\n'), QLatin1String("<br />"));

    emit changelogFetched(log);
}

void FlatpakResource::fetchScreenshots()
{
    QList<QUrl> thumbnails, screenshots;

    Q_FOREACH (const AppStream::Screenshot &s, m_appdata->screenshots()) {
        const QUrl thumbnail = imageOfKind(s.images(), AppStream::Image::KindThumbnail);
        const QUrl plain = imageOfKind(s.images(), AppStream::Image::KindSource);
        if (plain.isEmpty())
            qWarning() << "invalid screenshot for" << name();

        screenshots << plain;
        thumbnails << (thumbnail.isEmpty() ? plain : thumbnail);
    }

    Q_EMIT screenshotsFetched(thumbnails, screenshots);
}

void FlatpakResource::setArch(const QString &arch)
{
    m_arch = arch;
}

void FlatpakResource::setBranch(const QString &branch)
{
    m_branch = branch;
}

void FlatpakResource::setBundledIcon(const QPixmap &pixmap)
{
    m_bundledIcon = pixmap;
}

void FlatpakResource::setCommit(const QString &commit)
{
    m_commit = commit;
}

void FlatpakResource::setDownloadSize(int size)
{
    m_downloadSize = size;

    setPropertyState(DownloadSize, AlreadyKnown);

    Q_EMIT sizeChanged();
}

void FlatpakResource::setFlatpakFileType(const QString &fileType)
{
    m_flatpakFileType = fileType;
}

void FlatpakResource::setFlatpakName(const QString &name)
{
    m_flatpakName = name;
}

void FlatpakResource::setIconPath(const QString &path)
{
    m_iconPath = path;
}

void FlatpakResource::setInstalledSize(int size)
{
    m_installedSize = size;

    setPropertyState(InstalledSize, AlreadyKnown);

    Q_EMIT sizeChanged();
}

void FlatpakResource::setOrigin(const QString &origin)
{
    m_origin = origin;
}

void FlatpakResource::setPropertyState(FlatpakResource::PropertyKind kind, FlatpakResource::PropertyState state)
{
    m_propertyStates[kind] = state;

    Q_EMIT propertyStateChanged(kind, state);
}

void FlatpakResource::setResourceFile(const QUrl &url)
{
    m_resourceFile = url;
}

void FlatpakResource::setRuntime(const QString &runtime)
{
    m_runtime = runtime;

    setPropertyState(RequiredRuntime, AlreadyKnown);
}

void FlatpakResource::setState(AbstractResource::State state)
{
    if (m_state != state) {
        m_state = state;

        Q_EMIT stateChanged();
    }
}

void FlatpakResource::setType(FlatpakResource::ResourceType type)
{
    m_type = type;
}

QString FlatpakResource::installationPath() const
{
    return installationPath(m_installation);
}

QString FlatpakResource::installationPath(FlatpakInstallation* flatpakInstallation)
{
    g_autoptr(GFile) path = flatpak_installation_get_path(flatpakInstallation);
    return QString::fromUtf8(g_file_get_path(path));
}
