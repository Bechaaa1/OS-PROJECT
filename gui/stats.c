// stats.c — Module simplifié pour affichage des statistiques
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

/* Couleurs pour le thème */
static const double COLOR_BG_R = 0.06, COLOR_BG_G = 0.08, COLOR_BG_B = 0.10;
static const double COLOR_CARD_R = 0.16, COLOR_CARD_G = 0.23, COLOR_CARD_B = 0.32;
static const double COLOR_ACCENT_R = 0.35, COLOR_ACCENT_G = 0.77, COLOR_ACCENT_B = 1.0;
static const double COLOR_TEXT_R = 0.91, COLOR_TEXT_G = 0.94, COLOR_TEXT_B = 1.0;
static const double COLOR_SUCCESS_R = 0.18, COLOR_SUCCESS_G = 0.80, COLOR_SUCCESS_B = 0.44;
static const double COLOR_WARNING_R = 1.0, COLOR_WARNING_G = 0.76, COLOR_WARNING_B = 0.03;

/* Charger les statistiques depuis output.txt */
void load_statistics(void) {
    FILE *f = fopen("output.txt", "r");
    if (!f) {
        avg_rotation = 0.0;
        avg_attente = 0.0;
        avg_duree = 0.0;
        process_count = 0;
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
    
    g_print("✓ Statistiques chargées\n");
    g_print("  → Rotation moyenne : %.2f\n", avg_rotation);
    g_print("  → Attente moyenne : %.2f\n", avg_attente);
    g_print("  → Durée moyenne : %.2f\n", avg_duree);
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
    
    /* Cartes de métriques (positionnées en haut) */
    double card_w = (width - 100) / 3;
    double card_y = 80;  /* Position fixe en haut au lieu de centré */
    
    draw_metric_card(cr, 30, card_y, card_w, 90, "Rotation moyenne", 
                     avg_rotation, " unités", 
                     COLOR_ACCENT_R, COLOR_ACCENT_G, COLOR_ACCENT_B);
    
    draw_metric_card(cr, 40 + card_w, card_y, card_w, 90, "Attente moyenne", 
                     avg_attente, " unités", 
                     COLOR_WARNING_R, COLOR_WARNING_G, COLOR_WARNING_B);
    
    draw_metric_card(cr, 50 + 2*card_w, card_y, card_w, 90, "Durée moyenne", 
                     avg_duree, " unités", 
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
    if (stats_drawing_area) {
        gtk_widget_queue_draw(stats_drawing_area);
    }
}