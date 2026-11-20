# Manual Simulator Verification

This directory contains short assembly programs for checking Posit16, Quire,
and binary32 behavior in Kites. All seven programs have now been executed in
the Kites GUI by the project developer, and the observed register and memory
results matched the documented expectations. Programs 02 and 03 remain
identified as **reconstructed replacements** because their original source
files were not recovered; their verification applies to the current programs.

## Common assembly structure

Each program is an independent source file beginning in `.text`, or in
`.data` followed by `.text` when it has literal data. Kites accepts these
section directives and the `.float` directive used by test 07. A label is not
required to establish the program entry point: execution begins at the first
assembled text instruction. `.globl` and `.entry` are not used because the
Kites parser does not list them among its supported directives. Programs use
explicit `xN`, `fN`, and `pN` register names and Kites' custom Posit/Quire
mnemonics. They end with `addi x17, x0, 10` and `ecall`, which is the exit
sequence used by the existing Posit demo and RVSS syscall implementation.

## Launch and run

Launch the Kites GUI using the project's installed application or the built
executable (for this checkout, typically `./build/debug/Kites` from the
repository root). In the GUI, open a `.s` file from this directory using the
application's Open File action, assemble it, then use Run to execute it. For
test 06, use the GUI's Step, Undo, and Redo actions as described below.

If the GUI reports `Failed to parse file` for the Quire mnemonics, rebuild the
GUI target from the current checkout:

```sh
cmake --build build/debug --target Kites -j4
```

The configured GUI target is `Kites` (`DISABLE_GUI=OFF`). Its executable was
older than the Quire assembler registration; the target has now been rebuilt
and links the current `kites_core`. Rebuilding does not count as manually
verifying a program.

Kites also provides a documented headless runner. From the repository root,
after building `kites-cli`, run an individual program with:

```sh
./build/debug/kites-cli test/manual_simulator/04_quire_basic.s --vm rvss
```

Replace `04_quire_basic.s` with another filename to run that program. The CLI
does not provide interactive step, Undo, or Redo, and its register dump omits
PPR and Quire state. It also currently crashes in this checkout; see the
limitations below. Use the GUI to inspect PPR values and QROUND checkpoints. See
[`docs/CLI.md`](../../docs/CLI.md) for build and CLI details.

The assembly files exit with the simulator's RVSS exit syscall. Before each
GUI run, load the file and assemble it in the normal editor workflow. The GUI
register table shows GPR, FPR and PPR banks. There is no dedicated Quire view;
observe a Quire value by executing `qround` into a PPR register.

## Test index and verification record

| # | Program | Category | Status | Expected and observed result |
|---|---|---|---|---|
| 01 | `01_posit_arithmetic.s` | Posit16 PADD, PSUB, PMUL, PDIV | Manually verified | p3=3, p4=1, p5=2, p6=2; x3=3. Observed values matched. |
| 02 | `02_posit_conversions.s` | Posit16 integer and binary32 conversions | Manually verified (reconstructed replacement; category previously verified) | p1=6 (`0x5400`), f1=6.0 (`0x40c00000`), p2=6 (`0x5400`), x3=6. Observed values matched. |
| 03 | `03_posit_memory.s` | Posit16 PLH and PSH | Manually verified (reconstructed replacement) | p1=6, p2=6; address 256 contains Posit16(6) = `0x5400` = `0101010000000000`, stored as bytes `00 54` (little-endian). Observed values matched. |
| 04 | `04_quire_basic.s` | QCLR, QMA, QMS, QROUND | Manually verified | Q: 0 → 6 → 4; p1=2 (`0x4800`), p2=3 (`0x4c00`), p3=1 (`0x4000`), p4=4 (`0x5000`). Observed values matched. |
| 05 | `05_quire_accumulation.s` | Accumulation and cancellation | Manually verified | `3*4 + 2*3 - 3*4 - 2*3 + 2*3 = 6`; p10=6 (`0x5400`). Observed value matched. |
| 06 | `06_quire_undo_redo.s` | Quire instruction history | Manually verified | QROUND checkpoints: p10=6 (`0x5400`); p11=4 (`0x5000`); p12=0 (`0x0000`). Observed checkpoints matched. |
| 07 | `07_fp32_regression.s` | FP32 arithmetic and all four FP32 FMA forms | Manually verified | f1=5.0 (`0x40a00000`), f2=6.0 (`0x40c00000`), f3=6.5 (`0x40d00000`), f4=5.5 (`0x40b00000`), f5=-5.5 (`0xc0b00000`), f6=-6.5 (`0xc0d00000`). Observed values matched. |

These runs are simulator-level functional verification of the listed programs.
They do not independently establish every internal Quire arithmetic,
rounding, or fused-operation detail; those require the separate numerical and
automated reference tests.

## How to check each program

