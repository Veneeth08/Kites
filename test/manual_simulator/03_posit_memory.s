# Reconstructed replacement for the previously manually verified PLH/PSH test.
# Expected: Posit16(6) = 0x5400 = 0101010000000000; memory bytes are
# 00 54 at addresses 256..257 (little-endian). p1=6 and p2=6.

.text
    addi x1, x0, 6
    addi x10, x0, 256
    pcvt.p.w p1, x1
    psh p1, 0(x10)
    plh p2, 0(x10)
    pcvt.w.p x2, p2

    addi x17, x0, 10
    ecall
