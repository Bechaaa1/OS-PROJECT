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
static GtkWidget *scrolled_window = NULL;
static double animation_progress = 0.0;
static guint animation_timer = 0;
static double scroll_offset = 0.0;
static gboolean show_cpu_utilization = TRUE;
static double cpu_utilization = 0.0;

static const double PI = 3.14159265358979323846;

/* Enhanced color palette - sleek blue theme */
typedef struct {
    double r, g, b;
} Color;

static const Color COLOR_BG = {0.04, 0.06, 0.10};           // Deep dark blue-black
static const Color COLOR_CARD_BG = {0.08, 0.12, 0.18};      // Card backgrounds
static const Color COLOR_ACTIVE = {0.25, 0.65, 0.95};       // Bright cyan-blue
static const Color COLOR_ACTIVE_GLOW = {0.40, 0.75, 1.0};   // Lighter glow
static const Color COLOR_IDLE = {0.12, 0.16, 0.22};         // Dark idle state
static const Color COLOR_BORDER = {0.18, 0.28, 0.42};       // Subtle borders
static const Color COLOR_GRID = {0.10, 0.15, 0.25};         // Grid lines
static const Color COLOR_TEXT = {0.88, 0.92, 0.98};         // Crisp white-blue text
static const Color COLOR_TEXT_MUTED = {0.50, 0.60, 0.75};   // Muted text
static const Color COLOR_ACCENT = {0.30, 0.70, 1.0};        // Accent blue
static const Color COLOR_SUCCESS = {0.20, 0.80, 0.60};      // Success green-cyan
static const Color COLOR_WARNING = {1.0, 0.65, 0.25};       // Warm orange

/* Calculate CPU utilization */
static void calculate_cpu_utilization(void) {
    if (row_count == 0 || max_slots == 0) {
        cpu_utilization = 0.0;
        return;
    }
    
    int total_slots = 0;
    int active_slots = 0;
    
    for (int r = 0; r < row_count; r++) {
        total_slots += rows[r].count;
        for (int i = 0; i < rows[r].count; i++) {
            if (strcmp(rows[r].slots[i], "NULL") != 0) {
                active_slots++;
            }
        }
    }
    
    cpu_utilization = total_slots > 0 ? (double)active_slots / total_slots * 100.0 : 0.0;
}

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
    
    calculate_cpu_utilization();
}

/* Helper function for rounded rectangle */
static void draw_simple_rounded_rect(cairo_t *cr, double x, double y, double w, double h, double r) {
    cairo_new_path(cr);
    cairo_arc(cr, x + r, y + r, r, PI, 3*PI/2);
    cairo_arc(cr, x + w - r, y + r, r, 3*PI/2, 0);
    cairo_arc(cr, x + w - r, y + h - r, r, 0, PI/2);
    cairo_arc(cr, x + r, y + h - r, r, PI/2, PI);
    cairo_close_path(cr);
}

