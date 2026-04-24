# Build Workflow

Governed by AGENTS.md §58–64.

## Scripts

### `src/script/buildc` — full rebuild

Must contain **one `dcc` command for every C source file in `src/c/`**. Keep it complete and current; this is the canonical "rebuild everything" script.

### `src/script/patchc` — fast iterative deploy

Only the `dcc` commands for C sources **changed in the current task**. After each source change:

1. Update `patchc` to contain only the changed-file dcc commands.
2. **Remove unchanged build calls** whenever updating it.

Intent: `patchc` stays minimal and task-specific; `buildc` stays complete.

### `build.sh`

Root-level shell script (78 lines — not yet ingested in detail). Likely orchestrates the above and handles disk image operations.

## Disk image discipline

`disks/ppsrc.dsk` is **owned by the deploy/build workflow** — not by normal source edits.

- **Do NOT** update `disks/ppsrc.dsk` during normal source or script edits. The build/deploy path handles copying changed files into the disk image.
- **Only** modify `disks/ppsrc.dsk` when the user **explicitly** requests disk-image changes or when the task is specifically about disk-image contents.

## Source locations

- **Active PoC / program source:** `src/c/`.
- **Verified PoCs:** should be moved to `src/poc/` once confirmed stable. (Not yet created.)

## Compile flags in current PoCs

| PoC | Stack | Output |
|-----|-------|--------|
| `poc_vsync.c` | `-m=4k` | `/dd/cmds/pocvsync` |
| `poc_cvdg.c` | `-s -m=4k` | `/dd/cmds/poccvdg` |
| `poc_cvdg16.c` | `-s -m=4k` | `/dd/cmds/poccvd16` |
| `poc_gfx.c` | `-s -m=8k` | `/dd/cmds/pocgfx` |
| `poc_cwext.c` | `-s -m=8k` | `/dd/cmds/poccwext` |
| `poc_tiles.c` | `-s -m=24k` | `/dd/cmds/poctiles` |
| `poc_sound.c` | (default) | `/dd/cmds/pocsound` |

The `-s` (speed) flag is used in most PoCs but should be applied cautiously in the main program — only after stack use is verified (GDD §23.8).

## Sources

- `AGENTS.md` §58–64
- `src/c/poc_*.c` compile directives in headers
