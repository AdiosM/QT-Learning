#pragma once

#include <QList>
#include <QObject>
#include <QString>

class AppSettings;

// 翻译服务抽象接口：一次调用翻译一批文本
// 实现类只负责发出请求并报告结果；重试/取消策略由 TranslationQueue 负责
class Translator : public QObject
{
    Q_OBJECT

public:
    using Texts = QList<QString>;

    explicit Translator(QObject *parent = nullptr) : QObject(parent) {}

    virtual void translate(const Texts &texts, const QString &from,
                           const QString &to, quint64 jobId) = 0;
    virtual void cancelAll() = 0;

signals:
    void batchFinished(quint64 jobId, const Texts &translations);
    // retryable=false 表示重试无意义（如 401/403 密钥类错误），队列将直接判失败
    void batchFailed(quint64 jobId, const QString &error, bool retryable = true);
};

// 工厂：根据设置创建翻译服务实例（模拟 / Azure）
Translator *createTranslator(const AppSettings &settings, QObject *parent = nullptr);
