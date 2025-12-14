// console_gantt.c — Diagramme de Gantt simple en ASCII
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_SLOTS 256

typedef struct {
    char name[64];
    char *slots[MAX_SLOTS];
    int count;
} Row;

static Row rows[2];
static int row_count = 0;
static int max_slots = 0;

/* Charger les données depuis output.txt */
static void load_output_file(void) {
    FILE *f = fopen("output.txt", "r");
    if (!f) return;

    char line[256];
    Row *current = NULL;
    row_count = 0;
    max_slots = 0;

    // Libérer la mémoire précédente
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < rows[i].count; j++) {
            if (rows[i].slots[j]) {
                free(rows[i].slots[j]);
                rows[i].slots[j] = NULL;
            }
        }
        rows[i].count = 0;
    }

    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "calcul") || strstr(line, "E/S")) {
            if (row_count < 2) {
                current = &rows[row_count++];
                sscanf(line, "%63s", current->name);
                current->count = 0;
            }
        }
        else if (current && strstr(line, ":")) {
            int index;
            char proc[64];
            if (sscanf(line, "%d : %63s", &index, proc) == 2) {
                current->slots[current->count] = strdup(proc);
                current->count++;
                if (current->count > max_slots)
                    max_slots = current->count;
            }
        }
    }
    fclose(f);
}

/* Trouver la longueur maximale des noms de processus */
static int get_max_name_length(void) {
    int max_len = 4; // Minimum pour "NULL"
    for (int r = 0; r < row_count; r++) {
        for (int i = 0; i < rows[r].count; i++) {
            int len = strlen(rows[r].slots[i]);
            if (len > max_len) max_len = len;
        }
    }
    return max_len;
}

/* Afficher le diagramme de Gantt simple */
void display_gantt_in_console(void) {
    load_output_file();

    if (row_count == 0 || max_slots == 0) {
        printf("\n[Aucune donnée à afficher]\n\n");
        return;
    }

    printf("\n");
    printf("==========================================\n");
    printf("       DIAGRAMME DE GANTT\n");
    printf("==========================================\n\n");

    int cell_width = get_max_name_length() + 2;
    if (cell_width < 6) cell_width = 6;

    // Afficher l'échelle de temps
    printf("       ");
    for (int t = 0; t < max_slots; t++) {
        printf("| T%-*d", cell_width - 3, t);
    }
    printf("|\n");

    // Ligne de séparation
    printf("-------");
    for (int t = 0; t < max_slots; t++) {
        for (int i = 0; i < cell_width + 1; i++) printf("-");
    }
    printf("-\n");

    // Pour chaque rangée (calcul et E/S)
    for (int r = 0; r < row_count; r++) {
        // Nom de la rangée
        printf("%-7s", rows[r].name);

        // Contenu des slots
        for (int i = 0; i < rows[r].count; i++) {
            char *proc = rows[r].slots[i];
            int proc_len = strlen(proc);
            int padding_left = (cell_width - proc_len) / 2;
            int padding_right = cell_width - proc_len - padding_left;

            printf("|");
            for (int p = 0; p < padding_left; p++) printf(" ");
            printf("%s", proc);
            for (int p = 0; p < padding_right; p++) printf(" ");
        }
        printf("|\n");

        // Ligne de séparation
        printf("-------");
        for (int t = 0; t < rows[r].count; t++) {
            for (int i = 0; i < cell_width + 1; i++) printf("-");
        }
        printf("-\n");
    }

    printf("\n");

    // Calculer l'utilisation CPU
    int total_cpu = rows[0].count;
    int active_cpu = 0;
    for (int i = 0; i < rows[0].count; i++) {
        if (strcmp(rows[0].slots[i], "NULL") != 0) {
            active_cpu++;
        }
    }
    double cpu_util = total_cpu > 0 ? (double)active_cpu / total_cpu * 100.0 : 0.0;
    
    printf("Utilisation CPU : %.1f%% (%d/%d slots)\n", cpu_util, active_cpu, total_cpu);
    printf("\n==========================================\n\n");
}