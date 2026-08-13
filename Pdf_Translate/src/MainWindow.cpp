#include "MainWindow.h"

#include "PdfViewWidget.h"
#include "TranslationManager.h"
#include "TranslationView.h"
#include "settings/AppSettings.h"
#include "settings/SettingsDialog.h"

#include <QAction>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("PDF 翻译阅读器"));
    resize(1200, 800);

    createActions();
    createToolBar();
    createStatusBar();

    m_pdfView = new PdfViewWidget(this);
    m_translateView = new TranslationView(this);

    // 左右分栏，等宽显示
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->addWidget(m_pdfView);
    m_splitter->addWidget(m_translateView);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setChildrenCollapsible(false);
    m_splitter->setSizes({600, 600});   // 初始左右一致
    setCentralWidget(m_splitter);

    m_manager = new TranslationManager(m_pdfView, m_translateView, this);

    connect(m_pdfView, &PdfViewWidget::pageChanged, this, [this](int page) {
        QSignalBlocker blocker(m_pageSpin);
        m_pageSpin->setValue(page + 1);
        updatePageInfo();
    });
    connect(m_pdfView, &PdfViewWidget::documentReady, this, [this](int count) {
        m_docReady = true;
        m_lastDocError.clear();
        m_manager->setDocument(m_currentPdfPath);
        setDocumentActionsEnabled(true);
        m_pageSpin->setRange(1, qMax(1, count));
        m_pageSpin->setEnabled(true);
        m_pageSpin->setValue(1);
        setWindowTitle(QStringLiteral("%1 - PDF 翻译阅读器")
                           .arg(QFileInfo(m_currentPdfPath).fileName()));
        updatePageInfo();
        statusBar()->showMessage(QStringLiteral("已打开：%1").arg(m_currentPdfPath),
                                 5000);
    });
    connect(m_pdfView, &PdfViewWidget::documentFailed, this, [this](const QString &msg) {
        m_docReady = false;
        m_lastDocError = msg;
        m_manager->stop();
        setDocumentActionsEnabled(false);
        m_pageSpin->setRange(1, 1);
        m_pageSpin->setEnabled(false);
        setWindowTitle(QStringLiteral("PDF 翻译阅读器"));
        m_pageInfoLabel->setText(QStringLiteral("未打开文件"));
        QMessageBox::warning(this, QStringLiteral("打开失败"), msg);
    });

    connect(m_manager, &TranslationManager::runFinished, this, [this](int ok, int failed) {
        m_runFinished = true;
        m_lastRunOk = ok;
        m_lastRunFailed = failed;
        setTranslatingUi(false);
        statusBar()->showMessage(
            failed == 0 ? QStringLiteral("翻译完成：成功 %1 段").arg(ok)
                        : QStringLiteral("翻译完成：成功 %1 段，失败 %2 段").arg(ok).arg(failed),
            8000);
    });
    connect(m_manager, &TranslationManager::progressChanged, this, [this](int done, int total) {
        m_progressBar->setMaximum(qMax(1, total));
        m_progressBar->setValue(done);
    });
    connect(m_manager, &TranslationManager::statusMessage, this, [this](const QString &text) {
        m_lastStatus = text;
        statusBar()->showMessage(text, 5000);
    });

    // 滚动同步（左 -> 右 / 右 -> 左），由工具栏开关控制
    connect(m_pdfView, &PdfViewWidget::pageChanged, this, [this](int page) {
        if (m_syncScrollAct->isChecked())
            m_translateView->jumpToPage(page);
    });
    connect(m_translateView, &TranslationView::pageRequested, this, [this](int page) {
        if (m_syncScrollAct->isChecked() && page >= 0)
            m_pdfView->goToPage(page);
    });

    // 恢复上次的窗口几何与状态
    const AppSettings &settings = AppSettings::instance();
    if (!settings.windowGeometry().isEmpty())
        restoreGeometry(settings.windowGeometry());
    if (!settings.windowState().isEmpty())
        restoreState(settings.windowState());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    AppSettings &settings = AppSettings::instance();
    settings.setWindowGeometry(saveGeometry());
    settings.setWindowState(saveState());
    settings.sync();
    QMainWindow::closeEvent(event);
}

