#pragma once

#include "processor/posit.h"
#include "processor/registers.h"

#include <QPointer>
#include <QWidget>
#include <QString>

#include <cstdint>

class QComboBox;
class QLabel;
class QPlainTextEdit;

namespace Kites
{
enum class QuireDisplayMode
{
    Binary,
    Hex,
    IntegerDecimal
};

class QuireDisplay : public QWidget
{
  public:
    explicit QuireDisplay(QWidget *parent = nullptr, RegisterFile *registerFile = nullptr);

    void setRegisterFile(RegisterFile *registerFile);
    void setQuireValue(const posit::Quire256 &value);

    static QString statusText(const posit::Quire256 &value);
    static QString formatValue(const posit::Quire256 &value, QuireDisplayMode mode);

  private:
    void updateDisplay();

    QPointer<RegisterFile> m_registerFile{};
    uint64_t m_connectionGeneration{0};
    posit::Quire256 m_value{};
    QComboBox *m_displayType{nullptr};
    QLabel *m_status{nullptr};
    QPlainTextEdit *m_valueDisplay{nullptr};
};

} // namespace Kites
