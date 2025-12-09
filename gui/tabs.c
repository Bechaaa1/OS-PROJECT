#include <gtk/gtk.h>
#include <cairo.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
    char name[64];
    char *slots[256];
    int count;
} Row;

static Row rows[2];
static int row_count = 0;
static int max_slots = 0;
static GtkWidget *drawing_area = NULL;
static double animation_progress = 0.0;
static guint animation_timer = 0;

static const double PI = 3.14159265358979323846;

/* Color palette */
typedef struct {
    double r, g, b;
} Color;

static const Color COLOR_BG = {0.06, 0.08, 0.10};
static const Color COLOR_ACTIVE = {0.35, 0.77, 1.0};
static const Color COLOR_IDLE = {0.16, 0.23, 0.32};
static const Color COLOR_BORDER = {0.24, 0.36, 0.48};
static const Color COLOR_GRID = {0.16, 0.23, 0.32};
static const Color COLOR_TEXT = {0.91, 0.94, 1.0};
static const Color COLOR_ACCENT = {0.35, 0.77, 1.0};

/* Load data from output.txt */
static void load_output_file(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        g_printerr("Could not open %s\n", filename);
        return;
    }

    char line[256];
    Row *current = NULL;
    row_count = 0;
    max_slots = 0;

    // Nettoyer les anciennes données
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < rows[i].count; j++) {
            free(rows[i].slots[j]);
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

/* Enhanced rounded rectangle with shadow */
static void draw_rounded_rect_with_shadow(cairo_t *cr, double x, double y, double w, double h, 
                                          double radius, Color fill, gboolean active) {
    // Shadow
    if (active) {
        cairo_set_source_rgba(cr, 0.15, 0.60, 0.85, 0.3);
        for (int i = 0; i < 3; i++) {
            cairo_new_path(cr);
            cairo_arc(cr, x + radius + i,     y + radius + i,     radius, PI, 3*PI/2);
            cairo_arc(cr, x + w - radius + i, y + radius + i,     radius, 3*PI/2, 0);
            cairo_arc(cr, x + w - radius + i, y + h - radius + i, radius, 0, PI/2);
            cairo_arc(cr, x + radius + i,     y + h - radius + i, radius, PI/2, PI);
            cairo_close_path(cr);
            cairo_fill(cr);
        }
    }

    // Main rectangle
    cairo_new_path(cr);
    cairo_arc(cr, x + radius,     y + radius,     radius, PI, 3*PI/2);
    cairo_arc(cr, x + w - radius, y + radius,     radius, 3*PI/2, 0);
    cairo_arc(cr, x + w - radius, y + h - radius, radius, 0, PI/2);
    cairo_arc(cr, x + radius,     y + h - radius, radius, PI/2, PI);
    cairo_close_path(cr);

    // Gradient fill for active boxes
    if (active) {
        cairo_pattern_t *pat = cairo_pattern_create_linear(x, y, x, y + h);
        cairo_pattern_add_color_stop_rgb(pat, 0, 0.40, 0.82, 1.0);
        cairo_pattern_add_color_stop_rgb(pat, 1, 0.30, 0.72, 0.95);
        cairo_set_source(cr, pat);
        cairo_fill_preserve(cr);
        cairo_pattern_destroy(pat);
    } else {
        cairo_set_source_rgb(cr, fill.r, fill.g, fill.b);
        cairo_fill_preserve(cr);
    }

    // Border
    if (active) {
        cairo_set_source_rgba(cr, 0.50, 0.85, 1.0, 0.8);
        cairo_set_line_width(cr, 2.0);
    } else {
        cairo_set_source_rgb(cr, COLOR_BORDER.r, COLOR_BORDER.g, COLOR_BORDER.b);
        cairo_set_line_width(cr, 1.5);
    }
    cairo_stroke(cr);
}

/* Draw empty state */
static void draw_empty_state(cairo_t *cr, int width, int height) {
    // Icon circle
    cairo_arc(cr, width/2, height/2 - 60, 50, 0, 2*PI);
    cairo_set_source_rgba(cr, COLOR_ACCENT.r, COLOR_ACCENT.g, COLOR_ACCENT.b, 0.15);
    cairo_fill(cr);

    // Chart icon
    cairo_set_source_rgb(cr, COLOR_ACCENT.r, COLOR_ACCENT.g, COLOR_ACCENT.b);
    cairo_set_line_width(cr, 3);
    cairo_move_to(cr, width/2 - 25, height/2 - 40);
    cairo_line_to(cr, width/2 - 25, height/2 - 80);
    cairo_line_to(cr, width/2 + 25, height/2 - 80);
    cairo_stroke(cr);

    for (int i = 0; i < 3; i++) {
        cairo_rectangle(cr, width/2 - 20 + i*15, height/2 - 75 + i*8, 10, 35 - i*8);
        cairo_fill(cr);
    }

    // Text
    cairo_set_source_rgb(cr, COLOR_TEXT.r, COLOR_TEXT.g, COLOR_TEXT.b);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 22);
    
    const char *msg = "Cliquez sur \"Démarrer\" pour visualiser l'ordonnancement";
    cairo_text_extents_t ext;
    cairo_text_extents(cr, msg, &ext);
    cairo_move_to(cr, width/2 - ext.width/2, height/2 + 40);
    cairo_show_text(cr, msg);

}