void MainWindow::createActions()
{
    // 文件
    m_openAct = new QAction(QStringLiteral("打开PDF(&O)..."), this);
    m_openAct->setShortcut(QKeySequence::Open);
    connect(m_openAct, &QAction::triggered, this, &MainWindow::openFileDialog);

    // 导航 / 缩放
    m_prevPageAct = new QAction(QStringLiteral("上一页"), this);
    connect(m_prevPageAct, &QAction::triggered, this, [this] {
        m_pdfView->goToPage(m_pdfView->currentPage() - 1);
    });
    m_nextPageAct = new QAction(QStringLiteral("下一页"), this);
    connect(m_nextPageAct, &QAction::triggered, this, [this] {
        m_pdfView->goToPage(m_pdfView->currentPage() + 1);
    });
    m_zoomInAct = new QAction(QStringLiteral("放大"), this);
    connect(m_zoomInAct, &QAction::triggered, this, [this] { m_pdfView->zoomIn(); });
    m_zoomOutAct = new QAction(QStringLiteral("缩小"), this);
    connect(m_zoomOutAct, &QAction::triggered, this, [this] { m_pdfView->zoomOut(); });
    m_fitWidthAct = new QAction(QStringLiteral("适应宽度"), this);
    connect(m_fitWidthAct, &QAction::triggered, this, [this] { m_pdfView->fitWidth(); });

    // 翻译
    m_translatePageAct = new QAction(QStringLiteral("翻译本页"), this);
    connect(m_translatePageAct, &QAction::triggered, this, [this] {
        setTranslatingUi(true);
        m_manager->translateCurrentPage();
    });
    m_translateAllAct = new QAction(QStringLiteral("翻译全文"), this);
    connect(m_translateAllAct, &QAction::triggered, this, [this] {
        setTranslatingUi(true);
        m_manager->translateWholeDocument();
    });
    m_stopAct = new QAction(QStringLiteral("停止"), this);
    connect(m_stopAct, &QAction::triggered, this, [this] { m_manager->stop(); });
    m_syncScrollAct = new QAction(QStringLiteral("同步滚动"), this);     // M5 启用
    m_syncScrollAct->setCheckable(true);

    // 设置 / 关于
    m_settingsAct = new QAction(QStringLiteral("设置(&S)..."), this);
    connect(m_settingsAct, &QAction::triggered, this, [this] {
        SettingsDialog dialog(this);
        if (dialog.exec() == QDialog::Accepted) {
            m_manager->reloadSettings();
            statusBar()->showMessage(QStringLiteral("设置已保存"), 3000);
        }
    });
    QAction *aboutAct = new QAction(QStringLiteral("关于(&A)"), this);
    connect(aboutAct, &QAction::triggered, this, &MainWindow::showAbout);

    setDocumentActionsEnabled(false);

    // 菜单
    QMenu *fileMenu = menuBar()->addMenu(QStringLiteral("文件(&F)"));
    fileMenu->addAction(m_openAct);
    fileMenu->addSeparator();
    fileMenu->addAction(QStringLiteral("退出(&X)"), this, &QWidget::close);

    QMenu *helpMenu = menuBar()->addMenu(QStringLiteral("帮助(&H)"));
    helpMenu->addAction(aboutAct);
}

void MainWindow::createToolBar()
{
    QToolBar *bar = addToolBar(QStringLiteral("主工具栏"));
    bar->setMovable(false);
    bar->setToolButtonStyle(Qt::ToolButtonTextOnly);

    bar->addAction(m_openAct);
    bar->addSeparator();
    bar->addAction(m_prevPageAct);

    m_pageSpin = new QSpinBox(bar);
    m_pageSpin->setRange(1, 1);
    m_pageSpin->setEnabled(false);
    m_pageSpin->setFixedWidth(70);
    m_pageSpin->setToolTip(QStringLiteral("页码"));
    connect(m_pageSpin, qOverload<int>(&QSpinBox::valueChanged), this,
            [this](int value) { m_pdfView->goToPage(value - 1); });
    bar->addWidget(m_pageSpin);

    bar->addAction(m_nextPageAct);
    bar->addSeparator();
    bar->addAction(m_zoomInAct);
    bar->addAction(m_zoomOutAct);
    bar->addAction(m_fitWidthAct);
    bar->addSeparator();
    bar->addAction(m_translatePageAct);
    bar->addAction(m_translateAllAct);
    bar->addAction(m_stopAct);
    bar->addSeparator();
    bar->addAction(m_syncScrollAct);
    bar->addAction(m_settingsAct);
}

void MainWindow::createStatusBar()
{
    m_pageInfoLabel = new QLabel(QStringLiteral("未打开文件"), this);
    statusBar()->addWidget(m_pageInfoLabel);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setFixedWidth(160);
    m_progressBar->setVisible(false);
    statusBar()->addPermanentWidget(m_progressBar);
}

