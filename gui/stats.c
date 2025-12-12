// stats.c — Module pour affichage des statistiques avec badge CPU
#include <gtk/gtk.h>
#include <cairo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Variables globales pour les statistiques */
static double avg_rotation = 0.0;
static double avg_attente = 0.0;
static double avg_duree = 0.0;
static int process_count = 0;
static GtkWidget *stats_drawing_area = NULL;

/* Variables pour l'utilisation CPU */
static gboolean show_cpu_utilization = TRUE;
static double cpu_utilization = 0.0;

/* Couleurs pour le thème */
static const double COLOR_BG_R = 0.06, COLOR_BG_G = 0.08, COLOR_BG_B = 0.10;
static const double COLOR_CARD_R = 0.16, COLOR_CARD_G = 0.23, COLOR_CARD_B = 0.32;
static const double COLOR_ACCENT_R = 0.35, COLOR_ACCENT_G = 0.77, COLOR_ACCENT_B = 1.0;
static const double COLOR_TEXT_R = 0.91, COLOR_TEXT_G = 0.94, COLOR_TEXT_B = 1.0;
static const double COLOR_TEXT_MUTED_R = 0.50, COLOR_TEXT_MUTED_G = 0.60, COLOR_TEXT_MUTED_B = 0.75;
static const double COLOR_SUCCESS_R = 0.18, COLOR_SUCCESS_G = 0.80, COLOR_SUCCESS_B = 0.44;
static const double COLOR_WARNING_R = 1.0, COLOR_WARNING_G = 0.76, COLOR_WARNING_B = 0.03;
static const double COLOR_BORDER_R = 0.18, COLOR_BORDER_G = 0.28, COLOR_BORDER_B = 0.42;

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
        return;
    }

    char line[256];
    double total_duree = 0.0;
    int proc_count = 0;
    
    avg_rotation = 0.0;
    avg_attente = 0.0;
    
    int in_calcul = 0;
    
    while (fgets(line, sizeof(line), f)) {
        /* Lire les moyennes depuis la section Statistiques */
        if (strstr(line, "Temps de rotation moyen")) {
            sscanf(line, "Temps de rotation moyen : %lf", &avg_rotation);
            continue;
        }
        if (strstr(line, "Temps d'attente moyen")) {
            sscanf(line, "Temps d'attente moyen : %lf", &avg_attente);
            continue;
        }
        
        /* Calculer la durée moyenne depuis le diagramme */
        if (strstr(line, "calcul :")) {
            in_calcul = 1;
            continue;
        }
        if (strstr(line, "E/S :") || strstr(line, "--- Statistiques ---")) {
            in_calcul = 0;
            continue;
        }
        
        /* Compter les processus uniques dans la section calcul */
        if (in_calcul) {
            int time_val;
            char proc_name[64];
            if (sscanf(line, "%d : %63s", &time_val, proc_name) == 2) {
                if (strcmp(proc_name, "NULL") != 0) {
                    proc_count++;
                }
            }
        }
    }
    
    fclose(f);
    
    /* Calculer durée moyenne approximative */
    if (avg_rotation > 0 && avg_attente >= 0) {
        avg_duree = avg_rotation - avg_attente;
    }
    
    process_count = (proc_count > 0) ? 1 : 0;
    
    /* Calculer l'utilisation CPU */
    calculate_cpu_utilization();
    
    g_print("✓ Statistiques chargées\n");
    g_print("  → Rotation moyenne : %.2f\n", avg_rotation);
    g_print("  → Attente moyenne : %.2f\n", avg_attente);
    g_print("  → Durée moyenne : %.2f\n", avg_duree);
    g_print("  → Utilisation CPU : %.1f%%\n", cpu_utilization);
}

/* Helper: rounded rectangle */
static void draw_simple_rounded_rect(cairo_t *cr, double x, double y, double w, double h, double r) {
    const double PI = 3.14159265358979323846;
    cairo_new_path(cr);
    cairo_arc(cr, x + r, y + r, r, PI, 3*PI/2);
    cairo_arc(cr, x + w - r, y + r, r, 3*PI/2, 0);
    cairo_arc(cr, x + w - r, y + h - r, r, 0, PI/2);
    cairo_arc(cr, x + r, y + h - r, r, PI/2, PI);
    cairo_close_path(cr);
}

/* Dessiner une carte avec ombre et coins arrondis */
void draw_card(cairo_t *cr, double x, double y, double w, double h, double radius) {
    const double PI = 3.14159265358979323846;
    
    /* Ombre */
    cairo_set_source_rgba(cr, 0, 0, 0, 0.3);
    cairo_new_path(cr);
    cairo_arc(cr, x + radius + 2, y + radius + 2, radius, PI, 3*PI/2);
    cairo_arc(cr, x + w - radius + 2, y + radius + 2, radius, 3*PI/2, 0);
    cairo_arc(cr, x + w - radius + 2, y + h - radius + 2, radius, 0, PI/2);
    cairo_arc(cr, x + radius + 2, y + h - radius + 2, radius, PI/2, PI);
    cairo_close_path(cr);
    cairo_fill(cr);
    
    /* Carte principale */
    cairo_new_path(cr);
    cairo_arc(cr, x + radius, y + radius, radius, PI, 3*PI/2);
    cairo_arc(cr, x + w - radius, y + radius, radius, 3*PI/2, 0);
    cairo_arc(cr, x + w - radius, y + h - radius, radius, 0, PI/2);
    cairo_arc(cr, x + radius, y + h - radius, radius, PI/2, PI);
    cairo_close_path(cr);
    
    cairo_set_source_rgb(cr, COLOR_CARD_R, COLOR_CARD_G, COLOR_CARD_B);
    cairo_fill(cr);
}

