/*
 * Copyright (c) 2019 boud <boud@valdyas.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#include "TestTagFilterResourceProxyModel.h"

#include <QTest>
#include <QStandardPaths>
#include <QDir>
#include <QVersionNumber>
#include <QDirIterator>
#include <QSqlError>
#include <QSqlQuery>

#include <kconfig.h>
#include <kconfiggroup.h>
#include <ksharedconfig.h>

#include <KisResourceCacheDb.h>
#include <KisResourceLocator.h>
#include <KisResourceLoaderRegistry.h>
#include <KisResourceModel.h>
#include <KisTagFilterResourceProxyModel.h>
#include <KisResourceModelProvider.h>

#include <DummyResource.h>
#include <ResourceTestHelper.h>

#ifndef FILES_DATA_DIR
#error "FILES_DATA_DIR not set. A directory with the data used for testing installing resources"
#endif

#ifndef FILES_DEST_DIR
#error "FILES_DEST_DIR not set. A directory where data will be written to for testing installing resources"
#endif


void TestTagFilterResourceProxyModel::initTestCase()
{
    ResourceTestHelper::initTestDb();
    ResourceTestHelper::createDummyLoaderRegistry();

    m_srcLocation = QString(FILES_DATA_DIR);
    QVERIFY2(QDir(m_srcLocation).exists(), m_srcLocation.toUtf8());

    m_dstLocation = QString(FILES_DEST_DIR);
    ResourceTestHelper::cleanDstLocation(m_dstLocation);

    KConfigGroup cfg(KSharedConfig::openConfig(), "");
    cfg.writeEntry(KisResourceLocator::resourceLocationKey, m_dstLocation);

    m_locator = KisResourceLocator::instance();

    if (!KisResourceCacheDb::initialize(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))) {
        qDebug() << "Could not initialize KisResourceCacheDb on" << QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    }
    QVERIFY(KisResourceCacheDb::isValid());

    KisResourceLocator::LocatorError r = m_locator->initialize(m_srcLocation);
    if (!m_locator->errorMessages().isEmpty()) qDebug() << m_locator->errorMessages();

    QVERIFY(r == KisResourceLocator::LocatorError::Ok);
    QVERIFY(QDir(m_dstLocation).exists());
}


void TestTagFilterResourceProxyModel::testRowCount()
{
    QSqlQuery q;
    QVERIFY(q.prepare("SELECT count(*)\n"
                      "FROM   resources\n"
                      ",      resource_types\n"
                      "WHERE  resources.resource_type_id = resource_types.id\n"
                      "AND    resource_types.name = :resource_type"));
    q.bindValue(":resource_type", resourceType);
    QVERIFY(q.exec());
    q.first();
    int rowCount = q.value(0).toInt();
    QVERIFY(rowCount == 3);

    KisTagFilterResourceProxyModel proxyModel(resourceType);


    QCOMPARE(proxyModel.rowCount(), rowCount);
}

void TestTagFilterResourceProxyModel::testData()
{
    KisResourceModel *resourceModel = KisResourceModelProvider::resourceModel(resourceType);
    KisTagFilterResourceProxyModel proxyModel(resourceType);

    QStringList names = QStringList() << "test0.kpp"
                                      << "test1.kpp"
                                      << "test2.kpp";
    for (int i = 0; i < proxyModel.rowCount(); ++i)  {
        QVariant v = resourceModel->data(proxyModel.mapToSource(proxyModel.index(i, 0)), Qt::UserRole + KisAbstractResourceModel::Name);
        QVERIFY(names.contains(v.toString()));
    }
}


void TestTagFilterResourceProxyModel::testResource()
{
    KisResourceModel *resourceModel = KisResourceModelProvider::resourceModel(resourceType);
    KisTagFilterResourceProxyModel proxyModel(resourceType);

    KoResourceSP resource = resourceModel->resourceForIndex(proxyModel.mapToSource(proxyModel.index(0, 0)));
    QVERIFY(resource);
}

void TestTagFilterResourceProxyModel::testFilterByTag()
{
    KisResourceModel *resourceModel = KisResourceModelProvider::resourceModel(ResourceType::PaintOpPresets);
    KisTagModel *tagModel = KisResourceModelProvider::tagModel(ResourceType::PaintOpPresets);
    KisTagFilterResourceProxyModel proxyModel(resourceType);

    KoResourceSP resource = resourceModel->resourceForName("test2.kpp");
    QVERIFY(resource);

    KisTagSP tag = tagModel->tagForIndex(tagModel->index(2, 0));
    QVERIFY(tag);

    proxyModel.setTagFilter(tag);
    int rowCount = proxyModel.rowCount();

    proxyModel.tagResource(tag, resource);
    QCOMPARE(proxyModel.rowCount(), rowCount + 1);

    proxyModel.untagResource(tag, resource);
    QCOMPARE(proxyModel.rowCount(), rowCount);
}

void TestTagFilterResourceProxyModel::testFilterByResource()
{
    KisResourceModel *resourceModel = KisResourceModelProvider::resourceModel(ResourceType::PaintOpPresets);
    KisTagModel *tagModel = KisResourceModelProvider::tagModel(ResourceType::PaintOpPresets);

    KisTagFilterResourceProxyModel proxyModel(resourceType);

    KoResourceSP resource = resourceModel->resourceForName("test2.kpp");
    QVERIFY(resource);

    tagModel->addNewTag("testtag1", QVector<KoResourceSP>() << resource);
    tagModel->addNewTag("testtag2", QVector<KoResourceSP>() << resource);

    int rowCount = proxyModel.rowCount();

    proxyModel.setResourceFilter(resource);
    QCOMPARE(proxyModel.rowCount(), 2);

    proxyModel.setResourceFilter(0);
    QCOMPARE(proxyModel.rowCount(), rowCount);

}

void TestTagFilterResourceProxyModel::testFilterByString()
{
    KisResourceModel *resourceModel = KisResourceModelProvider::resourceModel(ResourceType::PaintOpPresets);
    KisTagModel *tagModel = KisResourceModelProvider::tagModel(ResourceType::PaintOpPresets);

    KisTagFilterResourceProxyModel proxyModel(resourceType);
    proxyModel.setSearchText("test2");
    QCOMPARE(proxyModel.rowCount(), 1);

    KoResourceSP resource = resourceModel->resourceForName("test2.kpp");
    QVERIFY(resource);

    KisTagSP tag = tagModel->tagForIndex(tagModel->index(2, 0));
    QVERIFY(tag);

    proxyModel.tagResource(tag, resource);
    proxyModel.setTagFilter(tag);
    proxyModel.setFilterInCurrentTag(true);

    QCOMPARE(proxyModel.rowCount(), 1);
}

void TestTagFilterResourceProxyModel::testDataWhenSwitchingBetweenTagAllAllUntagged()
{
    KisResourceModel *resourceModel = KisResourceModelProvider::resourceModel(ResourceType::PaintOpPresets);
    KisTagModel *tagModel = KisResourceModelProvider::tagModel(ResourceType::PaintOpPresets);
    KisTagFilterResourceProxyModel proxyModel(resourceType);

    KoResourceSP resource = resourceModel->resourceForName("test2.kpp");
    QModelIndex idx = proxyModel.indexForResource(resource);

    QVERIFY(idx.isValid());

    QString name = proxyModel.data(idx, Qt::UserRole + KisAbstractResourceModel::Name).toString();
    QCOMPARE(name, "test2.kpp");

    QImage thumbnail = proxyModel.data(idx, Qt::UserRole + KisAbstractResourceModel::Thumbnail).value<QImage>();
    QVERIFY(!thumbnail.isNull());

    proxyModel.setSearchText("test2");
    idx = proxyModel.indexForResource(resource);


}


void TestTagFilterResourceProxyModel::cleanupTestCase()
{
    ResourceTestHelper::rmTestDb();
    ResourceTestHelper::cleanDstLocation(m_dstLocation);
}


QTEST_MAIN(TestTagFilterResourceProxyModel)

