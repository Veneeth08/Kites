#include "assembler/code_generator.h"
#include "common/instructions.h"

#include <array>
#include <cstdint>
#include <iostream>

struct Case
{
    const char *opcode;
    const char *rd;
    const char *rs1;
    const char *rs2;
    const char *imm;
    uint32_t expected;
    const char *disassembly;
};

int main()
{
    const std::array<Case, 10> cases{{
        {"padd", "p3", "p1", "p2", "", 0x0020818b, "padd p3, p1, p2"},
        {"psub", "p3", "p1", "p2", "", 0x0020918b, "psub p3, p1, p2"},
        {"pmul", "p3", "p1", "p2", "", 0x0020a18b, "pmul p3, p1, p2"},
        {"pdiv", "p3", "p1", "p2", "", 0x0020b18b, "pdiv p3, p1, p2"},
        {"pcvt.s.p", "f5", "p4", "", "", 0x0202028b, "pcvt.s.p f5, p4"},
        {"pcvt.p.s", "p5", "f4", "", "", 0x0202128b, "pcvt.p.s p5, f4"},
        {"pcvt.w.p", "x5", "p4", "", "", 0x0202228b, "pcvt.w.p x5, p4"},
        {"pcvt.p.w", "p5", "x4", "", "", 0x0202328b, "pcvt.p.w p5, x4"},
        {"plh", "p6", "x7", "", "-2", 0xffe3c30b, "plh p6, -2(x7)"},
        {"psh", "", "x9", "p8", "-4", 0xfe84de0b, "psh p8, -4(x9)"},
    }};
    for (const Case &c : cases)
    {
        Kites::ICUnit block;
        block.setOpcode(c.opcode);
        block.setRd(c.rd);
        block.setRs1(c.rs1);
        block.setRs2(c.rs2);
        block.setImm(c.imm);
        const uint32_t encoded = Kites::generatePositMachineCode(block);
        const std::string decoded = Kites::instruction_set::disassemble(encoded);
        if (encoded != c.expected || decoded != c.disassembly)
        {
            std::cerr << c.opcode << " expected 0x" << std::hex << c.expected
                      << " got 0x" << encoded << std::dec << " disassembles as "
                      << decoded << '\n';
            return 1;
        }
    }
    std::cout << "10 Posit encodings and disassemblies match expected values\n";
}
