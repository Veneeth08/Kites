# Developer-reported GUI verification; date/build and separate evidence were not recorded.
# Products: +3*4 +2*3 -3*4 -2*3 +2*3 = 6.
# Expected after QROUND: p10=6. The additions/subtractions accumulate exactly
# in Quire; no intermediate Posit result is written.

.text
    addi x1, x0, 3
    addi x2, x0, 4
    addi x3, x0, 2
    pcvt.p.w p1, x1
    pcvt.p.w p2, x2
    pcvt.p.w p3, x3

    qclr
    qma p1, p2
    qma p3, p1
    qms p1, p2
    qms p3, p1
    qma p3, p1
    qround p10

    addi x17, x0, 10
    ecall
