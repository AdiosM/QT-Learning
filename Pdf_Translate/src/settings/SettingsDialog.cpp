#include "SettingsDialog.h"
#include "AppSettings.h"
#include "core/TranslationCache.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("设置"));
    setMinimumWidth(400);

    m_modeCombo = new QComboBox(this);
    m_modeCombo->addItem(tr("模拟翻译（无需密钥，用于测试）"), 0);
    m_modeCombo->addItem(tr("Microsoft Azure Translator"), 1);
    m_modeCombo->addItem(tr("DeepSeek（大模型翻译）"), 2);

    m_endpointCombo = new QComboBox(this);
    m_endpointCombo->addItem(tr("全球 (api.cognitive.microsofttranslator.com)"), 0);
    m_endpointCombo->addItem(tr("中国 (api.translator.azure.cn)"), 1);

    m_keyEdit = new QLineEdit(this);
    m_keyEdit->setEchoMode(QLineEdit::Password);
    m_keyEdit->setPlaceholderText(tr("Azure 密钥（Subscription Key）"));

    m_regionEdit = new QLineEdit(this);
    m_regionEdit->setPlaceholderText(tr("资源所在区域，如 eastasia"));

    m_deepSeekEndpointEdit = new QLineEdit(this);
    m_deepSeekEndpointEdit->setPlaceholderText(tr("https://api.deepseek.com"));

    m_deepSeekKeyEdit = new QLineEdit(this);
    m_deepSeekKeyEdit->setEchoMode(QLineEdit::Password);
    m_deepSeekKeyEdit->setPlaceholderText(tr("DeepSeek API Key（sk-...）"));

    m_deepSeekModelCombo = new QComboBox(this);
    m_deepSeekModelCombo->setEditable(true);
    m_deepSeekModelCombo->addItems({
        QStringLiteral("deepseek-chat"),
        QStringLiteral("deepseek-v4-flash"),
        QStringLiteral("deepseek-v4-pro"),
        QStringLiteral("deepseek-reasoner")});

    m_sourceCombo = new QComboBox(this);
    m_sourceCombo->addItem(tr("自动检测"), QString());
    const QStringList langCodes = {
        QStringLiteral("en"), QStringLiteral("zh-Hans"), QStringLiteral("zh-Hant"),
        QStringLiteral("ja"), QStringLiteral("ko"), QStringLiteral("de"),
        QStringLiteral("fr"), QStringLiteral("ru")};
    const QStringList langNames = {
        tr("英语"), tr("中文简体"), tr("中文繁体"), tr("日语"),
        tr("韩语"), tr("德语"), tr("法语"), tr("俄语")};
    for (int i = 0; i < langCodes.size(); ++i)
        m_sourceCombo->addItem(langNames.at(i), langCodes.at(i));

    m_targetCombo = new QComboBox(this);
    for (int i = 0; i < langCodes.size(); ++i)
        m_targetCombo->addItem(langNames.at(i), langCodes.at(i));

    m_delaySpin = new QSpinBox(this);
    m_delaySpin->setRange(0, 5000);
    m_delaySpin->setSuffix(tr(" 毫秒"));

    m_failSpin = new QSpinBox(this);
    m_failSpin->setRange(0, 100);
    m_failSpin->setToolTip(tr("0 = 从不失败；用于测试重试机制"));

    // 缓存大小与清理
    m_cacheSizeLabel = new QLabel(this);
    auto *clearCacheButton = new QPushButton(tr("清空缓存"), this);
    connect(clearCacheButton, &QPushButton::clicked, this, [this] {
        TranslationCache cache(TranslationCache::defaultDbPath());
        if (cache.init(nullptr) && cache.clear())
            updateCacheSizeLabel();
    });
    auto *cacheRow = new QWidget(this);
    auto *cacheLayout = new QHBoxLayout(cacheRow);
    cacheLayout->setContentsMargins(0, 0, 0, 0);
    cacheLayout->addWidget(m_cacheSizeLabel);
    cacheLayout->addStretch();
    cacheLayout->addWidget(clearCacheButton);

    auto *form = new QFormLayout;
    form->addRow(tr("翻译模式："), m_modeCombo);
    form->addRow(tr("Azure 终端节点："), m_endpointCombo);
    form->addRow(tr("Azure 密钥："), m_keyEdit);
    form->addRow(tr("Azure 区域："), m_regionEdit);
    form->addRow(tr("DeepSeek 地址："), m_deepSeekEndpointEdit);
    form->addRow(tr("DeepSeek 密钥："), m_deepSeekKeyEdit);
    form->addRow(tr("DeepSeek 模型："), m_deepSeekModelCombo);
    form->addRow(tr("源语言："), m_sourceCombo);
    form->addRow(tr("目标语言："), m_targetCombo);
    form->addRow(tr("模拟延迟："), m_delaySpin);
    form->addRow(tr("每 N 批模拟失败："), m_failSpin);
    form->addRow(tr("翻译缓存："), cacheRow);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);

    connect(m_modeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this](int) { updateFieldEnabled(); });

    loadFromSettings();
    updateFieldEnabled();
    updateCacheSizeLabel();
}

