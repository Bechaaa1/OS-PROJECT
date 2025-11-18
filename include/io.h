#ifndef IO_H
#define IO_H

#include "types.h"

typedef enum { IO_FIFO = 0, IO_PARALLEL = 1 } IoMode;

void io_init(IoMode m);
int  io_try_start(int proc_idx, int cur_io_time);
void io_advance(int *io_time); /* advance I/O clock and finish bursts */

/* exported state (used by main.c sync logic) */
extern IoMode io_mode;
extern int io_free_at;           /* FIFO: device free at this time */
extern int io_active;            /* FIFO: active proc index or -1 */
extern int io_finish[MAX_PROCS]; /* PARALLEL: per-proc finish times */

#endif /* IO_H */
