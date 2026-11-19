# Posit16 in Kites

Kites supports Posit16 (ES=2) arithmetic and conversion in RVSS. The 16-bit format gives programs compact real values, with conversions to and from binary32 and signed integers.

## Implementation and validation

The Posit numerical class was implemented first, then checked with exact-rational reference tests against the 2022 Posit Standard. Testing caught an early rounding error: arithmetic midpoints do not always match the Standard. Section 4.1 uses the value of the appended n+1-bit U1 posit and U's low bit to decide the rounded result. The implementation and checks were corrected for positive and negative boundaries.

Ten instructions use custom-0 (`0x0b`): `PADD`, `PSUB`, `PMUL`, and `PDIV`; `PCVT.S.P`, `PCVT.P.S`, `PCVT.W.P`, and `PCVT.P.W`; and `PLH` and `PSH`. Arithmetic and conversion operations are distinguished by `funct3` and `funct7`; the memory instructions use I- and S-type immediate layouts. Custom-1 (`0x2b`), custom-2 (`0x5b`), and custom-3 (`0x7b`) remain unused for future instructions.

All ten instructions are integrated into RVSS, enabling compact Posit arithmetic and exchange of values with binary32 and integer code. The full GUI build and demo execution remain to be verified in an environment with Qt 6.9.2.

## Next steps

Add Quire and fused multiply-add (FMA), implement the Posit instructions in RV5S, and consider Posit32 as a future extension.

See [the assembly demo](../examples/posit16_demo.s).
