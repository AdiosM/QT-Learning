#include "Translator.h"

#include "AzureTranslator.h"
#include "DeepSeekTranslator.h"
#include "MockTranslator.h"
#include "settings/AppSettings.h"

// 根据设置创建翻译服务实例
Translator *createTranslator(const AppSettings &settings, QObject *parent)
{
    switch (settings.service()) {
    case AppSettings::Service::Azure:
        return new AzureTranslator(settings.endpointUrl(), settings.apiKey(),
                                   settings.region(), parent);
    case AppSettings::Service::DeepSeek:
        return new DeepSeekTranslator(settings.deepSeekEndpoint(),
                                      settings.deepSeekApiKey(),
                                      settings.deepSeekModel(), parent);
    case AppSettings::Service::Mock:
    default:
        return new MockTranslator(settings.mockDelayMs(),
                                  settings.mockFailEveryNth(), parent);
    }
}
