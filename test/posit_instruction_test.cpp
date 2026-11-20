#include <gtest/gtest.h>

#include <memory>
#include <sstream>
#include <string>

#include "assembler/assembler.h"
#include "common/instructions.h"
#include "common/register_names.h"
#include "language_service/completion_provider.h"
#include "processor/posit.h"
#include "processor/rvss/rvss_processor.h"
#include "processor/registers.h"
#include "ui/register_table/registermodel.h"
#include "utils/utils.h"

TEST(PositRegisterFile, NamesAndDebuggerEditingUseThePprBank)
{
    EXPECT_TRUE(Kites::IsValidPositRegister("p0"));
    EXPECT_TRUE(Kites::IsValidPositRegister("p31"));
    EXPECT_FALSE(Kites::IsValidPositRegister("p32"));
    EXPECT_FALSE(Kites::IsValidGeneralPurposeRegister("p1"));
    EXPECT_FALSE(Kites::IsValidFloatingPointRegister("p1"));
    Kites::RegisterFile registers;
    registers.ModifyRegister("p31", 0xffff);
    EXPECT_EQ(registers.ReadPreg(31), 0xffffu);

    Kites::RegisterModel model(nullptr, &registers);
    EXPECT_EQ(model.rowCount(), 96);
    EXPECT_EQ(model.index(64, 0).data().toString(), "p0");
    EXPECT_EQ(model.index(95, 1).data().toString(), "pt11");
    EXPECT_EQ(model.index(95, 2).data().toString(), "0xFFFF");
    registers.WritePreg(30, 0x4000);
    EXPECT_TRUE(model.index(94, 2).data(Qt::BackgroundRole).isValid());
    model.setDisplayBase(Kites::Base::Decimal);
    EXPECT_EQ(model.index(94, 2).data().toString(), "1");
}

TEST(PositRegisterAliases, MapToAllNumericRegistersAndAppearInEditorCompletion)
{
    Kites::RegisterFile registers;
    for (size_t i = 0; i < Kites::register_names::kPprAliases.size(); ++i)
    {
        const std::string alias = Kites::register_names::kPprAliases[i];
        const std::string numeric = Kites::register_names::kPprNames[i];
        ASSERT_TRUE(Kites::IsValidPositRegister(alias)) << alias;
        EXPECT_FALSE(Kites::IsValidGeneralPurposeRegister(alias)) << alias;
        EXPECT_FALSE(Kites::IsValidFloatingPointRegister(alias)) << alias;
        EXPECT_EQ(Kites::reg_alias_to_name.at(alias), numeric) << alias;
        EXPECT_TRUE(Kites::IsValidPositRegister(numeric)) << numeric;

        const uint16_t value = static_cast<uint16_t>(0x4000u + i);
        registers.ModifyRegister(alias, value);
        EXPECT_EQ(registers.ReadPreg(i), value) << alias;
    }

    const auto &symbols = Kites::CompletionProvider::getInstance().staticSymbols();
    EXPECT_TRUE(symbols.contains(QStringLiteral("p0")));
    for (const char *alias : Kites::register_names::kPprAliases)
        EXPECT_TRUE(symbols.contains(QString::fromLatin1(alias))) << alias;
}

