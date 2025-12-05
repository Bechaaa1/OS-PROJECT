#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>  // Pour strcasecmp

void run_algorithm(const char *algo, int quantum)
{
    char cmd[256];

    printf(">>> Exécution de l'algorithme : %s\n", algo);

    // Nettoyage avant exécution
    remove("output.txt");

    // Construction de la commande (comparaison insensible à la casse)
    if (strcasecmp(algo, "rr") == 0 || strcasecmp(algo, "roundrobin") == 0) {
        printf("→ Quantum utilisé : %d\n", quantum);

        // RR nécessite un quantum -> on utilise un écho
        snprintf(cmd, sizeof(cmd),
                 "echo \"quantum %d\" | ./build/rr < config/input.txt > output.txt",
                 quantum);
    }
    else if (strcasecmp(algo, "fifo") == 0) {
        snprintf(cmd, sizeof(cmd),
                 "./build/fifo < config/input.txt > output.txt");
    }
    else if (strcasecmp(algo, "srt") == 0 || strcasecmp(algo, "srtf") == 0) {
        snprintf(cmd, sizeof(cmd),
                 "./build/srt < config/input.txt > output.txt");
    }
    else {
        printf("❌ Algorithme inconnu : %s\n", algo);
        return;
    }

    printf("→ Commande exécutée : %s\n", cmd);

    // Lancer le programme
    int status = system(cmd);

    if (status == -1)
        printf("❌ Erreur lors de l'exécution.\n");
    else
        printf("✔ Algorithme terminé. Résultat écrit dans output.txt\n");
}