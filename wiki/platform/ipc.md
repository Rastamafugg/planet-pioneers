# Inter-Process Communication

Patterns for multi-process architecture on NitrOS-9 EOU. Derived from the [stocks-and-bonds reference repo](../sources/stocks-and-bonds.md). Planet Pioneers will use these to split the game into a **logic process + render child + sound child**, primarily to relieve the 64 KB per-process [memory budget](memory.md).

## Building blocks

### C signal intercept

The DCC libc `intercept(handler)` registers a C function as the recipient of signals delivered via `F$Send`. The handler runs in interrupt-ish context — keep it tiny:

```c
static int lastsig;
static int hitcount;

static sigtrap(sig) int sig; {
    lastsig = sig;
    ++hitcount;
    return 0;
}
```

A wrapper exposes three actions to caller code: **install**, **read-and-clear**, **clear**. See `D:\retro\stocks-and-bonds\src\c\SLPICPT.c` for the canonical implementation; we will port it directly as `slpicpt.c` in Planet Pioneers.

Limits:
- Single static buffer → one active intercept per process.
- Handler should not touch globals beyond the two counters or call non-reentrant libc.

### Syscalls used

| Code | Name | Use |
|------|------|-----|
| $03  | F$Fork  | Spawn child process (load module + run) |
| $04  | F$Wait  | Reap exited child; status in B |
| $08  | F$Send  | Send signal `B` to PID `A` |
| $0A  | F$Sleep | Sleep `X` ticks; signal interrupts the sleep |
| $0C  | F$ID    | Own PID in A |

### Discipline: signal = wakeup, payload elsewhere

Signals carry **only the sig number** (one byte). Always store the actual command/data in a separately readable region (file, data module, or mapped block) and use the signal purely as the "go check the mailbox" knock.

## Shared-state transports

Three confirmed candidates, each appropriate for different rates:

| Transport | Rate | Use for | Notes |
|-----------|------|---------|-------|
| **Disk file** with full-file or section locking | low (turn boundaries, rare events) | Sound commands; shutdown; coarse state sync | Pattern from stocks-and-bonds. Locks confirmed working on EOU (user-confirmed). Cheap to build, slow per op. |
| **`F$AllRAM` + `F$MapBlk` by block number** ⭐ recommended baseline | high (per-frame) | Frame state visible to render child; sound queue | Bearer-style block IDs (confirmed against [`fallram.asm`](D:\retro\nitros9\level2\modules\kernel\fallram.asm) and [`fmapblk.asm`](D:\retro\nitros9\level2\modules\kernel\fmapblk.asm)) — anyone with the block number can map it. |
| **Runtime-built memory module** (`F$AllRAM` → write header + body + CRC → `F$VModul` → children `F$Link` by name) | medium-high | Same as above, but with named lookup and refcount-managed lifetime | Confirmed possible by [`fvmodul.asm`](D:\retro\nitros9\level1\modules\kernel\fvmodul.asm) marking blocks with `ModBlock` so they persist after allocator exits. Heavier setup; preferred when persistence-past-parent matters. |

### Confirmed facts (from Tech Ref + assembly source)

- **Process logical 64 KB is private.** MMU window, swapped on context switch. Physical RAM is shared only via mapping mechanisms.
- **`F$AllRAM` block IDs are bearer-style.** `fallram.asm` just marks blocks "used" in `D.BlkMap` — no per-process ownership table. Any process with the block number can `F$MapBlk` it. Caveat: blocks are **not auto-freed on process exit** — the allocator must call `F$ClrBlk` or they leak until reboot.
- **`F$MapBlk` performs no ownership check.** It finds free space in the caller's DAT image and inserts the block(s) there.
- **`F$VModul` registers a runtime-built module in the module directory.** Marks underlying physical blocks with the `ModBlock` flag, so the module persists past the allocator's lifetime, governed by the standard `F$Link` / `F$UnLink` refcount.
- **`F$Wait` returns `A=0` if interrupted by a signal** delivered to the caller (vs. `A=child_pid` on a real child exit) — important for the parent's main loop when it both waits on children and accepts signals.
- **`F$Move` is privileged** ("usually system to user, or vice versa") — not the right primary mechanism for user-process IPC.
- **`SS.ScInf` + `F$MapBlk`** maps GrfDrv-owned screen RAM into a user process ([poc_cwext.c](../implementation/poc-catalog.md)) — proves cross-process mapping is in routine use.

