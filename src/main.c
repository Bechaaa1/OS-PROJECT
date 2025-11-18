// src/main.c — FINAL VERSION THAT REALLY WORKS ON YOUR SYSTEM
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "types.h"
#include "policy.h"
#include "gui.h"

// Global widgets (defined in gui.h)
AppWidgets app = {0};

// Global config path
char app_config_path[512] = {0};

// Local parsing data (exactly like before)
typedef struct {
    char name[50];
    int time_of_entry;
    char cycles[100][12];
    char type[100][10];
    int num_cycles;
    int priority;
} Process;

static Process parsed_processes[MAX_PROCS];
static int parsed_count = 0;

// ===========================================
// GUI PRINTF
// ===========================================
int gui_printf(const char *format, ...)
{
    char buffer[2048];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    GtkTextIter end;
    gtk_text_buffer_get_end_iter(app.log_buffer, &end);
    gtk_text_buffer_insert(app.log_buffer, &end, buffer, -1);
    gtk_text_buffer_insert(app.log_buffer, &end, "\n", -1);

    GtkTextMark *mark = gtk_text_buffer_get_insert(app.log_buffer);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(app.log_view), mark, 0.0, TRUE, 0.0, 1.0);

    while (g_main_context_pending(NULL))
        g_main_context_iteration(NULL, FALSE);

    return 0;
}

// ===========================================
// LOAD CONFIG FILE
// ===========================================
static gboolean load_config_file(const char *path)
{
    FILE *fp = fopen(path, "r");
    if (!fp) {
        gui_printf("ERROR: Cannot open file: %s", path);
        return FALSE;
    }

    char line[512];
    parsed_count = 0;

    while (fgets(line, sizeof(line), fp) && parsed_count < MAX_PROCS) {
        if (line[0] == '#' || line[0] == '\n' || (line[0] == '/' && line[1] == '/'))
            continue;

        Process p = {0};
        char *t = strtok(line, " \t\n");
        if (!t) continue;
        strncpy(p.name, t, 49);

        t = strtok(NULL, " \t\n");
        if (!t) continue;
        p.time_of_entry = atoi(t);

        char *tokens[100];
        int cnt = 0;
        while ((t = strtok(NULL, " \t\n")) != NULL)
            tokens[cnt++] = t;

        if (cnt < 1) continue;

        p.priority = atoi(tokens[cnt-1]);
        p.num_cycles = cnt - 1;

        for (int i = 0; i < p.num_cycles && i < 100; i++) {
            strncpy(p.cycles[i], tokens[i], 11);
            strncpy(p.type[i], (i%2==0) ? "calcul" : "E/S", 9);
        }

        parsed_processes[parsed_count++] = p;
        gui_printf("Loaded: %s (arr:%d prio:%d cycles:%d)",
                   p.name, p.time_of_entry, p.priority, p.num_cycles);
    }
    fclose(fp);

    if (parsed_count > 0) {
        gui_printf("Successfully loaded %d processes", parsed_count);
        return TRUE;
    }
    gui_printf("ERROR: No valid processes found");
    return FALSE;
}

// ===========================================
// FILL SCHEDULER STRUCTURES
// ===========================================
static void fill_scheduler_structures(void)
{
    num_procs = parsed_count;
    for (int i = 0; i < parsed_count; i++) {
        Proc *dst = &proc_list[i];
        Process *src = &parsed_processes[i];

        strncpy(dst->name, src->name, 49);
        dst->arrival = src->time_of_entry;
        dst->priority = src->priority;
        dst->finished = 0;

        // IMPORTANT: Copy ALL cycles in order (even I/O), but only CPU bursts will run
        dst->num_cycles = src->num_cycles;
        for (int j = 0; j < src->num_cycles && j < MAX_CYCLES; j++) {
            dst->cycles[j] = atoi(src->cycles[j]);
            strncpy(dst->type[j], src->type[j], 9);  // "calcul" or "E/S"
        }
    }
}

