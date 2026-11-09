# Posit Extension Verification Record

This is the cumulative record of Posit extension tests and verification. Each
execution is appended to the history; later runs must not replace earlier
results. A test appearing under “written” was not necessarily executed.

## Test strategy and scope

- Verify Posit16 ES=2 representation independently of the simulator’s IEEE
  floating-point path.
- Cover special values, regime/exponent boundaries, rounding, arithmetic and
  exceptional cases before adding instruction execution.
- Later phases should add instruction-level assembler/encoding/execution tests
  and RVSS/RV5S integration tests. Those phases have not been implemented yet.
- Keep written tests separate from tests actually executed. Record each command,
  outcome, environment, discovered defect, correction and retest below.

## Audit methodology and reference sources

The implementation audit was source-based and read-only. It compared the changed
register code and numerical helpers with the existing register file, RVSS
execution path, ALU, instruction definitions, assembler data, register UI/model,
pipeline state, and CMake source/test discovery. The audit used Git status and
diff inspection to distinguish tracked modifications from untracked files.

Local reference files inspected included `src/processor/registers.h/.cpp`,
`src/processor/rvss/rvss_processor.cpp`, `src/processor/alu.h/.cpp`,
`src/common/instructions.h/.cpp`, RV5S processor/control-unit sources,
`src/ui/register_table/registermodel.cpp`, `src/processor/pipeline_registers.h`,
`CMakeLists.txt`, and `test/CMakeLists.txt`. These were architecture references,
not independent Posit numerical references.

The earlier audit commands included:

```sh
git branch --show-current && git status --short --branch && git diff --stat && git diff --cached --stat
rg --files src test | sort
rg -n "RegisterFile|FADD|FCVT|RType|instruction|RVSS|RV5S|enum.*Instruction|funct7" src
```

Relevant files were then read with `sed`, and register/UI/pipeline call sites
were located with `rg`. The broad architecture search output was truncated, so
specific source files were subsequently inspected directly. No test execution
was part of that audit.

## Environment and dependencies

Latest recorded environment (2026-11-09):

- Compiler: `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`.
- Language mode used for the numerical core check: C++20.
- CMake: unavailable (`cmake` command not found).
- GoogleTest headers: unavailable at `/usr/include/gtest/gtest.h`.
- Boost.Multiprecision header: unavailable at
  `/usr/include/boost/multiprecision/cpp_int.hpp`; the numerical implementation
  does not depend on Boost.
- The project configuration requires Qt 6.9.2. A full project configuration
  or build was not run in this environment.

## Numerical representation tests

### Written tests

The current `test/posit_test.cpp` contains tests for zero, NaR, minpos/maxpos,
negative one, known powers, and round trips for every finite 16-bit encoding.
The all-finite-encoding round-trip test was also exercised by a separate
standalone harness (see execution history).

Exact inputs and expectations in the current GoogleTest source are:

| Test | Inputs | Expected result |
|---|---|---|
| `DecodesSpecialAndBoundaryValues` | `decode(0x0000)`, `decode(0x8000)`, `decode(0x0001)`, `decode(0x7fff)`, `decode(0xc000)` | Zero: non-NaR, significand 0; `0x8000`: NaR; minpos: positive, significand 1, scale -56; maxpos: significand 1, scale 56; negative one: negative, significand 1, scale 0 |
| `EncodesKnownPowersAndSigns` | `encode(false,0,0)`, `encode(false,1,0)`, `encode(false,1,1)`, `encode(false,1,2)`, `encode(false,1,-1)`, `encode(true,1,0)` | `0x0000`, `0x4000`, `0x4800`, `0x5000`, `0x3800`, `0xc000`, respectively |
| `EveryFiniteEncodingRoundTrips` | Every raw value `0x0000..0xffff`, excluding NaR `0x8000` | `encode(decode(raw)) == raw` |
| `RoundsNearestTiesToEven` | `encode(false,4097,-12)`, `encode(false,4099,-12)` | `0x4000` (even lower), `0x4002` (even upper), respectively |
| `HandlesUnderflowOverflowAndNar` | `encode(false,1,-57)`, `encode(false,1,-56)`, `encode(false,1,57)`, `encode(true,1,57)` | `0x0000`, `0x0001`, `0x7fff`, `0x8001`; also checks NaR propagation for `add(0x8000,0x4000)` and `mul(0x8000,0x0000)`, and NaR for `div(0x4000,0x0000)`, `div(0xc000,0x0000)`, `div(0x0000,0x0000)` |
| `ArithmeticUsesExactPositResults` | `add(0x4000,0x4000)`, `sub(0x4800,0x4000)`, `mul(0x4800,0x4800)`, `div(0x4800,0x4800)`, `add(0xc000,0x4000)`, `mul(0xc000,0x4800)`, `div(0x0001,0x7fff)`, `add(0x7fff,0x7fff)` | `0x4800`, `0x4000`, `0x5000`, `0x4000`, `0x0000`, `0xb800`, `0x0000`, `0x7fff`, respectively |

