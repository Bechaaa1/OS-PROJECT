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
    int bursts[MAX_BURSTS]; // Alternance: [0]=CPU, [1]=IO, [2]=CPU...
    int num_bursts;
    int current_burst_idx;
    int remaining_time; // Temps restant pour le burst actuel
    int priority;       
    int finished;
    

    // --- AJOUT : Variables pour les statistiques ---
    int end_time;       // Temps de fin d'exécution
    int total_duration; // Durée totale des bursts (CPU + E/S) pour calculer l'attente
} Process;

// --- STRUCTURES LISTE CHAINEE ---
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

// Ajout standard (utilisé pour la file E/S FIFO et pour ajouter dans la file CPU)
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

// Retrait standard FIFO (utilisé uniquement pour la file E/S)
Process* dequeueFIFO(Queue* q) {
    if (q->head == NULL) return NULL;
    Node* temp = q->head;
    Process* p = temp->p;
    q->head = q->head->next;
    if (q->head == NULL) q->tail = NULL;
    free(temp);
    return p;
}

// --- NOUVEAU POUR SRT : Retrait du plus court job (Shortest Remaining Time) ---
// Cette fonction cherche le processus avec le plus petit remaining_time dans la file,
// le retire de la liste et le renvoie.
Process* dequeueShortest(Queue* q) {
    if (q->head == NULL) return NULL;

    Node *prev = NULL, *curr = q->head;
    Node *minPrev = NULL, *minNode = q->head;

    // 1. Trouver le noeud avec le temps restant le plus court
    // En cas d'égalité, on garde le premier trouvé (FIFO pour les égalités)
    while (curr != NULL) {
        if (curr->p->remaining_time < minNode->p->remaining_time) {
            minNode = curr;
            minPrev = prev;
        }
        prev = curr;
        curr = curr->next;
    }

    // 2. Retirer le minNode de la liste
    if (minPrev == NULL) {
        // C'était la tête
        q->head = minNode->next;
    } else {
        // C'était au milieu ou à la fin
        minPrev->next = minNode->next;
    }

    // Mise à jour de la queue (tail) si nécessaire
    if (minNode == q->tail) {
        q->tail = minPrev;
    }

    Process* p = minNode->p;
    free(minNode);
    return p;
}

// Fonction pour juste "regarder" le temps du plus court sans le retirer
// Sert à décider s'il faut préempter le processus actuel
int getShortestTimeInQueue(Queue* q) {
    if (q->head == NULL) return 999999; // Valeur infinie si vide
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

    // --- 1. LECTURE DU FICHIER ---

    FILE *file = stdin;  // Lire depuis l'entrée standard comme fifo.c et rr.c
    if (file == NULL) {
        printf("Erreur : Impossible d'ouvrir le fichier inputSRT.txt\n");
        return 1;
    }

    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || line[0] == '/' || line[0] == '\n' || line[0] == '\r') continue;
        if (strstr(line, "quantum") != NULL) continue; // On ignore la ligne quantum

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
        p->total_duration = 0; // --- AJOUT : Init durée totale
        
        for (i = 0; i < p->num_bursts; i++) {
            p->bursts[i] = numbers[i];
            p->total_duration += numbers[i]; // --- AJOUT : Somme des bursts
        }
        
        p->current_burst_idx = 0;
        p->remaining_time = p->bursts[0];
        p->finished = 0;
        p->end_time = 0; // --- AJOUT : Init temps fin
        
        process_count++;
    }

    // --- 2. SIMULATION SRT ---
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
        
        // A. Vérifier les arrivées
        for (i = 0; i < process_count; i++) {
            if (processes[i].arrival_time == time) {
                enqueue(&cpuQ, &processes[i]);
            }
        }

        // B. GESTION CPU (SRT)
        
        // 1. Préemption : Vérifier si un processus en attente est strictement plus court que l'actif
        if (active_cpu != NULL && !isQueueEmpty(&cpuQ)) {
            int shortestInQueue = getShortestTimeInQueue(&cpuQ);
            if (shortestInQueue < active_cpu->remaining_time) {
                // On préempte : l'actif retourne dans la file
                enqueue(&cpuQ, active_cpu);
                active_cpu = NULL;
            }
        }

        // 2. Sélection : Si personne sur le CPU, prendre le plus court
        if (active_cpu == NULL && !isQueueEmpty(&cpuQ)) {
            active_cpu = dequeueShortest(&cpuQ);
        }

        // 3. Exécution
        if (active_cpu != NULL) {
            history_cpu[time] = active_cpu->name;
            active_cpu->remaining_time--;

            if (active_cpu->remaining_time == 0) {
                // Fin du burst CPU
                active_cpu->current_burst_idx++;
                
                if (active_cpu->current_burst_idx >= active_cpu->num_bursts) {
                    active_cpu->finished = 1;
                    // --- AJOUT : Capture du temps de fin
                    active_cpu->end_time = time + 1;
                    processes_finished++;
                    active_cpu = NULL;
                } else {
                    // Aller en E/S
                    active_cpu->remaining_time = active_cpu->bursts[active_cpu->current_burst_idx];
                    enqueue(&ioQ, active_cpu);
                    active_cpu = NULL;
                }
            }
        }

        // C. GESTION E/S (FIFO Standard)
        if (active_io == NULL && !isQueueEmpty(&ioQ)) {
            active_io = dequeueFIFO(&ioQ); // FIFO pour les I/O
        }

        if (active_io != NULL) {
            history_io[time] = active_io->name;
            active_io->remaining_time--;

            if (active_io->remaining_time == 0) {
                // Fin du burst IO
                active_io->current_burst_idx++;
                
                if (active_io->current_burst_idx >= active_io->num_bursts) {
                    active_io->finished = 1;
                    // --- AJOUT : Capture du temps de fin (si fini sur IO)
                    active_io->end_time = time + 1;
                    processes_finished++;
                    active_io = NULL;
                } else {
                    // Retour en CPU (Ready Queue)
                    active_io->remaining_time = active_io->bursts[active_io->current_burst_idx];
                    enqueue(&cpuQ, active_io);
                    active_io = NULL;
                }
            }
        }

        time++;
    }

    // --- 3. AFFICHAGE ---
    printf("OUTPUT :\n\n");
    
    printf("calcul :\n");
    for (i = 0; i < time; i++) {
        // Affichage commençant à 0
        printf("%d : %s\n", i, history_cpu[i]);
    }

    printf("\nE/S :\n");
    for (i = 0; i < time; i++) {
        // Affichage commençant à 0
        printf("%d : %s\n", i, history_io[i]);
    }

    // --- AJOUT : CALCUL ET AFFICHAGE DES STATISTIQUES ---
    printf("\n--- Statistiques ---\n");
    double total_rotation = 0;
    double total_attente = 0;

    for (i = 0; i < process_count; i++) {
        Process *p = &processes[i];
        
        // Rotation = Fin - Arrivée
        int rotation = p->end_time - p->arrival_time;
        
        // Attente = Rotation - Durée totale d'exécution (Service Time)
        int attente = rotation - p->total_duration;

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