// ===========================================
// UPDATE UI STATE
// ===========================================
static void update_ui_state(gboolean running)
{
    gtk_widget_set_sensitive(app.start_button, !running);
    gtk_widget_set_sensitive(app.algo_combo, !running);
    gtk_widget_set_sensitive(app.quantum_spin, !running);

    if (running) {
        gtk_button_set_label(GTK_BUTTON(app.start_button), "Running...");
        gtk_widget_remove_css_class(app.start_button, "suggested-action");
    } else {
        gtk_button_set_label(GTK_BUTTON(app.start_button), "Start Simulation");
        gtk_widget_add_css_class(app.start_button, "suggested-action");
    }
}

// ===========================================
// FILE CHOOSER CALLBACK (classic, works everywhere)
// ===========================================
static void on_file_chooser_response(GtkDialog *dialog, gint response_id, gpointer user_data)
{
    if (response_id == GTK_RESPONSE_ACCEPT) {
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        GFile *file = gtk_file_chooser_get_file(chooser);
        char *path = g_file_get_path(file);

        strncpy(app_config_path, path, 511);
        app_config_path[511] = '\0';

        const char *name = strrchr(path, '/');
        name = name ? name + 1 : path;

        gui_printf("CONFIG LOADED: %s", name);
        gtk_widget_set_sensitive(app.start_button, TRUE);

        g_free(path);
        g_object_unref(file);
    } else {
        gui_printf("File selection cancelled");
    }
    gtk_window_destroy(GTK_WINDOW(dialog));
}

static void on_choose_file_clicked(GtkButton *button, gpointer data)
{
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Select Configuration File",
        GTK_WINDOW(app.window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open",   GTK_RESPONSE_ACCEPT,
        NULL);

    // Filter
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Text files (*.txt *.conf)");
    gtk_file_filter_add_pattern(filter, "*.txt");
    gtk_file_filter_add_pattern(filter, "*.conf");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    g_signal_connect(dialog, "response", G_CALLBACK(on_file_chooser_response), NULL);
    gtk_widget_show(dialog);
}

// ===========================================
// START SIMULATION
// ===========================================
void start_simulation(GtkWidget *w, gpointer d)
{
    update_ui_state(TRUE);
    gui_printf("Starting simulation...");
    gui_printf("═══════════════════════════════════════");

    if (strlen(app_config_path) == 0) {
        gui_printf("ERROR: No configuration file selected!");
        update_ui_state(FALSE);
        return;
    }

    if (!load_config_file(app_config_path)) {
        update_ui_state(FALSE);
        return;
    }

    fill_scheduler_structures();

    int algo = gtk_drop_down_get_selected(GTK_DROP_DOWN(app.algo_combo)) + 1;
    int q = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(app.quantum_spin));
    const char *names[] = {"FIFO", "Priority", "Round-Robin", "Aging"};

    gui_printf("Algorithm: %s | Quantum: %d", names[algo-1], q);

    switch (algo) {
        case 1: fifo_scheduler(); break;
        case 2: priority_scheduler(); break;
        case 3: round_robin_scheduler(q); break;
        case 4: aging_scheduler(q); break;
    }

    gtk_widget_queue_draw(app.gantt_area);
    gui_printf("═══════════════════════════════════════");
    gui_printf("Simulation completed!");
    update_ui_state(FALSE);
}

