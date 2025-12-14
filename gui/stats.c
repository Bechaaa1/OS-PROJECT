// stats.c — Module pour affichage des statistiques (VERSION CORRIGÉE)
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Variables globales pour les statistiques */
static double avg_rotation = 0.0;
static double avg_attente = 0.0;
static double avg_duree = 0.0;
static int process_count = 0;
static double cpu_utilization = 0.0;
static int has_valid_data = 0;  // Nouveau flag

/* Widgets pour l'affichage */
static GtkWidget *stats_container = NULL;
static GtkWidget *cpu_label = NULL;
static GtkWidget *rotation_label = NULL;
static GtkWidget *attente_label = NULL;
static GtkWidget *duree_label = NULL;
static GtkWidget *empty_state_box = NULL;
static GtkWidget *stats_grid = NULL;

/* Calculer l'utilisation CPU depuis output.txt */
static void calculate_cpu_utilization(void) {
    FILE *f = fopen("output.txt", "r");
    if (!f) {
        cpu_utilization = 0.0;
        return;
    }

    char line[256];
    int total_slots = 0;
    int active_slots = 0;
    int in_calcul = 0;

    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "calcul :")) {
            in_calcul = 1;
            continue;
        }
        if (strstr(line, "E/S :") || strstr(line, "--- Statistiques ---")) {
            in_calcul = 0;
            continue;
        }

        if (in_calcul) {
            int time_val;
            char proc_name[64];
            if (sscanf(line, "%d : %63s", &time_val, proc_name) == 2) {
                total_slots++;
                if (strcmp(proc_name, "NULL") != 0) {
                    active_slots++;
                }
            }
        }
    }

    fclose(f);
    cpu_utilization = total_slots > 0 ? (double)active_slots / total_slots * 100.0 : 0.0;
}

/* Charger les statistiques depuis output.txt */
void load_statistics(void) {
    FILE *f = fopen("output.txt", "r");
    if (!f) {
        avg_rotation = 0.0;
        avg_attente = 0.0;
        avg_duree = 0.0;
        process_count = 0;
        cpu_utilization = 0.0;
        has_valid_data = 0;
        return;
    }

    char line[256];
    int found_stats = 0;
    
    // Réinitialiser
    avg_rotation = 0.0;
    avg_attente = 0.0;
    avg_duree = 0.0;
    process_count = 0;
    
    while (fgets(line, sizeof(line), f)) {
        /* Lire les statistiques depuis la section dédiée */
        if (strstr(line, "--- Statistiques ---")) {
            found_stats = 1;
            continue;
        }
        
        if (found_stats) {
            if (strstr(line, "Temps de rotation moyen")) {
                if (sscanf(line, "Temps de rotation moyen : %lf", &avg_rotation) == 1) {
                    // Valide
                }
                continue;
            }
            if (strstr(line, "Temps d'attente moyen")) {
                if (sscanf(line, "Temps d'attente moyen : %lf", &avg_attente) == 1) {
                    // Valide
                }
                continue;
            }
        }
    }
    
    fclose(f);
    
    /* La durée moyenne est simplement rotation - attente (Service Time moyen) */
    avg_duree = avg_rotation - avg_attente;
    if (avg_duree < 0) avg_duree = 0.0;  // Sécurité
    
    /* Calculer l'utilisation CPU */
    calculate_cpu_utilization();
    
    /* Déterminer si on a des données valides :
     * On a des données si rotation > 0 (ce qui signifie qu'au moins un processus a été traité)
     */
    has_valid_data = (avg_rotation > 0.0001);  // Tolérance pour les flottants
    
    if (has_valid_data) {
        /* Compter approximativement le nombre de processus depuis le fichier d'entrée */
        FILE *input_file = fopen("input.txt", "r");
        if (input_file) {
            process_count = 0;
            char input_line[256];
            while (fgets(input_line, sizeof(input_line), input_file)) {
                // Ignorer les commentaires et lignes vides
                if (input_line[0] == '#' || input_line[0] == '/' || 
                    input_line[0] == '\n' || input_line[0] == '\r') continue;
                if (strstr(input_line, "quantum") != NULL) continue;
                
                // Si la ligne contient au moins un mot, c'est probablement un processus
                char first_word[64];
                if (sscanf(input_line, "%63s", first_word) == 1) {
                    process_count++;
                }
            }
            fclose(input_file);
        }
    }
    
    g_print("✓ Statistiques chargées\n");
    g_print("  → Rotation moyenne : %.2f\n", avg_rotation);
    g_print("  → Attente moyenne : %.2f\n", avg_attente);
    g_print("  → Durée moyenne : %.2f\n", avg_duree);
    g_print("  → Utilisation CPU : %.1f%%\n", cpu_utilization);
    g_print("  → Processus détectés : %d\n", process_count);
    g_print("  → Données valides : %s\n", has_valid_data ? "OUI" : "NON");
}