## Recommended design for `poc_shmem`

Use **`F$AllRAM` + `F$MapBlk` with block-number passing** as the baseline; add `F$VModul` later only if needed.

```
Parent (logic process):
  1. F$AllRAM(B=N)              -> physical block_num in D
  2. F$MapBlk(B=N, X=block_num) -> local addr in U; init shared-state header
  3. F$Fork(child)              -> child_pid, passing block_num in the param area
  4. main loop: write state, F$Send(child_pid, sig_render), F$Sleep
  5. on shutdown: F$Send quit; F$Wait; F$ClrBlk (unmap); F$DelRAM (free physical)

Child (render process):
  1. read block_num from argv
  2. F$MapBlk(B=N, X=block_num) -> local addr in U (same physical bytes; different logical addr)
  3. install_intercept(); main loop: F$Sleep, on signal read shared state, render
  4. on quit signal: F$ClrBlk (unmap own copy); exit. Parent calls F$DelRAM.
```

Atomicity for multi-byte fields uses one of:
- **Sequence-number / double-buffer.** Parent writes to slot `(seq^1)`, then increments `seq`. Child reads `seq`, reads slot `seq`, re-checks `seq` — retry if changed.
- **Disabled interrupts.** `orcc #$50` / `andcc #$AF` around the write. Cheap but blocks signal delivery briefly.

## Coordination protocol (from TSTBUS4/TSTBUSW)

## Coordination protocol (from TSTBUS4/TSTBUSW)

Generic targeted-request pattern that Planet Pioneers can adopt:

```
Parent:
  reqSeq++; targetMask = role_bit; cmd = ...; busSignal = N
  write_state()
  for each child: F$Send(child_pid, busSignal)
  loop: F$Sleep, read_state, until <role>RspSeq == reqSeq

Child (per role):
  install_intercept()
  claim_role()  -- atomic update of state
  loop:
    clear_intercept()
    F$Sleep(N)             -- interruptible
    read_state()
    if reqSeq != lastReq and (targetMask & roleMask):
        lastReq = reqSeq
        do_work()
        <role>RspSeq = reqSeq
        write_state()
        if parentSleep: F$Send(parent_pid, ack_sig)
        if cmd == QUIT: exit
```

Quit (`cmdCode=9`) → child acks with status=40 → exits → parent reaps via `F$Wait`.

## Open questions

- **Signal-number allocation.** Reference uses 130–142 ad-hoc. Planet Pioneers should reserve a small numbered range with documented meanings (e.g. 130=tick, 131=play-sound, 132=render-now, 140-142=quit-by-role). Pin before phase 2a.
- **Block-leak safety.** `F$AllRAM` blocks survive process exit. Need a tear-down convention so a crashed parent doesn't permanently leak blocks. Candidates: child detects parent death (signal-on-orphan?) and frees; or a small "reclaim" boot script. Defer to `poc_ipc` work.
- **Child fork mechanism in C.** DCC's libc `system()` / `os9fork()` vs. raw `F$Fork` syscall — which is the canonical path for spawning a sibling C executable? Confirm during phase 2a port of `SLPICPT.c`.

## Related

- [memory.md](memory.md) — why we want multiple processes (each gets its own 64 KB).
- [sound.md](sound.md) — `SS.Tone` is blocking; sound child built on this IPC pattern is the production solution.
- [stocks-and-bonds source page](../sources/stocks-and-bonds.md) — reference implementation.
- [implementation/roadmap.md](../implementation/roadmap.md) — phases 2a/2b are the gating PoCs.

## Sources

- `D:\retro\stocks-and-bonds\src\c\SLPICPT.c`
- `D:\retro\stocks-and-bonds\src\basic\TSTBUS4.b09`
- `D:\retro\stocks-and-bonds\src\basic\TSTBUSW.b09`
- `docs/reference/NitrOS-9 EOU Technical Reference.md` — F$AllRAM (line 2640+), F$Link (3241+), F$Load (3297+), F$MapBlk (3353+), F$Mem (3383+), F$Move (5450+), F$SRqMem (5688+), F$UnLink (3834+), F$UnLoad (3865+), F$VModul (5889+).
- `D:\retro\nitros9\level2\modules\kernel\fallram.asm`, `fmapblk.asm`, `fmove.asm` (Level 2 implementations).
- `D:\retro\nitros9\level1\modules\kernel\fvmodul.asm` (F$VModul / module-directory registration).