/* Badge d'utilisation CPU */
static void draw_cpu_badge(cairo_t *cr, double x, double y, double w, double h) {
    const double PI = 3.14159265358979323846;
    if (!show_cpu_utilization) return;

    /* Fond de la carte */
    draw_card(cr, x, y, w, h, 12);
    
    /* Gradient sur la carte */
    draw_simple_rounded_rect(cr, x, y, w, h, 12);
    cairo_pattern_t *bg_pat = cairo_pattern_create_linear(x, y, x, y + h);
    cairo_pattern_add_color_stop_rgba(bg_pat, 0, 0.10, 0.14, 0.20, 0.3);
    cairo_pattern_add_color_stop_rgba(bg_pat, 1, 0.08, 0.12, 0.18, 0.3);
    cairo_set_source(cr, bg_pat);
    cairo_fill(cr);
    cairo_pattern_destroy(bg_pat);

    /* Bordure */
    draw_simple_rounded_rect(cr, x, y, w, h, 12);
    cairo_set_source_rgba(cr, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, 0.3);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);

    /* Couleur du badge selon le niveau d'utilisation */
    double badge_r = cpu_utilization >= 70 ? COLOR_SUCCESS_R :
                     cpu_utilization >= 40 ? COLOR_WARNING_R : COLOR_ACCENT_R;
    double badge_g = cpu_utilization >= 70 ? COLOR_SUCCESS_G :
                     cpu_utilization >= 40 ? COLOR_WARNING_G : COLOR_ACCENT_G;
    double badge_b = cpu_utilization >= 70 ? COLOR_SUCCESS_B :
                     cpu_utilization >= 40 ? COLOR_WARNING_B : COLOR_ACCENT_B;

    /* Icône CPU avec cercle de fond */
    cairo_arc(cr, x + 30, y + h/2, 18, 0, 2*PI);
    cairo_set_source_rgba(cr, badge_r, badge_g, badge_b, 0.2);
    cairo_fill(cr);
    
    cairo_arc(cr, x + 30, y + h/2, 18, 0, 2*PI);
    cairo_set_source_rgba(cr, badge_r, badge_g, badge_b, 0.8);
    cairo_set_line_width(cr, 2);
    cairo_stroke(cr);

    /* Dessin simplifié du CPU */
    cairo_set_source_rgb(cr, badge_r, badge_g, badge_b);
    cairo_set_line_width(cr, 2.5);
    cairo_rectangle(cr, x + 23, y + h/2 - 7, 14, 14);
    cairo_stroke(cr);

    int i;
    for (i = 0; i < 3; i++) {
        cairo_move_to(cr, x + 20, y + h/2 - 5 + i*4);
        cairo_line_to(cr, x + 23, y + h/2 - 5 + i*4);
        cairo_move_to(cr, x + 37, y + h/2 - 5 + i*4);
        cairo_line_to(cr, x + 40, y + h/2 - 5 + i*4);
    }
    cairo_set_line_width(cr, 1.5);
    cairo_stroke(cr);

    /* Titre */
    cairo_set_source_rgba(cr, COLOR_TEXT_R, COLOR_TEXT_G, COLOR_TEXT_B, 0.7);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 13);
    cairo_move_to(cr, x + 60, y + 30);
    cairo_show_text(cr, "Utilisation CPU");

    /* Pourcentage */
    char percent[16];
    snprintf(percent, sizeof(percent), "%.1f%%", cpu_utilization);
    
    cairo_set_source_rgb(cr, COLOR_TEXT_R, COLOR_TEXT_G, COLOR_TEXT_B);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 24);
    cairo_move_to(cr, x + 60, y + 62);
    cairo_show_text(cr, percent);
}

/* Dessiner un indicateur de métrique */
void draw_metric_card(cairo_t *cr, double x, double y, double w, double h,
                      const char *title, double value, const char *unit,
                      double color_r, double color_g, double color_b) {
    draw_card(cr, x, y, w, h, 12);
    
    /* Icône colorée */
    cairo_arc(cr, x + 30, y + h/2, 18, 0, 2*M_PI);
    cairo_set_source_rgba(cr, color_r, color_g, color_b, 0.2);
    cairo_fill(cr);
    
    cairo_arc(cr, x + 30, y + h/2, 18, 0, 2*M_PI);
    cairo_set_source_rgba(cr, color_r, color_g, color_b, 0.8);
    cairo_set_line_width(cr, 2);
    cairo_stroke(cr);
    
    /* Titre */
    cairo_set_source_rgba(cr, COLOR_TEXT_R, COLOR_TEXT_G, COLOR_TEXT_B, 0.7);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 13);
    cairo_move_to(cr, x + 60, y + 30);
    cairo_show_text(cr, title);
    
    /* Valeur avec unité sur la même ligne */
    char full_text[128];
    snprintf(full_text, sizeof(full_text), "%.2f %s", value, unit);
    
    cairo_set_source_rgb(cr, COLOR_TEXT_R, COLOR_TEXT_G, COLOR_TEXT_B);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 24);
    cairo_move_to(cr, x + 60, y + 62);
    cairo_show_text(cr, full_text);
}

