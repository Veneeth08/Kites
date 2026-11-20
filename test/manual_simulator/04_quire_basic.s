# Developer-reported GUI verification; date/build and separate evidence were not recorded.
# Expected: QCLR sets Q=0; QMA adds 2*3=6; QMS subtracts 1*2=2;
# QROUND writes 4 to p4. The Quire itself may not be shown in the GUI.

.text
    addi x1, x0, 2
    addi x2, x0, 3
    addi x3, x0, 1
    pcvt.p.w p1, x1
    pcvt.p.w p2, x2
    pcvt.p.w p3, x3

    qclr
    qma p1, p2
    qms p3, p1
    qround p4

    addi x17, x0, 10
    ecall
