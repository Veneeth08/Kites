#include <gtest/gtest.h>

#include <sstream>

#include "assembler/assembler.h"
#include "processor/rvss/rvss_processor.h"
#include "utils/utils.h"

namespace
{
Kites::posit::Quire256 quirePattern(uint32_t high, uint32_t low)
{
    Kites::posit::Quire256 value;
    value.limbs[0] = low;
    value.limbs[1] = 0x11223344u;
    value.limbs[2] = 0x55667788u;
    value.limbs[3] = 0x99aabbccu;
    value.limbs[4] = 0xddeeff00u;
    value.limbs[5] = 0x13579bdfu;
    value.limbs[6] = 0x2468ace0u;
    value.limbs[7] = high;
    return value;
}
}

TEST(QuireRegisterFile, InitializesResetsAndStaysIndependent)
{
    Kites::RegisterFile registers;
    EXPECT_EQ(registers.ReadQuire().limbs, (std::array<uint32_t, 8>{}));

    const auto value = quirePattern(0x7abcdef0u, 0x01234567u);
    registers.WriteQuire(value);
    EXPECT_EQ(registers.ReadQuire().limbs, value.limbs);

    registers.WriteGpr(1, 11);
    registers.WriteFpr(2, 0x40000000u);
    registers.WritePreg(3, 0x4567);
    EXPECT_EQ(registers.ReadQuire().limbs, value.limbs);
    EXPECT_EQ(registers.ReadGpr(1), 11u);
    EXPECT_EQ(registers.ReadFpr(2), 0x40000000u);
    EXPECT_EQ(registers.ReadPreg(3), 0x4567u);

    registers.Reset();
    EXPECT_EQ(registers.ReadQuire().limbs, (std::array<uint32_t, 8>{}));
}

TEST(QuireHistory, UndoRedoRestoresAllLimbsWithoutChangingOtherBanks)
{
    Kites::setupVmStateDirectory();
    Kites::RVSSProcessor vm;
    const auto oldValue = quirePattern(0x01234567u, 0x89abcdefu);
    const auto newValue = quirePattern(0xfedcba98u, 0x76543210u);

    vm.registers_.WriteQuire(newValue);
    vm.registers_.WriteGpr(1, 0x1111);
    vm.registers_.WriteFpr(2, 0x2222);
    vm.registers_.WritePreg(3, 0x3333);
    vm.registers_.WriteCsr(4, 0x4444);

    Kites::StepDelta delta;
    delta.old_pc = 4;
    delta.new_pc = 8;
    delta.quire_change = Kites::QuireChange{oldValue, newValue};
    vm.undo_stack_.push(delta);
    vm.program_counter_ = delta.new_pc;
    vm.cycle_s_ = 1;
    vm.instructions_retired_ = 1;

    vm.Undo();
    EXPECT_EQ(vm.registers_.ReadQuire().limbs, oldValue.limbs);
    EXPECT_EQ(vm.program_counter_, delta.old_pc);
    EXPECT_EQ(vm.registers_.ReadGpr(1), 0x1111u);
    EXPECT_EQ(vm.registers_.ReadFpr(2), 0x2222u);
    EXPECT_EQ(vm.registers_.ReadPreg(3), 0x3333u);
    EXPECT_EQ(vm.registers_.ReadCsr(4), 0x4444u);

    vm.Redo();
    EXPECT_EQ(vm.registers_.ReadQuire().limbs, newValue.limbs);
    EXPECT_EQ(vm.program_counter_, delta.new_pc);
    EXPECT_EQ(vm.registers_.ReadGpr(1), 0x1111u);
    EXPECT_EQ(vm.registers_.ReadFpr(2), 0x2222u);
    EXPECT_EQ(vm.registers_.ReadPreg(3), 0x3333u);
    EXPECT_EQ(vm.registers_.ReadCsr(4), 0x4444u);
}

TEST(QuireHistory, ResetClearsQuireStacksAndPendingSnapshot)
{
    Kites::setupVmStateDirectory();
    Kites::RVSSProcessor vm;
    std::istringstream qmaSource("qma p1, p2\n");
    vm.LoadProgram(Kites::assemble(qmaSource));
    vm.registers_.WritePreg(1, Kites::posit::fromInt32(2));
    vm.registers_.WritePreg(2, Kites::posit::fromInt32(3));
    vm.Step();
    ASSERT_FALSE(vm.undo_stack_.empty());
    ASSERT_NE(vm.registers_.ReadQuire().limbs, (std::array<uint32_t, 8>{}));

    vm.Undo();
    ASSERT_FALSE(vm.redo_stack_.empty());
    vm.undo_stack_.push(Kites::StepDelta{}); // Reset must clear both history stacks.
    vm.current_delta_.quire_change = Kites::QuireChange{
        quirePattern(0x01234567u, 0x89abcdefu), quirePattern(0xfedcba98u, 0x76543210u)};

    vm.Reset();
    EXPECT_EQ(vm.registers_.ReadQuire().limbs, (std::array<uint32_t, 8>{}));
    EXPECT_TRUE(vm.undo_stack_.empty());
    EXPECT_TRUE(vm.redo_stack_.empty());
    EXPECT_FALSE(vm.current_delta_.quire_change.has_value());

    std::istringstream addiSource("addi x1, x0, 1\n");
    vm.LoadProgram(Kites::assemble(addiSource));
    vm.Step();
    vm.Undo();
    EXPECT_EQ(vm.registers_.ReadQuire().limbs, (std::array<uint32_t, 8>{}));
    vm.Redo();
    EXPECT_EQ(vm.registers_.ReadQuire().limbs, (std::array<uint32_t, 8>{}));
}

TEST(PositRVSS, RedoRestoresPositRegisterWrite)
{
    Kites::setupVmStateDirectory();
    std::istringstream stream("addi x1, x0, 7\n"
                              "pcvt.p.w p3, x1\n");
    Kites::RVSSProcessor vm;
    vm.LoadProgram(Kites::assemble(stream));
    vm.Step();
    vm.Step();

    const uint16_t result = Kites::posit::fromInt32(7);
    ASSERT_EQ(vm.registers_.ReadPreg(3), result);
    vm.Undo();
    EXPECT_EQ(vm.registers_.ReadPreg(3), 0u);
    vm.Redo();
    EXPECT_EQ(vm.registers_.ReadPreg(3), result);
}
