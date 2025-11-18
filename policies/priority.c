#include <stdio.h>
#include <string.h>
#include "policy.h"
#include "types.h"

extern Proc proc_list[MAX_PROCS];
extern int num_procs;

void priority_scheduler(void)
{
    int t = 0;
    int finished = 0;

    while (finished < num_procs) {
        int best = -1;

        /* Find highest-priority ready process */
        for (int i = 0; i < num_procs; i++) {
            if (proc_list[i].finished) continue;
            if (proc_list[i].arrival > t) continue;

            if (best == -1 || proc_list[i].priority > proc_list[best].priority)
                best = i;
        }

        if (best == -1) {               // no process ready → advance time
            t++;
            continue;
        }

        Proc *p = &proc_list[best];
        p->remaining = p->cycles[0];

        printf("t=%d: %s starts calcul (%d units)\n", t, p->name, p->remaining);

        while (p->remaining > 0) {
            t++;
            p->remaining--;

            /* Check for preemption */
            for (int i = 0; i < num_procs; i++) {
                if (proc_list[i].finished || proc_list[i].arrival > t) continue;
                if (proc_list[i].priority > p->priority) {
                    printf("t=%d: %s preempts %s\n", t, proc_list[i].name, p->name);
                    best = i;
                    p = &proc_list[best];
                    p->remaining = p->cycles[0];
                    printf("t=%d: %s starts calcul (%d units)\n", t, p->name, p->remaining);
                    break;
                }
            }
        }

        p->finished = 1;
        finished++;
    }
    printf("Simulation finished at t=%d\n", t);
}