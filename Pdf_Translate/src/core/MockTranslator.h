#pragma once

#include "Translator.h"

class QTimer;

// 开发用模拟翻译器：延迟后返回 "【模拟译文】" + 原文
// 可配置延迟毫秒数，以及每第 N 批失败一次（用于测试重试路径）
class MockTranslator : public Translator
{
    Q_OBJECT

public:
    MockTranslator(int delayMs = 300, int failEveryNth = 0,
                   QObject *parent = nullptr);

    void translate(const Texts &texts, const QString &from,
                   const QString &to, quint64 jobId) override;
    void cancelAll() override;

private:
    int m_delayMs;
    int m_failEveryNth;          // 0 = 从不失败
    int m_batchCount = 0;
    QList<QTimer *> m_pending;
};
