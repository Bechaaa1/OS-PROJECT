#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --- détecter dynamiquement les fichiers .c du dossier policies/ --- */
char **detect_algorithms(int *count)
{
    DIR *dir = opendir("policies");
    if (!dir) {
        printf("Erreur : impossible d’ouvrir le dossier policies/\n");
        *count = 0;
        return NULL;
    }

    char **list = malloc(sizeof(char*) * 100);
    int i = 0;
    struct dirent *e;

    while ((e = readdir(dir)) != NULL)
    {
        if (strstr(e->d_name, ".c"))
        {
            char *name = strdup(e->d_name);
            char *dot = strrchr(name, '.');
            if (dot) *dot = '\0';
            list[i++] = name;
        }
    }

    closedir(dir);
    *count = i;
    return list;
}
