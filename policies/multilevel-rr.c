#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_PROCESS 100
#define MAX_BURSTS 50
#define MAX_TIME 1000
#define MAX_PRIORITY 10

typedef struct {
    char name[10];
    int arrival_time;
    int bursts[MAX_BURSTS];
    int num_bursts;
    int current_burst_idx;
    int remaining_time;
    int priority;       // Fixed priority level
    int finished;
    int end_time;
    int total_duration;
} Process;

typedef struct Node {
    Process* p;
    struct Node* next;
} Node;

typedef struct {
    Node *head, *tail;
} Queue;

void initQueue(Queue* q) {
    q->head = q->tail = NULL;
}

void enqueue(Queue* q, Process* p) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->p = p;
    newNode->next = NULL;
    if (q->tail == NULL) {
        q->head = q->tail = newNode;
    } else {
        q->tail->next = newNode;
        q->tail = newNode;
    }
}

Process* dequeue(Queue* q) {
    if (q->head == NULL) return NULL;
    Node* temp = q->head;
    Process* p = temp->p;
    q->head = q->head->next;
    if (q->head == NULL) q->tail = NULL;
    free(temp);
    return p;
}

int isQueueEmpty(Queue* q) {
    return q->head == NULL;
}

char* history_cpu[MAX_TIME];
char* history_io[MAX_TIME];

