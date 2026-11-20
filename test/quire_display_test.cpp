#include <gtest/gtest.h>

#include <array>
#include <sstream>
#include <vector>

#include <QStringList>
#include <QApplication>
#include <QComboBox>
#include <QLabel>
#include <QPlainTextEdit>

#include "assembler/assembler.h"
#include "processor/posit.h"
#include "processor/registers.h"
#include "processor/rvss/rvss_processor.h"
#include "ui/register_table/quiredisplay.h"
#include "utils/utils.h"

namespace
{
using Kites::posit::Quire256;
using Kites::QuireDisplay;
using Kites::QuireDisplayMode;

Quire256 rawValue(uint32_t lowWord)
{
    Quire256 value;
    value.limbs[0] = lowWord;
    return value;
}

QString groupedZeroBinary()
{
    QString word(32, QChar('0'));
    QStringList words;
    for (int i = 0; i < 8; ++i)
        words.push_back(word);
    return words.join(' ');
}
}

TEST(QuireDisplayFormatting, ZeroAndNaRAreCompleteInEveryMode)
{
    const auto zero = Kites::posit::quireZero();
    const auto nar = Kites::posit::quireNaR();

    EXPECT_EQ(QuireDisplay::statusText(zero), "Zero");
    EXPECT_EQ(QuireDisplay::formatValue(zero, QuireDisplayMode::Binary), groupedZeroBinary());
    EXPECT_EQ(QuireDisplay::formatValue(zero, QuireDisplayMode::Hex),
              "0x00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000");
    EXPECT_EQ(QuireDisplay::formatValue(zero, QuireDisplayMode::IntegerDecimal), "0");

    EXPECT_EQ(QuireDisplay::statusText(nar), "NaR");
    QString narBinary = QuireDisplay::formatValue(nar, QuireDisplayMode::Binary);
    narBinary.remove(' ');
    EXPECT_EQ(narBinary.size(), 256);
    EXPECT_TRUE(narBinary.startsWith('1'));
    EXPECT_EQ(QuireDisplay::formatValue(nar, QuireDisplayMode::Hex),
              "0x80000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000");
    EXPECT_EQ(QuireDisplay::formatValue(nar, QuireDisplayMode::IntegerDecimal), "NaR");
}

TEST(QuireDisplayFormatting, ExactDecimalPreservesFractionalQuirePrecision)
{
    const auto positiveMinUnit = rawValue(1);
    auto negativeMinUnit = positiveMinUnit;
    for (uint32_t &word : negativeMinUnit.limbs)
        word = ~word;
    for (uint32_t &word : negativeMinUnit.limbs)
    {
        ++word;
        if (word != 0)
            break;
    }

    EXPECT_EQ(QuireDisplay::statusText(positiveMinUnit), "Normal");
    EXPECT_EQ(QuireDisplay::statusText(negativeMinUnit), "Normal");
    EXPECT_EQ(QuireDisplay::formatValue(positiveMinUnit, QuireDisplayMode::Hex),
              "0x00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000001");
    EXPECT_EQ(QuireDisplay::formatValue(positiveMinUnit, QuireDisplayMode::IntegerDecimal),
              "0.0000000000000000000000000000000001925929944387235853055977942584927318538101648215388195239938795566558837890625");
    EXPECT_EQ(QuireDisplay::formatValue(negativeMinUnit, QuireDisplayMode::IntegerDecimal),
              "-0.0000000000000000000000000000000001925929944387235853055977942584927318538101648215388195239938795566558837890625");

    QString bits = QuireDisplay::formatValue(positiveMinUnit, QuireDisplayMode::Binary);
    bits.remove(' ');
    EXPECT_EQ(bits.size(), 256);
    EXPECT_TRUE(bits.endsWith("00000000000000000000000000000001"));

    Quire256 one;
    one.limbs[3] = 1u << 16; // 2^112 * 2^-112
    EXPECT_EQ(QuireDisplay::formatValue(one, QuireDisplayMode::IntegerDecimal), "1");
}

