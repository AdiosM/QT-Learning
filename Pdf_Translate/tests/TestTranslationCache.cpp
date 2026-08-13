#include "TestTranslationCache.h"

#include <QtTest>

#include <QTemporaryDir>

#include "TranslationCache.h"

namespace {
const QString kDocKey = QStringLiteral("doc123");
const QString kLangPair = QStringLiteral("->zh-Hans");
} // namespace

void TestTranslationCache::roundtrip()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    TranslationCache cache(dir.path() + QStringLiteral("/cache.db"));
    QVERIFY(cache.init(nullptr));

    const QStringList sources = {QStringLiteral("hello world"),
                                 QStringLiteral("second paragraph")};
    const QStringList translations = {QStringLiteral("你好世界"), QStringLiteral("第二段")};
    QVERIFY(cache.store(kDocKey, kLangPair, 0, sources, translations));

    const QList<CacheQuery> queries = {{0, sources.at(0)}, {0, sources.at(1)}};
    const QStringList found = cache.lookup(kDocKey, kLangPair, queries);
    QCOMPARE(found.size(), 2);
    QCOMPARE(found.at(0), translations.at(0));
    QCOMPARE(found.at(1), translations.at(1));
}

void TestTranslationCache::missReturnsEmpty()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    TranslationCache cache(dir.path() + QStringLiteral("/cache.db"));
    QVERIFY(cache.init(nullptr));

    const QList<CacheQuery> queries = {{0, QStringLiteral("unknown text")}};
    const QStringList found = cache.lookup(kDocKey, kLangPair, queries);
    QCOMPARE(found.size(), 1);
    QVERIFY(found.at(0).isEmpty());
}

void TestTranslationCache::partialHit()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    TranslationCache cache(dir.path() + QStringLiteral("/cache.db"));
    QVERIFY(cache.init(nullptr));

    QVERIFY(cache.store(kDocKey, kLangPair, 0, {QStringLiteral("stored text")},
                        {QStringLiteral("已存译文")}));

    // 同页不同段：一段命中一段未命中；不同页也不命中
    const QList<CacheQuery> queries = {
        {0, QStringLiteral("stored text")},
        {0, QStringLiteral("other text")},
        {1, QStringLiteral("stored text")}};
    const QStringList found = cache.lookup(kDocKey, kLangPair, queries);
    QCOMPARE(found.size(), 3);
    QCOMPARE(found.at(0), QStringLiteral("已存译文"));
    QVERIFY(found.at(1).isEmpty());
    QVERIFY(found.at(2).isEmpty());
}

void TestTranslationCache::clearRemovesAll()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    TranslationCache cache(dir.path() + QStringLiteral("/cache.db"));
    QVERIFY(cache.init(nullptr));

    QVERIFY(cache.store(kDocKey, kLangPair, 0, {QStringLiteral("text")},
                        {QStringLiteral("译文")}));
    QVERIFY(cache.clear());
    const QStringList found = cache.lookup(
        kDocKey, kLangPair, {{0, QStringLiteral("text")}});
    QVERIFY(found.at(0).isEmpty());
}

void TestTranslationCache::docKeyVariesWithFile()
{
    // 路径/大小/修改时间任一不同 => 不同键
    const QDateTime t1 = QDateTime::fromMSecsSinceEpoch(1000000);
    const QDateTime t2 = QDateTime::fromMSecsSinceEpoch(2000000);
    const QString k1 = TranslationCache::docKey(QStringLiteral("a.pdf"), 100, t1);
    const QString k2 = TranslationCache::docKey(QStringLiteral("b.pdf"), 100, t1);
    const QString k3 = TranslationCache::docKey(QStringLiteral("a.pdf"), 200, t1);
    const QString k4 = TranslationCache::docKey(QStringLiteral("a.pdf"), 100, t2);
    const QString k5 = TranslationCache::docKey(QStringLiteral("a.pdf"), 100, t1);
    QVERIFY(k1 != k2);
    QVERIFY(k1 != k3);
    QVERIFY(k1 != k4);
    QCOMPARE(k1, k5);   // 相同输入 => 相同键（确定性）
}
