#include <gtest/gtest.h>

#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

#include "assembler/assembler.h"
#include "common/instructions.h"
#include "processor/posit.h"
#include "processor/rvss/rvss_processor.h"
#include "utils/utils.h"

namespace
{
Kites::posit::Quire256 makeQuirePattern()
{
    Kites::posit::Quire256 q;
    q.limbs = {0x89abcdefu, 0x01234567u, 0x76543210u, 0xfedcba98u,
               0x13579bdfu, 0x2468ace0u, 0x0badc0deu, 0x12345678u};
    return q;
}

std::unique_ptr<Kites::RVSSProcessor> loadProgram(const std::string &source)
{
    Kites::setupVmStateDirectory();
    auto vm = std::make_unique<Kites::RVSSProcessor>();
    std::istringstream input(source);
    vm->LoadProgram(Kites::assemble(input));
    return vm;
}
}

TEST(PositQuireEncoding, ExactWordsAndDisassembly)
{
    struct Case
    {
        const char *source;
        uint32_t word;
        const char *disassembly;
    };
    const Case cases[] = {
        {"qclr", 0x0000600bu, "qclr"},
        {"qma p1, p2", 0x0220e00bu, "qma p1, p2"},
        {"qms p1, p2", 0x0420e00bu, "qms p1, p2"},
        {"qround p5", 0x0600628bu, "qround p5"},
        {"qma p31, p0", 0x020fe00bu, "qma p31, p0"},
        {"qms p0, p31", 0x05f0600bu, "qms p0, p31"},
        {"qround p0", 0x0600600bu, "qround p0"},
    };

    for (const Case &test : cases)
    {
        std::istringstream input(test.source);
        const auto program = Kites::assemble(input);
        ASSERT_EQ(program.text_buffer.size(), 1u) << test.source;
        EXPECT_EQ(program.text_buffer[0], test.word) << test.source;
        EXPECT_EQ(Kites::instruction_set::disassemble(test.word), test.disassembly);
    }
}

TEST(PositQuireAssembler, RejectsMalformedSyntaxAndRegisters)
{
    for (const char *source : {"qclr p1", "qma p1", "qma p1, x2", "qma p1, p2, p3",
                               "qms x1, p2", "qround", "qround x1", "qround p32"})
    {
        std::istringstream input(source);
        EXPECT_THROW(Kites::assemble(input), std::runtime_error) << source;
    }
}

TEST(PositQuireRVSS, AssemblyExecutionAccumulatesAndRounds)
{
    auto vm = loadProgram("addi x1, x0, 2\n"
                          "addi x2, x0, 3\n"
                          "pcvt.p.w p1, x1\n"
                          "pcvt.p.w p2, x2\n"
                          "qclr\n"
                          "qma p1, p2\n"
                          "qma p1, p1\n"
                          "qms p2, p1\n"
                          "qround p3\n");
    vm->registers_.WriteQuire(makeQuirePattern());
    vm->DebugRun();

    auto expected = Kites::posit::quireZero();
    expected = Kites::posit::quireMulAdd(expected, Kites::posit::fromInt32(2),
                                         Kites::posit::fromInt32(3));
    expected = Kites::posit::quireMulAdd(expected, Kites::posit::fromInt32(2),
                                         Kites::posit::fromInt32(2));
    expected = Kites::posit::quireMulSub(expected, Kites::posit::fromInt32(3),
                                         Kites::posit::fromInt32(2));
    EXPECT_EQ(vm->registers_.ReadQuire().limbs, expected.limbs);
    EXPECT_EQ(vm->registers_.ReadPreg(3), Kites::posit::quireToPosit(expected));
    EXPECT_EQ(vm->registers_.ReadPreg(3), Kites::posit::fromInt32(4));
}

TEST(PositQuireRVSS, QclrClearsExistingQuire)
{
    auto vm = loadProgram("qclr\n");
    vm->registers_.WriteQuire(makeQuirePattern());
    vm->Step();
    EXPECT_EQ(vm->registers_.ReadQuire().limbs, (std::array<uint32_t, 8>{}));
}

