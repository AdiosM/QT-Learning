#include <QApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QIcon>
#include <QLibraryInfo>
#include <QLocale>
#include <QTimer>
#include <QTranslator>

#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("Graturate"));
    QCoreApplication::setApplicationName(QStringLiteral("PdfTranslator"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/app.ico")));

    // 加载 Qt 自带的中文翻译，让 QMessageBox 等标准对话框按钮显示"确定/取消"
    QTranslator qtTranslator;
    if (qtTranslator.load(QLocale(QLocale::Chinese, QLocale::China),
                          QStringLiteral("qtbase"), QStringLiteral("_"),
                          QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        app.installTranslator(&qtTranslator);
    }

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("PDF 翻译阅读器"));
    parser.addHelpOption();
    parser.addVersionOption();
    // 冒烟测试：启动 2.5 秒后把状态写入 smoke.log 再退出
    parser.addOption(QCommandLineOption(QStringLiteral("smoke"),
                                        QStringLiteral("启动后自动退出（冒烟测试）")));
    parser.addOption(QCommandLineOption(
        QStringLiteral("smoke-ms"), QStringLiteral("冒烟测试等待毫秒数（默认 2500）"),
        QStringLiteral("ms"), QStringLiteral("2500")));
    parser.addOption(QCommandLineOption(QStringLiteral("open"),
                                        QStringLiteral("启动时打开的 PDF 文件"),
                                        QStringLiteral("file")));
    parser.addOption(QCommandLineOption(QStringLiteral("auto-translate"),
                                        QStringLiteral("冒烟测试：打开文档后自动翻译当前页")));
    parser.addOption(QCommandLineOption(QStringLiteral("auto-translate-all"),
                                        QStringLiteral("冒烟测试：打开文档后自动翻译全文")));
    parser.process(app);

    MainWindow w;
    if (parser.isSet(QStringLiteral("open")))
        w.openPdf(parser.value(QStringLiteral("open")));
    if (parser.isSet(QStringLiteral("auto-translate")))
        w.autoTranslateForSmoke(false);
    if (parser.isSet(QStringLiteral("auto-translate-all")))
        w.autoTranslateForSmoke(true);
    w.show();

    if (parser.isSet(QStringLiteral("smoke"))) {
        QTimer::singleShot(parser.value(QStringLiteral("smoke-ms")).toInt(), &app, [&w, &app] {
            QFile log(QStringLiteral("smoke.log"));
            if (log.open(QIODevice::WriteOnly | QIODevice::Append))
                log.write((w.smokeStatus() + u'\n').toUtf8());
            app.quit();
        });
    }

    return app.exec();
}
