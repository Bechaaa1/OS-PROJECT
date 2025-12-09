#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>

/* Déclarations externes */
void gui_init(int argc, char **argv);
void set_input_file(const char *filepath);

int main(int argc, char **argv)
{
    /* Parser les arguments */
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--input=", 8) == 0) {
            set_input_file(argv[i] + 8);
            printf("✓ Utilisation du fichier : %s\n", argv[i] + 8);
            
            /* Retirer cet argument pour GTK */
            for (int j = i; j < argc - 1; j++) {
                argv[j] = argv[j + 1];
            }
            argc--;
            i--;
        }
    }

    if (argc == 1) {
        printf("ℹ Utilisation du fichier par défaut : config/input.txt\n");
    }

    gui_init(argc, argv);
    return 0;
}