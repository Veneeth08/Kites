# Posit16 (ES=2) RVSS integration demonstration.
# Expected: p8 = 6, p9 = 6, p10 = 6, p11 = 6, x12 = 6.
# Memory at byte address 256 contains the 16-bit Posit encoding of 6 (0x5800).

    addi x1, x0, 6
    addi x2, x0, 2

    # Signed integer -> Posit, then exercise all four arithmetic operations.
    pcvt.p.w p1, x1             # p1 = 6
    pcvt.p.w p2, x2             # p2 = 2
    padd p3, p1, p2             # p3 = 8
    psub p4, p3, p2             # p4 = 6
    pmul p5, p2, p2             # p5 = 4
    pdiv p6, p4, p2             # p6 = 3

    # Round-trip the binary32 conversion path through f1.
    pcvt.s.p f1, p4             # f1 = binary32 6.0
    pcvt.p.s p7, f1             # p7 = 6

    # Posit -> signed integer.
    pcvt.w.p x12, p7            # x12 = 6

    # Persist and reload the result using the raw 16-bit memory operations.
    addi x10, x0, 256
    psh p7, 0(x10)              # memory[256..257] = p7, little-endian
    plh p8, 0(x10)              # p8 = 6

    # Combined check: (6 + 2 - 2) * 2 / 2 = 6.
    padd p9, p8, p2             # p9 = 8
    psub p9, p9, p2             # p9 = 6
    pmul p9, p9, p2             # p9 = 12
    pdiv p9, p9, p2             # p9 = 6
    padd p10, p9, p0            # preserve final result in a second register
    padd p11, p10, p0           # and a third for convenient GUI inspection

    addi x10, x0, 0
    addi x17, x0, 10             # RVSS exit syscall
    ecall
