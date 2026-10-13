# Pacer Firmware Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build `firmware/pacer.c` firmware that runs the automatic 4-set Pomodoro cycle with shift-register LEDs, break LED, and non-blocking end-of-phase jingles. Includes power-on self-test (all LEDs + startup jingle) and configurable timings (debug seconds now, minutes later).

**Architecture:** Single main state machine (`BOOT` self-test → `FOCUS`/`BREAK`/`LONG_BREAK`) driven by `funSysTick32()` ticks; hardware-SPI 74HC595 driver modeled on `hc595_demo.c` (extended to 24 bits, latch on PC4); tick-polling melody player on top of `tone.h` (TIM2) so countdown starts the same tick the jingle starts. No `Delay_Ms` between notes (unlike `tone_demo.c`).

**Tech Stack:** C, ch32fun (CH32V003), `tone.h` (TIM2 square-wave), 74HC595 via hardware SPI (PC5 SCK / PC6 MOSI / PC4 LATCH).

**Spec:** `README.md` (cycle order), `AGENTS.md` (pins, LED map, 12 MHz HSE). Reference implementations reviewed: `firmware/ch32fun/examples/hc595_demo/hc595_demo.c` (SPI pattern), `firmware/tone.h` + `firmware/tone_demo.c` (buzzer; demo is blocking and must not be copied for melodies).

## Global Constraints

- TARGET_MCU is `CH32V003` (`firmware/Makefile`).
- External crystal is 12 MHz: `funconfig.h` must set `FUNCONF_USE_HSE=1`, `HSE_VALUE=12000000`, `FUNCONF_SYSTEM_CORE_CLOCK=24000000` (12 MHz x PLL Mul2 on V003).
- Wirings (verbatim from spec): `PC7` = break LED, `PC6` = shift data (MOSI), `PC5` = shift clock (SCK), `PC4` = shift latch, `PD4` = buzzer. Note: `hc595_demo.c` uses PC3 as latch example — this project uses PC4.
- LED totals: 20 remaining-time + 4 set-counter + 1 break = 25 LEDs; 24 bits via 3x74HC595 + 1 direct GPIO.
- Bit map (1A→3H): `1A`=Set4, `1B`=Set3, `1C`=Set2, `1D`=Set1, `1E`=remaining rightmost … `3H`=remaining leftmost (bits 0–3 sets, bits 4–23 remaining 0–19 right→left).
- No hardcoded phase lengths: all durations live in `firmware/config.h` (`FOCUS_MS`, `BREAK_MS`, `LONG_BREAK_MS`, `STARTUP_TEST_MS`). Debug defaults: `FOCUS_MS=25000`, `BREAK_MS=5000`, `LONG_BREAK_MS=30000` (25 s / 5 s / 30 s for quick debugging; production minutes documented in comments as 25/5/30 min). Cycle order from README (3x focus+break, 1x focus+long-break, reset to set 1) is unchanged.
- Power-on self-test: on boot, light ALL LEDs (24 shift bits + PC7 HIGH) for `STARTUP_TEST_MS` (debug default 3000 ms) while playing dedicated `JINGLE_STARTUP`; countdown of Set 1 Focus starts only when this sequence ends.
- Countdown starts the same tick the end-of-phase jingle starts (non-blocking sound, no `Delay_Ms` between notes — `tone_demo.c` blocking pattern is forbidden in the melody player).
- Remaining bar starts fully lit (20/20), extinguishes right→left as time elapses.
- Set LEDs show completed sets so far (0–4 lit).
- Break LED PC7 HIGH only in BREAK/LONG_BREAK during normal cycle (exception: also HIGH during BOOT self-test lamp check).
- `tone.h` uses TIM2 — do not use TIM2 for anything else. `melody.h` must not rely on `tone.h` statics (`tone_active` is file-static); it keeps its own `next_note_tick` schedule and calls `tone()` per note.

## Review Focus