These are written expectations only unless explicitly marked as executed in the
history below.

### Numerical reference

For the initial numerical-core checks, no external Posit conformance suite,
published reference implementation, or independent numerical oracle was used;
the expected constants were selected by hand. The independent audit appended
below later used the 2022 Posit Standard’s field equation and rounding procedure
with exact rational arithmetic. Stillwater Universal source was inspected, but
its executable was not run and its source snapshot was not pinned to an exact
commit.

## Arithmetic tests

### Written tests

The current `test/posit_test.cpp` includes exact examples for addition,
subtraction, multiplication and division, cancellation, negative multiplication,
underflow and finite overflow saturation. These GoogleTests have **not** been
executed because GoogleTest is unavailable in the recorded environment.

## Edge-case tests

### Written tests

The current GoogleTest file covers NaR propagation, division by zero, zero over
zero, half-minpos tie-to-even underflow, minpos, positive and negative overflow,
and ties-to-even at two adjacent-value midpoints. These GoogleTests have **not**
been executed through GoogleTest.

The standalone harness did execute checks for both midpoint cases, underflow,
overflow saturation, division by zero, basic arithmetic, and finite encoding
round trips. It did not execute the written NaR-propagation assertions. See the
exact executed assertions in the execution history summary.

## Instruction-level tests (when implemented)

**Status: NOT IMPLEMENTED / NOT RUN.** No posit instruction definitions,
assembler support, disassembly, or execution paths are present in this phase.

## Integration tests (RVSS and RV5S)

**Status: NOT IMPLEMENTED / NOT RUN.** Posit numerical helpers and the separate
register bank are not yet connected to RVSS or any RV5S variant.

## Execution history

### 2026-11-09 — initial Posit change audit

- **Purpose:** Review the previously created numerical helper and Posit register
  bank; determine what testing had already occurred.
- **Procedure:** Ran the Git branch/status/stat commands and source listing and
  search commands shown in “Audit methodology and reference sources”; read the
  changed files and relevant local architecture files. No test or build command
  was run during that audit.
- **Expected/actual:** The original `test/posit_test.cpp` contained zero/NaR and
  basic arithmetic assertions. Their execution was not established.
- **Original test inputs and expectations (written only):**
  `fromLongDouble(0.0L) == 0x0000`,
  `fromLongDouble(-0.0L) == 0x0000`, and `isnan(toLongDouble(0x8000))`; then
  `one=fromLongDouble(1.0L)`, `two=fromLongDouble(2.0L)`, with decoded results
  expected as 2.0 for `add(one,one)`, 1.0 for `sub(two,one)`, 4.0 for
  `mul(two,two)`, and 1.0 for `div(two,two)`. These tests used host floating
  point and were not executed or retained in the current test file.
- **Result:** **NOT RUN** for tests and build. The file contents were inspected
  only. The original tests were later replaced by the expanded suite described
  above; this historical record is retained.
- **Finding:** The original numerical helper used `long double`, contrary to
  the IEEE-independent design, and mapped infinite intermediate results to
  finite extrema. These issues prompted replacement of the numerical core.
- **Reference sources:** The local architecture files listed above. No external
  Posit standard text, independent reference implementation, or test vectors
  were consulted. Findings about numerical behavior were from code inspection,
  not runtime comparison.

### 2026-11-09 — initial standalone harness failure and correction

- **Purpose:** Exercise finite encode/decode round trips, midpoint rounding and
  basic arithmetic using a temporary C++ assertion harness.
- **Command:**

  ```sh
  g++ -std=c++20 -O2 -Wall -Wextra -Werror -Isrc src/processor/posit.cpp /tmp/posit_core_check.cpp -o /tmp/posit_core_check && /tmp/posit_core_check
  ```

- **Inputs reached before failure:** The loop checked raw patterns `0x0000` to
  `0xffff` inclusive, skipping `0x8000`, and re-encoded each decoded pattern.
  It then asserted `encode(false,4097,-12)==0x4000`,
  `encode(false,4099,-12)==0x4002`, and incorrectly asserted
  `add(0x4000,0x4000)==0x5000`.
- **Expected:** all assertions pass. **Actual:** process aborted at the addition
  assertion (exit status 134). **Result: FAIL.** The expectation encoded 2.0 as
  `0x5000`, while the Posit16 ES=2 bit layout uses `0x4800` for 2.0 and
  `0x5000` for 4.0. This was a test expectation discrepancy, not a source
  correction.
- **Correction and retest:** Updated temporary harness constants for 2.0 and 4.0
  (`0x4800` and `0x5000`) and reran the same compile/run command; the round-trip,
  midpoint and arithmetic assertions then passed (exit status 0). The harness
  was later expanded with the exact additional assertions listed in the final
  verification record below and passed again.