int main() {
    Process processes[MAX_PROCESS];
    int process_count = 0;
    char line[256];
    int i;
    int quantum = 2; // Default quantum for round-robin

    // --- LECTURE STDIN : QUANTUM (optionnel) PUIS PROCESSUS ---
    if (fgets(line, sizeof(line), stdin)) {
        if (sscanf(line, "quantum %d", &quantum) != 1) {
            quantum = 2; // Default if not specified
            
            // Treat this line as a process
            if (line[0] != '#' && line[0] != '/' && line[0] != '\n') {
                char temp_name[10];
                int temp_arrival;
                
                char* token = strtok(line, " \t\n");
                if (token) {
                    strcpy(temp_name, token);
                    token = strtok(NULL, " \t\n");
                    if (token) {
                        temp_arrival = atoi(token);
                        
                        int numbers[MAX_BURSTS + 1];
                        int count = 0;
                        while ((token = strtok(NULL, " \t\n")) != NULL) {
                            numbers[count++] = atoi(token);
                        }
                        
                        if (count >= 2) {
                            Process* p = &processes[process_count];
                            strcpy(p->name, temp_name);
                            p->arrival_time = temp_arrival;
                            p->priority = numbers[count - 1];
                            p->num_bursts = count - 1;
                            p->total_duration = 0;
                            
                            for (i = 0; i < p->num_bursts; i++) {
                                p->bursts[i] = numbers[i];
                                p->total_duration += numbers[i];
                            }
                            
                            p->current_burst_idx = 0;
                            p->remaining_time = p->bursts[0];
                            p->finished = 0;
                            p->end_time = 0;
                            
                            process_count++;
                        }
                    }
                }
            }
        }
    }

    // Read remaining processes
    while (fgets(line, sizeof(line), stdin)) {
        if (line[0] == '#' || line[0] == '/' || line[0] == '\n' || line[0] == '\r') continue;
        if (strstr(line, "quantum") != NULL) continue;

        char temp_name[10];
        int temp_arrival;
        
        char* token = strtok(line, " \t\n");
        if (!token) continue;
        strcpy(temp_name, token);

        token = strtok(NULL, " \t\n");
        if (!token) continue;
        temp_arrival = atoi(token);

        int numbers[MAX_BURSTS + 1];
        int count = 0;
        while ((token = strtok(NULL, " \t\n")) != NULL) {
            numbers[count++] = atoi(token);
        }

        if (count < 2) continue;

        Process* p = &processes[process_count];
        strcpy(p->name, temp_name);
        p->arrival_time = temp_arrival;
        p->priority = numbers[count - 1];
        p->num_bursts = count - 1;
        p->total_duration = 0;
        
        for (i = 0; i < p->num_bursts; i++) {
            p->bursts[i] = numbers[i];
            p->total_duration += numbers[i];
        }
        
        p->current_burst_idx = 0;
        p->remaining_time = p->bursts[0];
        p->finished = 0;
        p->end_time = 0;
        
        process_count++;
    }

    // --- SIMULATION MULTILEVEL ROUND ROBIN ---
    int time = 0;
    int processes_finished = 0;
    
    // Create priority queues (0 = highest priority)
    Queue cpu_queues[MAX_PRIORITY];
    for (i = 0; i < MAX_PRIORITY; i++) {
        initQueue(&cpu_queues[i]);
    }
    
    Queue ioQ;
    initQueue(&ioQ);

    Process* active_cpu = NULL;
    Process* active_io = NULL;
    int current_quantum_consumed = 0;

    // Initialize history
    for(i=0; i<MAX_TIME; i++) {
        history_cpu[i] = "NULL";
        history_io[i] = "NULL";
    }

    while (processes_finished < process_count && time < MAX_TIME) {
        
        // A. Check arrivals and enqueue to appropriate priority queue
        for (i = 0; i < process_count; i++) {
            if (processes[i].arrival_time == time && !processes[i].finished) {
                int priority = processes[i].priority;
                if (priority >= MAX_PRIORITY) priority = MAX_PRIORITY - 1;
                if (priority < 0) priority = 0;
                
                // If a lower priority process is running, preempt it
                if (active_cpu != NULL && processes[i].priority < active_cpu->priority) {
                    // Enqueue the current running process back to its queue
                    enqueue(&cpu_queues[active_cpu->priority], active_cpu);
                    active_cpu = NULL;
                    current_quantum_consumed = 0;
                }
                
                enqueue(&cpu_queues[priority], &processes[i]);
            }
        }

        // B. GESTION CPU (Multilevel Round Robin)
        
        // If current process finished quantum or is null, select from highest priority queue
        if (active_cpu == NULL) {
            for (i = 0; i < MAX_PRIORITY; i++) {
                if (!isQueueEmpty(&cpu_queues[i])) {
                    active_cpu = dequeue(&cpu_queues[i]);
                    current_quantum_consumed = 0;
                    break;
                }
            }
        }

        if (active_cpu != NULL) {
            history_cpu[time] = active_cpu->name;
            active_cpu->remaining_time--;
            current_quantum_consumed++;

            if (active_cpu->remaining_time == 0) {
                // Burst finished
                active_cpu->current_burst_idx++;
                
                if (active_cpu->current_burst_idx >= active_cpu->num_bursts) {
                    // Process finished
                    active_cpu->finished = 1;
                    active_cpu->end_time = time + 1;
                    processes_finished++;
                    active_cpu = NULL;
                } else {
                    // Switch to I/O
                    active_cpu->remaining_time = active_cpu->bursts[active_cpu->current_burst_idx];
                    enqueue(&ioQ, active_cpu);
                    active_cpu = NULL;
                }
                current_quantum_consumed = 0;
            }
            else if (current_quantum_consumed == quantum) {
                // Quantum expired, re-enqueue to same priority queue
                enqueue(&cpu_queues[active_cpu->priority], active_cpu);
                active_cpu = NULL;
                current_quantum_consumed = 0;
            }
        }

        // C. GESTION E/S (FIFO)
        if (active_io == NULL && !isQueueEmpty(&ioQ)) {
            active_io = dequeue(&ioQ);
        }

        if (active_io != NULL) {
            history_io[time] = active_io->name;
            active_io->remaining_time--;

            if (active_io->remaining_time == 0) {
                active_io->current_burst_idx++;
                
                if (active_io->current_burst_idx >= active_io->num_bursts) {
                    active_io->finished = 1;
                    active_io->end_time = time + 1;
                    processes_finished++;
                    active_io = NULL;
                } else {
                    active_io->remaining_time = active_io->bursts[active_io->current_burst_idx];
                    
                    // Enqueue back to CPU with its priority
                    int priority = active_io->priority;
                    if (priority >= MAX_PRIORITY) priority = MAX_PRIORITY - 1;
                    if (priority < 0) priority = 0;
                    enqueue(&cpu_queues[priority], active_io);
                    active_io = NULL;
                }
            }
        }

        time++;
    }

    // --- OUTPUT ---
    printf("OUTPUT :\n\n");
    
    printf("calcul :\n");
    for (i = 0; i < time; i++) {
        printf("%d : %s\n", i, history_cpu[i]);
    }

    printf("\nE/S :\n");
    for (i = 0; i < time; i++) {
        printf("%d : %s\n", i, history_io[i]);
    }

    // --- STATISTICS ---
    printf("\n--- Statistiques ---\n");
    double total_rotation = 0;
    double total_attente = 0;

    for (i = 0; i < process_count; i++) {
        Process *p = &processes[i];
        int rotation = p->end_time - p->arrival_time;
        int attente = rotation - p->total_duration;
        total_rotation += rotation;
        total_attente += attente;
    }

    if (process_count > 0) {
        printf("Temps de rotation moyen : %.2f\n", total_rotation / process_count);
        printf("Temps d'attente moyen : %.2f\n", total_attente / process_count);
    }

    return 0;
}