#pragma once

#include <QPdfDocument>
#include <QPdfDocumentRenderOptions>
#include <QPdfPageRenderer>
#include <QWidget>

class QLabel;
class QScrollArea;
class QTimer;

// 左栏：渲染并显示 PDF 页面
// - 懒加载虚拟化：只为可见页(±预取)创建 QLabel，滚出视口的页面销毁
// - QImage LRU 缓存（数量 + 总字节双上限），缩放变化时整体失效
// - 缩放代次计数：缩放后到达的过期渲染回调直接丢弃
class PdfViewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PdfViewWidget(QWidget *parent = nullptr);

    bool openDocument(const QString &path, QString *error);
    int pageCount() const;
    int currentPage() const;
    double zoomFactor() const;
    void setZoom(double factor);
    void zoomIn();
    void zoomOut();
    void fitWidth();
    void goToPage(int page);
    QPdfDocument *document();

signals:
    void pageChanged(int page);
    void documentReady(int pageCount);
    void documentFailed(const QString &message);

private slots:
    void onDocumentStatusChanged(QPdfDocument::Status status);
    void onPageRendered(int pageNumber, QSize imageSize, const QImage &image,
                        QPdfDocumentRenderOptions options, quint64 requestId);
    void onScrollChanged();
    void updateVisiblePages();

private:
    QSize displaySizeFor(int page) const;   // 控件显示尺寸（设备无关像素）
    QSize renderSizeFor(int page) const;    // 渲染目标尺寸（含 DPR）
    void relayout();                        // 重新计算页面位置并重建可见页
    void requestRender(int page);
    void setLabelImage(QLabel *label, int page);
    void evictCacheToLimits();

    QPdfDocument m_doc;          // 必须比 m_renderer 先析构（声明顺序保证）
    QPdfPageRenderer m_renderer;
    QScrollArea *m_scroll = nullptr;
    QWidget *m_container = nullptr;
    QTimer *m_settleTimer = nullptr;      // 滚动停止后再补齐可见页

    QHash<int, QLabel *> m_pageWidgets;   // 当前实例化的页面控件
    QHash<int, QImage> m_imageCache;      // LRU 缓存
    QList<int> m_cacheOrder;              // 最近使用在前
    qint64 m_cacheBytes = 0;
    QHash<int, QSize> m_requested;        // 已请求渲染的页面 -> 目标尺寸
    QHash<quint64, QPair<int, int>> m_pending;  // requestId -> (page, generation)

    QList<int> m_pageTops;                // 每页在容器内的 y 偏移
    QList<int> m_pageWidths;              // 每页显示宽度
    QList<int> m_pageHeights;             // 每页显示高度
    int m_maxPageWidth = 0;
    double m_zoom = 1.0;
    int m_generation = 0;
    int m_currentPage = -1;

    QString m_pendingPath;
    bool m_passwordPrompted = false;
};