- **Reference:** The corrected expected encoding was manually derived; no
  independent Posit implementation or published test-vector source was used.

### 2026-11-09 — integer numerical core verification

- **Purpose:** Check that the replacement numerical source compiles cleanly and
  that exact representation, rounding and selected arithmetic edge cases work.
- **Environment:** `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`, C++20. No
  external numerical library was used.
- **Source syntax command:**

  ```sh
  g++ -std=c++20 -Wall -Wextra -Werror -Isrc -fsyntax-only src/processor/posit.cpp
  ```

  **Expected:** no diagnostics and exit status 0. **Actual:** no diagnostics,
  exit status 0. **Result: PASS.**

- **Executed standalone harness command:**

  ```sh
  g++ -std=c++20 -O2 -Wall -Wextra -Werror -Isrc src/processor/posit.cpp /tmp/posit_core_check.cpp -o /tmp/posit_core_check && /tmp/posit_core_check
  ```

  The final temporary harness inputs and exact expected results were:

  - For each raw input `0x0000..0xffff`, skip `0x8000`; assert
    `encode(decode(raw)) == raw`.
  - `encode(false,4097,-12) == 0x4000` and
    `encode(false,4099,-12) == 0x4002`.
  - `encode(false,1,1) == 0x4800`, `encode(false,1,2) == 0x5000`,
    `encode(false,1,-1) == 0x3800`, and `encode(true,1,0) == 0xc000`.
  - `encode(false,1,-57) == 0x0000`, `encode(false,1,-56) == 0x0001`,
    `encode(false,1,57) == 0x7fff`, and `encode(true,1,57) == 0x8001`.
  - `add(0x4000,0x4000) == 0x4800`,
    `sub(0x4800,0x4000) == 0x4000`,
    `mul(0x4800,0x4800) == 0x5000`, and
    `div(0x4800,0x4800) == 0x4000`.
  - `div(0x4000,0x0000) == 0x8000`,
    `div(0x0001,0x7fff) == 0x0000`,
    `add(0x7fff,0x7fff) == 0x7fff`, and
    `mul(0xc000,0x4800) == 0xb800`.
  - `decode(0x0001).scale == -56` and `decode(0x7fff).scale == 56`.

  **Expected:** all listed assertions pass. **Actual:** exit status 0.
  **Result: PASS.** This was a standalone C++ assertion harness, not the
  GoogleTest suite. It did not assert NaR propagation for arithmetic operands.

  **Expected:** all assertions pass and exit status 0. **Actual:** exit status
  0. **Result: PASS.** This was a standalone C++ assertion harness, not the
  GoogleTest suite.

- **Whitespace check command:**

  ```sh
  git diff --check
  ```

  **Expected:** no whitespace errors. **Actual:** no output, exit status 0.
  **Result: PASS** for files represented in the tracked Git diff at that time.
  Git does not include untracked files in this check, so it did not verify the
  then-untracked numerical source or test file.

- **Project build and GoogleTest command:** **NOT RUN.** CMake and GoogleTest
  were unavailable, so the project could not be configured/built and the
  focused GoogleTest suite could not be launched.
- **Reference:** No independent implementation or external Posit test-vector
  source was used. The standalone checks were self-authored and do not count as
  an independent oracle.

### 2026-11-09 — verification record formatting check

- **Purpose:** Check the newly created cumulative record for trailing
  whitespace.
- **Command:**

  ```sh
  if rg -n '[[:blank:]]+$' docs/posit-extension/verification.md; then exit 1; else echo 'No trailing whitespace found'; fi
  ```

- **Expected:** no matching lines and exit status 0. **Actual:** “No trailing
  whitespace found”, exit status 0. **Result: PASS.**
- **Scope:** This checks document whitespace only; it does not validate source,
  tests, or numerical behavior.

### 2026-11-09 — cumulative record audit update

- **Purpose:** Compare the stored verification history with the recorded audit
  and test commands, add the initial failed assertion and correction, and list
  exact input/expected-output pairs for written tests and the executed harness.
- **Source/reference:** Prior session command outputs, the temporary
  `/tmp/posit_core_check.cpp` harness, and the current `test/posit_test.cpp`.
  No external numerical source was introduced. No source or test files were
  modified during this documentation audit.
- **Document whitespace command:**

  ```sh
  if rg -n '[[:blank:]]+$' docs/posit-extension/verification.md; then exit 1; else echo 'No trailing whitespace found'; fi
  ```

- **Expected:** no matching lines and exit status 0. **Actual:** “No trailing
  whitespace found”, exit status 0. **Result: PASS.** This verifies document
  whitespace only; it does not retest numerical behavior.

## Known issues and unverified functionality

The bullets below were written before the independent audit appended later in
this document. The latest status and corrections are in “Independent Posit
Standard audit — 2026-11-09”; historical execution records remain unchanged.

