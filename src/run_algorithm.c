#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

/* Variable globale pour stocker le fichier d'entrée */
static char input_file[256] = "config/input.txt";  // Valeur par défaut

/* Fonction pour définir le fichier d'entrée */
void set_input_file(const char *filepath) {
    if (filepath && filepath[0] != '\0') {
        snprintf(input_file, sizeof(input_file), "%s", filepath);
        printf("✓ Fichier d'entrée défini : %s\n", input_file);
    }
}

/* Fonction pour obtenir le fichier d'entrée actuel */
const char* get_input_file(void) {
    return input_file;
}

/* Fonction pour vérifier si un algorithme nécessite un quantum */
static int requires_quantum(const char *algo) {
    if (!algo) return 0;
    
    /* Convertir en minuscules pour la comparaison */
    char lower_algo[64];
    int i;
    for (i = 0; algo[i] && i < 63; i++) {
        lower_algo[i] = tolower(algo[i]);
    }
    lower_algo[i] = '\0';
    
    /* Liste des algorithmes qui nécessitent un quantum */
    return (strcmp(lower_algo, "rr") == 0 || 
            strcmp(lower_algo, "multilevel-aging-rr") == 0
            ||
            strcmp(lower_algo, "multilevel") == 0);
}

/* Fonction pour créer un fichier temporaire avec le quantum en première ligne */
static void create_temp_input_with_quantum(int quantum, const char *temp_file) {
    FILE *in = fopen(input_file, "r");
    FILE *out = fopen(temp_file, "w");
    
    if (!in || !out) {
        printf("❌ Erreur : impossible de créer le fichier temporaire\n");
        if (in) fclose(in);
        if (out) fclose(out);
        return;
    }
    
    /* Écrire le quantum en première ligne */
    fprintf(out, "quantum %d\n", quantum);
    
    /* Copier le reste du fichier d'entrée */
    char line[256];
    while (fgets(line, sizeof(line), in)) {
        /* Ignorer les lignes quantum existantes */
        if (strncmp(line, "quantum", 7) != 0) {
            fputs(line, out);
        }
    }
    
    fclose(in);
    fclose(out);
}

void run_algorithm(const char *algo, int quantum)
{
    char cmd[512];
    char temp_file[300] = "temp_input.txt";

    if (!algo) {
        printf("❌ Erreur : algorithme invalide\n");
        return;
    }

    /* Vérifier si l'algorithme nécessite un quantum */
    if (requires_quantum(algo)) {
        /* Utiliser le quantum fourni ou la valeur par défaut */
        int actual_quantum = (quantum > 0) ? quantum : 2;
        
        printf("ℹ L'algorithme %s nécessite un quantum\n", algo);
        printf("→ Quantum utilisé : %d\n", actual_quantum);
        
        /* Créer un fichier temporaire avec le quantum */
        create_temp_input_with_quantum(actual_quantum, temp_file);
        
        /* Exécuter avec le fichier temporaire */
        snprintf(cmd, sizeof(cmd), "./build/%s < %s > output.txt", algo, temp_file);
    } else {
        /* Algorithme sans quantum : ajout de "quantum 0" en première ligne */
        printf("ℹ L'algorithme %s ne nécessite pas de quantum\n", algo);
        printf("→ Ajout de 'quantum 0' par défaut\n");
        
        /* Créer un fichier temporaire avec quantum 0 */
        create_temp_input_with_quantum(0, temp_file);
        
        /* Exécuter avec le fichier temporaire */
        snprintf(cmd, sizeof(cmd), "./build/%s < %s > output.txt", algo, temp_file);
    }

    printf("→ Commande exécutée : %s\n", cmd);

    int ret = system(cmd);
    
    /* Supprimer le fichier temporaire */
    remove(temp_file);
    
    if (ret == 0) {
        printf("✔ Algorithme terminé. Résultat écrit dans output.txt\n");
    } else {
        printf("❌ Erreur lors de l'exécution (code %d)\n", ret);
    }
}