/* Créer une carte de métrique avec son label parent */
static GtkWidget* create_metric_card(const char *title, GtkWidget **value_label_out) {
    /* Frame principal */
    GtkWidget *event_box = gtk_event_box_new();
    
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(box), 20);
    gtk_container_add(GTK_CONTAINER(event_box), box);
    
    /* Titre */
    GtkWidget *title_label = gtk_label_new(NULL);
    char title_markup[256];
    snprintf(title_markup, sizeof(title_markup), 
             "<span foreground='#8090A0' size='11000'>%s</span>", title);
    gtk_label_set_markup(GTK_LABEL(title_label), title_markup);
    gtk_widget_set_halign(title_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(box), title_label, FALSE, FALSE, 0);
    
    /* Valeur */
    GtkWidget *value_label = gtk_label_new("--");
    gtk_widget_set_halign(value_label, GTK_ALIGN_START);
    gtk_widget_set_margin_top(value_label, 5);
    gtk_box_pack_start(GTK_BOX(box), value_label, FALSE, FALSE, 0);
    
    *value_label_out = value_label;
    
    /* Style CSS pour la carte */
    GtkCssProvider *css_provider = gtk_css_provider_new();
    const char *css = 
        "* {"
        "  background-color: #1A2332;"
        "  border-radius: 12px;"
        "  border: 1px solid #2D3D54;"
        "}";
    gtk_css_provider_load_from_data(css_provider, css, -1, NULL);
    gtk_style_context_add_provider(gtk_widget_get_style_context(event_box),
                                   GTK_STYLE_PROVIDER(css_provider),
                                   GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref(css_provider);
    
    return event_box;
}

/* Mettre à jour les labels avec les valeurs actuelles */
static void update_stats_display(void) {
    if (!cpu_label || !rotation_label || !attente_label || !duree_label) {
        return;
    }
    
    /* Afficher/cacher selon la présence de données valides */
    if (empty_state_box) {
        if (has_valid_data) {
            gtk_widget_hide(empty_state_box);
        } else {
            gtk_widget_show(empty_state_box);
        }
    }
    if (stats_grid) {
        if (has_valid_data) {
            gtk_widget_show(stats_grid);
        } else {
            gtk_widget_hide(stats_grid);
        }
    }
    
    if (!has_valid_data) {
        return;
    }
    
    /* Mettre à jour CPU */
    char cpu_text[128];
    const char *cpu_color = cpu_utilization >= 70 ? "#2ECC71" :
                            cpu_utilization >= 40 ? "#FFC107" : "#59B3FF";
    snprintf(cpu_text, sizeof(cpu_text), 
             "<span font='22' weight='bold' foreground='%s'>%.1f%%</span>", 
             cpu_color, cpu_utilization);
    gtk_label_set_markup(GTK_LABEL(cpu_label), cpu_text);
    
    /* Mettre à jour Rotation */
    char rotation_text[128];
    snprintf(rotation_text, sizeof(rotation_text), 
             "<span font='22' weight='bold' foreground='#59B3FF'>%.2f</span> "
             "<span foreground='#8090A0' size='10000'>unités</span>", 
             avg_rotation);
    gtk_label_set_markup(GTK_LABEL(rotation_label), rotation_text);
    
    /* Mettre à jour Attente */
    char attente_text[128];
    snprintf(attente_text, sizeof(attente_text), 
             "<span font='22' weight='bold' foreground='#FFC107'>%.2f</span> "
             "<span foreground='#8090A0' size='10000'>unités</span>", 
             avg_attente);
    gtk_label_set_markup(GTK_LABEL(attente_label), attente_text);
    
    /* Mettre à jour Durée */
    char duree_text[128];
    snprintf(duree_text, sizeof(duree_text), 
             "<span font='22' weight='bold' foreground='#2ECC71'>%.2f</span> "
             "<span foreground='#8090A0' size='10000'>unités</span>", 
             avg_duree);
    gtk_label_set_markup(GTK_LABEL(duree_label), duree_text);
}