TEST(PositRegisterAliases, AllPositInstructionFamiliesAcceptAliases)
{
    const std::vector<std::pair<std::string, std::string>> cases = {
        {"padd pa0, pt0, ps0", "padd p10, p0, p8"},
        {"psub ps2, pa1, pt11", "psub p18, p11, p31"},
        {"pmul pt8, ps1, pa7", "pmul p28, p9, p17"},
        {"pdiv ps11, pt7, pa0", "pdiv p27, p7, p10"},
        {"pcvt.s.p fa0, ps0", "pcvt.s.p f10, p8"},
        {"pcvt.p.s pa0, fa0", "pcvt.p.s p10, f10"},
        {"pcvt.w.p a0, ps0", "pcvt.w.p x10, p8"},
        {"pcvt.p.w pa0, a0", "pcvt.p.w p10, x10"},
        {"plh pt11, -2(a0)", "plh p31, -2(x10)"},
        {"psh ps0, -4(a0)", "psh p8, -4(x10)"},
        {"qma pa0, ps0", "qma p10, p8"},
        {"qms ps2, pt11", "qms p18, p31"},
        {"qround pa7", "qround p17"},
        {"qclr", "qclr"},
    };

    for (const auto &[aliased, numeric] : cases)
    {
        std::istringstream aliasSource(aliased + "\n");
        std::istringstream numericSource(numeric + "\n");
        const auto aliasProgram = Kites::assemble(aliasSource);
        const auto numericProgram = Kites::assemble(numericSource);
        ASSERT_EQ(aliasProgram.text_buffer.size(), 1u) << aliased;
        ASSERT_EQ(numericProgram.text_buffer.size(), 1u) << numeric;
        EXPECT_EQ(aliasProgram.text_buffer[0], numericProgram.text_buffer[0]) << aliased;
        EXPECT_EQ(Kites::instruction_set::disassemble(aliasProgram.text_buffer[0]),
                  Kites::instruction_set::disassemble(numericProgram.text_buffer[0]))
            << aliased;
    }
}

TEST(PositRegisterAliases, ArithmeticConversionMemoryAndQuireExecuteOnAliasedRegisters)
{
    Kites::setupVmStateDirectory();
    std::istringstream stream("addi x1, x0, 2\n"
                              "addi x2, x0, 3\n"
                              "pcvt.p.w pa0, x1\n"
                              "pcvt.p.w ps0, x2\n"
                              "padd pa1, pa0, ps0\n"
                              "psub ps2, pa1, pa0\n"
                              "pmul pt8, ps0, pa0\n"
                              "pdiv ps3, pt8, ps0\n"
                              "pcvt.s.p fa0, pt8\n"
                              "pcvt.p.s pa2, fa0\n"
                              "pcvt.w.p a3, pa2\n"
                              "addi x5, x0, 256\n"
                              "psh pt8, 0(x5)\n"
                              "plh pt9, 0(x5)\n"
                              "qclr\n"
                              "qma pa0, ps0\n"
                              "qms ps0, pa0\n"
                              "qround pa4\n");
    Kites::RVSSProcessor vm;
    vm.LoadProgram(Kites::assemble(stream));
    vm.DebugRun();

    EXPECT_EQ(vm.registers_.ReadPreg(11), Kites::posit::fromInt32(5));
    EXPECT_EQ(vm.registers_.ReadPreg(18), Kites::posit::fromInt32(3));
    EXPECT_EQ(vm.registers_.ReadPreg(28), Kites::posit::fromInt32(6));
    EXPECT_EQ(vm.registers_.ReadPreg(19), Kites::posit::fromInt32(2));
    EXPECT_EQ(vm.registers_.ReadPreg(12), Kites::posit::fromInt32(6));
    EXPECT_EQ(vm.registers_.ReadPreg(29), Kites::posit::fromInt32(6));
    EXPECT_EQ(vm.registers_.ReadPreg(14), Kites::posit::fromInt32(0));
    EXPECT_EQ(vm.registers_.ReadFpr(10), Kites::posit::toFloat32Bits(Kites::posit::fromInt32(6)));
    EXPECT_EQ(vm.registers_.ReadGpr(13), 6u);
    EXPECT_EQ(vm.registers_.ReadQuire().limbs, Kites::posit::quireZero().limbs);
}

TEST(PositRVSS, ArithmeticAndIntegerConversionsUseIndependentBanks)
{
    Kites::setupVmStateDirectory();
    std::istringstream stream("addi x1, x0, 1\n"
                              "addi x2, x0, 2\n"
                              "pcvt.p.w p1, x1\n"
                              "pcvt.p.w p2, x2\n"
                              "padd p3, p1, p2\n"
                              "psub p4, p2, p1\n"
                              "pmul p5, p1, p2\n"
                              "pdiv p6, p2, p1\n"
                              "pcvt.w.p x3, p3\n");
    Kites::RVSSProcessor vm;
    vm.LoadProgram(Kites::assemble(stream));
    vm.DebugRun();
    EXPECT_EQ(vm.registers_.ReadPreg(3), Kites::posit::fromInt32(3));
    EXPECT_EQ(vm.registers_.ReadPreg(4), Kites::posit::fromInt32(1));
    EXPECT_EQ(vm.registers_.ReadPreg(5), Kites::posit::fromInt32(2));
    EXPECT_EQ(vm.registers_.ReadPreg(6), Kites::posit::fromInt32(2));
    EXPECT_EQ(vm.registers_.ReadGpr(3), 3u);
    EXPECT_EQ(vm.registers_.ReadGpr(1), 1u);
}