// ===========================================
// CREATE CONTROL PANEL
// ===========================================
static GtkWidget* create_control_panel(void)
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
    gtk_widget_set_margin_top(box, 10);
    gtk_widget_set_margin_bottom(box, 10);
    gtk_widget_set_margin_start(box, 10);
    gtk_widget_set_margin_end(box, 10);
    gtk_widget_add_css_class(box, "toolbar");

    // CHOOSE FILE BUTTON — THIS ONE WORKS
    GtkWidget *btn = gtk_button_new_with_label("Choose Config File");
    gtk_widget_add_css_class(btn, "suggested-action");
    gtk_widget_set_hexpand(btn, TRUE);
    g_signal_connect(btn, "clicked", G_CALLBACK(on_choose_file_clicked), NULL);
    gtk_box_append(GTK_BOX(box), btn);

    // Algorithm
    GtkWidget *abox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_append(GTK_BOX(abox), gtk_label_new("Algorithm:"));
    const char *algos[] = {"FIFO", "Priority", "Round-Robin", "Aging", NULL};
    app.algo_combo = gtk_drop_down_new_from_strings(algos);
    gtk_box_append(GTK_BOX(abox), app.algo_combo);
    gtk_box_append(GTK_BOX(box), abox);

    // Quantum
    GtkWidget *qbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_append(GTK_BOX(qbox), gtk_label_new("Quantum:"));
    app.quantum_spin = gtk_spin_button_new_with_range(1, 20, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(app.quantum_spin), 3);
    gtk_box_append(GTK_BOX(qbox), app.quantum_spin);
    gtk_box_append(GTK_BOX(box), qbox);

    // Start button
    app.start_button = gtk_button_new_with_label("Start Simulation");
    gtk_widget_add_css_class(app.start_button, "suggested-action");
    gtk_widget_set_sensitive(app.start_button, FALSE);
    gtk_box_append(GTK_BOX(box), app.start_button);
    g_signal_connect(app.start_button, "clicked", G_CALLBACK(start_simulation), NULL);

    return box;
}

// ===========================================
// CREATE MAIN CONTENT
// ===========================================
static GtkWidget* create_main_content(void)
{
    GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);

    GtkWidget *gframe = gtk_frame_new("Gantt Chart");
    gtk_widget_add_css_class(gframe, "content-frame");
    app.gantt_area = gtk_drawing_area_new();
    gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(app.gantt_area), 700);
    gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(app.gantt_area), 400);
    gtk_frame_set_child(GTK_FRAME(gframe), app.gantt_area);
    gtk_paned_set_start_child(GTK_PANED(paned), gframe);
    setup_gantt_chart(app.gantt_area);

    GtkWidget *lframe = gtk_frame_new("Simulation Log");
    gtk_widget_add_css_class(lframe, "content-frame");
    GtkWidget *scrolled = gtk_scrolled_window_new();
    app.log_view = gtk_text_view_new();
    app.log_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app.log_view));
    gtk_text_view_set_editable(GTK_TEXT_VIEW(app.log_view), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(app.log_view), TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), app.log_view);
    gtk_frame_set_child(GTK_FRAME(lframe), scrolled);
    gtk_paned_set_end_child(GTK_PANED(paned), lframe);

    gtk_paned_set_position(GTK_PANED(paned), 900);
    return paned;
}

// ===========================================
// STYLES
// ===========================================
static void apply_styles(void)
{
    GtkCssProvider *p = gtk_css_provider_new();
    const char *css =
        ".toolbar { background: #f0f0f0; padding: 12px; border-bottom: 1px solid #ccc; }"
        "window { background: white; }";
    gtk_css_provider_load_from_data(p, css, -1);
    gtk_style_context_add_provider_for_display(gdk_display_get_default(),
        GTK_STYLE_PROVIDER(p), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(p);
}

// ===========================================
// ACTIVATE
// ===========================================
static void activate(GtkApplication *gtk_app, gpointer user_data)
{
    apply_styles();

    GtkWidget *win = gtk_application_window_new(gtk_app);
    gtk_window_set_title(GTK_WINDOW(win), "CPU Scheduler Simulator");
    gtk_window_set_default_size(GTK_WINDOW(win), 1400, 850);

    GtkWidget *main = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(win), main);

    GtkWidget *title = gtk_label_new("CPU Process Scheduler Simulation");
    gtk_widget_add_css_class(title, "title-1");
    gtk_widget_set_margin_top(title, 20);
    gtk_box_append(GTK_BOX(main), title);

    gtk_box_append(GTK_BOX(main), create_control_panel());
    gtk_box_append(GTK_BOX(main), create_main_content());

    gui_printf("CPU Scheduler Ready!");
    gui_printf("Click 'Choose Config File' and select your .txt file");

    app.window = win;
    gtk_window_present(GTK_WINDOW(win));
}

// ===========================================
// MAIN
// ===========================================
int main(int argc, char **argv)
{
    GtkApplication *gtk_app = gtk_application_new("dev.satokazuma.scheduler", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(gtk_app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(gtk_app), argc, argv);
    g_object_unref(gtk_app);
    return status;
}