- No independent implementation or third-party conformance vectors were run.
  The later audit compared against the normative Standard with an independent
  exact-rational script, but still lacks an executable Stillwater comparison.
- The expanded GoogleTest suite is written but has not been run.
- The full project has not been configured or built in the recorded
  environment.
- Posit assembler/instruction execution, conversions, raw memory operations,
  register display/dumps, RVSS integration and all RV5S integration remain
  unimplemented and unverified.
- At the time of the original standalone checks, no numerical failures were
  known against their self-authored expectations. The later normative audit
  found rounding failures; see the appended audit record.

### Independent Posit Standard audit — 2026-11-09

This audit supplements, and does not erase, the earlier execution records. It
found that some earlier passing assertions encoded the wrong underflow
expectation. A PASS against an incorrect expected value is retained as an
execution fact, but it is not evidence of conformance.

#### Normative and independent references

- **Normative:** *Posit Standard (2022)*, `https://posithub.org/docs/posit_standard-2.pdf`.
  This audit used §3.2–3.3 for value range, NaR and the posit value equation;
  §4.1 for rounding (including projection of every nonzero magnitude below
  minPos to signed minPos, saturation above maxPos, and the n+1-bit `U1`
  midpoint/tie rule); and §5.1/§5.4 for NaR propagation and binary arithmetic.
- **Independent implementation source inspected:** Stillwater Universal,
  `https://github.com/stillwater-sc/universal/blob/main/include/sw/universal/number/posit/posit_impl.hpp`.
  The fetched `main` source was inspected on 2026-11-09, including posit
  decoding, `convert_` projection/guard-round-sticky logic, and arithmetic
  special-case paths. The fetched page did not expose a verifiable full commit
  SHA. Therefore the source review is **not pinned to an exact commit**, and a
  Stillwater executable comparison is **NOT RUN**. The source was a secondary
  cross-check; the 2022 Standard remained normative.
- No external executable, reference vectors, or external libraries were used
  to calculate the recorded oracle outputs. The audit scripts used Python
  `fractions.Fraction` and the Standard’s bit-field equation and §4.1 rounding
  procedure, independently of Kites `decode()` and `encode()`.

#### Environment and exact reference procedure

- Compiler: `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`.
- Reference script runtime: Python 3.12.3.
- CMake, GoogleTest, Clang and an installed Stillwater checkout were
  unavailable. No project build, GoogleTest run, Clang build, MSVC build, or
  Stillwater runtime comparison was performed.
- Temporary audit utilities were placed under `/tmp`; no repository source or
  test code was changed. The reference decoder parsed raw sign/regime/exponent/
  fraction fields and evaluated the Standard’s exact rational equation. For
  rounding, the oracle used exact fractions, the Standard’s minPos/maxPos
  projection rules, and for adjacent positive 16-bit values used the exact
  17-bit posit formed by appending `1` to the lower raw code as §4.1 specifies.

#### Exhaustive representation checks

**A. Decode every raw pattern, including NaR — PASS.**

Command/procedure:

```sh
g++ -std=c++20 -O2 -Wall -Wextra -Werror -Isrc src/processor/posit.cpp /tmp/posit_decode_dump.cpp -o /tmp/posit_decode_dump && /tmp/posit_decode_dump > /tmp/posit_decode_dump.txt && wc -l /tmp/posit_decode_dump.txt
python3 /tmp/posit_standard_decode_check.py
```

Exact input set: every `uint16_t` raw code from `0x0000` through `0xffff`.
Expected: 65,535 finite values match the independent exact rational value and
the sole NaR code `0x8000` is classified NaR. Actual: `checked=65535
mismatches=0`; NaR classification matched; dump contained 65,536 rows. **PASS.**

Independent spot values from the same oracle:

| Raw | Expected exact value/class | Actual comparison |
|---|---|---|
| `0x0000` | zero | matched |
| `0x0001` | `+2^-56` (minPos) | matched |
| `0x4000` | `+1` | matched |
| `0x7fff` | `+2^56` (maxPos) | matched |
| `0x8000` | NaR | matched |
| `0x8001` | `-2^56` (negative maxPos) | matched |
| `0xc000` | `-1` | matched |
| `0xffff` | `-2^-56` (negative minPos) | matched |

The oracle used the Standard equation directly, including the negative-sign
factor; it did not validate Kites decode by reusing Kites encode.

**B. Encode each independently derived finite value — PASS.**

Commands:

```sh
g++ -std=c++20 -O2 -Wall -Wextra -Werror -Isrc src/processor/posit.cpp /tmp/posit_encode_cli.cpp -o /tmp/posit_encode_cli
python3 /tmp/posit_standard_encode_roundtrip.py
```