void SettingsDialog::updateCacheSizeLabel()
{
    const qint64 bytes =
        TranslationCache(TranslationCache::defaultDbPath()).sizeBytes();
    if (bytes >= 1024 * 1024)
        m_cacheSizeLabel->setText(tr("%1 MB").arg(bytes / 1024.0 / 1024.0, 0, 'f', 1));
    else if (bytes >= 1024)
        m_cacheSizeLabel->setText(tr("%1 KB").arg(bytes / 1024.0, 0, 'f', 1));
    else
        m_cacheSizeLabel->setText(tr("%1 字节").arg(bytes));
}

void SettingsDialog::loadFromSettings()
{
    const AppSettings &s = AppSettings::instance();
    switch (s.service()) {
    case AppSettings::Service::Azure:    m_modeCombo->setCurrentIndex(1); break;
    case AppSettings::Service::DeepSeek: m_modeCombo->setCurrentIndex(2); break;
    case AppSettings::Service::Mock:
    default:                             m_modeCombo->setCurrentIndex(0); break;
    }
    m_endpointCombo->setCurrentIndex(
        s.endpoint() == AppSettings::Endpoint::Global ? 0 : 1);
    m_keyEdit->setText(s.apiKey());
    m_regionEdit->setText(s.region());
    m_deepSeekEndpointEdit->setText(s.deepSeekEndpoint());
    m_deepSeekKeyEdit->setText(s.deepSeekApiKey());
    m_deepSeekModelCombo->setCurrentText(s.deepSeekModel());
    m_sourceCombo->setCurrentIndex(qMax(0, m_sourceCombo->findData(s.sourceLang())));
    m_targetCombo->setCurrentIndex(qMax(0, m_targetCombo->findData(s.targetLang())));
    m_delaySpin->setValue(s.mockDelayMs());
    m_failSpin->setValue(s.mockFailEveryNth());
}

void SettingsDialog::updateFieldEnabled()
{
    const bool azure = m_modeCombo->currentIndex() == 1;
    const bool deepSeek = m_modeCombo->currentIndex() == 2;
    const bool mock = m_modeCombo->currentIndex() == 0;
    m_endpointCombo->setEnabled(azure);
    m_keyEdit->setEnabled(azure);
    m_regionEdit->setEnabled(azure);
    m_deepSeekEndpointEdit->setEnabled(deepSeek);
    m_deepSeekKeyEdit->setEnabled(deepSeek);
    m_deepSeekModelCombo->setEnabled(deepSeek);
    m_delaySpin->setEnabled(mock);
    m_failSpin->setEnabled(mock);
}

void SettingsDialog::accept()
{
    AppSettings &s = AppSettings::instance();
    switch (m_modeCombo->currentIndex()) {
    case 1:  s.setService(AppSettings::Service::Azure);    break;
    case 2:  s.setService(AppSettings::Service::DeepSeek); break;
    default: s.setService(AppSettings::Service::Mock);     break;
    }
    s.setEndpoint(m_endpointCombo->currentIndex() == 0
                      ? AppSettings::Endpoint::Global
                      : AppSettings::Endpoint::China);
    s.setApiKey(m_keyEdit->text().trimmed());
    s.setRegion(m_regionEdit->text().trimmed());
    s.setDeepSeekEndpoint(m_deepSeekEndpointEdit->text().trimmed());
    s.setDeepSeekApiKey(m_deepSeekKeyEdit->text().trimmed());
    s.setDeepSeekModel(m_deepSeekModelCombo->currentText().trimmed());
    s.setSourceLang(m_sourceCombo->currentData().toString());
    s.setTargetLang(m_targetCombo->currentData().toString());
    s.setMockDelayMs(m_delaySpin->value());
    s.setMockFailEveryNth(m_failSpin->value());
    s.sync();
    QDialog::accept();
}
