/***********************************************************************
 * slpicpt.c  -  Signal-intercept wrapper.
 *
 * Direct port of stocks-and-bonds SLPICPT.c. Wraps DCC libc intercept()
 * with a single-buffer model exposing three actions:
 *   1 = install handler, clear counters
 *   2 = read lastsig + hitcount
 *   3 = clear counters (handler stays installed)
 *
 * Non-reentrant: one active intercept per process. The trailing
 * _stkcheck / vsect block from the source repo's standalone-module
 * version is omitted here; this file is linked into a host program
 * that supplies its own runtime.
 *
 * See wiki/platform/ipc.md.
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
#asm
    ldy 6,s
#endasm

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