- Long-phase SysTick wraparound (30 min in ms fits u32, but `funSysTick32()` wraps ~49 days) — phase compare must use signed `TimeElapsed32()` difference, expect correct expiry across wrap.
- 74HC595 chain order reversed (first byte clocked ends at 3H far end) — expect leftmost LED corresponds to first bit shifted, verified visually/logic-test.
- `tone()` blocking misuse (`Delay_Ms` between notes stalls countdown) — expect countdown tick advances during jingle, verified by melody scheduler test.
- Break LED (PC7) left on into focus or off during break — expect PC7 HIGH only in BREAK/LONG_BREAK (+ BOOT test), verified in transition sim.
- Power-on state is BOOT self-test (all LEDs + startup jingle for STARTUP_TEST_MS), not immediate focus countdown — expect focus `phase_start_tick` == boot_tick + STARTUP_TEST_MS.

---

### Task 1: Clock + project scaffolding + timing config

**Files:**
- Modify: `firmware/funconfig.h`
- Modify: `firmware/Makefile` (TARGET `tone_demo` → `pacer`)
- Create: `firmware/config.h`
- Create: `firmware/pacer.c`

**Interfaces:**
- Consumes: ch32fun `SystemInit()`, `funGpioInitAll()`, `funSysTick32()`, `TimeElapsed32()`.
- Produces: `config.h` with `FOCUS_MS`, `BREAK_MS`, `LONG_BREAK_MS`, `STARTUP_TEST_MS`; buildable `pacer` target for later tasks.

- [ ] **Step 1: Update `funconfig.h` for 12 MHz HSE**

```c
// funconfig.h must contain:
#define FUNCONF_USE_HSE 1
#define HSE_VALUE 12000000
#define FUNCONF_SYSTEM_CORE_CLOCK 24000000
```

- [ ] **Step 2: Update `firmware/Makefile` TARGET to `pacer`**

```makefile
TARGET:=pacer
```

- [ ] **Step 3: Create `firmware/config.h` with debug timings (no hardcoded phase lengths)**

```c
// Debug defaults (quick check): FOCUS_MS 25000, BREAK_MS 5000, LONG_BREAK_MS 30000, STARTUP_TEST_MS 3000
// Production (commented): 25*60*1000, 5*60*1000, 30*60*1000
```

- [ ] **Step 4: Create minimal `firmware/pacer.c` that inits and blinks PC7**

```c
// main(): SystemInit(); funGpioInitAll(); funPinMode(PC7, output); loop toggle PC7 with Delay_Ms(500)
```

- [ ] **Step 5: Verify build passes**

Run: `make clean && make` in `firmware/`
Expected: PASS — produces `pacer.elf`/`pacer.hex`, no errors.

- [ ] **Step 6: Commit**

```bash
git add firmware/funconfig.h firmware/Makefile firmware/config.h firmware/pacer.c
git commit -m "feat: scaffold pacer firmware with 12MHz HSE clock"
```

### Task 2: 74HC595 display driver + LED map

**Files:**
- Create: `firmware/display.h`
- Modify: `firmware/pacer.c` to use it

**Interfaces:**
- Consumes: pins `PC4` (LATCH), `PC5` (SCK), `PC6` (MOSI), `PC7` (BREAK_LED).
- Produces: `void display_init(void); void display_show(uint8_t sets_completed, uint8_t bars_lit, uint8_t break_on); uint32_t display_word(uint8_t sets_completed, uint8_t bars_lit);`

- [ ] **Step 1: Write host-testable spec of `display_word` mapping**

```c
// display_word(0,20) == 0xFFFFF0  (bits 4..23 set, bits 0..3 clear)
// display_word(1,20) == 0xFFFFF8  (bit3 = Set1 + full bar)
// display_word(4,0)  == 0x00000F  (all 4 set bits, no bar)
// display_word(0,0)  == 0x000000
// display_word(0,1)  == 0x800000  (only leftmost remaining bit = bit23)
// display_word(0,19) == 0xFFFFE0  (rightmost remaining bit = bit4 out first)
```

- [ ] **Step 2: Implement `display.h` with SPI + `display_word()` + `display_show()`**

