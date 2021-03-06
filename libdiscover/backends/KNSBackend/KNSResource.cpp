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

#include "KNSResource.h"
#include "KNSBackend.h"
#include <KNSCore/Engine>
#include <QRegularExpression>
#include <knewstuff_version.h>

KNSResource::KNSResource(const KNSCore::EntryInternal& entry, QStringList categories, KNSBackend* parent)
    : AbstractResource(parent)
    , m_categories(std::move(categories))
    , m_entry(entry)
{
    connect(this, &KNSResource::stateChanged, parent, &KNSBackend::updatesCountChanged);
}

KNSResource::~KNSResource() = default;

AbstractResource::State KNSResource::state()
{
    switch(m_entry.status()) {
        case KNS3::Entry::Invalid:
            return Broken;
        case KNS3::Entry::Downloadable:
            return None;
        case KNS3::Entry::Installed:
            return Installed;
        case KNS3::Entry::Updateable:
            return Upgradeable;
        case KNS3::Entry::Deleted:
        case KNS3::Entry::Installing:
        case KNS3::Entry::Updating:
            return None;
    }
    return None;
}

KNSBackend * KNSResource::knsBackend() const
{
    return qobject_cast<KNSBackend*>(parent());
}

QVariant KNSResource::icon() const
{
    return knsBackend()->iconName();
}

QString KNSResource::comment()
{
    QString ret = m_entry.shortSummary();
    if(ret.isEmpty()) {
        ret = m_entry.summary();
        int newLine = ret.indexOf(QLatin1Char('\n'));
        if(newLine>0) {
            ret=ret.left(newLine);
        }
        ret = ret.replace(QRegularExpression(QStringLiteral("\\[/?[a-z]*\\]")), QString());
    }
    return ret;
}

QString KNSResource::longDescription()
{
    QString ret = m_entry.summary();
    if (m_entry.shortSummary().isEmpty()) {
        const int newLine = ret.indexOf(QLatin1Char('\n'));
        if (newLine<0)
            ret.clear();
        else
            ret = ret.mid(newLine+1).trimmed();
    }
    ret = ret.replace(QLatin1Char('\r'), QString());
    ret = ret.replace(QStringLiteral("[li]"), QStringLiteral("\n* "));
    ret = ret.replace(QRegularExpression(QStringLiteral("\\[/?[a-z]*\\]")), QString());
    return ret;
}

QString KNSResource::name()
{
    return m_entry.name();
}

QString KNSResource::packageName() const
{
    return m_entry.uniqueId();
}

QStringList KNSResource::categories()
{
    return m_categories;
}

QUrl KNSResource::homepage()
{
    return m_entry.homepage();
}

QUrl KNSResource::thumbnailUrl()
{
    return QUrl(m_entry.previewUrl(KNSCore::EntryInternal::PreviewSmall1));
}

QUrl KNSResource::screenshotUrl()
{
    return QUrl(m_entry.previewUrl(KNSCore::EntryInternal::PreviewBig1));

}
void KNSResource::setEntry(const KNSCore::EntryInternal& entry)
{
    const bool diff = entry.status() != m_entry.status();
    m_entry = entry;
    if (diff)
        Q_EMIT stateChanged();
}

KNSCore::EntryInternal KNSResource::entry() const
{
    return m_entry;
}

QString KNSResource::license()
{
    return m_entry.license();
}

int KNSResource::size()
{
    const auto downloadInfo = m_entry.downloadLinkInformationList();
    return downloadInfo.isEmpty() ? 0 : downloadInfo.at(0).size;
}

QString KNSResource::installedVersion() const
{
    return m_entry.version();
}

QString KNSResource::availableVersion() const
{
    return !m_entry.updateVersion().isEmpty() ? m_entry.updateVersion() : m_entry.version();
}

QString KNSResource::origin() const
{
    return m_entry.providerId();
}

QString KNSResource::section()
{
    return m_entry.category();
}

static void appendIfValid(QList<QUrl>& list, const QUrl &value)
{
    if (value.isValid() && !value.isEmpty())
        list << value;
}

void KNSResource::fetchScreenshots()
{
    QList<QUrl> preview;
    appendIfValid(preview, QUrl(m_entry.previewUrl(KNSCore::EntryInternal::PreviewSmall1)));
    appendIfValid(preview, QUrl(m_entry.previewUrl(KNSCore::EntryInternal::PreviewSmall2)));
    appendIfValid(preview, QUrl(m_entry.previewUrl(KNSCore::EntryInternal::PreviewSmall3)));

    QList<QUrl> screenshots;
    appendIfValid(screenshots, QUrl(m_entry.previewUrl(KNSCore::EntryInternal::PreviewBig1)));
    appendIfValid(screenshots, QUrl(m_entry.previewUrl(KNSCore::EntryInternal::PreviewBig2)));
    appendIfValid(screenshots, QUrl(m_entry.previewUrl(KNSCore::EntryInternal::PreviewBig3)));

    emit screenshotsFetched(preview, screenshots);
}

void KNSResource::fetchChangelog()
{
    emit changelogFetched(m_entry.changelog());
}

QStringList KNSResource::extends() const
{
    return knsBackend()->extends();
}

QStringList KNSResource::executables() const
{
    if (knsBackend()->engine()->hasAdoptionCommand())
        return {knsBackend()->engine()->adoptionCommand(m_entry)};
    else
        return {};
}