/* Fonction publique pour créer l'onglet statistiques */
GtkWidget* create_stats_page(void) {
    /* Conteneur principal */
    stats_container = gtk_event_box_new();
    
    /* Style CSS pour le fond sombre */
    GtkCssProvider *bg_css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(bg_css,
                                     "* { background-color: #0F1419; }",
                                     -1, NULL);
    gtk_style_context_add_provider(gtk_widget_get_style_context(stats_container),
                                   GTK_STYLE_PROVIDER(bg_css),
                                   GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref(bg_css);
    
    /* Box principal avec marges */
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_container_set_border_width(GTK_CONTAINER(main_box), 30);
    gtk_container_add(GTK_CONTAINER(stats_container), main_box);
    
    /* Titre principal */
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), 
                        "<span font='22' weight='bold' foreground='#E8F0FF'>Statistiques d'ordonnancement</span>");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(main_box), title, FALSE, FALSE, 0);
    
    /* État vide - centré verticalement */
    empty_state_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_valign(empty_state_box, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(empty_state_box, GTK_ALIGN_CENTER);
    gtk_widget_set_vexpand(empty_state_box, TRUE);
    
    GtkWidget *empty_icon = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(empty_icon),
                        "<span font='48' foreground='#59B3FF'>📊</span>");
    gtk_box_pack_start(GTK_BOX(empty_state_box), empty_icon, FALSE, FALSE, 0);
    
    GtkWidget *empty_msg = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(empty_msg),
                        "<span font='16' weight='bold' foreground='#E8F0FF'>"
                        "Lancez une simulation pour voir les statistiques</span>");
    gtk_label_set_justify(GTK_LABEL(empty_msg), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start(GTK_BOX(empty_state_box), empty_msg, FALSE, FALSE, 10);
    
    gtk_box_pack_start(GTK_BOX(main_box), empty_state_box, TRUE, TRUE, 0);
    
    /* Grille de statistiques (2x2) */
    stats_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(stats_grid), 20);
    gtk_grid_set_column_spacing(GTK_GRID(stats_grid), 20);
    gtk_widget_set_hexpand(stats_grid, TRUE);
    gtk_widget_set_vexpand(stats_grid, TRUE);
    gtk_box_pack_start(GTK_BOX(main_box), stats_grid, TRUE, TRUE, 0);
    
    /* Créer les 4 cartes */
    GtkWidget *cpu_card = create_metric_card("Utilisation CPU", &cpu_label);
    gtk_widget_set_hexpand(cpu_card, TRUE);
    gtk_widget_set_vexpand(cpu_card, TRUE);
    gtk_grid_attach(GTK_GRID(stats_grid), cpu_card, 0, 0, 1, 1);
    
    GtkWidget *rotation_card = create_metric_card("Rotation moyenne", &rotation_label);
    gtk_widget_set_hexpand(rotation_card, TRUE);
    gtk_widget_set_vexpand(rotation_card, TRUE);
    gtk_grid_attach(GTK_GRID(stats_grid), rotation_card, 1, 0, 1, 1);
    
    GtkWidget *attente_card = create_metric_card("Attente moyenne", &attente_label);
    gtk_widget_set_hexpand(attente_card, TRUE);
    gtk_widget_set_vexpand(attente_card, TRUE);
    gtk_grid_attach(GTK_GRID(stats_grid), attente_card, 0, 1, 1, 1);
    
    GtkWidget *duree_card = create_metric_card("Durée moyenne", &duree_label);
    gtk_widget_set_hexpand(duree_card, TRUE);
    gtk_widget_set_vexpand(duree_card, TRUE);
    gtk_grid_attach(GTK_GRID(stats_grid), duree_card, 1, 1, 1, 1);
    
    /* Initialement, afficher l'état vide */
    gtk_widget_show_all(empty_state_box);
    gtk_widget_hide(stats_grid);
    
    /* Afficher tous les widgets */
    gtk_widget_show_all(stats_container);
    
    return stats_container;
}

/* Fonction publique pour rafraîchir les statistiques */
void refresh_statistics(void) {
    load_statistics();
    update_stats_display();
}

/* Fonction publique pour réinitialiser les statistiques */
void reset_statistics(void) {
    avg_rotation = 0.0;
    avg_attente = 0.0;
    avg_duree = 0.0;
    process_count = 0;
    cpu_utilization = 0.0;
    has_valid_data = 0;
    update_stats_display();
}