/***************************************************************************
 *   Copyright ?? 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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

#ifndef ABSTRACTRESOURCESBACKEND_H
#define ABSTRACTRESOURCESBACKEND_H

#include <QtCore/QObject>
#include <QtCore/QPair>
#include <QtCore/QVector>

#include "AbstractResource.h"
#include "Transaction/AddonList.h"

#include "discovercommon_export.h"

class QAction;
class Transaction;
class Category;
class AbstractReviewsBackend;
class AbstractBackendUpdater;
class KActionCollection;

class DISCOVERCOMMON_EXPORT ResultsStream : public QObject
{
    Q_OBJECT
    public:
        ResultsStream(const QString &objectName);

        /// assumes all the information is in @p resources
        ResultsStream(const QString &objectName, const QVector<AbstractResource*>& resources);
        ~ResultsStream() override;

    Q_SIGNALS:
        void resourcesFound(const QVector<AbstractResource*>& resources);
};

/**
 * \class AbstractResourcesBackend  AbstractResourcesBackend.h "AbstractResourcesBackend.h"
 *
 * \brief This is the base class of all resource backends.
 * 
 * For writing basic new resource backends, we need to implement two classes: this and the 
 * AbstractResource one. Basic questions on how to build your plugin with those classes
 * can be answered by looking at the dummy plugin.
 * 
 * As this is the base class of a backend, we save all the created resources here and also
 * accept calls to install and remove applications or to cancel transactions.
 * 
 * To show resources in Muon, we need to initialize all resources we want to show beforehand,
 * we should not create resources in the search function. When we reload the resources
 * (e.g. when initializing), the backend needs change the fetching property throughout the
 * processs.
 */