/* Enhanced rounded rectangle with glow effect */
static void draw_rounded_rect_with_glow(cairo_t *cr, double x, double y, double w, double h, 
                                        double radius, Color fill, gboolean active) {
    // Outer glow for active boxes
    if (active) {
        for (int i = 8; i > 0; i--) {
            double alpha = 0.08 * (1.0 - i/8.0);
            cairo_new_path(cr);
            cairo_arc(cr, x + radius, y + radius, radius + i, PI, 3*PI/2);
            cairo_arc(cr, x + w - radius, y + radius, radius + i, 3*PI/2, 0);
            cairo_arc(cr, x + w - radius, y + h - radius, radius + i, 0, PI/2);
            cairo_arc(cr, x + radius, y + h - radius, radius + i, PI/2, PI);
            cairo_close_path(cr);
            cairo_set_source_rgba(cr, COLOR_ACTIVE_GLOW.r, COLOR_ACTIVE_GLOW.g, COLOR_ACTIVE_GLOW.b, alpha);
            cairo_fill(cr);
        }
    }

    // Main rectangle
    cairo_new_path(cr);
    cairo_arc(cr, x + radius, y + radius, radius, PI, 3*PI/2);
    cairo_arc(cr, x + w - radius, y + radius, radius, 3*PI/2, 0);
    cairo_arc(cr, x + w - radius, y + h - radius, radius, 0, PI/2);
    cairo_arc(cr, x + radius, y + h - radius, radius, PI/2, PI);
    cairo_close_path(cr);

    // Gradient fill for active boxes
    if (active) {
        cairo_pattern_t *pat = cairo_pattern_create_linear(x, y, x, y + h);
        cairo_pattern_add_color_stop_rgba(pat, 0, 0.30, 0.70, 1.0, 0.95);
        cairo_pattern_add_color_stop_rgba(pat, 1, 0.20, 0.60, 0.90, 0.85);
        cairo_set_source(cr, pat);
        cairo_fill_preserve(cr);
        cairo_pattern_destroy(pat);
        
        // Inner highlight
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.15);
        cairo_set_line_width(cr, 1.5);
        cairo_stroke_preserve(cr);
    } else {
        // Subtle gradient for idle
        cairo_pattern_t *pat = cairo_pattern_create_linear(x, y, x, y + h);
        cairo_pattern_add_color_stop_rgb(pat, 0, fill.r + 0.02, fill.g + 0.02, fill.b + 0.02);
        cairo_pattern_add_color_stop_rgb(pat, 1, fill.r, fill.g, fill.b);
        cairo_set_source(cr, pat);
        cairo_fill_preserve(cr);
        cairo_pattern_destroy(pat);
    }

    // Border
    if (active) {
        cairo_set_source_rgba(cr, 0.45, 0.80, 1.0, 0.6);
        cairo_set_line_width(cr, 2.0);
    } else {
        cairo_set_source_rgba(cr, COLOR_BORDER.r, COLOR_BORDER.g, COLOR_BORDER.b, 0.4);
        cairo_set_line_width(cr, 1.0);
    }
    cairo_stroke(cr);
}

/* Draw CPU utilization badge - enhanced design */
static void draw_cpu_badge(cairo_t *cr, int x, int y) {
    if (!show_cpu_utilization) return;
    
    // Badge background with gradient
    draw_simple_rounded_rect(cr, x, y, 190, 58, 12);
    cairo_pattern_t *bg_pat = cairo_pattern_create_linear(x, y, x, y + 58);
    cairo_pattern_add_color_stop_rgba(bg_pat, 0, 0.10, 0.14, 0.20, 0.95);
    cairo_pattern_add_color_stop_rgba(bg_pat, 1, 0.08, 0.12, 0.18, 0.95);
    cairo_set_source(cr, bg_pat);
    cairo_fill(cr);
    cairo_pattern_destroy(bg_pat);
    
    // Subtle border
    draw_simple_rounded_rect(cr, x, y, 190, 58, 12);
    cairo_set_source_rgba(cr, COLOR_BORDER.r, COLOR_BORDER.g, COLOR_BORDER.b, 0.5);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);
    
    // Icon circle with glow
    Color badge_color = cpu_utilization >= 70 ? COLOR_SUCCESS : 
                        cpu_utilization >= 40 ? COLOR_WARNING : COLOR_ACCENT;
    
    // Glow effect
    for (int i = 4; i > 0; i--) {
        cairo_arc(cr, x + 28, y + 29, 16 + i*2, 0, 2*PI);
        cairo_set_source_rgba(cr, badge_color.r, badge_color.g, badge_color.b, 0.1 * (1.0 - i/4.0));
        cairo_fill(cr);
    }
    
    cairo_arc(cr, x + 28, y + 29, 16, 0, 2*PI);
    cairo_set_source_rgba(cr, badge_color.r, badge_color.g, badge_color.b, 0.25);
    cairo_fill(cr);
    
    // CPU icon (chip design)
    cairo_set_source_rgb(cr, badge_color.r, badge_color.g, badge_color.b);
    cairo_set_line_width(cr, 2.5);
    cairo_rectangle(cr, x + 21, y + 22, 14, 14);
    cairo_stroke(cr);
    
    // Chip pins
    for (int i = 0; i < 3; i++) {
        // Left pins
        cairo_move_to(cr, x + 18, y + 24 + i*4);
        cairo_line_to(cr, x + 21, y + 24 + i*4);
        // Right pins
        cairo_move_to(cr, x + 35, y + 24 + i*4);
        cairo_line_to(cr, x + 38, y + 24 + i*4);
    }
    cairo_set_line_width(cr, 1.5);
    cairo_stroke(cr);
    
    // Label
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED.r, COLOR_TEXT_MUTED.g, COLOR_TEXT_MUTED.b, 0.9);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 11);
    cairo_move_to(cr, x + 52, y + 22);
    cairo_show_text(cr, "Utilisation CPU");
    
    // Percentage with shadow
    cairo_set_source_rgba(cr, 0, 0, 0, 0.3);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 20);
    char percent[16];
    snprintf(percent, sizeof(percent), "%.1f%%", cpu_utilization);
    cairo_move_to(cr, x + 53, y + 44);
    cairo_show_text(cr, percent);
    
    // Actual percentage
    cairo_set_source_rgb(cr, badge_color.r, badge_color.g, badge_color.b);
    cairo_move_to(cr, x + 52, y + 43);
    cairo_show_text(cr, percent);
}

