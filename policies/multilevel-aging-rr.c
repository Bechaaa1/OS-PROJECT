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
    int priority;       // Plus grand nombre = Plus grande priorité
    int finished;
    
    // --- VARIABLES STATISTIQUES ---
    int end_time;       
    int total_duration; 
    int waiting_time_cpu; // Compteur strict
    
    // --- TIMER AGING ---
    int aging_timer;    // Compteur pour le bonus (+1 tous les 4 tics)
} Process;

// --- FILES D'ATTENTE ---
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

// Insère trié par priorité décroissante.
// En cas d'égalité, insère APRES (FIFO pour respecter l'ordre d'arrivée/RR)
void enqueuePriority(Queue* q, Process* p) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->p = p;
    newNode->next = NULL;

    // Cas 1 : Tête de liste
    if (q->head == NULL || p->priority > q->head->p->priority) {
        newNode->next = q->head;
        q->head = newNode;
        if (q->tail == NULL) q->tail = newNode;
    } 
    // Cas 2 : Insertion milieu/fin
    else {
        Node* current = q->head;
        // On avance tant que le suivant a une priorité >= (pour se mettre derrière les égaux)
        while (current->next != NULL && current->next->p->priority >= p->priority) {
            current = current->next;
        }
        newNode->next = current->next;
        current->next = newNode;
        if (newNode->next == NULL) q->tail = newNode;
    }
    
    // Sécurité queue
    if (q->tail != NULL && q->tail->next != NULL) {
        Node* temp = q->head;
        while(temp->next != NULL) temp = temp->next;
        q->tail = temp;
    }
}

