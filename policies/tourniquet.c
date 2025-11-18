#include <stdio.h>
#include "policy.h"
#include "types.h"

extern Proc proc_list[MAX_PROCS];
extern int num_procs;

void round_robin_scheduler(int quantum)
{
    int t = 0;
    int finished = 0;

    while (finished < num_procs) {
        int did_work = 0;

        for (int i = 0; i < num_procs; i++) {
            Proc *p = &proc_list[i];
            if (p->finished || p->arrival > t) continue;

            int run = (p->cycles[0] < quantum) ? p->cycles[0] : quantum;
            printf("t=%d: %s runs calcul for %d units\n", t, p->name, run);

            p->cycles[0] -= run;
            t += run;
            did_work = 1;

            if (p->cycles[0] == 0) {
                p->finished = 1;
                finished++;
            }
        }

        if (!did_work) t++;   // idle when nothing is ready
    }
    printf("Simulation finished at t=%d\n", t);
}