Exact input set: for each of the 65,535 non-NaR raw codes, the Python oracle
independently evaluated its exact Standard value and passed that rational as an
integer significand and binary scale to Kites `encode()`. Expected output was
the original raw code. Actual: 65,535 tested, zero mismatches. **PASS.** This is
not merely a Kites decode/encode round trip; the input values came from the
independent Standard formula.

#### Rounding checks — FAIL

Command:

```sh
g++ -std=c++20 -O2 -Wall -Wextra -Werror -Isrc src/processor/posit.cpp /tmp/posit_encode_cli.cpp -o /tmp/posit_encode_cli && python3 /tmp/posit_standard_round_check.py
```

Exact input set: all 32,766 adjacent positive intervals from lower raw code
`0x0001` through `0x7ffe`, each tested at the exact 17-bit `U1` value mandated
by §4.1, plus `+2^-57` and `-2^-57`. Expected at each `U1`: lower raw code when
the lower code’s LSB is zero, otherwise upper raw code. Expected for any
nonzero magnitude below minPos: signed minPos. Actual: five mismatches:

| Exact input | Expected raw | Actual raw | Reason |
|---|---:|---:|---|
| `+2^-54` (`U1` for interval `0x0001`–`0x0002`) | `0x0002` | `0x0001` | Kites uses the arithmetic midpoint; Standard `U1` threshold is lower here and odd lower code ties upward |
| `+2^-49` (`U1` for interval `0x0003`–`0x0004`) | `0x0004` | `0x0003` | Same regime-boundary threshold mismatch |
| `+2^51` (`U1` for interval `0x7ffd`–`0x7ffe`) | `0x7ffe` | `0x7ffd` | Same regime-boundary threshold mismatch |
| `+2^-57` | `0x0001` | `0x0000` | Standard projects nonzero values below minPos to minPos |
| `-2^-57` | `0xffff` | `0x0000` | Standard projects nonzero values below minPos to negative minPos |

Actual command summary: `positive midpoint cases checked: 32766`, `below-minpos
cases checked: 2`, `mismatches: 5`; process exit status 1 due to detected
mismatches. **FAIL.** The written tie tests around `1.0` (`encode(false,4097,-12)`
and `encode(false,4099,-12)`) match, but do not cover the regime-dominated
boundary intervals.

This also corrects the earlier underflow expectation in the written tests and
the earlier standalone assertion. `encode(false,1,-57) == 0x0000` was asserted
and passed against that old expectation, but the Standard requires `0x0001`.
Likewise the earlier expected zero for `div(0x0001,0x7fff)` is incorrect; the
Standard requires minPos `0x0001`. Those historical checks are retained but
their conformance interpretation is now **FAIL**.

#### Arithmetic checks — FAIL (sampled pairs; not exhaustive)

The independent arithmetic oracle decoded operands as exact Standard fractions,
computed the mathematical operation exactly, then rounded using §4.1. NaR
inputs were expected to propagate to NaR; division by zero was expected to
return NaR.

Build and run command:

```sh
g++ -std=c++20 -O2 -Wall -Wextra -Werror -Isrc src/processor/posit.cpp /tmp/posit_arith_cli.cpp -o /tmp/posit_arith_cli && python3 /tmp/posit_standard_arithmetic_check.py
```

Exact corpus: 262,144 identity cases, four for each raw code `0x0000..0xffff`
(`A raw 0`, `S raw 0`, `M raw 0x4000`, `D raw 0x4000`), plus 2,500
deterministic random pairs for each of `A`, `S`, `M`, and `D` (10,000 pairs
total), generated with Python `random.Random(20261109)` and `randrange(0x10000)`.
This includes all raw inputs including NaR in identity cases; it is not all
possible binary operand pairs.

Expected: all 272,144 outputs equal the exact rational oracle. Actual: two
mismatches; all other corpus cases matched. Command summary was
`cases=272144 (identity coverage=262144, deterministic pairs=10000)` and
`mismatches=2`; exit status 1. **FAIL.**

| Operation and exact raw inputs | Exact mathematical result | Expected raw | Actual raw |
|---|---:|---:|---:|
| `M(0x7cbb, 0x7fbb)` | `693929276080128` | `0x7ffd` | `0x7ffc` |
| `M(0xfff4, 0xffd0)` | `1/75557863725914323419136` | `0x0001` | `0x0000` |

The first result differs because the exact product lies between the Standard’s
17-bit `U1` threshold and the arithmetic midpoint used by Kites near the upper
regime boundary. The second is a nonzero positive result below minPos and
therefore must project to minPos.

#### Focused sign, cancellation, overflow, NaR and zero-divisor cases

Procedure: `python3 /tmp/posit_targeted_cases.py` fed the exact operation/value
triples below to the compiled `/tmp/posit_arith_cli`. The same inputs were also
run with the UBSan-compiled executable (see integer/UB checks).

