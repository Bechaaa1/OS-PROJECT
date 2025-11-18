#include <stdio.h>
#include "policy.h"
#include "types.h"

extern Proc proc_list[MAX_PROCS];
extern int num_procs;

void fifo_scheduler(void)
{
    int t = 0;
    int finished = 0;

    while (finished < num_procs) {
        int candidate = -1;
        int earliest_arrival = -1;

        /* Find the first ready process (FIFO = earliest arrival) */
        for (int i = 0; i < num_procs; i++) {
            if (proc_list[i].finished) continue;
            if (proc_list[i].arrival > t) continue;
            if (candidate == -1 || proc_list[i].arrival < earliest_arrival) {
                candidate = i;
                earliest_arrival = proc_list[i].arrival;
            }
        }

        if (candidate == -1) {
            /* No process ready → jump to the next arrival */
            for (int i = 0; i < num_procs; i++) {
                if (!proc_list[i].finished && proc_list[i].arrival > t) {
                    if (earliest_arrival == -1 || proc_list[i].arrival < earliest_arrival)
                        earliest_arrival = proc_list[i].arrival;
                }
            }
            t = (earliest_arrival != -1) ? earliest_arrival : t + 1;
            continue;
        }

        Proc *p = &proc_list[candidate];
        printf("t=%d: %s starts calcul (%d units)\n", t, p->name, p->cycles[0]);
        t += p->cycles[0];
        p->finished = 1;
        finished++;
    }
    printf("Simulation finished at t=%d\n", t);
}