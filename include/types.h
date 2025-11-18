#ifndef SCHEDULER_TYPES_H
#define SCHEDULER_TYPES_H

#define MAX_PROCS 10

typedef struct {
    char name[50];
    int arrival;
    int cycles[100];
    char type[100][10];
    int num_cycles;
    int current_cycle;
    int priority;
    int remaining;
    int finished;
} Proc;

/* ONLY DECLARATIONS — no storage here! */
extern Proc proc_list[MAX_PROCS];
extern int num_procs;

#endif