| Exact inputs | Expected raw | Actual raw | Status |
|---|---:|---:|---|
| `A(0xc000,0x4000)` (`-1 + 1`) | `0x0000` | `0x0000` | PASS |
| `S(0x4000,0x4000)` (`1 - 1`) | `0x0000` | `0x0000` | PASS |
| `M(0xc000,0x4800)` (`-1 * 2`) | `0xb800` | `0xb800` | PASS |
| `D(0xc000,0x4800)` (`-1 / 2`) | `0xc800` | `0xc800` | PASS |
| `M(0x0001,0x0001)` (minPos squared) | `0x0001` | `0x0000` | FAIL: under-minPos projection |
| `M(0x0001,0xffff)` (minPos × negative minPos) | `0xffff` | `0x0000` | FAIL: negative under-minPos projection |
| `D(0x0001,0x7fff)` (minPos / maxPos) | `0x0001` | `0x0000` | FAIL: under-minPos projection |
| `D(0xffff,0x7fff)` (negative minPos / maxPos) | `0xffff` | `0x0000` | FAIL: negative under-minPos projection |
| `M(0x7fff,0x4800)` (positive overflow) | `0x7fff` | `0x7fff` | PASS |
| `M(0x8001,0x4800)` (negative overflow) | `0x8001` | `0x8001` | PASS |
| `M(0x7cbb,0x7fbb)` (upper regime boundary) | `0x7ffd` | `0x7ffc` | FAIL: rounding threshold |

NaR/division-zero procedure: exact cases
`A(4000,8000)`, `A(8000,4000)`, `S(4000,8000)`, `S(8000,4000)`,
`M(4000,8000)`, `M(8000,4000)`, `D(4000,8000)`, `D(8000,4000)`,
`D(4000,0000)`, `D(c000,0000)`, and `D(0000,0000)`. Expected every result
`0x8000`; actual every result `0x8000`. **PASS.** The first attempt at feeding
this batch used literal backslash-n separators and only supplied its first
case; that procedure error was detected by the row count and corrected to use
actual newline separators before the 11-case result above. The first case by
itself returned `0x8000`, as expected.

#### Integer overflow, undefined behavior and portability

**Strict compile — PASS.** Command:

```sh
g++ -std=c++20 -Wall -Wextra -Werror -Wconversion -Wsign-conversion -pedantic -Isrc -fsyntax-only src/processor/posit.cpp
```

Expected no warnings/errors; actual no diagnostics, exit status 0.

**Undefined-behavior sanitizer on all decode patterns — PASS.** Command:

```sh
g++ -std=c++20 -O1 -g -fsanitize=undefined -fno-sanitize-recover=all -Wall -Wextra -Werror -Isrc src/processor/posit.cpp /tmp/posit_decode_dump.cpp -o /tmp/posit_decode_dump_ubsan && /tmp/posit_decode_dump_ubsan > /tmp/posit_decode_dump_ubsan.txt && wc -l /tmp/posit_decode_dump_ubsan.txt
python3 /tmp/posit_standard_decode_check.py /tmp/posit_decode_dump_ubsan.txt
```

Expected no sanitizer report and 65,536 rows matching the exact oracle. Actual:
65,536 rows, 65,535 finite comparisons, zero value/classification mismatches,
and no UBSan diagnostic. **PASS.**

**Undefined-behavior sanitizer on focused arithmetic — PASS for UB, semantic
cases still have the FAIL results above.** Command:

```sh
g++ -std=c++20 -O1 -g -fsanitize=undefined -fno-sanitize-recover=all -Wall -Wextra -Werror -Isrc src/processor/posit.cpp /tmp/posit_arith_cli.cpp -o /tmp/posit_arith_cli_ubsan && UBSAN_OPTIONS=halt_on_error=1 python3 /tmp/posit_targeted_cases.py /tmp/posit_arith_cli_ubsan
```

Expected no sanitizer diagnostic. Actual: no UBSan diagnostic; the process
exited 0 after printing the same expected-vs-actual rows, including known
rounding failures. **PASS** for absence of detected UB on these cases. This is
not a proof for every possible pair.

**Host-floating-point independence — PASS (source scan).** Command:

```sh
rg -n '\b(float|double|long double)\b|<cmath>|std::(isnan|isfinite|ldexp)' src/processor/posit.cpp src/processor/posit.h
```

Expected no host FP type/math use. Actual no matches; **PASS**. This source scan
does not itself establish numerical correctness.

The implementation uses bounded unsigned 32-bit limbs and exact integer
fractions; no compiler-specific `__int128` or host floating-point type appears
in these two files. For Posit16 operands, decoded significands/scales keep the
largest multiply/add/rounding intermediates below the 256-bit `Big` capacity.
The tested builds emitted no conversion/sign warnings and UBSan found no
undefined behavior in the checked inputs. Unsigned wrap is defined by C++, but
the fixed-width arithmetic still has a static range argument rather than a
formal proof or exhaustive check of all binary operand pairs. Clang/MSVC and
other platform builds are **NOT RUN**.