/* Main drawing function */
static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
    int width  = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);

    // Background
    cairo_set_source_rgb(cr, COLOR_BG.r, COLOR_BG.g, COLOR_BG.b);
    cairo_paint(cr);

    if (row_count == 0 || max_slots == 0) {
        draw_empty_state(cr, width, height);
        return FALSE;
    }

    const int box_w = 90;
    const int box_h = 62;
    const int margin_left = 180;
    const int margin_top  = 80;
    const int row_spacing = 110;
    const int box_gap = 16;

    // Title
    cairo_set_source_rgb(cr, COLOR_TEXT.r, COLOR_TEXT.g, COLOR_TEXT.b);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 24);
    cairo_move_to(cr, margin_left, 40);
    cairo_show_text(cr, "Diagramme d'ordonnancement");

    // Time headers
    cairo_select_font_face(cr, "Monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 13);

    for (int t = 0; t < max_slots; t++) {
        int x = margin_left - 45 + t * (box_w + box_gap);
        double alpha = (t < animation_progress * max_slots) ? 1.0 : 0.3;

        // Time label
        cairo_set_source_rgba(cr, COLOR_ACCENT.r, COLOR_ACCENT.g, COLOR_ACCENT.b, alpha);
        char time_str[8];
        snprintf(time_str, sizeof(time_str), "T%d", t);
        cairo_text_extents_t ext;
        cairo_text_extents(cr, time_str, &ext);
        cairo_move_to(cr, x + box_w/2 - ext.width/2, margin_top - 20);
        cairo_show_text(cr, time_str);

        // Vertical grid line
        cairo_set_source_rgba(cr, COLOR_GRID.r, COLOR_GRID.g, COLOR_GRID.b, alpha * 0.5);
        cairo_set_line_width(cr, 1.0);
        cairo_move_to(cr, x + box_w/2, margin_top - 5);
        cairo_line_to(cr, x + box_w/2, margin_top + row_count * row_spacing + 20);
        cairo_stroke(cr);
    }

    // Process rows
    for (int r = 0; r < row_count; r++) {
        int base_y = margin_top + r * row_spacing;

        // Process label background
        cairo_rectangle(cr, 20, base_y + 10, 145, 42);
        cairo_set_source_rgba(cr, 0.15, 0.20, 0.28, 0.9);
        cairo_fill(cr);

        // Process name
        cairo_set_source_rgb(cr, COLOR_TEXT.r, COLOR_TEXT.g, COLOR_TEXT.b);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 15);
        cairo_move_to(cr, 35, base_y + box_h/2 + 15);
        cairo_show_text(cr, rows[r].name);

        // Time slots
        for (int i = 0; i < rows[r].count; i++) {
            double slot_progress = (i < animation_progress * rows[r].count) ? 1.0 : 
                                  (animation_progress * rows[r].count - i);
            if (slot_progress < 0) slot_progress = 0;
            if (slot_progress > 1) slot_progress = 1;

            int x = margin_left + i * (box_w + box_gap);
            int y = base_y;

            gboolean is_idle = (strcmp(rows[r].slots[i], "NULL") == 0);
            
            // Scale effect during animation
            double scale = 0.7 + 0.3 * slot_progress;
            double scaled_w = box_w * scale;
            double scaled_h = box_h * scale;
            double offset_x = (box_w - scaled_w) / 2;
            double offset_y = (box_h - scaled_h) / 2;

            cairo_save(cr);
            cairo_translate(cr, x + offset_x, y + offset_y);

            draw_rounded_rect_with_shadow(cr, 0, 0, scaled_w, scaled_h, 8, 
                                         is_idle ? COLOR_IDLE : COLOR_ACTIVE, !is_idle);

            // Text for active processes
            if (!is_idle && slot_progress > 0.5) {
                cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, (slot_progress - 0.5) * 2);
                cairo_select_font_face(cr, "Monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
                cairo_set_font_size(cr, 14);

                cairo_text_extents_t ext;
                cairo_text_extents(cr, rows[r].slots[i], &ext);
                double tx = (scaled_w - ext.width) / 2;
                double ty = scaled_h/2 + ext.height/2;

                cairo_move_to(cr, tx, ty);
                cairo_show_text(cr, rows[r].slots[i]);
            }

            cairo_restore(cr);
        }
    }

    return FALSE;
}