Signature: `void display_init(void); uint32_t display_word(uint8_t sets_completed, uint8_t bars_lit); void display_show(uint8_t sets_completed, uint8_t bars_lit, uint8_t break_on);`
Approach: hardware SPI per `hc595_demo.c:25-46` (SPI1 master, PC5/PC6 AF-PP, `SPI_CPHA_1Edge|SPI_CPOL_Low`, Master, Prescaler_2, NSS_Soft, 8-bit, FullDuplex) but latch on PC4 (not demo PC3); add `hc595_send24()` (3x `SPI_transfer_8` MSB-first, demo only has send8/16); shift MSB-first 24 bits (bit23=3H first, bit0=1A last), pulse latch; `display_word` builds u24: bits0-3 = sets (bit3=Set1 … bit0=Set4: `sets_completed` lights Set1..SetN), bits4-23 = bar anchored at the leftmost LED (bit23=3H): with N bars lit, bits (24-N)..23 are set, so the rightmost LED (bit4=1E) goes out first and the leftmost last.

- [ ] **Step 3: Verify with host gcc logic test (no MCU headers)**

Run: compile a temp `host_test_display.c` that `#include`s the pure `display_word` logic (copy or factor to pure function) with `gcc -o /tmp/dt host_test_display.c && /tmp/dt`
Expected: PASS — all 5 asserts above hold.

- [ ] **Step 4: Verify firmware build still passes**

Run: `make` in `firmware/`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add firmware/display.h firmware/pacer.c
git commit -m "feat: add 74HC595 display driver and LED map"
```

### Task 3: Non-blocking jingle player (distinct focus/break/long-break/startup sounds)

**Files:**
- Create: `firmware/melody.h`
- Modify: `firmware/pacer.c` to poll it

**Interfaces:**
- Consumes: `tone.h` `tone_impl(pin,freq,dur_ms)` / `noTone(pin)`, `funSysTick32()`, buzzer `PD4`.
- Produces: `typedef enum { JINGLE_FOCUS_END, JINGLE_BREAK_END, JINGLE_LONG_BREAK_END, JINGLE_STARTUP } jingle_t; void melody_start(jingle_t j); void melody_poll(void); uint8_t melody_busy(void);`

- [ ] **Step 1: Define jingle note tables (distinct sounds)**

```c
// FOCUS_END (upbeat major arpeggio): C5 523Hz/150ms, E5 659/150, G5 784/300
// BREAK_END (two-note chime): A4 440/200, D5 587/400
// LONG_BREAK_END (fanfare 4 notes): G4 392/150, C5 523/150, E5 659/150, C6 1047/500
// STARTUP (dedicated boot sound, plays during lamp test): E5 659/120, G5 784/120, C6 1047/250
// Gaps between notes: 30ms silence (player stops tone, waits gap, starts next)
```

- [ ] **Step 2: Implement `melody.h` non-blocking scheduler**

Signature: `void melody_start(jingle_t j); void melody_poll(void); uint8_t melody_busy(void);`
Approach: static index + `next_note_tick` (in SysTick ticks via `Ticks_from_Ms`) compared with signed `TimeElapsed32(funSysTick32(), next_note_tick)`; on `melody_start` call `tone(PD4, freq, dur)` for note 0 immediately and schedule note 1 at `now + Ticks_from_Ms(dur+gap)`; `melody_poll` advances through table calling `tone()` per note, sets idle at end. This is the non-blocking fix vs `tone_demo.c` (which blocks on `Delay_Ms` between `tone()` calls): the poller never blocks so the phase countdown keeps advancing. Never calls `Delay_Ms`. Own schedule only — never reads `tone.h` statics.

- [ ] **Step 3: Verify countdown-overlap property by host simulation**

Host test: stub `tone()` to log start-tick, run fake tick loop calling `melody_poll()` while incrementing a fake `phase_elapsed` counter; assert `phase_elapsed` advances during all notes (i.e. no blocking loop) and all note frequencies fire in order for each of the 4 jingles.

- [ ] **Step 4: Verify firmware build passes**

Run: `make` in `firmware/`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add firmware/melody.h firmware/pacer.c
git commit -m "feat: add non-blocking end-of-phase jingles"
```

### Task 4: Pomodoro state machine + main loop

**Files:**
- Modify: `firmware/pacer.c`

