#include "quiredisplay.h"

#include "processor/registers.h"

#include <QComboBox>
#include <QFontDatabase>
#include <QFormLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QStringList>
#include <QVBoxLayout>

#include <algorithm>
#include <array>
#include <string>

namespace Kites
{
namespace
{
bool isZero(const posit::Quire256 &value)
{
    return std::all_of(value.limbs.begin(), value.limbs.end(),
                       [](uint32_t limb) { return limb == 0; });
}

std::array<uint32_t, 8> magnitude(const posit::Quire256 &value)
{
    auto words = value.limbs;
    if ((words.back() & 0x80000000u) == 0)
        return words;

    for (uint32_t &word : words)
        word = ~word;
    for (uint32_t &word : words)
    {
        ++word;
        if (word != 0)
            break;
    }
    return words;
}

std::string decimalMagnitude(const std::array<uint32_t, 8> &magnitudeWords)
{
    auto words = magnitudeWords;
    std::string digits;
    while (std::any_of(words.begin(), words.end(), [](uint32_t word) { return word != 0; }))
    {
        uint64_t remainder = 0;
        for (size_t i = words.size(); i-- > 0;)
        {
            const uint64_t current = (remainder << 32) | words[i];
            words[i] = static_cast<uint32_t>(current / 10);
            remainder = current % 10;
        }
        digits.push_back(static_cast<char>('0' + remainder));
    }
    if (digits.empty())
        return "0";
    std::reverse(digits.begin(), digits.end());
    return digits;
}

std::string fixedPointDecimal(const posit::Quire256 &value)
{
    const bool negative = (value.limbs.back() & 0x80000000u) != 0;
    std::string digits = decimalMagnitude(magnitude(value));

    // Multiplying by 5^112 and then placing the decimal point 112 digits from
    // the right represents the exact product signed_integer * 2^-112.
    for (int i = 0; i < 112; ++i)
    {
        unsigned carry = 0;
        for (size_t j = digits.size(); j-- > 0;)
        {
            const unsigned product = static_cast<unsigned>(digits[j] - '0') * 5 + carry;
            digits[j] = static_cast<char>('0' + product % 10);
            carry = product / 10;
        }
        while (carry != 0)
        {
            digits.insert(digits.begin(), static_cast<char>('0' + carry % 10));
            carry /= 10;
        }
    }

    if (digits.size() <= 112)
        digits.insert(0, 113 - digits.size(), '0');
    digits.insert(digits.size() - 112, 1, '.');

    while (digits.back() == '0')
        digits.pop_back();
    if (digits.back() == '.')
        digits.pop_back();
    if (negative)
        digits.insert(digits.begin(), '-');
    return digits;
}

} // namespace

QuireDisplay::QuireDisplay(QWidget *parent, RegisterFile *registerFile) : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 4, 6, 4);
    layout->setSpacing(4);

    auto *controls = new QFormLayout();
    controls->setContentsMargins(0, 0, 0, 0);
    m_displayType = new QComboBox(this);
    m_displayType->setObjectName("quireDisplayType");
    m_displayType->addItems({"Binary", "Hex", "Integer/Decimal"});
    m_displayType->setCurrentText("Hex");
    controls->addRow("Display Type:", m_displayType);

    m_status = new QLabel(this);
    m_status->setObjectName("quireStatus");
    controls->addRow("Status:", m_status);
    layout->addLayout(controls);

    m_valueDisplay = new QPlainTextEdit(this);
    m_valueDisplay->setReadOnly(true);
    m_valueDisplay->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    m_valueDisplay->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_valueDisplay->setMinimumHeight(70);
    m_valueDisplay->setMaximumHeight(100);
    m_valueDisplay->setObjectName("quireValueDisplay");
    layout->addWidget(m_valueDisplay);

    connect(m_displayType, &QComboBox::currentIndexChanged, this,
            [this]() { updateDisplay(); });
    setRegisterFile(registerFile);
}

void QuireDisplay::setRegisterFile(RegisterFile *registerFile)
{
    if (m_registerFile == registerFile)
    {
        if (m_registerFile)
            setQuireValue(m_registerFile->ReadQuire());
        return;
    }
    if (m_registerFile)
        QObject::disconnect(m_registerFile.data(), nullptr, this, nullptr);

    m_registerFile = registerFile;
    if (m_registerFile)
    {
        const uint64_t connectionGeneration = ++m_connectionGeneration;
        connect(m_registerFile.data(), &RegisterFile::updateQuireValue, this,
                [this, connectionGeneration](posit::Quire256 value)
                {
                    if (connectionGeneration == m_connectionGeneration)
                        setQuireValue(value);
                });
        setQuireValue(m_registerFile->ReadQuire());
    }
    else
    {
        ++m_connectionGeneration;
        setQuireValue(posit::quireZero());
    }
}

void QuireDisplay::setQuireValue(const posit::Quire256 &value)
{
    m_value = value;
    updateDisplay();
}

QString QuireDisplay::statusText(const posit::Quire256 &value)
{
    if (posit::isQuireNaR(value))
        return QStringLiteral("NaR");
    if (isZero(value))
        return QStringLiteral("Zero");
    return QStringLiteral("Normal");
}

QString QuireDisplay::formatValue(const posit::Quire256 &value, QuireDisplayMode mode)
{
    if (posit::isQuireNaR(value) && mode == QuireDisplayMode::IntegerDecimal)
        return QStringLiteral("NaR");
    if (isZero(value) && mode == QuireDisplayMode::IntegerDecimal)
        return QStringLiteral("0");

    if (mode == QuireDisplayMode::IntegerDecimal)
        return QString::fromStdString(fixedPointDecimal(value));

    QStringList words;
    for (size_t i = value.limbs.size(); i-- > 0;)
    {
        if (mode == QuireDisplayMode::Hex)
            words.push_back(QStringLiteral("%1").arg(value.limbs[i], 8, 16, QChar('0')).toUpper());
        else
        {
            QString bits;
            bits.reserve(32);
            for (int bit = 31; bit >= 0; --bit)
                bits.append((value.limbs[i] & (uint32_t{1} << bit)) ? QChar('1') : QChar('0'));
            words.push_back(bits);
        }
    }
    const QString separator = mode == QuireDisplayMode::Hex ? QStringLiteral("_")
                                                            : QStringLiteral(" ");
    const QString prefix = mode == QuireDisplayMode::Hex ? QStringLiteral("0x") : QString{};
    return prefix + words.join(separator);
}

void QuireDisplay::updateDisplay()
{
    m_status->setText(statusText(m_value));
    QuireDisplayMode mode = QuireDisplayMode::Hex;
    switch (m_displayType->currentIndex())
    {
    case 0:
        mode = QuireDisplayMode::Binary;
        break;
    case 2:
        mode = QuireDisplayMode::IntegerDecimal;
        break;
    default:
        break;
    }
    m_valueDisplay->setPlainText(formatValue(m_value, mode));
}

} // namespace Kites
