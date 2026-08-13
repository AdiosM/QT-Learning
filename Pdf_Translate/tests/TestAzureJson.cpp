#include "TestAzureJson.h"

#include <QtTest>

#include "AzureJson.h"

void TestAzureJson::parse_success()
{
    const QByteArray json =
        R"([{"detectedLanguage":{"language":"en","score":1.0},
             "translations":[{"text":"你好","to":"zh-Hans"}]},
            {"translations":[{"text":"世界","to":"zh-Hans"}]}])";
    QStringList out;
    QString error;
    QVERIFY(AzureJson::parseTranslations(json, 2, &out, &error));
    QCOMPARE(out.size(), 2);
    QCOMPARE(out.at(0), QStringLiteral("你好"));
    QCOMPARE(out.at(1), QStringLiteral("世界"));
    QVERIFY(error.isEmpty());
}

void TestAzureJson::parse_mismatchedCount()
{
    const QByteArray json = R"([{"translations":[{"text":"only one"}]}])";
    QStringList out;
    QString error;
    QVERIFY(!AzureJson::parseTranslations(json, 2, &out, &error));
    QVERIFY(error.contains(QStringLiteral("数量")));
}

void TestAzureJson::parse_malformed()
{
    QStringList out;
    QString error;
    QVERIFY(!AzureJson::parseTranslations(QByteArray("not json"), 1, &out, &error));
    QVERIFY(!error.isEmpty());
}

void TestAzureJson::parse_errorMessageExtraction()
{
    const QByteArray json =
        R"({"error":{"code":401000,
            "message":"Access denied due to invalid subscription key"}})";
    QCOMPARE(AzureJson::extractErrorMessage(json),
             QStringLiteral("Access denied due to invalid subscription key"));
}
