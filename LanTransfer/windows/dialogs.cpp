#include "dialogs.h"

#include <QDialogButtonBox>
#include <QFont>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {

QString formatSizeText(quint64 bytes)
{
    constexpr double MB = 1000.0 * 1000.0;
    constexpr double GB = 1000.0 * MB;
    if (bytes >= GB)
        return QStringLiteral("%1 GB").arg(double(bytes) / GB, 0, 'f', 2);
    if (bytes >= MB)
        return QStringLiteral("%1 MB").arg(double(bytes) / MB, 0, 'f', 1);
    if (bytes >= 1000)
        return QStringLiteral("%1 KB").arg(double(bytes) / 1000.0, 0, 'f', 1);
    return QStringLiteral("%1 B").arg(bytes);
}

} // namespace

// ---------------------------------------------------------------------------
// IncomingPairDialog
// ---------------------------------------------------------------------------
IncomingPairDialog::IncomingPairDialog(const QString& peerName, const QString& fileName,
                                       quint64 fileSize, const QString& pin, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("收到文件传输请求"));
    setModal(false);
    setMinimumWidth(380);

    auto* layout = new QVBoxLayout(this);

    auto* title = new QLabel(QStringLiteral("<b>%1</b> 想发送文件给你").arg(peerName.toHtmlEscaped()), this);
    layout->addWidget(title);

    auto* info = new QLabel(QStringLiteral("%1（%2）")
                                .arg(fileName.toHtmlEscaped(), formatSizeText(fileSize)), this);
    info->setWordWrap(true);
    layout->addWidget(info);

    auto* pinTitle = new QLabel(QStringLiteral("配对确认码（请与发送方核对）"), this);
    layout->addWidget(pinTitle);
    auto* pinLabel = new QLabel(pin, this);
    QFont pinFont = pinLabel->font();
    pinFont.setPointSize(28);
    pinFont.setBold(true);
    pinLabel->setFont(pinFont);
    pinLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(pinLabel);

    auto* hint = new QLabel(QStringLiteral("确认对方是你期望的设备后再接受。PIN 仅用于配对确认，不加密传输内容。"), this);
    hint->setWordWrap(true);
    layout->addWidget(hint);

    auto* buttons = new QDialogButtonBox(this);
    auto* acceptBtn = buttons->addButton(QStringLiteral("接受"), QDialogButtonBox::AcceptRole);
    auto* rejectBtn = buttons->addButton(QStringLiteral("拒绝"), QDialogButtonBox::RejectRole);
    layout->addWidget(buttons);

    connect(acceptBtn, &QPushButton::clicked, this, [this] {
        emit acceptedPair(true);
        close();
    });
    connect(rejectBtn, &QPushButton::clicked, this, [this] {
        emit acceptedPair(false);
        close();
    });
}

// ---------------------------------------------------------------------------
// PinInputDialog
// ---------------------------------------------------------------------------
PinInputDialog::PinInputDialog(const QString& peerName, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("输入配对确认码"));
    setModal(false);
    setMinimumWidth(360);

    auto* layout = new QVBoxLayout(this);

    auto* hint = new QLabel(
        QStringLiteral("请在 <b>%1</b> 的屏幕上查看 6 位配对确认码，并输入到下方。")
            .arg(peerName.toHtmlEscaped()), this);
    hint->setWordWrap(true);
    layout->addWidget(hint);

    auto* edit = new QLineEdit(this);
    edit->setMaxLength(6);
    edit->setAlignment(Qt::AlignCenter);
    edit->setValidator(new QRegularExpressionValidator(QRegularExpression(QStringLiteral("[0-9]{0,6}")), edit));
    QFont editFont = edit->font();
    editFont.setPointSize(24);
    edit->setFont(editFont);
    layout->addWidget(edit);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, [this, edit] {
        emit pinEntered(edit->text());
        close();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, [this] {
        emit pinEntered(QString());
        close();
    });
}

// ---------------------------------------------------------------------------
// ConflictDialog
// ---------------------------------------------------------------------------
ConflictDialog::ConflictDialog(const QString& fileName, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("文件已存在"));
    setModal(false);
    setMinimumWidth(360);

    auto* layout = new QVBoxLayout(this);

    auto* label = new QLabel(
        QStringLiteral("接收目录中已存在 <b>%1</b>，如何处理？").arg(fileName.toHtmlEscaped()), this);
    label->setWordWrap(true);
    layout->addWidget(label);

    auto* buttons = new QDialogButtonBox(this);
    auto* overwrite = buttons->addButton(QStringLiteral("覆盖"), QDialogButtonBox::DestructiveRole);
    auto* rename = buttons->addButton(QStringLiteral("自动重命名"), QDialogButtonBox::ActionRole);
    auto* cancel = buttons->addButton(QStringLiteral("取消传输"), QDialogButtonBox::RejectRole);
    layout->addWidget(buttons);

    connect(overwrite, &QPushButton::clicked, this, [this] {
        emit resolved(lantransfer::ConflictResolution::Overwrite);
        close();
    });
    connect(rename, &QPushButton::clicked, this, [this] {
        emit resolved(lantransfer::ConflictResolution::AutoRename);
        close();
    });
    connect(cancel, &QPushButton::clicked, this, [this] {
        emit resolved(lantransfer::ConflictResolution::Cancel);
        close();
    });
}

// ---------------------------------------------------------------------------
// ManualConnectDialog
// ---------------------------------------------------------------------------
ManualConnectDialog::ManualConnectDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("手动连接设备"));
    setModal(true);
    setMinimumWidth(320);

    auto* layout = new QVBoxLayout(this);

    auto* hint = new QLabel(
        QStringLiteral("自动发现失败时，可输入对方设备的局域网 IP 直接连接。\n端口填 0 表示自动探测。"), this);
    hint->setWordWrap(true);
    layout->addWidget(hint);

    auto* form = new QGridLayout;
    form->addWidget(new QLabel(QStringLiteral("IP 地址："), this), 0, 0);
    m_ipEdit = new QLineEdit(this);
    m_ipEdit->setPlaceholderText(QStringLiteral("192.168.1.100"));
    static const QRegularExpression ipv4Regex(
        QStringLiteral("^(\\d{1,3}\\.){0,3}\\d{0,3}$"));
    m_ipEdit->setValidator(new QRegularExpressionValidator(ipv4Regex, m_ipEdit));
    form->addWidget(m_ipEdit, 0, 1);

    form->addWidget(new QLabel(QStringLiteral("TCP 端口："), this), 1, 0);
    m_portSpin = new QSpinBox(this);
    m_portSpin->setRange(0, 65535);
    m_portSpin->setValue(0);
    m_portSpin->setSpecialValueText(QStringLiteral("自动探测"));
    form->addWidget(m_portSpin, 1, 1);
    layout->addLayout(form);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QString ManualConnectDialog::ip() const
{
    return m_ipEdit->text().trimmed();
}

quint16 ManualConnectDialog::tcpPort() const
{
    return quint16(m_portSpin->value());
}
