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

#include "KNSBackendTest.h"
#include "utils.h"
#include <KNSBackend.h>
#include <KXmlGuiWindow>
#include <resources/AbstractBackendUpdater.h>
#include <resources/AbstractResource.h>
#include <resources/ResourcesModel.h>
#include <ReviewsBackend/AbstractReviewsBackend.h>
#include <ReviewsBackend/Review.h>
#include <ReviewsBackend/Rating.h>
#include <Category/Category.h>
#include <Category/CategoryModel.h>
#include <DiscoverBackendsFactory.h>
#include <QStandardPaths>

#include <qtest.h>
#include <qsignalspy.h>

QTEST_MAIN( KNSBackendTest )

KNSBackendTest::KNSBackendTest(QObject* parent)
    : QObject(parent)
    , m_r(nullptr)
{
    QStandardPaths::setTestModeEnabled(true);
    ResourcesModel* model = new ResourcesModel(QLatin1String("kns-backend"), this);
    Q_ASSERT(!model->backends().isEmpty());
    auto findTestBackend = [](AbstractResourcesBackend* backend) {
        return backend->name() == QLatin1String("discover_ktexteditor_codesnippets_core.knsrc");
    };
    m_backend = kFilter<QVector<AbstractResourcesBackend*>>(model->backends(), findTestBackend).at(0);

    if (!m_backend->isValid()) {
        qWarning() << "couldn't run the test";
        exit(0);
    }

    connect(m_backend->reviewsBackend(), &AbstractReviewsBackend::reviewsReady, this, &KNSBackendTest::reviewsArrived);
}

QVector<AbstractResource*> KNSBackendTest::getResources(ResultsStream* stream)
{
    Q_ASSERT(stream);
    Q_ASSERT(stream->objectName() != QLatin1String("KNS-void"));
    QSignalSpy spyResources(stream, &ResultsStream::destroyed);
    QVector<AbstractResource*> resources;
    connect(stream, &ResultsStream::resourcesFound, this, [&resources](const QVector<AbstractResource*>& res) { resources += res; });
    Q_ASSERT(spyResources.wait());
    Q_ASSERT(!resources.isEmpty());
    return resources;
}

QVector<AbstractResource*> KNSBackendTest::getAllResources(AbstractResourcesBackend* backend)
{
    AbstractResourcesBackend::Filters f;
    if (CategoryModel::global()->rootCategories().isEmpty())
        CategoryModel::global()->populateCategories();
    f.category = CategoryModel::global()->rootCategories().constFirst();
    return getResources(backend->search(f));
}

void KNSBackendTest::testRetrieval()
{
    QVERIFY(m_backend->backendUpdater());
    QCOMPARE(m_backend->updatesCount(), m_backend->backendUpdater()->toUpdate().count());

    const auto resources = getAllResources(m_backend);
    foreach(AbstractResource* res, resources) {
        QVERIFY(!res->name().isEmpty());
        QVERIFY(!res->categories().isEmpty());
        QVERIFY(!res->origin().isEmpty());
        QVERIFY(!res->icon().isNull());
//         QVERIFY(!res->comment().isEmpty());
//         QVERIFY(!res->longDescription().isEmpty());
//         QVERIFY(!res->license().isEmpty());
        QVERIFY(res->homepage().isValid() && !res->homepage().isEmpty());
        QVERIFY(res->state() > AbstractResource::Broken);
        QVERIFY(res->addonsInformation().isEmpty());
        QVERIFY(res->thumbnailUrl().isValid() || res->thumbnailUrl().isEmpty());
        QVERIFY(res->screenshotUrl().isValid() || res->screenshotUrl().isEmpty());

        QSignalSpy spy(res, &AbstractResource::screenshotsFetched);
        res->fetchScreenshots();
        QVERIFY(spy.count() || spy.wait());

        QSignalSpy spy1(res, &AbstractResource::changelogFetched);
        res->fetchChangelog();
        QVERIFY(spy1.count() || spy1.wait());
    }
}

void KNSBackendTest::testReviews()
{
    const QVector<AbstractResource*> resources = getAllResources(m_backend);
    AbstractReviewsBackend* rev = m_backend->reviewsBackend();
    QVERIFY(!rev->hasCredentials());
    foreach(AbstractResource* res, resources) {
        Rating* r = rev->ratingForApplication(res);
        QVERIFY(r);
        QCOMPARE(r->packageName(), res->packageName());
        QVERIFY(r->rating()>0 && r->rating()<=10);
    }

    auto res = resources.first();
    QSignalSpy spy(rev, &AbstractReviewsBackend::reviewsReady);
    rev->fetchReviews(res);
    QVERIFY(spy.count() || spy.wait());
}

void KNSBackendTest::reviewsArrived(AbstractResource* r, const QVector<ReviewPtr>& revs)
{
    m_r = r;
    m_revs = revs;
}

void KNSBackendTest::testResourceByUrl()
{
    const auto res = getResources(m_backend->findResourceByPackageName(QUrl(QStringLiteral("kns://") + m_backend->name() + QStringLiteral("/api.kde-look.org/1136471"))));
    QCOMPARE(res.count(), 1);
}