class DISCOVERCOMMON_EXPORT AbstractResourcesBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(AbstractReviewsBackend* reviewsBackend READ reviewsBackend CONSTANT)
    Q_PROPERTY(int updatesCount READ updatesCount NOTIFY updatesCountChanged)
    Q_PROPERTY(bool isFetching READ isFetching NOTIFY fetchingChanged)
    public:
        /**
         * Constructs an AbstractResourcesBackend
         * @param parent the parent of the class (the object will be deleted when the parent gets deleted)
         */
        explicit AbstractResourcesBackend(QObject* parent = nullptr);
        
        /**
         * @returns true when the backend is in a valid state, which means it is able to work
         * You must return true here if you want the backend to be loaded.
         */
        virtual bool isValid() const = 0;
        
        struct Filters {
            Category* category = nullptr;
            AbstractResource::State state = AbstractResource::Broken;
            QString mimetype;
            QString search;
            QString extends;
            QHash<QByteArray, QVariant> roles;

            bool shouldFilter(AbstractResource* res) const;
            void filterJustInCase(QVector<AbstractResource*>& input) const;
        };

        /**
         * @returns a stream that will provide elements that match the search
         */

        virtual ResultsStream* search(const Filters &search) = 0;//FIXME: Probably provide a standard implementation?!

        virtual ResultsStream* findResourceByPackageName(const QUrl &search) = 0;//FIXME: Probably provide a standard implementation?!
        
        /**
         * @returns the reviews backend of this AbstractResourcesBackend (which handles all ratings and reviews of resources)
         */
        virtual AbstractReviewsBackend* reviewsBackend() const = 0;//FIXME: Have a standard impl which returns 0?
        
        /**
         * @returns the class which is used by muon to update the users system, if you are unsure what to do
         * just return the StandardBackendUpdater
         */
        virtual AbstractBackendUpdater* backendUpdater() const = 0;//FIXME: Standard impl returning the standard updater?
        
        /**
         * @returns the number of resources for which an update is available, it should only count technical packages
         */
        virtual int updatesCount() const = 0;//FIXME: Probably provide a standard implementation?!

        /**
         * This method gets called while initializing the GUI, in case the backend needs to
         * integrate actions in the action collection.
         * @param w the KActionCollection the backend should integrate to
         */
        virtual void integrateActions(KActionCollection* w);

        /**
         * Tells whether the backend is fetching resources
         */
        virtual bool isFetching() const = 0;

        /**
         *  This method is used to integrate advanced functions into the Muon GUI.
         *
         *  In plasma-discover-updater, actions with HighPriority will be shown in a KMessageWidget,
         *  normal priority will go right on top of the more menu, low priority will go
         *  to the advanced menu.
         */
        virtual QList<QAction*> messageActions() const = 0;

        /**
         * @returns the appstream ids that this backend extends
         */
        virtual QStringList extends() const;

        /** @returns the plugin's name */
        QString name() const;

        /** @internal only to be used by the factory */
        void setName(const QString& name);

        /**
         * emits a change for all rating properties
         */
        void emitRatingsReady();

        virtual AbstractResource* resourceForFile(const QUrl &/*url*/) { return nullptr; }

        /**
         * @returns the root category tree
         */
        virtual QVector<Category*> category() const { return {}; }

    public Q_SLOTS:
        /**
         * This gets called when the backend should install an application.
         * The AbstractResourcesBackend should create a Transaction object, which
         * will provide Muon with information like the status and progress of a transaction,
         * and add it to the TransactionModel with the following line:
         * \code
         * TransactionModel::global()->addTransaction(transaction);
         * \endcode
         * where transaction is the newly created Transaction.
         * @param app the application to be installed
         * @param addons the addons which should be installed with the application
         */
        virtual void installApplication(AbstractResource *app, const AddonList& addons) = 0;
        
        /**
         * Overloaded function, which simply does the same, except not installing any addons.
         */
        virtual void installApplication(AbstractResource *app);
        
        /**
         * This gets called when the backend should remove an application.
         * Like in the installApplication() method, the AbstractResourcesBackend should
         * create a Transaction object and add it to the TransactionModel.
         * @see installApplication
         * @param app the application to be removed
         */
        virtual void removeApplication(AbstractResource *app) = 0;

    Q_SIGNALS:
        /**
         * Notify of a change in the backend
         */
        void fetchingChanged();

        /**
         * This should be emitted when the number of upgradeable packages changed.
         */
        void updatesCountChanged();
        /**
         * This should be emitted when all data of the backends resources changed. Internally it will emit
         * a signal in the model to show the view that all data of a certain backend changed.
         */
        void allDataChanged(const QVector<QByteArray> &propertyNames);

        /**
         * Allows to notify some @p properties in @p resource have changed
         */
        void resourcesChanged(AbstractResource* resource, const QVector<QByteArray> &properties);
        void resourceRemoved(AbstractResource* resource);

        void passiveMessage(const QString &message);

    private:
        QString m_name;
};

template <typename T, typename W>
static T containerValues(const W& container)
{
    T ret;
    ret.reserve(container.size());
    for(auto a : container) {
        ret.push_back(a);
    }
    return ret;
}

DISCOVERCOMMON_EXPORT QDebug operator<<(QDebug dbg, const AbstractResourcesBackend::Filters& filters);

/**
 * @internal Workaround because QPluginLoader enforces 1 instance per plugin
 */
class DISCOVERCOMMON_EXPORT AbstractResourcesBackendFactory : public QObject
{
    Q_OBJECT
public:
    virtual QVector<AbstractResourcesBackend*> newInstance(QObject* parent, const QString &name) const = 0;
};

#define MUON_BACKEND_PLUGIN(ClassName)\
    class ClassName##Factory : public AbstractResourcesBackendFactory {\
        Q_OBJECT\
        Q_PLUGIN_METADATA(IID "org.kde.muon.AbstractResourcesBackendFactory")\
        Q_INTERFACES(AbstractResourcesBackendFactory)\
        public:\
            QVector<AbstractResourcesBackend*> newInstance(QObject* parent, const QString &name) const override {\
                auto c = new ClassName(parent);\
                c->setName(name);\
                return {c};\
            }\
    };

Q_DECLARE_INTERFACE( AbstractResourcesBackendFactory, "org.kde.muon.AbstractResourcesBackendFactory" )

#endif // ABSTRACTRESOURCESBACKEND_H