/* Draw timeline indicator - enhanced */
static void draw_timeline_indicator(cairo_t *cr, int x, int y, int current_time, int max_time) {
    int bar_width = 220;
    int bar_height = 10;
    
    // Background with border
    draw_simple_rounded_rect(cr, x, y, bar_width, bar_height, 5);
    cairo_set_source_rgba(cr, COLOR_IDLE.r, COLOR_IDLE.g, COLOR_IDLE.b, 0.6);
    cairo_fill(cr);
    
    draw_simple_rounded_rect(cr, x, y, bar_width, bar_height, 5);
    cairo_set_source_rgba(cr, COLOR_BORDER.r, COLOR_BORDER.g, COLOR_BORDER.b, 0.4);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);
    
    // Progress bar with gradient
    if (max_time > 0) {
        int progress_width = (bar_width * current_time) / max_time;
        if (progress_width > 0) {
            draw_simple_rounded_rect(cr, x, y, progress_width, bar_height, 5);
            cairo_pattern_t *prog_pat = cairo_pattern_create_linear(x, y, x + progress_width, y);
            cairo_pattern_add_color_stop_rgb(prog_pat, 0, 0.30, 0.70, 1.0);
            cairo_pattern_add_color_stop_rgb(prog_pat, 1, 0.40, 0.80, 1.0);
            cairo_set_source(cr, prog_pat);
            cairo_fill(cr);
            cairo_pattern_destroy(prog_pat);
            
            // Glow on progress
            draw_simple_rounded_rect(cr, x, y, progress_width, bar_height, 5);
            cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.3);
            cairo_set_line_width(cr, 1.5);
            cairo_stroke(cr);
        }
    }
    
    // Label with shadow
    cairo_set_source_rgba(cr, 0, 0, 0, 0.4);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 11);
    char time_str[32];
    snprintf(time_str, sizeof(time_str), "Progression: %d / %d", current_time, max_time);
    cairo_move_to(cr, x + 1, y - 9);
    cairo_show_text(cr, time_str);
    
    cairo_set_source_rgba(cr, COLOR_TEXT_MUTED.r, COLOR_TEXT_MUTED.g, COLOR_TEXT_MUTED.b, 0.9);
    cairo_move_to(cr, x, y - 10);
    cairo_show_text(cr, time_str);
}