#### Stillwater and full-suite status

- Stillwater Universal source inspection at the mutable `main` URL found
  decode special-case handling, minpos/maxpos inward projection, guard/round/
  sticky rounding, NaR propagation, and divide-by-zero-to-NaR paths consistent
  with the normative rules at a structural level. **Source review: performed.**
- Exact Universal commit identification and executable comparison:
  **NOT RUN / unavailable** in this environment. The GitHub source page did not
  provide a verifiable full SHA, and no checkout/compiler setup was available.
- Project build and Kites GoogleTest suite: **NOT RUN** (CMake and GoogleTest
  unavailable).
- Exhaustive binary arithmetic over all `65,536 × 65,536` operand pairs for
  each operator: **NOT RUN**. Arithmetic coverage above is identity coverage
  for every raw value plus deterministic sampling.

#### Audit discrepancies and current status

1. **Confirmed defect — under-minPos rounding.** Kites `roundFraction()` can
   return zero for nonzero magnitudes below half-minPos, but Posit Standard
   (2022) §4.1 requires signed minPos for every nonzero magnitude below minPos.
   Reproductions: `encode(false,1,-57)` returns `0x0000`, expected `0x0001`;
   `encode(true,1,-57)` returns `0x0000`, expected `0xffff`;
   `M(0x0001,0x0001)` returns `0x0000`, expected `0x0001`.
2. **Confirmed defect — regime-boundary rounding.** Kites compares against the
   arithmetic midpoint of adjacent values. At regime-dominated boundaries the
   Standard’s n+1-bit `U1` threshold is not the arithmetic midpoint. Exact
   failures are listed in the rounding table; arithmetic reproduction is
   `M(0x7cbb,0x7fbb)`, expected `0x7ffd`, actual `0x7ffc`.
3. **No decode discrepancy found** across all raw patterns against the
   independent Standard formula, including NaR.
4. **No mismatch found** in the independent encoding check for all 65,535
   finite exact values. This does not negate the rounding failures for
   in-between values.
5. **No NaR/division-zero discrepancy found** in the focused two-sided tests.
6. A temporary targeted case initially mislabeled raw `0x8001` as negative
   minPos; it is negative maxPos. The input was corrected to `0xffff` for
   negative minPos, and all resulting expected values were recomputed from the
   Standard formula. This was an audit-input error, not a Kites defect.

The current source and test files remain unchanged by this audit. The current
GoogleTest file still contains the nonconforming underflow expectations noted
above; its tests remain written but **NOT RUN**. No source/test correction was
made because this audit was read-only.

**Verification-document whitespace check:**

```sh
if rg -n '[[:blank:]]+$' docs/posit-extension/verification.md; then exit 1; else echo 'No trailing whitespace found'; fi
```

Expected no matches and exit status 0. Actual: printed “No trailing whitespace
found”, exit status 0. **PASS.** This check only covers the verification
document.

## Follow-up audit: negative U1 boundary rounding

**Audit scope:** read-only review of the negative side of Section 4.1 rounding
after the numerical-core correction. No source, test, or reference code was
changed during this follow-up. The normative reference is the *Posit Standard
(2022)*, §4.1. The calculations below use exact rational values decoded from
raw fields; the current C++ encode adapter was used only to obtain actual Kites
outputs.

### Original negative U1 mismatch

The previously reported case was the interval `u=0x8002`, `w=0x8003`, with
expected `0x8002` and actual `0x8003` from the earlier arithmetic-midpoint
rounder. The raw 16-bit endpoint values and appended threshold are:

| Item | Exact value / encoding |
|---|---:|
| `u=0x8002` | `-2^52` |
| `w=0x8003` | `-2^50` |
| `U1=(u<<1)|1`, as 17 bits | `0x10005` |
| Standard decode of `U1` | `-2^51` |
| Arithmetic midpoint `(u+w)/2` | `-5 × 2^49` |

At `x=-2^51`, the Standard's threshold is reached exactly. The lower numeric
endpoint's raw encoding `u=0x8002` is even, so §4.1 selects `u`. Expected:
`0x8002`. The previous implementation compared against the arithmetic
midpoint instead. Since `-2^51` is numerically greater than `-5×2^49`, that
comparison selected the upper numeric endpoint `w=0x8003`. This explains the
reported mismatch without changing its expected value.

### U1 threshold versus arithmetic midpoint

Section 4.1 defines the rounding boundary using `U1`, the `(n+1)`-bit posit
formed by appending `1` to the actual `n`-bit encoding `U` of the lower
numeric endpoint. The rule compares `x` to the exact value represented by
`U1`; at equality, the LSB of `U` chooses the endpoint. It does not generally
define the boundary as `(u+w)/2`. In the `0x8002` example, the U1 threshold is
`-2^51`, while the arithmetic midpoint is `-5×2^49`; they differ, and the
old midpoint comparison therefore chose the wrong endpoint.

