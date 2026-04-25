/***********************************************************************
 * slpicpt.c  -  Signal-intercept wrapper.
 *
 * Direct port of stocks-and-bonds SLPICPT.c. Wraps DCC libc intercept()
 * with a single-buffer model exposing three actions:
 *   1 = install handler, clear counters
 *   2 = read lastsig + hitcount
 *   3 = clear counters (handler stays installed)
 *
 * Non-reentrant: one active intercept per process.
 *
 * Two pieces of the source repo's version are intentionally omitted:
 *
 * 1. The trailing `_stkcheck` / `vsect` block (file-private libc
 *    runtime). The host program (poc_ipc.c) supplies its own runtime
 *    when linked together; including the standalone block would
 *    duplicate symbols.
 *
 * 2. The `#asm ldy 6,s #endasm` preamble at the top of slpicpt(). In
 *    stocks-and-bonds, slpicpt was invoked as a standalone module
 *    from Basic09 (`RUN`); the b09 calling convention does not set
 *    up Y the way DCC's C ABI expects, so the asm preamble re-syncs
 *    it from the stacked third argument. When linked into a normal
 *    C program (our case), Y already holds the data-segment pointer
 *    for global access; reloading it from `cmemsiz` clobbers Y and
 *    corrupts every subsequent global-variable access, hanging the
 *    caller. Observed 2026-04-25.
 *
 * See wiki/platform/ipc.md and wiki/implementation/lessons-learned.md.
 ***********************************************************************/
#include <signal.h>

static int lastsig;
static int hitcount;
static int installed;

static sigtrap(sig)
int sig;
{
    lastsig = sig;
    ++hitcount;
    return 0;
}

slpicpt(cnt, cmem, cmemsiz, action, actionsz, sigout, sigoutsz,
        cntout, cntoutsz, okout, okoutsz)
int cnt;
char *cmem;
int cmemsiz;
int *action;
int actionsz;
int *sigout;
int sigoutsz;
int *cntout;
int cntoutsz;
int *okout;
int okoutsz;
{
    *okout  = 0;
    *sigout = 0;
    *cntout = 0;

    if (*action == 1) {
        lastsig  = 0;
        hitcount = 0;
        if (!installed) {
            intercept(sigtrap);
            installed = 1;
        }
        *okout = 1;
        return 0;
    }

    if (*action == 2) {
        *sigout = lastsig;
        *cntout = hitcount;
        *okout  = 1;
        return 0;
    }

    if (*action == 3) {
        lastsig  = 0;
        hitcount = 0;
        *okout   = 1;
        return 0;
    }

    return 0;
}
