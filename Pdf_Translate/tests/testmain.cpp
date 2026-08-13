// 单元测试统一入口：逐个执行各测试类
#include "TestAzureJson.h"
#include "TestDeepSeekJson.h"
#include "TestParagraphSplitter.h"
#include "TestTranslationCache.h"

#include <QCoreApplication>
#include <QtTest>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    int status = 0;
    status |= QTest::qExec(new TestParagraphSplitter, argc, argv);
    status |= QTest::qExec(new TestAzureJson, argc, argv);
    status |= QTest::qExec(new TestDeepSeekJson, argc, argv);
    status |= QTest::qExec(new TestTranslationCache, argc, argv);
    return status;
}
