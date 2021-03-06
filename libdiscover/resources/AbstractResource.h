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

#ifndef ABSTRACTRESOURCE_H
#define ABSTRACTRESOURCE_H

#include <QtCore/QObject>
#include <QUrl>
#include <QStringList>
#include <QScopedPointer>
#include <QVector>
#include <QCollatorSortKey>

#include "discovercommon_export.h"
#include "PackageState.h"

class Category;
class Rating;
class AbstractResourcesBackend;

/**
 * \class AbstractResource  AbstractResource.h "AbstractResource.h"
 *
 * \brief This is the base class of all resources.
 *
 * Each backend must reimplement its own resource class which needs to derive from this one.
 */
class DISCOVERCOMMON_EXPORT AbstractResource : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString packageName READ packageName CONSTANT)
    Q_PROPERTY(QString comment READ comment CONSTANT)
    Q_PROPERTY(QVariant icon READ icon NOTIFY iconChanged)
    Q_PROPERTY(bool canExecute READ canExecute CONSTANT)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)
    Q_PROPERTY(QStringList category READ categories CONSTANT)
    Q_PROPERTY(bool isTechnical READ isTechnical CONSTANT)
    Q_PROPERTY(QUrl homepage READ homepage CONSTANT)
    Q_PROPERTY(QUrl thumbnailUrl READ thumbnailUrl CONSTANT)
    Q_PROPERTY(QUrl screenshotUrl READ screenshotUrl CONSTANT)
    Q_PROPERTY(bool canUpgrade READ canUpgrade NOTIFY stateChanged)
    Q_PROPERTY(bool isInstalled READ isInstalled NOTIFY stateChanged)
    Q_PROPERTY(QString license READ license CONSTANT)
    Q_PROPERTY(QString longDescription READ longDescription CONSTANT)
    Q_PROPERTY(QString origin READ origin CONSTANT)
    Q_PROPERTY(int size READ size NOTIFY sizeChanged)
    Q_PROPERTY(QString sizeDescription READ sizeDescription NOTIFY sizeChanged)
    Q_PROPERTY(QString installedVersion READ installedVersion NOTIFY stateChanged)
    Q_PROPERTY(QString availableVersion READ availableVersion NOTIFY stateChanged)
    Q_PROPERTY(QString section READ section CONSTANT)
    Q_PROPERTY(QStringList mimetypes READ mimetypes CONSTANT)
    Q_PROPERTY(AbstractResourcesBackend* backend READ backend CONSTANT)
    Q_PROPERTY(Rating* rating READ rating NOTIFY ratingFetched)
    Q_PROPERTY(QString appstreamId READ appstreamId CONSTANT)
    Q_PROPERTY(QString categoryDisplay READ categoryDisplay CONSTANT)
    public:
        /**
         * This describes the state of the resource
         */
        enum State {
            /**
             * When the resource is somehow broken
             */
            Broken,
            /**
             * This means that the resource is neither installed nor broken
             */
            None,
            /**
             * The resource is installed and up-to-date
             */
            Installed,
            /**
             * The resource is installed and an update is available
             */
            Upgradeable
        };
        Q_ENUM(State)

        /**
         * Constructs the AbstractResource with its corresponding backend
         */
        explicit AbstractResource(AbstractResourcesBackend* parent);

        ///used as internal identification of a resource
        virtual QString packageName() const = 0;

        ///resource name to be displayed
        virtual QString name() = 0;

        ///short description of the resource
        virtual QString comment() = 0;

        ///xdg-compatible icon name to represent the resource, url or QIcon
        virtual QVariant icon() const = 0;

        ///@returns whether invokeApplication makes something
        /// false if not overridden
        virtual bool canExecute() const;

        ///executes the resource, if applies.
        Q_SCRIPTABLE virtual void invokeApplication() const;

        virtual State state() = 0;

        virtual QStringList categories() = 0;
        ///@returns a URL that points to the content
        virtual QUrl homepage() = 0;

        virtual bool isTechnical() const;

        virtual QUrl thumbnailUrl() = 0;
        virtual QUrl screenshotUrl() = 0;

        virtual int size() = 0;
        virtual QString sizeDescription();
        virtual QString license() = 0;

        virtual QString installedVersion() const = 0;
        virtual QString availableVersion() const = 0;
        virtual QString longDescription() = 0;

        virtual QString origin() const = 0;
        virtual QString section() = 0;

        ///@returns what kind of mime types the resource can consume
        virtual QStringList mimetypes() const;

        virtual QList<PackageState> addonsInformation() = 0;
        bool isFromSecureOrigin() const;

        virtual QStringList executables() const;

        virtual QStringList extends() const;

        virtual QString appstreamId() const;

        void addMetadata(const QString &key, const QVariant &value);
        QVariant getMetadata(const QString &key);

        bool canUpgrade();
        bool isInstalled();

        ///@returns a user-readable explaination of the resource status
        ///by default, it will specify what state() is returning
        virtual QString status();

        AbstractResourcesBackend* backend() const;

        /**
         * @returns a name sort key for faster sorting
         */
        QCollatorSortKey nameSortKey();

        /**
         * Convenience method to fetch the resource's rating
         *
         * @returns the rating for the resource or null if not available
         */
        Rating* rating() const;

        /**
         * @returns a string defining the categories the resource belongs to
         */
        QString categoryDisplay() const;

        bool categoryMatches(Category* cat);

        QSet<Category*> categoryObjects() const;

    public Q_SLOTS:
        virtual void fetchScreenshots();
        virtual void fetchChangelog() = 0;

    Q_SIGNALS:
        void iconChanged();
        void sizeChanged();
        void stateChanged();
        void ratingFetched();

        ///response to the fetchScreenshots method
        ///@p thumbnails and @p screenshots should have the same number of elements
        void screenshotsFetched(const QList<QUrl>& thumbnails, const QList<QUrl>& screenshots);
        void changelogFetched(const QString& changelog);

    private:
        void reportNewState();

//         TODO: make it std::optional or make QCollatorSortKey()
        QScopedPointer<QCollatorSortKey> m_collatorKey;
        QVariantMap m_metadata;
};

Q_DECLARE_METATYPE(QVector<AbstractResource*>)

#endif // ABSTRACTRESOURCE_H
