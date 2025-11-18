#include <stdio.h>
#include <string.h>
#include "types.h"
#include "io.h"

/* exported state */
IoMode io_mode = IO_FIFO;
int io_free_at = 0;
int io_active = -1;
int io_finish[MAX_PROCS] = {0};

void io_init(IoMode m) {
    io_mode = m;
    io_free_at = 0;
    io_active = -1;
    memset(io_finish, 0, sizeof(io_finish));
}

/* I/O is DISABLED → never start anything */
int io_try_start(int idx, int cur) {
    (void)idx;
    (void)cur;
    return 0;   // always refuse I/O start
}

/* I/O is DISABLED → never advance anything */
void io_advance(int *io_time) {
    (void)io_time;
    return;     // nothing ever happens
}
