#pragma once

#include <QSettings>
#include <QString>

// 应用设置（QSettings 持久化到 HKCU 注册表）
class AppSettings
{
public:
    enum class Endpoint { Global, China };
    enum class Service { Mock, Azure, DeepSeek };

    static AppSettings &instance();

    // 翻译服务
    Service service() const;
    void setService(Service s);
    Endpoint endpoint() const;               // Azure 终端节点
    void setEndpoint(Endpoint e);
    QString apiKey() const;                  // Azure 密钥
    void setApiKey(const QString &key);
    QString region() const;                  // Azure 区域
    void setRegion(const QString &region);
    QString deepSeekEndpoint() const;        // DeepSeek 基地址
    void setDeepSeekEndpoint(const QString &endpoint);
    QString deepSeekApiKey() const;
    void setDeepSeekApiKey(const QString &key);
    QString deepSeekModel() const;
    void setDeepSeekModel(const QString &model);

    // 语言
    QString sourceLang() const;              // "" = 自动检测
    void setSourceLang(const QString &lang);
    QString targetLang() const;
    void setTargetLang(const QString &lang);

    // 模拟翻译
    int mockDelayMs() const;
    void setMockDelayMs(int ms);
    int mockFailEveryNth() const;            // 0 = 从不失败
    void setMockFailEveryNth(int n);

    QString endpointUrl() const;             // Azure 解析后的 API 基地址

    // 窗口
    QByteArray windowGeometry() const;
    void setWindowGeometry(const QByteArray &geometry);
    QByteArray windowState() const;
    void setWindowState(const QByteArray &state);
    QString lastOpenDir() const;
    void setLastOpenDir(const QString &dir);

    void sync();

private:
    AppSettings();
    QSettings m_settings;
};