// 打开/关闭文档后，启用或禁用依赖文档的操作
void MainWindow::setDocumentActionsEnabled(bool enabled)
{
    m_prevPageAct->setEnabled(enabled);
    m_nextPageAct->setEnabled(enabled);
    m_zoomInAct->setEnabled(enabled);
    m_zoomOutAct->setEnabled(enabled);
    m_fitWidthAct->setEnabled(enabled);
    m_translatePageAct->setEnabled(enabled);
    m_translateAllAct->setEnabled(enabled);
    m_stopAct->setEnabled(false);
    m_syncScrollAct->setEnabled(enabled);
}

// 翻译进行中：禁用打开/翻译，启用停止，显示进度条
void MainWindow::setTranslatingUi(bool translating)
{
    m_openAct->setEnabled(!translating);
    m_translatePageAct->setEnabled(!translating && m_docReady);
    m_translateAllAct->setEnabled(!translating && m_docReady);
    m_stopAct->setEnabled(translating);
    m_progressBar->setVisible(translating);
}

void MainWindow::updatePageInfo()
{
    const int count = m_pdfView->pageCount();
    m_pageInfoLabel->setText(
        count > 0 ? QStringLiteral("共 %1 页 | 第 %2 页 | 缩放 %3%")
                        .arg(count)
                        .arg(m_pdfView->currentPage() + 1)
                        .arg(qRound(m_pdfView->zoomFactor() * 100))
                  : QStringLiteral("未打开文件"));
}

void MainWindow::openFileDialog()
{
    const QString startDir = !m_currentPdfPath.isEmpty()
        ? m_currentPdfPath
        : AppSettings::instance().lastOpenDir();
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("选择 PDF 文件"), startDir,
        QStringLiteral("PDF 文件 (*.pdf)"));
    if (path.isEmpty())
        return;
    AppSettings::instance().setLastOpenDir(QFileInfo(path).absolutePath());
    openPdf(path);
}

void MainWindow::openPdf(const QString &path)
{
    m_currentPdfPath = path;
    m_docReady = false;
    m_lastDocError.clear();
    QString error;
    if (!m_pdfView->openDocument(path, &error)) {
        m_lastDocError = error;
        QMessageBox::warning(this, QStringLiteral("打开失败"), error);
        return;
    }
    statusBar()->showMessage(QStringLiteral("正在打开：%1").arg(path), 3000);
}

void MainWindow::autoTranslateForSmoke(bool wholeDocument)
{
    m_smokeAutoTranslate = true;
    const auto trigger = [this, wholeDocument] {
        if (!m_smokeAutoTranslate)
            return;
        setTranslatingUi(true);
        if (wholeDocument)
            m_manager->translateWholeDocument();
        else
            m_manager->translateCurrentPage();
    };
    if (m_docReady) {
        trigger();
        return;
    }
    connect(m_pdfView, &PdfViewWidget::documentReady, this, trigger,
            Qt::SingleShotConnection);
}

QString MainWindow::smokeStatus() const
{
    if (m_currentPdfPath.isEmpty())
        return QStringLiteral("window_ok");
    if (!m_docReady) {
        if (!m_lastDocError.isEmpty())
            return QStringLiteral("pdf_failed: %1").arg(m_lastDocError);
        return QStringLiteral("pdf_loading");
    }
    QString s = QStringLiteral("pdf_ok pages=%1").arg(m_pdfView->pageCount());
    QString modeName;
    switch (AppSettings::instance().service()) {
    case AppSettings::Service::Azure:    modeName = QStringLiteral("azure");    break;
    case AppSettings::Service::DeepSeek: modeName = QStringLiteral("deepseek"); break;
    case AppSettings::Service::Mock:
    default:                             modeName = QStringLiteral("mock");     break;
    }
    s += QStringLiteral(" mode=%1").arg(modeName);
    if (m_smokeAutoTranslate) {
        s += m_runFinished
            ? QStringLiteral(" run=ok:%1/fail:%2").arg(m_lastRunOk).arg(m_lastRunFailed)
            : QStringLiteral(" run=running");
    }
    if (!m_lastStatus.isEmpty())
        s += QStringLiteral(" status=[%1]").arg(m_lastStatus);
    return s;
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, QStringLiteral("关于 PDF 翻译阅读器"),
                       QStringLiteral("<b>PDF 翻译阅读器</b> v0.1.0<br><br>"
                                      "左侧阅读原始 PDF，右侧通过 Microsoft Azure "
                                      "Translator 提供中文翻译。<br>"
                                      "技术栈：C++17 / Qt 6.8"));
}