/* État vide */
void draw_stats_empty_state(cairo_t *cr, int width, int height) {
    cairo_arc(cr, width/2, height/2 - 60, 50, 0, 2*M_PI);
    cairo_set_source_rgba(cr, COLOR_ACCENT_R, COLOR_ACCENT_G, COLOR_ACCENT_B, 0.15);
    cairo_fill(cr);
    
    /* Icône de graphique */
    cairo_set_source_rgb(cr, COLOR_ACCENT_R, COLOR_ACCENT_G, COLOR_ACCENT_B);
    cairo_set_line_width(cr, 3);
    
    int i;
    for (i = 0; i < 3; i++) {
        cairo_rectangle(cr, width/2 - 25 + i*20, height/2 - 70 + i*10, 15, 50 - i*10);
        cairo_fill(cr);
    }
    
    cairo_set_source_rgb(cr, COLOR_TEXT_R, COLOR_TEXT_G, COLOR_TEXT_B);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 22);
    
    const char *msg = "Lancez une simulation pour voir les statistiques";
    cairo_text_extents_t ext;
    cairo_text_extents(cr, msg, &ext);
    cairo_move_to(cr, width/2 - ext.width/2, height/2 + 40);
    cairo_show_text(cr, msg);
}

/* Fonction de dessin principale */
static gboolean on_stats_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);
    
    /* Fond */
    cairo_set_source_rgb(cr, COLOR_BG_R, COLOR_BG_G, COLOR_BG_B);
    cairo_paint(cr);
    
    if (process_count == 0 || (avg_rotation == 0.0 && avg_attente == 0.0)) {
        draw_stats_empty_state(cr, width, height);
        return FALSE;
    }
    
    /* Titre principal */
    cairo_set_source_rgb(cr, COLOR_TEXT_R, COLOR_TEXT_G, COLOR_TEXT_B);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 24);
    cairo_move_to(cr, 30, 40);
    cairo_show_text(cr, "Statistiques d'ordonnancement");
    
    /* Layout 2x2 - 2 cartes par ligne */
    double margin = 30;
    double spacing = 20;
    double card_w = (width - 2 * margin - spacing) / 2;  /* 2 cartes par ligne */
    double card_h = 110;  /* Hauteur augmentée pour plus d'espace */
    
    /* Première ligne */
    double row1_y = 80;
    
    /* CPU Badge - en haut à gauche */
    draw_cpu_badge(cr, margin, row1_y, card_w, card_h);
    
    /* Rotation moyenne - en haut à droite */
    draw_metric_card(cr, margin + card_w + spacing, row1_y, card_w, card_h, 
                     "Rotation moyenne", avg_rotation, "unités", 
                     COLOR_ACCENT_R, COLOR_ACCENT_G, COLOR_ACCENT_B);
    
    /* Deuxième ligne */
    double row2_y = row1_y + card_h + spacing;
    
    /* Attente moyenne - en bas à gauche */
    draw_metric_card(cr, margin, row2_y, card_w, card_h, 
                     "Attente moyenne", avg_attente, "unités", 
                     COLOR_WARNING_R, COLOR_WARNING_G, COLOR_WARNING_B);
    
    /* Durée moyenne - en bas à droite */
    draw_metric_card(cr, margin + card_w + spacing, row2_y, card_w, card_h, 
                     "Durée moyenne", avg_duree, "unités", 
                     COLOR_SUCCESS_R, COLOR_SUCCESS_G, COLOR_SUCCESS_B);
    
    return FALSE;
}

/* Fonction publique pour créer l'onglet statistiques */
GtkWidget* create_stats_page(void) {
    stats_drawing_area = gtk_drawing_area_new();
    gtk_widget_set_hexpand(stats_drawing_area, TRUE);
    gtk_widget_set_vexpand(stats_drawing_area, TRUE);
    g_signal_connect(stats_drawing_area, "draw", G_CALLBACK(on_stats_draw), NULL);
    
    return stats_drawing_area;
}

/* Fonction publique pour rafraîchir les statistiques */
void refresh_statistics(void) {
    load_statistics();
    if (stats_drawing_area) {
        gtk_widget_queue_draw(stats_drawing_area);
    }
}

/* Fonction publique pour réinitialiser les statistiques */
void reset_statistics(void) {
    avg_rotation = 0.0;
    avg_attente = 0.0;
    avg_duree = 0.0;
    process_count = 0;
    cpu_utilization = 0.0;
    if (stats_drawing_area) {
        gtk_widget_queue_draw(stats_drawing_area);
    }
}