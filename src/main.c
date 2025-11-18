// src/main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "policy.h"
#include "types.h"

#define MAX_CYCLES       100
#define MAX_CYCLE_LENGTH 10
#define MAX_NAME_LENGTH  50
#define MAX_LINE_LENGTH  256

/* ------------------------------------------------------------------
   Temporary structure used only for parsing the config file
   ------------------------------------------------------------------ */
typedef struct {
    char name[MAX_NAME_LENGTH];
    int time_of_entry;
    char cycles[MAX_CYCLES][MAX_CYCLE_LENGTH];
    char type[MAX_CYCLES][10];
    int num_cycles;
    int priority;
} Process;

/* ------------------------------------------------------------------
   Global variables used only during parsing (local to main.c)
   ------------------------------------------------------------------ */
static Process parsed_processes[MAX_PROCS];
static int parsed_count = 0;

/* ------------------------------------------------------------------
   Parse one line from the configuration file
   ------------------------------------------------------------------ */
static Process parse_line(char *line)
{
    Process p = {0};
    char *token = strtok(line, " \t\n");
    if (!token) return p;

    strncpy(p.name, token, MAX_NAME_LENGTH - 1);

    token = strtok(NULL, " \t\n");
    p.time_of_entry = atoi(token);

    char *tokens[256];
    int count = 0;

    while ((token = strtok(NULL, " \t\n")) != NULL) {
        tokens[count++] = token;
    }
    if (count == 0) return p;

    p.priority   = atoi(tokens[count - 1]);
    p.num_cycles = count - 1;

    for (int i = 0; i < p.num_cycles; i++) {
        strncpy(p.cycles[i], tokens[i], MAX_CYCLE_LENGTH - 1);
        strncpy(p.type[i], (i % 2 == 0) ? "calcul" : "E/S", 9);
    }
    return p;
}

/* ------------------------------------------------------------------
   Fill the global proc_list[] used by all schedulers
   Only the first "calcul" burst is kept (as you requested)
   ------------------------------------------------------------------ */
static void fill_scheduler_structures(void)
{
    num_procs = parsed_count;

    for (int i = 0; i < parsed_count; i++) {
        Proc *dst = &proc_list[i];
        Process *src = &parsed_processes[i];

        strncpy(dst->name, src->name, 49);
        dst->name[49] = '\0';
        dst->arrival       = src->time_of_entry;
        dst->priority      = src->priority;
        dst->finished      = 0;
        dst->current_cycle = 0;
        dst->remaining     = 0;
        dst->num_cycles    = 1;               // we only simulate one burst

        /* Find the first "calcul" burst */
        for (int j = 0; j < src->num_cycles; j++) {
            if (strcmp(src->type[j], "calcul") == 0) {
                dst->cycles[0] = atoi(src->cycles[j]);
                strncpy(dst->type[0], "calcul", 9);
                break;
            }
        }
    }
}

/* ------------------------------------------------------------------ */
int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: %s <config_file>\n", argv[0]);
        printf("Example: %s config/example.txt\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        perror("Failed to open config file");
        return 1;
    }

    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), fp)) {
        /* Skip comments and empty lines */
        if (line[0] == '#' || (line[0] == '/' && line[1] == '/') || line[0] == '\n')
            continue;

        if (parsed_count >= MAX_PROCS) {
            printf("Warning: Too many processes (max %d)\n", MAX_PROCS);
            break;
        }
        parsed_processes[parsed_count++] = parse_line(line);
    }
    fclose(fp);

    if (parsed_count == 0) {
        printf("No processes loaded.\n");
        return 1;
    }

    printf("\n=== CPU Scheduler Simulator ===\n");
    printf("Loaded %d process(es)\n\n", parsed_count);
    printf("Select scheduling policy:\n");
    printf("1. FIFO\n");
    printf("2. Priority (Preemptive)\n");
    printf("3. Round-Robin\n");
    printf("4. Dynamic Priority with Aging (NEW)\n");
    printf("Enter choice (1-4): ");

    int choice;
    if (scanf("%d", &choice) != 1 || choice < 1 || choice > 4) {
        printf("Invalid → defaulting to FIFO\n");
        choice = 1;
    }

    int quantum = 2;
    if (choice == 3 || choice == 4) {
        printf("Enter time quantum (2-4 recommended): ");
        if (scanf("%d", &quantum) != 1 || quantum <= 0)
            quantum = 2;
    }

    fill_scheduler_structures();

    printf("\n--- Simulation start ---\n\n");

    switch (choice) {
        case 1: printf("FIFO Scheduler\n");              fifo_scheduler();              break;
        case 2: printf("Preemptive Priority Scheduler\n"); priority_scheduler();         break;
        case 3: printf("Round-Robin (Quantum=%d)\n", quantum); round_robin_scheduler(quantum); break;
        case 4: printf("Dynamic Priority + Aging (Quantum=%d)\n", quantum); aging_scheduler(quantum); break;
    }

    printf("\nSimulation finished.\n");
    return 0;
}