TEST(PositRVSS, ConvertsInt32MinToFinitePosit)
{
    Kites::setupVmStateDirectory();
    std::istringstream stream("lui x1, 0x80000\n"
                              "pcvt.p.w p3, x1\n"
                              "pcvt.w.p x2, p3\n");
    Kites::RVSSProcessor vm;
    vm.LoadProgram(Kites::assemble(stream));
    vm.DebugRun();

    EXPECT_EQ(vm.registers_.ReadGpr(1), 0xffffffff80000000ull);
    EXPECT_EQ(vm.registers_.ReadPreg(3), 0x8050u);
    EXPECT_EQ(vm.registers_.ReadGpr(2), 0xffffffff80000000ull);
}

TEST(PositRVSS, Binary32ConversionsAndRawHalfwordMemory)
{
    Kites::setupVmStateDirectory();
    std::istringstream stream("addi x1, x0, 128\n"
                              "pcvt.p.s p1, f1\n"
                              "pcvt.s.p f2, p1\n"
                              "psh p1, 0(x1)\n"
                              "plh p3, 0(x1)\n");
    Kites::RVSSProcessor vm;
    vm.registers_.WriteFpr(1, 0x3fc00000u); // binary32 1.5
    vm.LoadProgram(Kites::assemble(stream));
    vm.DebugRun();
    EXPECT_EQ(vm.registers_.ReadPreg(1), Kites::posit::fromFloat32Bits(0x3fc00000u));
    EXPECT_EQ(vm.registers_.ReadFpr(2), Kites::posit::toFloat32Bits(vm.registers_.ReadPreg(1)));
    EXPECT_EQ(vm.registers_.ReadPreg(3), vm.registers_.ReadPreg(1));
}

TEST(PositRVSS, PositHalfwordStoreUndoRedoRestoresBytesAcrossCacheLine)
{
    Kites::setupVmStateDirectory();
    std::istringstream stream("addi x1, x0, 15\n"
                              "psh p2, 0(x1)\n");
    Kites::RVSSProcessor vm;
    vm.LoadProgram(Kites::assemble(stream));
    vm.registers_.WritePreg(2, 0x1234);
    vm.memory_controller_.writeByte_d(15, 0xa5);
    vm.memory_controller_.writeByte_d(16, 0x5a);

    vm.Step();
    vm.Step();
    EXPECT_EQ(vm.memory_controller_.peekByte(15), 0x34u);
    EXPECT_EQ(vm.memory_controller_.peekByte(16), 0x12u);

    vm.Undo();
    EXPECT_EQ(vm.memory_controller_.peekByte(15), 0xa5u);
    EXPECT_EQ(vm.memory_controller_.peekByte(16), 0x5au);

    vm.Redo();
    EXPECT_EQ(vm.memory_controller_.peekByte(15), 0x34u);
    EXPECT_EQ(vm.memory_controller_.peekByte(16), 0x12u);
}

TEST(PositRVSS, UndoRestoresPositRegisterWrites)
{
    Kites::setupVmStateDirectory();
    std::istringstream stream("addi x1, x0, 3\n"
                              "pcvt.p.w p1, x1\n");
    Kites::RVSSProcessor vm;
    vm.LoadProgram(Kites::assemble(stream));
    vm.Step();
    vm.Step();
    ASSERT_EQ(vm.registers_.ReadPreg(1), Kites::posit::fromInt32(3));
    vm.Undo();
    EXPECT_EQ(vm.registers_.ReadPreg(1), 0u);
}