TEST(PositQuireRVSS, NarAndOverflowPropagateThroughQma)
{
    {
        auto vm = loadProgram("qma p1, p2\n");
        vm->registers_.WritePreg(1, 0x8000);
        vm->registers_.WritePreg(2, 0x4000);
        vm->Step();
        EXPECT_TRUE(Kites::posit::isQuireNaR(vm->registers_.ReadQuire()));
    }
    {
        auto vm = loadProgram("qma p1, p2\n");
        auto nearMax = makeQuirePattern();
        nearMax.limbs.fill(0xffffffffu);
        nearMax.limbs.back() = 0x7fffffffu;
        vm->registers_.WriteQuire(nearMax);
        vm->registers_.WritePreg(1, 0x4000);
        vm->registers_.WritePreg(2, 0x4000);
        vm->Step();
        EXPECT_TRUE(Kites::posit::isQuireNaR(vm->registers_.ReadQuire()));
    }
}

TEST(PositQuireRVSS, QmsPropagatesNarOperandsAndNarAccumulator)
{
    {
        auto vm = loadProgram("qms p1, p2\n");
        vm->registers_.WritePreg(1, 0x8000);
        vm->registers_.WritePreg(2, 0x4000);
        vm->Step();
        EXPECT_TRUE(Kites::posit::isQuireNaR(vm->registers_.ReadQuire()));
    }

    for (const char *instruction : {"qma p1, p2\n", "qms p1, p2\n"})
    {
        auto vm = loadProgram(instruction);
        vm->registers_.WriteQuire(Kites::posit::quireNaR());
        vm->registers_.WritePreg(1, 0x4000);
        vm->registers_.WritePreg(2, 0x4800);
        vm->Step();
        EXPECT_EQ(vm->registers_.ReadQuire().limbs, Kites::posit::quireNaR().limbs)
            << instruction;
    }
}

TEST(PositQuireRVSS, QmsOverflowPropagatesNar)
{
    auto vm = loadProgram("qms p1, p2\n");
    Kites::posit::Quire256 justAboveReservedNegative;
    justAboveReservedNegative.limbs.back() = 0x80000000u;
    justAboveReservedNegative.limbs[0] = 1u; // -2^255 + 1 Quire unit
    vm->registers_.WriteQuire(justAboveReservedNegative);
    vm->registers_.WritePreg(1, 0x0001);
    vm->registers_.WritePreg(2, 0x0001); // minpos * minpos = one Quire unit

    vm->Step();
    EXPECT_EQ(vm->registers_.ReadQuire().limbs, Kites::posit::quireNaR().limbs);
}

TEST(PositQuireRVSS, QclrClearsNarAndItsHistoryRestoresAllBits)
{
    auto vm = loadProgram("qclr\n");
    const auto nar = Kites::posit::quireNaR();
    vm->registers_.WriteQuire(nar);

    vm->Step();
    EXPECT_EQ(vm->registers_.ReadQuire().limbs, Kites::posit::quireZero().limbs);
    vm->Undo();
    EXPECT_EQ(vm->registers_.ReadQuire().limbs, nar.limbs);
    vm->Redo();
    EXPECT_EQ(vm->registers_.ReadQuire().limbs, Kites::posit::quireZero().limbs);
}

TEST(PositQuireHistory, QmaQmsAndQclrUndoRedoRestoreAllQuireBits)
{
    for (const char *instruction : {"qma p1, p2", "qms p1, p2", "qclr"})
    {
        auto vm = loadProgram(std::string(instruction) + "\n");
        const auto oldValue = makeQuirePattern();
        vm->registers_.WriteQuire(oldValue);
        vm->registers_.WritePreg(1, Kites::posit::fromInt32(3));
        vm->registers_.WritePreg(2, Kites::posit::fromInt32(5));
        vm->Step();
        const auto newValue = vm->registers_.ReadQuire();
        ASSERT_NE(newValue.limbs, oldValue.limbs) << instruction;

        vm->Undo();
        EXPECT_EQ(vm->registers_.ReadQuire().limbs, oldValue.limbs) << instruction;
        vm->Redo();
        EXPECT_EQ(vm->registers_.ReadQuire().limbs, newValue.limbs) << instruction;
    }
}