- **01:** inspect PPR p3–p6 and GPR x3. The integer conversions provide an
  independent visible check for the result 3.
- **02:** inspect PPR p1/p2, FPR f1, and GPR x3. The binary32 bit pattern for
  6.0 is `0x40c00000`.
- **03:** inspect p1 and p2 (both represent 6) and memory at byte address 256:
  Posit bits `0101010000000000`, hex `0x5400`, bytes `00 54` little-endian.
- **04:** assemble/run after rebuilding the GUI if needed. Inspect p1=2
  (`0x4800`), p2=3 (`0x4c00`), p3=1 (`0x4000`), and p4=4 (`0x5000`) after
  QROUND. The accumulator progression is zero, six, then four.
- **05:** inspect PPR p10 after QROUND; cancellation leaves 6.
- **06:** load/assemble the file, then Step six times for setup. Step QCLR,
  QMA, QROUND; p10 should be 6 (`0x5400`). Step QMS, QROUND; p11 should be 4
  (`0x5000`). Step QCLR, QROUND; p12 should be zero.

  For a QCLR history probe at the end, press Undo twice (QROUND p12, then
  QCLR), Redo once (QCLR), then Step once (QROUND p12). For a separate QMS
  probe, reload and assemble, step six setup instructions and the seven Quire
  instructions to the end, then Undo four times (QROUND p12, QCLR, QROUND p11,
  QMS), Redo once (QMS), and Step once (QROUND p11). For a separate QMA probe,
  reload and assemble, step to the end, Undo six times (QROUND p12, QCLR,
  QROUND p11, QMS, QROUND p10, QMA), Redo once (QMA), then Step once (QROUND
  p10). The GUI has no Quire display: PPR checkpoints show values after
  QROUND, while Undo/Redo directly exposes only PC and history movement. This
  GUI procedure cannot inspect the complete 256-bit restored value.
- **07:** inspect the listed FPRs. This checks representative FP32 arithmetic
  and `fmadd.s`, `fmsub.s`, `fnmsub.s`, and `fnmadd.s`; it does not test FP64.

## Manual run log

The project developer reports that all seven programs were run in the Kites
GUI and that the observed register and memory values matched the expected
results. The run date, exact GUI build identifier, and separate evidence files
were not recorded here. These results document simulator-level functional
verification of the current programs.

| Program | Run date / build | Actual results and observations | Evidence / follow-up |
|---|---|---|---|
| 01 | Kites GUI (date/build not recorded) | p3=3, p4=1, p5=2, p6=2; x3=3; matched expected values. | Developer-reported manual run; no separate artifact recorded. |
| 02 | Kites GUI (date/build not recorded) | p1=6 (`0x5400`), f1=6.0 (`0x40c00000`), p2=6 (`0x5400`), x3=6; matched expected values. | Developer-reported manual run of reconstructed replacement; no separate artifact recorded. |
| 03 | Kites GUI (date/build not recorded) | p1=p2=6; address 256 contains `0x5400` (bytes `00 54`, little-endian); matched expected values. | Developer-reported manual run of reconstructed replacement; no separate artifact recorded. |
| 04 | Kites GUI (date/build not recorded) | Q progression 0 → 6 → 4; p4=4 (`0x5000`); matched expected values. | Developer-reported manual run; no separate artifact recorded. |
| 05 | Kites GUI (date/build not recorded) | Accumulation/cancellation result p10=6 (`0x5400`); matched expected value. | Developer-reported manual run; no separate artifact recorded. |
| 06 | Kites GUI (date/build not recorded) | QROUND checkpoints p10=6 (`0x5400`), p11=4 (`0x5000`), p12=0 (`0x0000`); matched expected values. | Developer-reported manual run; no separate artifact recorded. |
| 07 | Kites GUI (date/build not recorded) | f1=5.0 (`0x40a00000`), f2=6.0 (`0x40c00000`), f3=6.5 (`0x40d00000`), f4=5.5 (`0x40b00000`), f5=-5.5 (`0xc0b00000`), f6=-6.5 (`0xc0d00000`); matched expected values. | Developer-reported manual run; no separate artifact recorded. |

## Limitations

- Quire is architectural state but has no dedicated GUI register display. Use
  QROUND checkpoints to make its current value visible in a PPR register.
- The CLI is noninteractive and its current dump does not include PPR or
  Quire; it cannot replace GUI inspection for these checks.
- The CLI has a separate unresolved segmentation fault: after rebuilding its
  target, it exited with status 139 on all seven manual programs and also on a
  no-argument invocation. No parser diagnostic was emitted for those runs.
  Use the GUI for manual verification; the CLI crash does not establish either
  an assembly failure or a passing run.
- The original conversion program for Test 02 was not recovered; that source is
  labeled as a reconstructed replacement. Test 03's replacement has since
  been manually run, with the observed address 256 value documented above.
