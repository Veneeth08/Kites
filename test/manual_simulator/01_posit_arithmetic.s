# Previously manually executed successfully in the Kites GUI.
# Expected: p3=3, p4=1, p5=2, p6=2; x3=3.

.text
    addi x1, x0, 1
    addi x2, x0, 2
    pcvt.p.w p1, x1
    pcvt.p.w p2, x2
    padd p3, p1, p2
    psub p4, p2, p1
    pmul p5, p1, p2
    pdiv p6, p2, p1
    pcvt.w.p x3, p3

    addi x17, x0, 10
    ecall
