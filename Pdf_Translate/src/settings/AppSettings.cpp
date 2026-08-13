#include "AppSettings.h"

AppSettings &AppSettings::instance()
{
    static AppSettings settings;
    return settings;
}

AppSettings::AppSettings()
    : m_settings()   // 组织名/应用名已在 main.cpp 设置
{
}

AppSettings::Service AppSettings::service() const
{
    const QString s = m_settings.value(QStringLiteral("translate/service")).toString();
    if (s == QLatin1String("azure"))
        return Service::Azure;
    if (s == QLatin1String("deepseek"))
        return Service::DeepSeek;
    if (s == QLatin1String("mock"))
        return Service::Mock;
    // 兼容旧版 useMock 键（无 service 键时）
    if (m_settings.contains(QStringLiteral("translate/useMock")))
        return m_settings.value(QStringLiteral("translate/useMock")).toBool()
            ? Service::Mock
            : Service::Azure;
    return Service::Mock;
}

void AppSettings::setService(Service s)
{
    QString name;
    switch (s) {
    case Service::Azure:    name = QStringLiteral("azure");    break;
    case Service::DeepSeek: name = QStringLiteral("deepseek"); break;
    case Service::Mock:
    default:                name = QStringLiteral("mock");     break;
    }
    m_settings.setValue(QStringLiteral("translate/service"), name);
}

AppSettings::Endpoint AppSettings::endpoint() const
{
    const bool china = m_settings.value(QStringLiteral("translate/endpoint"), false).toBool();
    return china ? Endpoint::China : Endpoint::Global;
}

void AppSettings::setEndpoint(Endpoint e)
{
    m_settings.setValue(QStringLiteral("translate/endpoint"), e == Endpoint::China);
}

QString AppSettings::apiKey() const
{
    return m_settings.value(QStringLiteral("translate/apiKey")).toString();
}

void AppSettings::setApiKey(const QString &key)
{
    m_settings.setValue(QStringLiteral("translate/apiKey"), key);
}

QString AppSettings::region() const
{
    return m_settings.value(QStringLiteral("translate/region")).toString();
}

void AppSettings::setRegion(const QString &region)
{
    m_settings.setValue(QStringLiteral("translate/region"), region);
}

QString AppSettings::deepSeekEndpoint() const
{
    return m_settings.value(QStringLiteral("translate/deepSeekEndpoint"),
                            QStringLiteral("https://api.deepseek.com")).toString();
}

void AppSettings::setDeepSeekEndpoint(const QString &endpoint)
{
    m_settings.setValue(QStringLiteral("translate/deepSeekEndpoint"), endpoint);
}

QString AppSettings::deepSeekApiKey() const
{
    return m_settings.value(QStringLiteral("translate/deepSeekApiKey")).toString();
}

void AppSettings::setDeepSeekApiKey(const QString &key)
{
    m_settings.setValue(QStringLiteral("translate/deepSeekApiKey"), key);
}

QString AppSettings::deepSeekModel() const
{
    return m_settings.value(QStringLiteral("translate/deepSeekModel"),
                            QStringLiteral("deepseek-chat")).toString();
}

void AppSettings::setDeepSeekModel(const QString &model)
{
    m_settings.setValue(QStringLiteral("translate/deepSeekModel"), model);
}

QString AppSettings::sourceLang() const
{
    return m_settings.value(QStringLiteral("translate/sourceLang")).toString();
}

void AppSettings::setSourceLang(const QString &lang)
{
    m_settings.setValue(QStringLiteral("translate/sourceLang"), lang);
}

QString AppSettings::targetLang() const
{
    return m_settings.value(QStringLiteral("translate/targetLang"),
                            QStringLiteral("zh-Hans")).toString();
}

void AppSettings::setTargetLang(const QString &lang)
{
    m_settings.setValue(QStringLiteral("translate/targetLang"), lang);
}

int AppSettings::mockDelayMs() const
{
    return m_settings.value(QStringLiteral("translate/mockDelayMs"), 300).toInt();
}

void AppSettings::setMockDelayMs(int ms)
{
    m_settings.setValue(QStringLiteral("translate/mockDelayMs"), ms);
}

int AppSettings::mockFailEveryNth() const
{
    return m_settings.value(QStringLiteral("translate/mockFailEveryNth"), 0).toInt();
}

void AppSettings::setMockFailEveryNth(int n)
{
    m_settings.setValue(QStringLiteral("translate/mockFailEveryNth"), n);
}

QString AppSettings::endpointUrl() const
{
    return endpoint() == Endpoint::Global
        ? QStringLiteral("https://api.cognitive.microsofttranslator.com")
        : QStringLiteral("https://api.translator.azure.cn");
}

QByteArray AppSettings::windowGeometry() const
{
    return m_settings.value(QStringLiteral("window/geometry")).toByteArray();
}

void AppSettings::setWindowGeometry(const QByteArray &geometry)
{
    m_settings.setValue(QStringLiteral("window/geometry"), geometry);
}

QByteArray AppSettings::windowState() const
{
    return m_settings.value(QStringLiteral("window/state")).toByteArray();
}

void AppSettings::setWindowState(const QByteArray &state)
{
    m_settings.setValue(QStringLiteral("window/state"), state);
}

QString AppSettings::lastOpenDir() const
{
    return m_settings.value(QStringLiteral("window/lastOpenDir")).toString();
}

void AppSettings::setLastOpenDir(const QString &dir)
{
    m_settings.setValue(QStringLiteral("window/lastOpenDir"), dir);
}

void AppSettings::sync()
{
    m_settings.sync();
}