TEST(PositQuireHistory, QroundChangesPprOnlyAndSupportsUndoRedo)
{
    auto vm = loadProgram("qround p5\n");
    auto q = Kites::posit::quireZero();
    q = Kites::posit::quireMulAdd(q, Kites::posit::fromInt32(3), Kites::posit::fromInt32(4));
    vm->registers_.WriteQuire(q);
    vm->registers_.WritePreg(5, 0xc000);

    vm->Step();
    const auto rounded = Kites::posit::quireToPosit(q);
    EXPECT_EQ(vm->registers_.ReadPreg(5), rounded);
    EXPECT_EQ(vm->registers_.ReadQuire().limbs, q.limbs);
    vm->Undo();
    EXPECT_EQ(vm->registers_.ReadPreg(5), 0xc000u);
    EXPECT_EQ(vm->registers_.ReadQuire().limbs, q.limbs);
    vm->Redo();
    EXPECT_EQ(vm->registers_.ReadPreg(5), rounded);
    EXPECT_EQ(vm->registers_.ReadQuire().limbs, q.limbs);
}

TEST(PositQuireHistory, QroundNarWritesNarAndSupportsUndoRedo)
{
    auto vm = loadProgram("qround p5\n");
    const auto nar = Kites::posit::quireNaR();
    vm->registers_.WriteQuire(nar);
    vm->registers_.WritePreg(5, 0x4400);

    vm->Step();
    EXPECT_EQ(vm->registers_.ReadPreg(5), 0x8000u);
    EXPECT_EQ(vm->registers_.ReadQuire().limbs, nar.limbs);
    vm->Undo();
    EXPECT_EQ(vm->registers_.ReadPreg(5), 0x4400u);
    EXPECT_EQ(vm->registers_.ReadQuire().limbs, nar.limbs);
    vm->Redo();
    EXPECT_EQ(vm->registers_.ReadPreg(5), 0x8000u);
    EXPECT_EQ(vm->registers_.ReadQuire().limbs, nar.limbs);
}

TEST(PositQuireHistory, UndoRedoRestoresNegativeFiniteQuireLimbs)
{
    for (const char *instruction : {"qma p1, p2\n", "qms p1, p2\n"})
    {
        auto vm = loadProgram(instruction);
        auto oldValue = makeQuirePattern();
        oldValue.limbs.back() = 0xf1234567u;
        oldValue.limbs[0] = 0x100u;
        vm->registers_.WriteQuire(oldValue);
        vm->registers_.WritePreg(1, 0x0001);
        vm->registers_.WritePreg(2, 0x0001);

        vm->Step();
        const auto newValue = vm->registers_.ReadQuire();
        ASSERT_NE(newValue.limbs, oldValue.limbs) << instruction;
        ASSERT_FALSE(Kites::posit::isQuireNaR(newValue));
        vm->Undo();
        EXPECT_EQ(vm->registers_.ReadQuire().limbs, oldValue.limbs) << instruction;
        vm->Redo();
        EXPECT_EQ(vm->registers_.ReadQuire().limbs, newValue.limbs) << instruction;
    }
}

TEST(PositQuireRVSS, RejectsReservedFieldViolations)
{
    const uint32_t invalid[] = {
        0x0000608bu, // qclr with rd != 0
        0x0220e08bu, // qma with rd != 0
        0x0600e00bu, // qround with rs1 != 0
        0x0610600bu, // qround with rs2 != 0
        0x0800600bu, // unallocated funct7
    };
    for (uint32_t word : invalid)
    {
        EXPECT_NE(Kites::instruction_set::disassemble(word).find("unknown("), std::string::npos)
            << std::hex << word;
        Kites::setupVmStateDirectory();
        Kites::RVSSProcessor vm;
        Kites::AssembledProgram program;
        program.text_buffer = {word};
        vm.LoadProgram(program);
        vm.DebugRun();
        EXPECT_TRUE(vm.IsStopRequested()) << std::hex << word;
    }
}