TEST(PositRVSS, InvalidUnaryEncodingStopsWithoutGprWriteback)
{
    Kites::setupVmStateDirectory();
    for (uint32_t invalid : {0x0220210bu, 0x0000610bu, 0x0400210bu})
    {
        Kites::RVSSProcessor vm;
        Kites::AssembledProgram program;
        program.text_buffer = {invalid};
        vm.LoadProgram(program);
        vm.registers_.WriteGpr(1, 77);
        vm.DebugRun();
        EXPECT_TRUE(vm.IsStopRequested());
        EXPECT_EQ(vm.registers_.ReadGpr(1), 77u);
    }
}

TEST(PositEncoding, AllInstructionFieldsAndDisassemblyMatchSpecification)
{
    struct Case
    {
        const char *source;
        uint32_t word;
        uint8_t funct3;
        uint8_t funct7;
        const char *disassembly;
    };
    const Case cases[] = {
        {"padd p3, p1, p2", 0x0020818b, 0, 0, "padd p3, p1, p2"},
        {"psub p3, p1, p2", 0x0020918b, 1, 0, "psub p3, p1, p2"},
        {"pmul p3, p1, p2", 0x0020a18b, 2, 0, "pmul p3, p1, p2"},
        {"pdiv p3, p1, p2", 0x0020b18b, 3, 0, "pdiv p3, p1, p2"},
        {"pcvt.s.p f5, p4", 0x0202028b, 0, 1, "pcvt.s.p f5, p4"},
        {"pcvt.p.s p5, f4", 0x0202128b, 1, 1, "pcvt.p.s p5, f4"},
        {"pcvt.w.p x5, p4", 0x0202228b, 2, 1, "pcvt.w.p x5, p4"},
        {"pcvt.p.w p5, x4", 0x0202328b, 3, 1, "pcvt.p.w p5, x4"},
        {"plh p6, -2(x7)", 0xffe3c30b, 4, 0x7f, "plh p6, -2(x7)"},
        {"psh p8, -4(x9)", 0xfe84de0b, 5, 0x7f, "psh p8, -4(x9)"},
    };

    for (const Case &c : cases)
    {
        std::istringstream input(c.source);
        const auto program = Kites::assemble(input);
        ASSERT_EQ(program.text_buffer.size(), 1u) << c.source;
        const uint32_t word = program.text_buffer.front();
        EXPECT_EQ(word, c.word) << c.source;
        EXPECT_EQ(word & 0x7f, 0x0b) << c.source;
        EXPECT_EQ((word >> 12) & 7, c.funct3) << c.source;
        if (c.funct3 < 4)
            EXPECT_EQ((word >> 25) & 0x7f, c.funct7) << c.source;
        if (c.funct3 == 0 && c.funct7 == 1)
            EXPECT_EQ((word >> 20) & 0x1f, 0u) << c.source;
        EXPECT_EQ(Kites::instruction_set::disassemble(word), c.disassembly);
    }
}

TEST(PositEncoding, RejectsMalformedAndUnallocatedCustomEncodings)
{
    const uint32_t invalid[] = {
        0x0220210b, // unary conversion with nonzero rs2
        0x0000610b, // unallocated funct3
        0x0400210b, // unallocated conversion funct7
        0x0000002b, // custom-1 is unallocated
        0x0000005b, // custom-2 is unallocated
        0x0000007b, // custom-3 is unallocated
    };
    for (uint32_t word : invalid)
    {
        EXPECT_NE(Kites::instruction_set::disassemble(word).find("unknown("), std::string::npos);
        Kites::setupVmStateDirectory();
        Kites::RVSSProcessor vm;
        Kites::AssembledProgram program;
        program.text_buffer = {word};
        vm.LoadProgram(program);
        vm.DebugRun();
        EXPECT_TRUE(vm.IsStopRequested()) << std::hex << word;
    }
}

TEST(PositAssembler, RejectsWrongRegisterClasses)
{
    for (const char *source : {"padd x3, p1, p2", "pcvt.s.p p3, p4", "plh x3, 0(x1)",
                               "psh x3, 0(x1)"})
    {
        std::istringstream input(source);
        EXPECT_THROW(Kites::assemble(input), std::runtime_error) << source;
    }
}