TEST(QuireDisplayWidget, SelectorIsReadOnlyAndRefreshesFromRegisterFile)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    int argc = 1;
    char appName[] = "quire-display-test";
    char *argv[] = {appName, nullptr};
    QApplication app(argc, argv);

    Kites::RegisterFile registers;
    QuireDisplay display(nullptr, &registers);
    auto *selector = display.findChild<QComboBox *>("quireDisplayType");
    const auto *status = display.findChild<QLabel *>("quireStatus");
    const auto *value = display.findChild<QPlainTextEdit *>("quireValueDisplay");
    ASSERT_NE(selector, nullptr);
    ASSERT_NE(status, nullptr);
    ASSERT_NE(value, nullptr);
    EXPECT_TRUE(value->isReadOnly());
    EXPECT_EQ(selector->count(), 3);
    EXPECT_EQ(selector->itemText(0), "Binary");
    EXPECT_EQ(selector->itemText(1), "Hex");
    EXPECT_EQ(selector->itemText(2), "Integer/Decimal");
    EXPECT_EQ(status->text(), "Zero");
    EXPECT_EQ(value->toPlainText(),
              "0x00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000");

    registers.WriteQuire(rawValue(1));
    EXPECT_EQ(status->text(), "Normal");
    selector->setCurrentText("Binary");
    QString binary = value->toPlainText();
    binary.remove(' ');
    EXPECT_EQ(binary.size(), 256);
    EXPECT_TRUE(binary.endsWith('1'));

    selector->setCurrentText("Integer/Decimal");
    EXPECT_EQ(value->toPlainText(),
              "0.0000000000000000000000000000000001925929944387235853055977942584927318538101648215388195239938795566558837890625");

    registers.WriteQuire(Kites::posit::quireNaR());
    EXPECT_EQ(status->text(), "NaR");
    EXPECT_EQ(value->toPlainText(), "NaR");
    selector->setCurrentText("Hex");
    EXPECT_EQ(value->toPlainText(),
              "0x80000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000");
    registers.Reset();
    EXPECT_EQ(status->text(), "Zero");
    EXPECT_EQ(value->toPlainText(),
              "0x00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000");

    auto *retiredRegisterFile = new Kites::RegisterFile();
    QuireDisplay rebound(nullptr, retiredRegisterFile);
    delete retiredRegisterFile;
    Kites::RegisterFile replacementRegisterFile;
    rebound.setRegisterFile(&replacementRegisterFile);
    replacementRegisterFile.WriteQuire(Kites::posit::quireNaR());
    EXPECT_EQ(rebound.findChild<QLabel *>("quireStatus")->text(), "NaR");
}

TEST(QuireDisplaySynchronization, InstructionsUndoRedoAndResetPublishSnapshots)
{
    Kites::setupVmStateDirectory();
    std::istringstream source("qclr\nqma p1, p2\nqms p2, p2\n");
    Kites::RVSSProcessor vm;
    vm.LoadProgram(Kites::assemble(source));
    vm.registers_.WritePreg(1, Kites::posit::fromInt32(2));
    vm.registers_.WritePreg(2, Kites::posit::fromInt32(3));

    std::vector<Quire256> updates;
    QObject::connect(&vm.registers_, &Kites::RegisterFile::updateQuireValue, &vm,
                     [&updates](Quire256 value) { updates.push_back(value); });

    const auto expectLast = [&updates](const Quire256 &expected)
    {
        ASSERT_FALSE(updates.empty());
        EXPECT_EQ(updates.back().limbs, expected.limbs);
    };
    const auto six = Kites::posit::quireMulAdd(Kites::posit::quireZero(),
                                               Kites::posit::fromInt32(2),
                                               Kites::posit::fromInt32(3));
    const auto negativeThree = Kites::posit::quireMulSub(six, Kites::posit::fromInt32(3),
                                                         Kites::posit::fromInt32(3));

    ASSERT_TRUE(vm.StepAndNotify());
    expectLast(Kites::posit::quireZero());
    ASSERT_TRUE(vm.StepAndNotify());
    expectLast(six);
    vm.StepAndNotify(); // Executes the final instruction and returns false afterward.
    expectLast(negativeThree);

    vm.Undo();
    expectLast(six);
    vm.Redo();
    expectLast(negativeThree);
    vm.Reset();
    expectLast(Kites::posit::quireZero());
}
