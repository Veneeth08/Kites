# Developer-reported GUI verification; date/build and separate evidence were not recorded.
# Load this file, then use the GUI's
# existing Step, Undo and Redo actions. Do not use Run before stepping.
# After setup, step QCLR, QMA, QROUND, QMS, QROUND, QCLR, QROUND individually.
# The sequence reaches Q=6 (p10 checkpoint), Q=4 (p11), then Q=0 (p12).
# Use Undo/Redo on the QMA, QMS and final QCLR instructions and confirm the
# instruction pointer/history actions. Quire itself cannot be read directly
# in the GUI; earlier QROUND checkpoints show the corresponding values, but
# do not independently expose the full hidden Quire snapshot after Undo/Redo.

.text
    addi x1, x0, 2
    addi x2, x0, 3
    addi x3, x0, 1
    pcvt.p.w p1, x1
    pcvt.p.w p2, x2
    pcvt.p.w p3, x3

    qclr
    qma p1, p2
    qround p10
    qms p1, p3
    qround p11
    qclr
    qround p12

    addi x17, x0, 10
    ecall