/* Draw empty state */
static void draw_empty_state(cairo_t *cr, int width, int height) {
    // Large icon circle with glow
    for (int i = 6; i > 0; i--) {
        cairo_arc(cr, width/2, height/2 - 60, 55 + i*3, 0, 2*PI);
        cairo_set_source_rgba(cr, COLOR_ACCENT.r, COLOR_ACCENT.g, COLOR_ACCENT.b, 0.03 * (1.0 - i/6.0));
        cairo_fill(cr);
    }
    
    cairo_arc(cr, width/2, height/2 - 60, 55, 0, 2*PI);
    cairo_set_source_rgba(cr, COLOR_ACCENT.r, COLOR_ACCENT.g, COLOR_ACCENT.b, 0.15);
    cairo_fill(cr);

    // Chart icon with modern look
    cairo_set_source_rgb(cr, COLOR_ACCENT.r, COLOR_ACCENT.g, COLOR_ACCENT.b);
    cairo_set_line_width(cr, 3.5);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
    
    cairo_move_to(cr, width/2 - 28, height/2 - 38);
    cairo_line_to(cr, width/2 - 28, height/2 - 82);
    cairo_line_to(cr, width/2 + 28, height/2 - 82);
    cairo_stroke(cr);

    // Bars with gradient
    for (int i = 0; i < 3; i++) {
        double bar_h = 38 - i*10;
        double bar_x = width/2 - 22 + i*16;
        double bar_y = height/2 - 78 + i*10;
        
        cairo_rectangle(cr, bar_x, bar_y, 12, bar_h);
        cairo_pattern_t *bar_pat = cairo_pattern_create_linear(bar_x, bar_y, bar_x, bar_y + bar_h);
        cairo_pattern_add_color_stop_rgba(bar_pat, 0, 0.35, 0.75, 1.0, 0.9);
        cairo_pattern_add_color_stop_rgba(bar_pat, 1, 0.25, 0.65, 0.95, 0.7);
        cairo_set_source(cr, bar_pat);
        cairo_fill(cr);
        cairo_pattern_destroy(bar_pat);
    }

    // Text with shadow
    cairo_set_source_rgba(cr, 0, 0, 0, 0.4);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 22);
    
    const char *msg = "Cliquez sur \"Démarrer\" pour visualiser l'ordonnancement";
    cairo_text_extents_t ext;
    cairo_text_extents(cr, msg, &ext);
    cairo_move_to(cr, width/2 - ext.width/2 + 1, height/2 + 41);
    cairo_show_text(cr, msg);
    
    cairo_set_source_rgb(cr, COLOR_TEXT.r, COLOR_TEXT.g, COLOR_TEXT.b);
    cairo_move_to(cr, width/2 - ext.width/2, height/2 + 40);
    cairo_show_text(cr, msg);
}

