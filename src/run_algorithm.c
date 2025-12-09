#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

void run_algorithm(const char *algo, int quantum)
{
    char cmd[512];
    char quantum_arg[64] = "";

    if (!algo) {
        printf("❌ Erreur : algorithme invalide\n");
        return;
    }

    // Construire l'argument quantum si nécessaire
    if (quantum > 0) {
        snprintf(quantum_arg, sizeof(quantum_arg), "echo 'quantum %d' | cat - %s", quantum, input_file);
        snprintf(cmd, sizeof(cmd), "%s | ./build/%s > output.txt", quantum_arg, algo);
    } else {
        snprintf(cmd, sizeof(cmd), "./build/%s < %s > output.txt", algo, input_file);
    }

    printf("\n→ Commande exécutée : %s\n", cmd);

    int ret = system(cmd);
    
    if (ret == 0) {
        printf("✔ Algorithme terminé. Résultat écrit dans output.txt\n");
    } else {
        printf("❌ Erreur lors de l'exécution (code %d)\n", ret);
    }
}