# Reconstructed replacement for the previously manually verified conversions test.
# Expected: p1=6; f1=6.0 binary32 (0x40c00000); p2=6; x3=6.

.text
    addi x1, x0, 6
    pcvt.p.w p1, x1
    pcvt.s.p f1, p1
    pcvt.p.s p2, f1
    pcvt.w.p x3, p2

    addi x17, x0, 10
    ecall
