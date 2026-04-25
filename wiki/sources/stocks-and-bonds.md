# stocks-and-bonds (reference repo)

Sibling project at `D:\retro\stocks-and-bonds\` — a Basic09 game targeting the same NitrOS-9 EOU platform. Contains working patterns for **multi-process architecture, signal-based IPC, and C signal intercepts** that Planet Pioneers will reuse.

## Key files

- `src/c/SLPICPT.c` — C signal intercept wrapper around libc `intercept()`. Small, self-contained, portable directly into the Planet Pioneers build.
- `src/basic/TSTBUS4.b09` — parent/harness that forks three workers and drives a targeted signal bus protocol.
- `src/basic/TSTBUSW.b09` — worker template: install intercept, claim role, sleep/wake/read/ack loop.

## What each file proves

### SLPICPT.c — C signal intercept

`intercept(sigtrap)` registers a C handler that fires when the process receives a signal via `F$Send`. Handler body is minimal (sets `lastsig`/`hitcount` static globals) — real work happens outside interrupt context. Action codes: **1=install**, **2=read-and-clear**, **3=clear**. Non-reentrant (single static buffer) but sufficient for one active intercept per process.

### TSTBUS4 / TSTBUSW — signal-bus coordination

Demonstrates:
- **Signal = wakeup only; payload lives in shared state.** `F$Send` just delivers a sig number; the receiver reads state from a shared file/region.
- **Worker event loop:** clear intercept → `F$Sleep` (interruptible by signal) → read state → if `targetMask & roleMask` match, do work → bump `<role>RspSeq` → optionally signal parent back.
- **Request sequencing** via monotonic `reqSeq`; parent polls `<role>RspSeq == reqSeq` to detect ack.
- **Shutdown protocol:** `cmdCode=9` quit request, worker acks with status=40 then exits; parent reaps via `F$Wait`.

## Syscall cheat sheet (observed in the reference)

| Code | Name | Purpose |
|------|------|---------|
| $03  | F$Fork  | Spawn child (register convention varies by caller) |
| $04  | F$Wait  | Reap child, exit status in B |
| $08  | F$Send  | Send signal: A=target PID, B=sig value |
| $0A  | F$Sleep | Sleep; interruptible by signal. X = ticks (0 = forever, 2 = short poll) |
| $0C  | F$ID    | Get own PID (returned in A) |

## Caveat

The reference uses a **disk file (`TSTBUSS`) as shared state**. Planet Pioneers' render child will exchange data at frame rate, which rules out disk I/O — we need a shared-memory mechanism (data module or `F$AllRam`/`F$MapBlk`). See [wiki/platform/ipc.md](../platform/ipc.md).

## Ingested

- 2026-04-24 — initial ingest of SLPICPT.c, TSTBUS4.b09, TSTBUSW.b09 into [wiki/platform/ipc.md](../platform/ipc.md).

## Sources

- `D:\retro\stocks-and-bonds\src\c\SLPICPT.c`
- `D:\retro\stocks-and-bonds\src\basic\TSTBUS4.b09`
- `D:\retro\stocks-and-bonds\src\basic\TSTBUSW.b09`
- Repo: https://github.com/Rastamafugg/stocks-and-bonds
