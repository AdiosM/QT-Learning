#pragma once

#include <QMainWindow>

class QAction;
class QLabel;
class QProgressBar;
class QSpinBox;
class QSplitter;
class PdfViewWidget;
class TranslationManager;
class TranslationView;

// 主窗口：左侧原始 PDF，右侧译文（左右等宽）
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    void openPdf(const QString &path);        // 打开指定 PDF（--open 参数使用）
    void autoTranslateForSmoke(bool wholeDocument);  // 冒烟测试：打开后自动翻译
    QString smokeStatus() const;              // 冒烟测试：状态摘要（写日志用）

private slots:
    void openFileDialog();
    void showAbout();

protected:
    void closeEvent(QCloseEvent *event) override;   // 保存窗口几何与状态

private:
    void createActions();
    void createToolBar();
    void createStatusBar();
    void setDocumentActionsEnabled(bool enabled);
    void setTranslatingUi(bool translating);
    void updatePageInfo();

    QSplitter *m_splitter = nullptr;
    PdfViewWidget *m_pdfView = nullptr;
    TranslationView *m_translateView = nullptr;
    TranslationManager *m_manager = nullptr;
    QLabel *m_pageInfoLabel = nullptr;        // 状态栏：页码信息
    QProgressBar *m_progressBar = nullptr;    // 状态栏：翻译进度
    QSpinBox *m_pageSpin = nullptr;           // 工具栏：页码输入

    QString m_currentPdfPath;
    bool m_docReady = false;                  // 文档是否已成功加载
    QString m_lastDocError;                   // 最近一次文档错误信息
    bool m_smokeAutoTranslate = false;
    bool m_runFinished = false;
    int m_lastRunOk = 0;
    int m_lastRunFailed = 0;
    QString m_lastStatus;                     // 最近一次状态消息（冒烟验证用）

    QAction *m_openAct = nullptr;
    QAction *m_prevPageAct = nullptr;
    QAction *m_nextPageAct = nullptr;
    QAction *m_zoomInAct = nullptr;
    QAction *m_zoomOutAct = nullptr;
    QAction *m_fitWidthAct = nullptr;
    QAction *m_translatePageAct = nullptr;
    QAction *m_translateAllAct = nullptr;
    QAction *m_stopAct = nullptr;
    QAction *m_syncScrollAct = nullptr;
    QAction *m_settingsAct = nullptr;
};