/* Animation timer callback */
static gboolean animate(gpointer user_data) {
    animation_progress += 0.04;
    if (animation_progress >= 1.0) {
        animation_progress = 1.0;
        animation_timer = 0;
        if (drawing_area)
            gtk_widget_queue_draw(drawing_area);
        return FALSE;
    }
    if (drawing_area)
        gtk_widget_queue_draw(drawing_area);
    return TRUE;
}

/* Fonction publique pour démarrer l'animation depuis gui.c */
void start_gantt_diagram(void) {
    load_output_file("output.txt");
    animation_progress = 0.0;
    
    if (animation_timer > 0)
        g_source_remove(animation_timer);
    
    animation_timer = g_timeout_add(25, animate, NULL);
    
    g_print("✓ Diagramme de Gantt chargé depuis output.txt\n");
}

/* Fonction publique pour réinitialiser le diagramme depuis gui.c */
void reset_gantt_diagram(void) {
    // Arrêter l'animation si elle est en cours
    if (animation_timer > 0) {
        g_source_remove(animation_timer);
        animation_timer = 0;
    }
    
    // Nettoyer les données
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < rows[i].count; j++) {
            free(rows[i].slots[j]);
            rows[i].slots[j] = NULL;
        }
        rows[i].count = 0;
        memset(rows[i].name, 0, sizeof(rows[i].name));
    }
    
    row_count = 0;
    max_slots = 0;
    animation_progress = 0.0;
    
    // Supprimer le fichier output.txt
    if (remove("output.txt") == 0) {
        g_print("✓ Fichier output.txt supprimé\n");
    } else {
        g_print("ℹ Fichier output.txt déjà absent\n");
    }
    
    // Redessiner pour afficher l'état vide
    if (drawing_area) {
        gtk_widget_queue_draw(drawing_area);
    }
    
    g_print("✓ Diagramme réinitialisé\n");
}

/* ============================================================
   create_tabs() : crée deux onglets : "Diagramme" et "Statistiques"
   ============================================================ */
GtkWidget* create_tabs(void)
{
    /* --- Notebook (barre d'onglets) --- */
    GtkWidget *notebook = gtk_notebook_new();

    /* ============================
       Onglet 1 : Diagramme de Gantt
       ============================ */
    GtkWidget *page_diagram = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    
    // Zone de dessin Cairo
    drawing_area = gtk_drawing_area_new();
    gtk_widget_set_hexpand(drawing_area, TRUE);
    gtk_widget_set_vexpand(drawing_area, TRUE);
    g_signal_connect(drawing_area, "draw", G_CALLBACK(on_draw), NULL);
    gtk_box_pack_start(GTK_BOX(page_diagram), drawing_area, TRUE, TRUE, 0);

    GtkWidget *tab_label1 = gtk_label_new("◈ Diagramme");
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_diagram, tab_label1);

    /* ============================
   Onglet 2 : Statistiques
   ============================ */
    extern GtkWidget* create_stats_page(void);  // Déclaration de stats.c
    GtkWidget *page_stats = create_stats_page();

    GtkWidget *tab_label2 = gtk_label_new("▣ Statistiques");
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_stats, tab_label2);
    return notebook;  // ← AJOUTER CETTE LIGNE (elle manque !)


    }