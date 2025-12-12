// policies/srt.c — Shortest Remaining Time (SRT) avec statistiques
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_PROCESS 100
#define MAX_BURSTS 50
#define MAX_TIME 1000

typedef struct {
    char name[10];
    int arrival_time;
    int bursts[MAX_BURSTS];
    int num_bursts;
    int current_burst_idx;
    int remaining_time;
    int priority;
    int finished;
    int end_time;
    int total_duration;
    int waiting_time_cpu;
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

Process* dequeueFIFO(Queue* q) {
    if (q->head == NULL) return NULL;
    Node* temp = q->head;
    Process* p = temp->p;
    q->head = q->head->next;
    if (q->head == NULL) q->tail = NULL;
    free(temp);
    return p;
}

Process* dequeueShortest(Queue* q) {
    if (q->head == NULL) return NULL;

    Node *prev = NULL, *curr = q->head;
    Node *minPrev = NULL, *minNode = q->head;

    while (curr != NULL) {
        if (curr->p->remaining_time < minNode->p->remaining_time) {
            minNode = curr;
            minPrev = prev;
        }
        prev = curr;
        curr = curr->next;
    }

    if (minPrev == NULL) {
        q->head = minNode->next;
    } else {
        minPrev->next = minNode->next;
    }

    if (minNode == q->tail) {
        q->tail = minPrev;
    }

    Process* p = minNode->p;
    free(minNode);
    return p;
}

int getShortestTimeInQueue(Queue* q) {
    if (q->head == NULL) return 999999;
    int minT = q->head->p->remaining_time;
    Node* curr = q->head->next;
    while (curr != NULL) {
        if (curr->p->remaining_time < minT) {
            minT = curr->p->remaining_time;
        }
        curr = curr->next;
    }
    return minT;
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

    /* Lecture depuis stdin */
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
        p->waiting_time_cpu = 0;
        
        process_count++;
    }

    /* Simulation SRT */
    int time = 0;
    int processes_finished = 0;
    
    Queue cpuQ, ioQ;
    initQueue(&cpuQ);
    initQueue(&ioQ);

    Process* active_cpu = NULL;
    Process* active_io = NULL;

    for(i=0; i<MAX_TIME; i++) {
        history_cpu[i] = "NULL";
        history_io[i] = "NULL";
    }

    while (processes_finished < process_count && time < MAX_TIME) {
        
        /* Vérifier les arrivées */
        for (i = 0; i < process_count; i++) {
            if (processes[i].arrival_time == time) {
                enqueue(&cpuQ, &processes[i]);
            }
        }

        /* SRT Préemption */
        if (active_cpu != NULL && !isQueueEmpty(&cpuQ)) {
            int shortestInQueue = getShortestTimeInQueue(&cpuQ);
            if (shortestInQueue < active_cpu->remaining_time) {
                enqueue(&cpuQ, active_cpu);
                active_cpu = NULL;
            }
        }

        /* Sélection CPU */
        if (active_cpu == NULL && !isQueueEmpty(&cpuQ)) {
            active_cpu = dequeueShortest(&cpuQ);
        }

        /* Sélection E/S */
        if (active_io == NULL && !isQueueEmpty(&ioQ)) {
            active_io = dequeueFIFO(&ioQ);
        }

        /* Execution CPU */
        if (active_cpu != NULL) {
            history_cpu[time] = active_cpu->name;
            active_cpu->remaining_time--;

            if (active_cpu->remaining_time == 0) {
                active_cpu->current_burst_idx++;
                
                if (active_cpu->current_burst_idx >= active_cpu->num_bursts) {
                    active_cpu->finished = 1;
                    active_cpu->end_time = time + 1;
                    processes_finished++;
                    active_cpu = NULL;
                } else {
                    active_cpu->remaining_time = active_cpu->bursts[active_cpu->current_burst_idx];
                    enqueue(&ioQ, active_cpu);
                    active_cpu = NULL;
                }
            }
        }

        /* Execution E/S */
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
                    enqueue(&cpuQ, active_io);
                    active_io = NULL;
                }
            }
        }

        /* Compteur d'attente pour processus en file CPU */
        Node* currNode = cpuQ.head;
        while(currNode != NULL) {
            currNode->p->waiting_time_cpu++;
            currNode = currNode->next;
        }

        time++;
    }

    /* Affichage */
    printf("OUTPUT :\n\n");
    
    printf("calcul :\n");
    for (i = 0; i < time; i++) {
        printf("%d : %s\n", i, history_cpu[i]);
    }

    printf("\nE/S :\n");
    for (i = 0; i < time; i++) {
        printf("%d : %s\n", i, history_io[i]);
    }

    /* Statistiques */
    printf("\n--- Statistiques ---\n");
    double total_rotation = 0;
    double total_attente = 0;

    for (i = 0; i < process_count; i++) {
        Process *p = &processes[i];
        int rotation = p->end_time - p->arrival_time;
        int attente = p->waiting_time_cpu;
        total_rotation += rotation;
        total_attente += attente;
    }

    if (process_count > 0) {
        printf("Temps de rotation moyen : %.2f\n", total_rotation / process_count);
        printf("Temps d'attente moyen : %.2f\n", total_attente / process_count);
    } else {
        printf("Aucun processus traite.\n");
    }

    return 0;
}