**Interfaces:**
- Consumes: `display_show()` from Task 2, `melody_start()/melody_poll()` from Task 3, `FOCUS_MS/BREAK_MS/LONG_BREAK_MS/STARTUP_TEST_MS` from `config.h` (Task 1).
- Produces: auto-running cycle: power-on → BOOT self-test → FOCUS set 1 … → LONG_BREAK → reset; `uint8_t bars_for(uint32_t elapsed_ms, uint32_t total_ms);`

- [ ] **Step 1: Implement `bars_for(elapsed_ms, total_ms)` helper (ms-based, no hardcoded lengths)**

```c
// bars_for(0,25000) == 20
// bars_for(25000,25000) == 0
// bars_for(12500,25000) == 10
// bars_for(24999,25000) == 1  (ceil, last LED out only at expiry)
```

Formula: `(total - elapsed) * 20 + (total - 1)) / total` clamped 0..20, with `elapsed >= total → 0`. Uses 64-bit intermediate to avoid `(total-elapsed)*20` overflow.

- [ ] **Step 2: Implement phase machine in `pacer.c` (with BOOT)**

States: `PH_BOOT`, `PH_FOCUS`, `PH_BREAK`, `PH_LONG_BREAK`; vars `phase` (starts `PH_BOOT`), `set_number` 1..4, `sets_completed` 0..4, `phase_start_tick`, `phase_len_ticks`.
Boot: at power-on record `boot_tick`, `display_show(4, 20, 1)` (all LEDs incl. PC7), `melody_start(JINGLE_STARTUP)`; when `TimeElapsed32(now, boot_tick + Ticks_from_Ms(STARTUP_TEST_MS)) >= 0`, enter FOCUS set 1 (`phase_start_tick = now`, countdown starts as boot sequence ends).
Transitions on `TimeElapsed32(now, phase_start_tick + len) >= 0`: FOCUS→ (if set<4: `melody_start(FOCUS_END)`, enter BREAK len BREAK_MS) else (`melody_start(FOCUS_END)`, enter LONG_BREAK len LONG_BREAK_MS); BREAK→ (`melody_start(BREAK_END)`, `sets_completed=set_number`, `set_number++`, enter FOCUS len FOCUS_MS); LONG_BREAK→ (`melody_start(LONG_BREAK_END)`, `sets_completed=0`, `set_number=1`, enter FOCUS). New `phase_start_tick = now` taken at the same tick as `melody_start` (countdown starts with sound). Break LED (PC7) HIGH only in BREAK/LONG_BREAK (+ BOOT); LOW in FOCUS.

- [ ] **Step 3: Implement 1-second display refresh + melody poll loop**

Loop: `melody_poll()` every iteration; recompute `elapsed_sec`, `bars_lit=bars_for(...)`, `display_show(sets_completed or preview during focus?, bars_lit, break_on)`; refresh at most every 100 ms (or when second changes). During FOCUS show `sets_completed` (completed so far); during BREAK show `sets_completed` including just-finished? Spec: set counter = completed sets — increment when focus finishes (visible during break), decrement/reset after long break.

- [ ] **Step 4: Host-verify `bars_for` + transition table**

Run host gcc test for the 4 `bars_for` asserts + a phase-transition simulation (BOOT → F1,B1,F2,B2,F3,B3,F4,LB → back to F1) asserting lengths from `config.h` debug values (25000/5000/30000), `sets_completed` sequence (BOOT:4 lamp-test, 0,1,1,2,2,3,3,3→0), break-LED flag (1 in BOOT/BREAK/LONG_BREAK, 0 in FOCUS), boot state (all LEDs + STARTUP jingle, focus `phase_start_tick` == boot_tick + STARTUP_TEST_MS), and that every `melody_start` tick == new `phase_start_tick`.
Expected: PASS.

- [ ] **Step 5: Verify firmware build passes**

Run: `make clean && make` in `firmware/`
Expected: PASS — `pacer.hex` produced.

- [ ] **Step 6: Commit**

```bash
git add firmware/pacer.c
git commit -m "feat: add pomodoro state machine and main loop"
```
