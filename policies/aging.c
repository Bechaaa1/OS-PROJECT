// policies/aging.c
// Dynamic Priority Scheduler with Aging
// Rule: Every time we go through the ready queue and a process is NOT selected,
//       its priority increases by 1 (aging). When it runs, priority decreases by 1 per quantum used.

#include <stdio.h>
#include <string.h>
#include "types.h"
#include "policy.h"

extern Proc proc_list[MAX_PROCS];
extern int num_procs;

void aging_scheduler(int quantum)
{
    int t = 0;
    int finished = 0;

    while (finished < num_procs) {
        int selected = -1;
        int highest_prio = -999;

        // Phase 1: Apply aging to all ready (not finished, arrived) processes
        for (int i = 0; i < num_procs; i++) {
            if (proc_list[i].finished || proc_list[i].arrival > t)
                continue;
            proc_list[i].priority += 1;  // Aging: +1 every full scan
        }

        // Phase 2: Find current highest priority ready process
        for (int i = 0; i < num_procs; i++) {
            if (proc_list[i].finished || proc_list[i].arrival > t)
                continue;

            if (proc_list[i].priority > highest_prio) {
                highest_prio = proc_list[i].priority;
                selected = i;
            }
        }

        if (selected == -1) {
            t++;  // Idle
            continue;
        }

        Proc *p = &proc_list[selected];
        int run_time = (p->cycles[0] < quantum) ? p->cycles[0] : quantum;

        printf("t=%d: %s runs calcul for %d units (prio=%d -> %d)\n",
               t, p->name, run_time, p->priority, p->priority - run_time);

        // Decrease priority by the number of units it actually runs
        p->priority -= run_time;
        if (p->priority < 0) p->priority = 0;  // Optional: prevent negative

        p->cycles[0] -= run_time;
        t += run_time;

        if (p->cycles[0] <= 0) {
            p->finished = 1;
            finished++;
            printf("t=%d: %s finished\n", t, p->name);
        }
    }

    printf("Aging Dynamic Priority simulation finished at t=%d\n", t);
}