/* Main drawing function */
static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
    int width  = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);

    // Background with subtle gradient
    cairo_pattern_t *bg_pat = cairo_pattern_create_linear(0, 0, 0, height);
    cairo_pattern_add_color_stop_rgb(bg_pat, 0, 0.04, 0.06, 0.10);
    cairo_pattern_add_color_stop_rgb(bg_pat, 1, 0.06, 0.08, 0.12);
    cairo_set_source(cr, bg_pat);
    cairo_paint(cr);
    cairo_pattern_destroy(bg_pat);

    if (row_count == 0 || max_slots == 0) {
        draw_empty_state(cr, width, height);
        return FALSE;
    }

    const int box_w = 95;
    const int box_h = 65;
    const int margin_left = 200;
    const int margin_top  = 130;
    const int row_spacing = 120;
    const int box_gap = 18;

    // Title with shadow
    cairo_set_source_rgba(cr, 0, 0, 0, 0.5);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 26);
    cairo_move_to(cr, margin_left + 1, 41);
    cairo_show_text(cr, "Diagramme d'ordonnancement");
    
    cairo_set_source_rgb(cr, COLOR_TEXT.r, COLOR_TEXT.g, COLOR_TEXT.b);
    cairo_move_to(cr, margin_left, 40);
    cairo_show_text(cr, "Diagramme d'ordonnancement");
    
    // CPU utilization badge
    draw_cpu_badge(cr, margin_left, 55);
    
    // Timeline indicator
    int current_visible_time = (int)(animation_progress * max_slots);
    draw_timeline_indicator(cr, margin_left + 210, 72, current_visible_time, max_slots);

    // Time headers
    cairo_select_font_face(cr, "Monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 12);

    for (int t = 0; t < max_slots; t++) {
        int x = margin_left - 48 + t * (box_w + box_gap);
        double alpha = (t < animation_progress * max_slots) ? 1.0 : 0.25;

        // Time label with better styling
        cairo_set_source_rgba(cr, COLOR_ACCENT.r, COLOR_ACCENT.g, COLOR_ACCENT.b, alpha);
        char time_str[8];
        snprintf(time_str, sizeof(time_str), "T%d", t);
        cairo_text_extents_t ext;
        cairo_text_extents(cr, time_str, &ext);
        cairo_move_to(cr, x + box_w/2 - ext.width/2, margin_top - 25);
        cairo_show_text(cr, time_str);

        // Vertical grid line with gradient
        cairo_pattern_t *grid_pat = cairo_pattern_create_linear(0, margin_top - 5, 0, margin_top + row_count * row_spacing + 20);
        cairo_pattern_add_color_stop_rgba(grid_pat, 0, COLOR_GRID.r, COLOR_GRID.g, COLOR_GRID.b, alpha * 0.2);
        cairo_pattern_add_color_stop_rgba(grid_pat, 0.5, COLOR_GRID.r, COLOR_GRID.g, COLOR_GRID.b, alpha * 0.4);
        cairo_pattern_add_color_stop_rgba(grid_pat, 1, COLOR_GRID.r, COLOR_GRID.g, COLOR_GRID.b, alpha * 0.2);
        cairo_set_source(cr, grid_pat);
        cairo_set_line_width(cr, 1.5);
        cairo_move_to(cr, x + box_w/2, margin_top - 5);
        cairo_line_to(cr, x + box_w/2, margin_top + row_count * row_spacing + 20);
        cairo_stroke(cr);
        cairo_pattern_destroy(grid_pat);
    }

    // Process rows
    for (int r = 0; r < row_count; r++) {
        int base_y = margin_top + r * row_spacing;

        // Process label card with gradient
        draw_simple_rounded_rect(cr, 20, base_y + 8, 165, 50, 10);
        cairo_pattern_t *label_pat = cairo_pattern_create_linear(20, base_y + 8, 20, base_y + 58);
        cairo_pattern_add_color_stop_rgba(label_pat, 0, 0.12, 0.16, 0.22, 0.95);
        cairo_pattern_add_color_stop_rgba(label_pat, 1, 0.10, 0.14, 0.20, 0.95);
        cairo_set_source(cr, label_pat);
        cairo_fill(cr);
        cairo_pattern_destroy(label_pat);
        
        // Border
        draw_simple_rounded_rect(cr, 20, base_y + 8, 165, 50, 10);
        cairo_set_source_rgba(cr, COLOR_BORDER.r, COLOR_BORDER.g, COLOR_BORDER.b, 0.5);
        cairo_set_line_width(cr, 1.0);
        cairo_stroke(cr);

        // Process name with shadow
        cairo_set_source_rgba(cr, 0, 0, 0, 0.4);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 15);
        cairo_move_to(cr, 40, base_y + box_h/2 + 16);
        cairo_show_text(cr, rows[r].name);
        
        cairo_set_source_rgb(cr, COLOR_TEXT.r, COLOR_TEXT.g, COLOR_TEXT.b);
        cairo_move_to(cr, 39, base_y + box_h/2 + 15);
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
            
            // Scale and bounce effect
            double scale = 0.65 + 0.35 * slot_progress;
            double bounce = slot_progress < 0.5 ? slot_progress * 2 : 2 - slot_progress * 2;
            bounce = bounce * 0.05;
            
            double scaled_w = box_w * scale;
            double scaled_h = box_h * scale;
            double offset_x = (box_w - scaled_w) / 2;
            double offset_y = (box_h - scaled_h) / 2 - bounce * 10;

            cairo_save(cr);
            cairo_translate(cr, x + offset_x, y + offset_y);

            draw_rounded_rect_with_glow(cr, 0, 0, scaled_w, scaled_h, 10, 
                                        is_idle ? COLOR_IDLE : COLOR_ACTIVE, !is_idle);

            // Text for active processes
            if (!is_idle && slot_progress > 0.4) {
                double text_alpha = (slot_progress - 0.4) / 0.6;
                
                // Shadow
                cairo_set_source_rgba(cr, 0, 0, 0, text_alpha * 0.5);
                cairo_select_font_face(cr, "Monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
                cairo_set_font_size(cr, 13);

                cairo_text_extents_t ext;
                cairo_text_extents(cr, rows[r].slots[i], &ext);
                double tx = (scaled_w - ext.width) / 2;
                double ty = scaled_h/2 + ext.height/2;

                cairo_move_to(cr, tx + 1, ty + 1);
                cairo_show_text(cr, rows[r].slots[i]);
                
                // Actual text
                cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, text_alpha);
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
    
    // Mettre à jour la taille de la zone de dessin pour le scrolling
    if (drawing_area) {
        int required_width = 200 + max_slots * (95 + 18) + 200;
        gtk_widget_set_size_request(drawing_area, required_width, 600);
    }
    
    g_print("✓ Diagramme de Gantt chargé depuis output.txt\n");
}

