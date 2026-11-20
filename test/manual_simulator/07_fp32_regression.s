# Developer-reported GUI verification; date/build and separate evidence were not recorded.
# No FP64 instructions are used.
# Data: a=2.0, b=3.0, c=0.5. Expected:
# f1=a+b=5.0; f2=a*b=6.0; FMA outputs f3=6.5, f4=5.5,
# f5=-5.5, f6=-6.5. FMA result bits: 0x40d00000, 0x40b00000,
# 0xc0b00000, 0xc0d00000 respectively.

.data
values: .float 2.0, 3.0, 0.5
.text
    lui x10, 0x10000
    flw f10, 0(x10)
    flw f11, 4(x10)
    flw f12, 8(x10)

    fadd.s f1, f10, f11
    fmul.s f2, f10, f11
    fmadd.s f3, f10, f11, f12
    fmsub.s f4, f10, f11, f12
    fnmsub.s f5, f10, f11, f12
    fnmadd.s f6, f10, f11, f12

    addi x17, x0, 10
    ecall