void enqueueFIFO(Queue* q, Process* p) {
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

// Fonction pour diminuer la priorité (Min 1)
// Appliquée UNIQUEMENT si le processus est éjecté du CPU
void applyPenalty(Process* p) {
    if (p->priority > 1) {
        p->priority--; 
    }
}

char* history_cpu[MAX_TIME];
char* history_io[MAX_TIME];

int main(int argc, char *argv[]) {
    Process processes[MAX_PROCESS];
    int process_count = 0;
    char line[256];
    int i; 
    int quantum = 2;  // Valeur par défaut



    // --- LECTURE DU QUANTUM (première ligne obligatoire) ---
    if (fgets(line, sizeof(line), stdin)) {
        if (sscanf(line, "quantum %d", &quantum) == 1) {
            if (quantum <= 0) quantum = 2; // Sécurité
        }
    }

    // --- LECTURE DES PROCESSUS ---
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
        p->aging_timer = 0; 
        
        process_count++;
    }

    // --- 2. SIMULATION ---
    int time = 0;
    int processes_finished = 0;
    
    Queue cpuQ, ioQ;
    initQueue(&cpuQ);
    initQueue(&ioQ);

    Process* active_cpu = NULL;
    Process* active_io = NULL;
    
    int current_quantum_consumed = 0;

    // Init historique
    for(i=0; i<MAX_TIME; i++) {
        history_cpu[i] = "NULL";
        history_io[i] = "NULL";
    }

    while (processes_finished < process_count && time < MAX_TIME) {
        
        // A. ARRIVÉES
        for (i = 0; i < process_count; i++) {
            if (processes[i].arrival_time == time) {
                // On ajoute le nouveau processus dans la file (trié)
                enqueuePriority(&cpuQ, &processes[i]);
                
                // PRÉEMPTION SUR ARRIVÉE (Cas Strict > ou Egal =)
                // "Si priorité égale, il est élu et le processus actif mis dans la file"
                if (active_cpu != NULL && processes[i].priority >= active_cpu->priority) {
                    applyPenalty(active_cpu); // Pénalité car éjecté
                    enqueuePriority(&cpuQ, active_cpu);
                    active_cpu = NULL;
                    current_quantum_consumed = 0;
                }
            }
        }

        // --- SÉLECTION (Phase 1) ---
        if (active_cpu == NULL && !isQueueEmpty(&cpuQ)) {
            active_cpu = dequeue(&cpuQ);
            current_quantum_consumed = 0;
            // Quand un processus est élu, on peut reset son aging_timer d'attente
            active_cpu->aging_timer = 0; 
        }

        if (active_io == NULL && !isQueueEmpty(&ioQ)) {
            active_io = dequeue(&ioQ);
        }

        // --- EXÉCUTION (Phase 2) ---

        // B. GESTION CPU
        if (active_cpu != NULL) {
            history_cpu[time] = active_cpu->name;
            active_cpu->remaining_time--;
            current_quantum_consumed++;

            if (active_cpu->remaining_time == 0) {
                // Fin Burst -> Ejection -> Pénalité
                applyPenalty(active_cpu);
                
                active_cpu->current_burst_idx++;
                if (active_cpu->current_burst_idx >= active_cpu->num_bursts) {
                    // FINI
                    active_cpu->finished = 1;
                    active_cpu->end_time = time + 1;
                    processes_finished++;
                    active_cpu = NULL;
                } else {
                    // Vers E/S
                    active_cpu->remaining_time = active_cpu->bursts[active_cpu->current_burst_idx];
                    enqueueFIFO(&ioQ, active_cpu); 
                    active_cpu = NULL;
                }
                current_quantum_consumed = 0;
            }
            else if (current_quantum_consumed == quantum) {
                // Quantum atteint.
                // On vérifie s'il faut switcher.
                // "Round Robin appliqué lorsque deux processus ont la MEME priorité"
                // On switch si quelqu'un de priorité ÉGALE ou SUPÉRIEURE attend.
                
                int switch_needed = 0;
                if (!isQueueEmpty(&cpuQ)) {
                    // cpuQ est triée, head a la plus haute priorité
                    if (cpuQ.head->p->priority >= active_cpu->priority) {
                        switch_needed = 1;
                    }
                }
                
                if (switch_needed) {
                    applyPenalty(active_cpu); // Ejection -> Pénalité
                    enqueuePriority(&cpuQ, active_cpu);
                    active_cpu = NULL;
                    current_quantum_consumed = 0;
                } else {
                    // Pas d'éjection -> Pas de pénalité. On continue.
                    // On reset juste le quantum pour un nouveau cycle virtuel.
                    current_quantum_consumed = 0;
                }
            }
        }

        // C. GESTION E/S
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
                    // Retour CPU
                    active_io->remaining_time = active_io->bursts[active_io->current_burst_idx];
                    enqueuePriority(&cpuQ, active_io);
                    
                    // Préemption sur retour E/S (Si priorité >=)
                    if (active_cpu != NULL && !isQueueEmpty(&cpuQ)) {
                        if (cpuQ.head->p->priority >= active_cpu->priority) {
                            applyPenalty(active_cpu); // Pénalité car éjecté
                            enqueuePriority(&cpuQ, active_cpu);
                            active_cpu = NULL;
                            current_quantum_consumed = 0;
                        }
                    }
                    active_io = NULL;
                }
            }
        }

        // --- D. CALCUL ATTENTE & AGING ---
        // On parcourt la file d'attente CPU
        int priorityChanged = 0;
        Node* currNode = cpuQ.head;
        while(currNode != NULL) {
            // Statistique : attente stricte
            currNode->p->waiting_time_cpu++;
            
            // Logique Bonus : +1 priorité tous les 4 instants
            currNode->p->aging_timer++;
            if (currNode->p->aging_timer >= 4) {
                currNode->p->priority++; // Bonus
                currNode->p->aging_timer = 0; // Reset timer
                priorityChanged = 1;
            }
            
            currNode = currNode->next;
        }

        // Si les priorités ont changé, il faut re-trier la file pour maintenir l'ordre
        if (priorityChanged) {
            Queue tempQ;
            initQueue(&tempQ);
            Process* p;
            while ((p = dequeue(&cpuQ)) != NULL) {
                enqueuePriority(&tempQ, p);
            }
            cpuQ = tempQ;
        }

        time++;
    }

    // --- 3. AFFICHAGE ---
    printf("OUTPUT :\n\n");
    
    printf("calcul :\n");
    for (i = 0; i < time; i++) {
        printf("%d : %s\n", i, history_cpu[i]);
    }

    printf("\nE/S :\n");
    for (i = 0; i < time; i++) {
        char* status = history_io[i];
        if (strcmp(status, "NULL") != 0) {
             printf("%d : %s\n", i, status);
        } else {
            printf("%d : NULL\n", i);
        }
    }

    // --- 4. STATISTIQUES ---
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