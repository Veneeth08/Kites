# Posit16 and Quire in Kites

## Overview

Kites provides Posit16 arithmetic and a Quire256 accumulator as custom instructions for the RVSS simulator. The original project proposal included both Posit16 and Posit32, with a Quire as a stretch goal. The implementation currently available in Kites is Posit16 with ES=2 and Quire256; Posit32 remains future work.

## Features

- Posit16 arithmetic, integer and binary32 conversions, and halfword memory operations.
- Fourteen custom instructions: ten Posit operations and four Quire operations.
- Thirty-two independent Posit processor registers (PPRs, `p0`–`p31`) with ABI-style aliases.
- RVSS execution, assembler and disassembler support, and reversible architectural state changes.
- PPR register display and editing in the GUI, with syntax highlighting and editor completion.
- Quire accumulation with exact products and rounding back to Posit16 on request.

## Using Posit instructions

Posit registers use `p0` through `p31`; aliases such as `pa0`, `ps0`, and `pt0` are also accepted. Conversion instructions transfer values between Posit, signed integer, and binary32 registers. `PLH` and `PSH` load and store raw 16-bit Posit encodings using a signed byte offset from a GPR.

```asm
# Convert signed integers, add their Posit values, and convert back
addi x5, x0, 2
addi x6, x0, 3
pcvt.p.w p1, x5
pcvt.p.w p2, x6
padd p3, p1, p2
pcvt.w.p x7, p3
```

`PADD`, `PSUB`, `PMUL`, and `PDIV` round each result back to Posit16. Use conversion instructions to exchange data with integer or binary32 code.

## Using the Quire

The Quire holds one running product sum. `QCLR` clears it, `QMA` adds a Posit product, `QMS` subtracts a product, and `QROUND` writes the rounded Posit16 result to a PPR register.

```asm
# Accumulate 2*3 + 3*4, then round once to Posit16 (18)
addi x5, x0, 2
addi x6, x0, 3
addi x8, x0, 4
pcvt.p.w p1, x5
pcvt.p.w p2, x6
pcvt.p.w p3, x8
qclr
qma p1, p2
qma p2, p3
qround p4
```

Ordinary Posit arithmetic rounds every operation result to Posit16. Quire accumulation retains product information at higher precision and rounds when `QROUND` executes. This can reduce intermediate rounding error in dot products, weighted sums, and similar product accumulations. A Quire does not automatically improve computations that do not benefit from accumulating products before rounding.

## Where this is useful

Posit arithmetic can be useful when a program needs a compact tapered-precision format and its range/precision tradeoff fits the data. Quire instructions can help numerical reductions that sum many Posit products. The examples in `test/manual_simulator/` demonstrate arithmetic, conversions, memory access, accumulation, and simulator history.

## Limitations

- Posit32 is not implemented.
- Posit and Quire instruction execution is implemented in RVSS, not RV5S.
- There is no Posit fused multiply-add instruction; QMA/QMS provide product accumulation through the separate Quire state.
- The GUI displays and edits PPR registers but has no dedicated Quire display. `QROUND` can expose the rounded accumulator value in a PPR.
- Posit-to-integer conversion returns `0x80000000` for NaR or an out-of-range result; that bit pattern also represents valid `INT32_MIN` input for integer-to-Posit conversion.

## Build and test

Configure and build the test-only target, then run the suite:

```sh
cmake --preset test
cmake --build --preset test-only
ctest --preset test-only
```

To select Posit and Quire tests:

```sh
ctest --preset test-only --output-on-failure -R 'Posit|Quire|posit|quire'
```

Independent numerical reference checks are documented in `test/posit_reference/README.md`. The seven manual simulator programs and their developer-reported GUI verification status are described in `test/manual_simulator/README.md`; this status is separate from automated test results.

## Further information

The source and test files contain detailed encoding, rounding, and edge-case behavior. The chronological Kites development log is maintained separately outside this repository. In the latest recorded full-suite comparison, the current tree had 353 tests registered: 344 passed, five pre-existing `FpFmaConversion.DoublePrecisionFma` cases failed, four were disabled, and one was skipped. The same five failures occurred on the committed baseline. A separate `FpFmaConversion.SinglePrecisionFma` selection passed 5/5; this does not establish exhaustive FP32 verification.
