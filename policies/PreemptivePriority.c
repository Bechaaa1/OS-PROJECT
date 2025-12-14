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
    int waiting_time_cpu; // Compteur strict d'attente
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
// En cas d'égalité, insère APRÈS (FIFO) pour respecter l'ordre d'arrivée.
void enqueuePriority(Queue* q, Process* p) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->p = p;
    newNode->next = NULL;

    // Cas 1 : Insertion en tête (File vide ou priorité strictement supérieure à la tête)
    if (q->head == NULL || p->priority > q->head->p->priority) {
        newNode->next = q->head;
        q->head = newNode;
        if (q->tail == NULL) q->tail = newNode;
    } 
    // Cas 2 : Insertion milieu/fin
    else {
        Node* current = q->head;
        // On avance tant que le suivant existe et a une priorité >= (on se met derrière les égaux)
        while (current->next != NULL && current->next->p->priority >= p->priority) {
            current = current->next;
        }
        newNode->next = current->next;
        current->next = newNode;
        if (newNode->next == NULL) q->tail = newNode;
    }
    
    // Mise à jour de sécurité pour tail
    if (q->tail != NULL && q->tail->next != NULL) {
        Node* temp = q->head;
        while(temp->next != NULL) temp = temp->next;
        q->tail = temp;
    }
}

// File FIFO classique pour les E/S
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

char* history_cpu[MAX_TIME];
char* history_io[MAX_TIME];

int main(int argc, char *argv[]) {
    Process processes[MAX_PROCESS];
    int process_count = 0;
    char line[256];
    int i; 

    // --- LECTURE DU QUANTUM (ignoré, mais format compatible) ---
    if (fgets(line, sizeof(line), stdin)) {
        // On ignore simplement la ligne quantum si elle existe
    }

    // --- LECTURE DES PROCESSUS ---
    while (fgets(line, sizeof(line), stdin)) {
        if (line[0] == '#' || line[0] == '/' || line[0] == '\n' || line[0] == '\r') continue;
        // On ignore la ligne quantum si elle est présente dans le fichier, car inutile ici
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
        p->priority = numbers[count - 1]; // Le dernier chiffre est la priorité
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

    // --- 2. SIMULATION (PRIORITÉ PRÉEMPTIVE) ---
    int time = 0;
    int processes_finished = 0;
    
    Queue cpuQ, ioQ;
    initQueue(&cpuQ);
    initQueue(&ioQ);

    Process* active_cpu = NULL;
    Process* active_io = NULL;
    
    // Initialisation historique
    for(i=0; i<MAX_TIME; i++) {
        history_cpu[i] = "NULL";
        history_io[i] = "NULL";
    }

    while (processes_finished < process_count && time < MAX_TIME) {
        
        // A. ARRIVÉES
        for (i = 0; i < process_count; i++) {
            if (processes[i].arrival_time == time) {
                // Ajout dans la file triée par priorité
                enqueuePriority(&cpuQ, &processes[i]);
                
                // --- PRÉEMPTION SUR ARRIVÉE ---
                // Condition stricte : Si le nouveau a une priorité STRICTEMENT plus grande
                if (active_cpu != NULL && processes[i].priority > active_cpu->priority) {
                    // Le processus actif est préempté
                    enqueuePriority(&cpuQ, active_cpu);
                    active_cpu = NULL;
                }
            }
        }

        // --- SÉLECTION (Phase 1) ---
        
        // Sélection CPU : On prend la tête de file (la plus haute priorité)
        if (active_cpu == NULL && !isQueueEmpty(&cpuQ)) {
            active_cpu = dequeue(&cpuQ);
        }

        // Sélection E/S
        if (active_io == NULL && !isQueueEmpty(&ioQ)) {
            active_io = dequeue(&ioQ);
        }

        // --- EXÉCUTION (Phase 2) ---

        // B. GESTION CPU
        if (active_cpu != NULL) {
            history_cpu[time] = active_cpu->name;
            active_cpu->remaining_time--;
            
            // Pas de gestion de quantum ici

            if (active_cpu->remaining_time == 0) {
                // Fin du burst
                active_cpu->current_burst_idx++;
                
                if (active_cpu->current_burst_idx >= active_cpu->num_bursts) {
                    // Processus Fini
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
                    
                    // --- PRÉEMPTION SUR RETOUR E/S ---
                    // Si le processus qui revient a une priorité STRICTEMENT supérieure à celui qui tourne
                    if (active_cpu != NULL && !isQueueEmpty(&cpuQ)) {
                        // On vérifie la tête de file (qui est le plus prioritaire en attente)
                        if (cpuQ.head->p->priority > active_cpu->priority) {
                            enqueuePriority(&cpuQ, active_cpu);
                            active_cpu = NULL;
                        }
                    }
                    active_io = NULL;
                }
            }
        }

        // --- D. CALCUL ATTENTE (Cohérence) ---
        // On parcourt la file d'attente CPU. Tous ceux qui y sont attendent.
        Node* currNode = cpuQ.head;
        while(currNode != NULL) {
            currNode->p->waiting_time_cpu++;
            currNode = currNode->next;
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