### Reference-checker changes and history limitation

The surviving `/tmp/posit_standard_round_check.py` was a pre-adaptation
positive-only checker. Comparing that file to
`test/posit_reference/posit_standard_round_check.py`, the preserved checker
was changed to:

1. Decode the supplied raw posit at the requested bit width using the signed
   Standard value equation (`standard_decode(raw, nbits)`).
2. For every negative interval, construct its actual lower numeric endpoint
   `negative_u`, append `1` to that literal 16-bit encoding, and decode the
   resulting 17-bit `negative_u1` directly.
3. Apply the tie decision directly to `negative_u`: select it when its raw LSB
   is zero, otherwise select the adjacent upper numeric endpoint.
4. Accept the C++ adapter path as a command-line argument, assert the adapter
   returned one line per input, and report positive and negative counts
   separately.

The negative expected values in this checker are therefore computed from the
negative raw U1 encoding and its own LSB rule. They are not generated merely
by negating positive outputs. The positive and negative U1 threshold values
are symmetric in this representation: for adjacent positive encodings `p`
and `p+1`, the matching negative lower encoding is the two's-complement code
of `p+1`; its appended 17-bit U1 code is the 17-bit two's-complement code of
the positive U1 code for `p`. The LSB decision maps consistently because
two's-complement negation preserves parity, while the numeric endpoint order
reverses. This is an equivalence check; the negative reference cases were
still directly constructed and decoded.

The exact failing negative-checker version from the earlier report could not
be recovered: it was untracked, is absent from Git history, and no separate
copy remains in `/tmp`. Thus the exact source-level edits between that
particular failing run and the later passing run cannot be established. The
diff from the surviving positive-only `/tmp` checker to the preserved current
checker is available and is described above. This limitation does not alter
the direct recomputation and current-run results below.

### Direct negative boundary examples

Each `U1` below was formed by appending `1` to the displayed 16-bit `u`.
Expected outputs use the directly decoded negative threshold and §4.1's LSB
rule. Actual outputs were obtained by encoding that exact rational input with
the C++ adapter; no expected value was derived from Kites output.

| `u` | `w` | 17-bit `U1` | Exact `x=decode(U1)` | Expected | Actual | Status |
|---|---|---|---:|---:|---:|---|
| `0x8002` | `0x8003` | `0x10005` | `-2^51` | `0x8002` | `0x8002` | PASS |
| `0x8003` | `0x8004` | `0x10007` | `-2^49` | `0x8004` | `0x8004` | PASS |
| `0x8004` | `0x8005` | `0x10009` | `-3×2^46` | `0x8004` | `0x8004` | PASS |
| `0x801f` | `0x8020` | `0x1003f` | `-9×2^33` | `0x8020` | `0x8020` | PASS |
| `0x83ff` | `0x8400` | `0x107ff` | `-65,792` | `0x8400` | `0x8400` | PASS |
| `0x87ff` | `0x8800` | `0x10fff` | `-4,104` | `0x8800` | `0x8800` | PASS |

### Complete U1 checker run

**Command executed:**

```sh
python3 test/posit_reference/posit_standard_round_check.py /tmp/posit-reference-build/posit_encode_cli
```

The checker independently constructs and tests **32,766 positive U1
thresholds** and **32,766 negative U1 thresholds**, plus **2 signed
under-minpos cases** (`+2^-57` expected `0x0001`; `-2^-57` expected
`0xffff`). Expected output at each U1 threshold is determined from the exact
17-bit Standard decode and the actual lower endpoint's LSB. Actual command
output:

```text
positive U1 thresholds checked: 32766
negative U1 thresholds checked: 32766
signed under-minpos cases checked: 2
total cases=65534 mismatches=0
```

Expected: zero mismatches across all 65,534 cases. Actual: zero mismatches;
the command exited successfully. **PASS.** This records a written-and-executed
standalone Python oracle check and C++ adapter comparison. It is separate from
the GoogleTest suite, which remains **NOT RUN** pending its build dependencies.

### Environment and procedure

This follow-up ran in WSL Ubuntu 24.04.4, x86_64, Python 3.12.3, and GCC/G++
13.3.0. The adapter binary was the existing
`/tmp/posit-reference-build/posit_encode_cli`, compiled from
`src/processor/posit.cpp` and `test/posit_reference/posit_encode_cli.cpp`
with `g++ -std=c++20 -O2 -Wall -Wextra -Werror -Isrc`. The audit procedure
also directly decoded the six listed 17-bit negative U1 values with Python
`fractions.Fraction` and compared the exact inputs' adapter outputs. These
were read-only checks; no dependency installation or project build was
performed for this follow-up.
