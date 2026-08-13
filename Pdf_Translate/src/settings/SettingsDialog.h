#pragma once

#include <QDialog>

class QComboBox;
class QLabel;
class QLineEdit;
class QSpinBox;

// 设置对话框：翻译服务、密钥、语言等
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    void accept() override;   // 保存到 AppSettings

private:
    void loadFromSettings();
    void updateFieldEnabled();
    void updateCacheSizeLabel();

    QComboBox *m_modeCombo = nullptr;       // 0=模拟 1=Azure 2=DeepSeek
    QComboBox *m_endpointCombo = nullptr;   // Azure: 0=全球 1=中国
    QLineEdit *m_keyEdit = nullptr;         // Azure 密钥
    QLineEdit *m_regionEdit = nullptr;      // Azure 区域
    QLineEdit *m_deepSeekEndpointEdit = nullptr;
    QLineEdit *m_deepSeekKeyEdit = nullptr;
    QComboBox *m_deepSeekModelCombo = nullptr;
    QComboBox *m_sourceCombo = nullptr;
    QComboBox *m_targetCombo = nullptr;
    QSpinBox *m_delaySpin = nullptr;
    QSpinBox *m_failSpin = nullptr;
    QLabel *m_cacheSizeLabel = nullptr;
};
