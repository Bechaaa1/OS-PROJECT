// policies/srt.c — Shortest Remaining Time (SRT) avec statistiques et DEBUG
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
    
    // --- Variables pour les statistiques ---
    int end_time;       // Temps de fin d'exécution
    int total_duration; // Durée totale des bursts (CPU + E/S) pour calculer l'attente
    
    // --- Compteur spécifique pour l'attente en file CPU uniquement ---
    int waiting_time_cpu;
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

// --- POUR SRT : Retrait du plus court job (Shortest Remaining Time) ---
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
    int i, j; 

    // --- 1. LECTURE DEPUIS STDIN (compatible avec le projet) ---
    
    // Lire la première ligne (quantum - ignoré pour SRT)
    if (fgets(line, sizeof(line), stdin)) {
        // On ignore la ligne quantum car SRT n'en a pas besoin
    }

    // Lire les processus
    while (fgets(line, sizeof(line), stdin)) {
        // Ignorer les commentaires et lignes vides
        if (line[0] == '#' || line[0] == '/' || line[0] == '\n' || line[0] == '\r') 
            continue;
        if (strstr(line, "quantum") != NULL) 
            continue;

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

    // --- DEBUG : AFFICHAGE DU PARSING ---
    fprintf(stderr, "\n=== DEBUG PARSING ===\n");
    for (i = 0; i < process_count; i++) {
        Process *p = &processes[i];
        fprintf(stderr, "%s: arrivée=%d, priorité=%d, num_bursts=%d, bursts=[", 
               p->name, p->arrival_time, p->priority, p->num_bursts);
        for (j = 0; j < p->num_bursts; j++) {
            fprintf(stderr, "%d", p->bursts[j]);
            if (j < p->num_bursts - 1) fprintf(stderr, ", ");
        }
        fprintf(stderr, "], total_duration=%d\n", p->total_duration);
    }
    fprintf(stderr, "=====================\n\n");

    // --- 2. SIMULATION SRT ---
    int time = 0;
    int processes_finished = 0;
    
    Queue cpuQ, ioQ;
    initQueue(&cpuQ);
    initQueue(&ioQ);

    Process* active_cpu = NULL;
    Process* active_io = NULL;

    // Initialisation historique
    for(i = 0; i < MAX_TIME; i++) {
        history_cpu[i] = "NULL";
        history_io[i] = "NULL";
    }

    while (processes_finished < process_count && time < MAX_TIME) {
        
        // A. Vérifier les arrivées
        for (i = 0; i < process_count; i++) {
            if (processes[i].arrival_time == time) {
                enqueue(&cpuQ, &processes[i]);
                fprintf(stderr, "[t=%d] %s arrive\n", time, processes[i].name);
            }
        }

        // --- PHASE DE SELECTION (AVANT EXECUTION) ---

        // 1. SRT Préemption : Vérifier si un processus en attente est strictement plus court que l'actif
        if (active_cpu != NULL && !isQueueEmpty(&cpuQ)) {
            int shortestInQueue = getShortestTimeInQueue(&cpuQ);
            if (shortestInQueue < active_cpu->remaining_time) {
                // On préempte : l'actif retourne dans la file
                fprintf(stderr, "[t=%d] Préemption: %s (remaining=%d) préempté par processus avec remaining=%d\n", 
                       time, active_cpu->name, active_cpu->remaining_time, shortestInQueue);
                enqueue(&cpuQ, active_cpu);
                active_cpu = NULL;
            }
        }

        // 2. Sélection CPU : Si personne sur le CPU, prendre le plus court
        if (active_cpu == NULL && !isQueueEmpty(&cpuQ)) {
            active_cpu = dequeueShortest(&cpuQ);
            fprintf(stderr, "[t=%d] CPU: %s démarre/reprend (remaining=%d)\n", 
                   time, active_cpu->name, active_cpu->remaining_time);
        }

        // 3. Sélection E/S : Si personne sur l'IO, prendre le suivant (FIFO)
        if (active_io == NULL && !isQueueEmpty(&ioQ)) {
            active_io = dequeueFIFO(&ioQ);
            fprintf(stderr, "[t=%d] E/S: %s démarre (remaining=%d)\n", 
                   time, active_io->name, active_io->remaining_time);
        }

        // --- PHASE D'EXECUTION ---

        // B. Execution CPU
        if (active_cpu != NULL) {
            history_cpu[time] = active_cpu->name;
            active_cpu->remaining_time--;

            if (active_cpu->remaining_time == 0) {
                // Fin du burst CPU
                active_cpu->current_burst_idx++;
                
                fprintf(stderr, "[t=%d] %s termine burst CPU (idx=%d, num_bursts=%d)\n", 
                       time, active_cpu->name, active_cpu->current_burst_idx, active_cpu->num_bursts);
                
                if (active_cpu->current_burst_idx >= active_cpu->num_bursts) {
                    active_cpu->finished = 1;
                    active_cpu->end_time = time + 1;
                    processes_finished++;
                    fprintf(stderr, "[t=%d] %s TERMINE complètement (end_time=%d)\n", 
                           time, active_cpu->name, active_cpu->end_time);
                    active_cpu = NULL;
                } else {
                    // Aller en E/S
                    active_cpu->remaining_time = active_cpu->bursts[active_cpu->current_burst_idx];
                    fprintf(stderr, "[t=%d] %s va en E/S (remaining=%d)\n", 
                           time, active_cpu->name, active_cpu->remaining_time);
                    enqueue(&ioQ, active_cpu);
                    active_cpu = NULL;
                }
            }
        }

        // C. Execution E/S
        if (active_io != NULL) {
            history_io[time] = active_io->name;
            active_io->remaining_time--;

            if (active_io->remaining_time == 0) {
                // Fin du burst IO
                active_io->current_burst_idx++;
                
                fprintf(stderr, "[t=%d] %s termine burst E/S (idx=%d, num_bursts=%d)\n", 
                       time, active_io->name, active_io->current_burst_idx, active_io->num_bursts);
                
                if (active_io->current_burst_idx >= active_io->num_bursts) {
                    active_io->finished = 1;
                    active_io->end_time = time + 1;
                    processes_finished++;
                    fprintf(stderr, "[t=%d] %s TERMINE complètement (end_time=%d)\n", 
                           time, active_io->name, active_io->end_time);
                    active_io = NULL;
                } else {
                    // Retour en CPU (Ready Queue)
                    active_io->remaining_time = active_io->bursts[active_io->current_burst_idx];
                    fprintf(stderr, "[t=%d] %s retourne en file CPU (remaining=%d)\n", 
                           time, active_io->name, active_io->remaining_time);
                    enqueue(&cpuQ, active_io);
                    active_io = NULL;
                }
            }
        }

        // --- CALCUL D'ATTENTE ---
        // On incrémente le compteur pour tous les processus qui sont dans la file d'attente CPU.
        Node* currNode = cpuQ.head;
        while(currNode != NULL) {
            currNode->p->waiting_time_cpu++;
            currNode = currNode->next;
        }

        time++;
    }

    fprintf(stderr, "\n=== FIN SIMULATION ===\n\n");

    // --- 3. AFFICHAGE ---
    printf("OUTPUT :\n\n");
    
    printf("calcul :\n");
    for (i = 0; i < time; i++) {
        printf("%d : %s\n", i, history_cpu[i]);
    }

    printf("\nE/S :\n");
    for (i = 0; i < time; i++) {
        printf("%d : %s\n", i, history_io[i]);
    }

    // --- CALCUL ET AFFICHAGE DES STATISTIQUES ---
    printf("\n--- Statistiques ---\n");
    double total_rotation = 0;
    double total_attente = 0;

    fprintf(stderr, "\n=== DÉTAIL STATISTIQUES ===\n");
    for (i = 0; i < process_count; i++) {
        Process *p = &processes[i];
        
        // Rotation = Fin - Arrivée
        int rotation = p->end_time - p->arrival_time;
        
        // Attente = Compteur strict CPU
        int attente = p->waiting_time_cpu;

        fprintf(stderr, "%s: rotation=%d (%d-%d), attente=%d\n", 
               p->name, rotation, p->end_time, p->arrival_time, attente);

        total_rotation += rotation;
        total_attente += attente;
    }
    fprintf(stderr, "===========================\n\n");

    if (process_count > 0) {
        printf("Temps de rotation moyen : %.2f\n", total_rotation / process_count);
        printf("Temps d'attente moyen : %.2f\n", total_attente / process_count);
    } else {
        printf("Aucun processus traité.\n");
    }

    return 0;
}