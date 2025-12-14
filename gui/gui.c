// gui.c — Contrôleur (Controller) : Logique et callbacks
#include <gtk/gtk.h>
#include <ctype.h>
#define DEFAULT_QUANTUM 2 

// Déclarations externes (depuis d'autres fichiers)
char **detect_algorithms(int *count);     // liste.c
void run_algorithm(const char *algo, int quantum);  // run_algorithm.c
void start_gantt_diagram(void);           // tabs.c
void reset_gantt_diagram(void);           // tabs.c
void refresh_statistics(void);            // stats.c
void reset_statistics(void);              // stats.c
const char* get_input_file(void);         // run_algorithm.c
void display_gantt_in_console(void);      // console_gantt.c

// Widgets globaux (utilisés par gui_builder.c et ce fichier)
GtkWidget *entry_quantum = NULL;
GtkWidget *box_quantum_container = NULL;

/* ============================================================
   FONCTION: Vérifier si un algorithme nécessite un quantum
   ============================================================ */
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
    return (g_ascii_strcasecmp(lower_algo, "rr") == 0 || 
            g_ascii_strcasecmp(lower_algo, "roundrobin") == 0|| 
            g_ascii_strcasecmp(lower_algo, "multilevel-aging-rr") == 0|| 
            g_ascii_strcasecmp(lower_algo, "multilevel") == 0);
}

/* ============================================================
   LOGIQUE: Afficher/cacher le champ quantum selon l'algorithme
   ============================================================ */
void update_quantum_state(const char *algo)
{
    if (!algo) {
        gtk_widget_hide(box_quantum_container);
        return;
    }
    
    /* Afficher le champ quantum uniquement pour RR */
    if (requires_quantum(algo)) {
        gtk_widget_show(box_quantum_container);
    } else {
        gtk_widget_hide(box_quantum_container);
    }
}

/* ============================================================
   CALLBACK: Changement d'algorithme dans la combobox
   ============================================================ */
void on_algo_changed(GtkComboBox *combo, gpointer data)
{
    char *name = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));
    if (!name) return;

    update_quantum_state(name);
    g_free(name);
}

/* ============================================================
   CALLBACK: Bouton Réinitialiser
   - Nettoie le diagramme
   - Supprime output.txt
   - Recharge la liste des algorithmes
   ============================================================ */
void on_refresh_clicked(GtkButton *btn, gpointer combo_ptr)
{
    GtkComboBoxText *combo = GTK_COMBO_BOX_TEXT(combo_ptr);

    g_print("\n========================================\n");
    g_print("→ Réinitialisation en cours...\n");
    g_print("========================================\n");
    
    // Réinitialiser le diagramme et supprimer output.txt
    reset_gantt_diagram();
    reset_statistics();

    // Vider la combobox
    gtk_combo_box_text_remove_all(combo);

    // Recharger les algorithmes disponibles
    int count = 0;
    int fifo_index = -1;
    char **algos = detect_algorithms(&count);

    if (!algos) {
        g_print("❌ Aucun algorithme détecté\n");
        return;
    }

    for (int i = 0; i < count; i++) {
        gtk_combo_box_text_append(combo, NULL, algos[i]);

        if (g_strcmp0(algos[i], "fifo") == 0)
            fifo_index = i;

        free(algos[i]);
    }
    free(algos);

    // Sélectionner FIFO par défaut si disponible
    if (fifo_index != -1) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), fifo_index);
        update_quantum_state("fifo");
    } else {
        update_quantum_state(NULL);
    }
    
    g_print("✓ Réinitialisation terminée\n");
    g_print("========================================\n\n");
}

/* ============================================================
   CALLBACK: Bouton Démarrer
   - Valide les entrées (algorithme, quantum si nécessaire)
   - Lance l'algorithme sélectionné
   - Affiche le diagramme de Gantt (GUI + CONSOLE)
   ============================================================ */
void on_start_clicked(GtkButton *btn, gpointer combo_ptr)
{
    GtkComboBoxText *combo = GTK_COMBO_BOX_TEXT(combo_ptr);
    char *algo = gtk_combo_box_text_get_active_text(combo);

    // Validation: algorithme sélectionné ?
    if (!algo) {
        g_print("\n❌ Erreur : Aucun algorithme choisi !\n");
        g_print("   Veuillez sélectionner une politique d'ordonnancement.\n\n");
        return;
    }

    int quantum = 0;  // Par défaut, 0 pour les algos qui n'en ont pas besoin

    // Si l'algorithme nécessite un quantum (Round Robin)
    if (requires_quantum(algo)) {
        const char *q = gtk_entry_get_text(GTK_ENTRY(entry_quantum));
        
        if (!q || !*q || strlen(q) == 0) {
            quantum = DEFAULT_QUANTUM;
            g_print("ℹ Quantum non spécifié, utilisation de la valeur par défaut : %d\n", quantum);
        } else {
            quantum = atoi(q);
            
            if (quantum <= 0) {
                g_print("\n❌ Erreur : Quantum invalide (%d) !\n", quantum);
                g_print("   Le quantum doit être un entier positif.\n");
                g_print("ℹ Utilisation du quantum par défaut : %d\n", DEFAULT_QUANTUM);
                quantum = DEFAULT_QUANTUM;
            }
        }
        
        g_print("→ Algorithme %s avec quantum=%d\n", algo, quantum);
    } else {
        g_print("→ Algorithme %s (pas de quantum requis)\n", algo);
    }

    // Exécuter l'algorithme
    g_print("\n========================================\n");
    g_print("→ Démarrage de %s\n", algo);
    g_print("→ Fichier d'entrée : %s\n", get_input_file());
    g_print("========================================\n");
    
    run_algorithm(algo, quantum);
    
    // Afficher le diagramme dans la console
    display_gantt_in_console();
    
    g_print("→ Chargement du diagramme graphique...\n");
    start_gantt_diagram();
    
    g_print("→ Chargement des statistiques...\n");
    refresh_statistics();
    
    g_print("\n✓ Simulation terminée\n");
    g_print("========================================\n\n");
    
    g_free(algo);
}