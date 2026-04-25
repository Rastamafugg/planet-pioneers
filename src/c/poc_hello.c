/***********************************************************************
 * poc_hello.c  -  Smallest possible DCC C program.
 *
 * If this crashes EOU with vertical stripes / system reset, the
 * problem is below user code (libc startup, dcc flag set, module
 * header, disk-image deploy, ...) and not in main.c logic.
 *
 * If it runs cleanly, the bug lives in main.c and we keep bisecting.
 *
 * Compile: dcc poc_hello.c -m=4k -f=/dd/cmds/pochello
 ***********************************************************************/
#include <stdio.h>
#include <stdlib.h>

main()
{
    printf("hello from poc_hello\n");
    exit(0);
}
