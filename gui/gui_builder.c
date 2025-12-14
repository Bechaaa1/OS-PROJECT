#include <gtk/gtk.h>

extern GtkWidget *entry_quantum;
extern GtkWidget *box_quantum_container;

void on_algo_changed(GtkComboBox *combo, gpointer data);
void on_refresh_clicked(GtkButton *btn, gpointer combo_ptr);
void on_start_clicked(GtkButton *btn, gpointer combo_ptr);
void update_quantum_state(const char *algo);
char **detect_algorithms(int *count);
void apply_app_styles(void)
{
    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css,
        "window {"
        "   background-color:#0F1419;"
        "   color:#E8F0FF;"
        "}"
        
        "headerbar {"
        "   background-color:#1A2332;"
        "   color:#58C4FF;"
        "   border-bottom:2px solid #2A3B52;"
        "}"
        "headerbar * {"
        "   background-color:#1A2332;"
        "   color:#58C4FF;"
        "}"
        
        "label, box {"
        "   background-color:transparent;"
        "   color:#E8F0FF;"
        "}"
        
        "frame {"
        "   background-color:transparent;"
        "   color:#E8F0FF;"
        "   border:none;"
        "}"
        "frame > border {"
        "   background-color:transparent;"
        "}"
        "frame > label {"
        "   background-color:transparent;"
        "   color:#8BA3C7;"
        "   font-weight:500;"
        "}"

        "combobox button {"
        "   background:#2A3B52;"
        "   color:#E8F0FF;"
        "   border:1px solid #3D5270;"
        "   border-radius:8px;"
        "   padding:8px 12px;"
        "   font-size:14px;"
        "}"
        "combobox button:hover {"
        "   background:#344766;"
        "   border-color:#4A6280;"
        "}"
        
        "combobox > window.popup, "
        "combobox > window.popup > * {"
        "   background-color:#2A3B52;"
        "   color:#E8F0FF;"
        "}"
        
        "combobox menu {"
        "   background-color:#2A3B52;"
        "   color:#E8F0FF;"
        "}"
        "combobox menu > * {"
        "   background-color:#2A3B52;"
        "   color:#E8F0FF;"
        "}"
        "combobox menuitem {"
        "   background-color:#2A3B52;"
        "   color:#E8F0FF;"
        "   padding:8px 12px;"
        "}"
        "combobox menuitem:hover {"
        "   background-color:#344766;"
        "   color:#FFFFFF;"
        "}"
        "combobox menuitem:selected {"
        "   background-color:#3D5270;"
        "   color:#58C4FF;"
        "}"

        "#btn_refresh, #btn_refresh * {"
        "   background:#2A3B52;"
        "   color:#8BA3C7;"
        "   border:1px solid #3D5270;"
        "   box-shadow:none;"
        "   border-image:none;"
        "   background-image:none;"
        "   -gtk-icon-shadow:none;"
        "   border-radius:8px;"
        "   padding:10px 16px;"
        "   font-size:14px;"
        "   font-weight:500;"
        "}"
        "#btn_refresh:hover, #btn_refresh:hover * {"
        "   background:#344766;"
        "   border-color:#4A6280;"
        "   color:#A8C0E0;"
        "}"

        "#btn_start, #btn_start * {"
        "   background:#2ECC71;"
        "   color:white;"
        "   border:none;"
        "   box-shadow:0 2px 8px rgba(46,204,113,0.3);"
        "   border-image:none;"
        "   background-image:none;"
        "   -gtk-icon-shadow:none;"
        "   border-radius:8px;"
        "   padding:10px 20px;"
        "   font-size:14px;"
        "   font-weight:600;"
        "}"
        "#btn_start:hover, #btn_start:hover * {"
        "   background:#27AE60;"
        "   box-shadow:0 3px 12px rgba(46,204,113,0.4);"
        "}"

        
        "entry {"
        "   background:#2A3B52;"
        "   color:#E8F0FF;"
        "   border-radius:8px;"
        "   border:1px solid #3D5270;"
        "   padding:8px 12px;"
        "   font-size:14px;"
        "}"
        "entry:focus {"
        "   border-color:#58C4FF;"
        "   box-shadow:0 0 0 2px rgba(88,196,255,0.2);"
        "}"
        
        "notebook {"
        "   background:#0F1419;"
        "   border:1px solid #2A3B52;"
        "   border-radius:12px;"
        "}"
        
        "notebook > header {"
        "   background:#1A2332;"
        "   border-bottom:2px solid #2A3B52;"
        "   padding:8px;"
        "   border-radius:12px 12px 0 0;"
        "}"
        
        "notebook > header > tabs > tab {"
        "   background:#2A3B52;"
        "   color:#8BA3C7;"
        "   border:1px solid #3D5270;"
        "   border-radius:8px 8px 0 0;"
        "   padding:12px 24px;"
        "   margin:0 4px;"
        "   font-size:14px;"
        "   font-weight:500;"
        "   border-bottom:none;"
        "}"
        
        "notebook > header > tabs > tab:hover {"
        "   background:#344766;"
        "   color:#A8C0E0;"
        "}"
        
        "notebook > header > tabs > tab:checked {"
        "   background:#58C4FF;"
        "   color:#FFFFFF;"
        "   border-color:#58C4FF;"
        "   font-weight:600;"
        "}"
        
        "notebook > header > tabs > tab > label {"
        "   background:transparent;"
        "   color:inherit;"
        "   font-weight:inherit;"
        "}"
        
        "notebook > stack {"
        "   background:#0F1419;"
        "   padding:20px;"
        "   border-radius:0 0 12px 12px;"
        "}"
    , -1, NULL);

    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );
}

GtkWidget* build_header(void)
{
    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_name(header, "custom_header");
    gtk_container_set_border_width(GTK_CONTAINER(header), 20);

    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title),
        "<span font='24' weight='bold' foreground='#58C4FF'>Ordonnanceur Multi-tâche</span>");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(header), title, FALSE, FALSE, 0);

    GtkWidget *subtitle = gtk_label_new("Simulation d'ordonnancement de processus sous Linux");
    gtk_widget_set_halign(subtitle, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(header), subtitle, FALSE, FALSE, 0);

    return header;
}

GtkWidget* build_control_panel(GtkWidget **combo_out)
{
    GtkWidget *control_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);

    GtkWidget *btn_start = gtk_button_new_with_label("▶ Démarrer");
    gtk_widget_set_name(btn_start, "btn_start");
    gtk_button_set_relief(GTK_BUTTON(btn_start), GTK_RELIEF_NONE);
    gtk_widget_set_size_request(btn_start, 140, 40);
    gtk_box_pack_start(GTK_BOX(control_box), btn_start, FALSE, FALSE, 0);

    GtkWidget *btn_refresh = gtk_button_new_with_label("↻ Réinitialiser");
    gtk_widget_set_name(btn_refresh, "btn_refresh");
    gtk_button_set_relief(GTK_BUTTON(btn_refresh), GTK_RELIEF_NONE);
    gtk_widget_set_size_request(btn_refresh, 140, 40);
    gtk_box_pack_start(GTK_BOX(control_box), btn_refresh, FALSE, FALSE, 0);

    GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(control_box), spacer, TRUE, TRUE, 0);

    GtkWidget *label_politique = gtk_label_new("Politique:");
    gtk_box_pack_start(GTK_BOX(control_box), label_politique, FALSE, FALSE, 0);

    GtkWidget *combo = gtk_combo_box_text_new();
    gtk_widget_set_size_request(combo, 200, 40);
    gtk_box_pack_start(GTK_BOX(control_box), combo, FALSE, FALSE, 0);

    box_quantum_container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_pack_start(GTK_BOX(control_box), box_quantum_container, FALSE, FALSE, 0);

    GtkWidget *label_quantum = gtk_label_new("Quantum:");
    gtk_box_pack_start(GTK_BOX(box_quantum_container), label_quantum, FALSE, FALSE, 0);

    entry_quantum = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_quantum), "Défaut: 2");
    gtk_widget_set_size_request(entry_quantum, 80, -1);
    gtk_box_pack_start(GTK_BOX(box_quantum_container), entry_quantum, FALSE, FALSE, 0);
    
    gtk_widget_hide(box_quantum_container);

    int count = 0, fifo_index = -1;
    char **algos = detect_algorithms(&count);

    if (algos) {
        for (int i = 0; i < count; i++) {
            gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo), NULL, algos[i]);
            if (g_strcmp0(algos[i], "fifo") == 0)
                fifo_index = i;
            free(algos[i]);
        }
        free(algos);
    }

    if (fifo_index != -1) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), fifo_index);
        update_quantum_state("fifo");
    }

    g_signal_connect(combo, "changed", G_CALLBACK(on_algo_changed), NULL);
    g_signal_connect(btn_refresh, "clicked", G_CALLBACK(on_refresh_clicked), combo);
    g_signal_connect(btn_start, "clicked", G_CALLBACK(on_start_clicked), combo);

    *combo_out = combo;
    return control_box;
}

void show_main_window(GtkApplication *app)
{
    apply_app_styles();

    GtkWidget *win = gtk_application_window_new(app);
    gtk_window_set_default_size(GTK_WINDOW(win), 1000, 600);
    gtk_container_set_border_width(GTK_CONTAINER(win), 0);

    GtkWidget *main_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(win), main_container);

    GtkWidget *header = build_header();
    gtk_box_pack_start(GTK_BOX(main_container), header, FALSE, FALSE, 0);

    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_container_set_border_width(GTK_CONTAINER(content), 20);
    gtk_box_pack_start(GTK_BOX(main_container), content, TRUE, TRUE, 0);

    GtkWidget *combo;
    GtkWidget *control_box = build_control_panel(&combo);
    gtk_box_pack_start(GTK_BOX(content), control_box, FALSE, TRUE, 0);

    extern GtkWidget* create_tabs(void);
    GtkWidget *tabs = create_tabs();
    gtk_box_pack_start(GTK_BOX(content), tabs, TRUE, TRUE, 10);

    gtk_widget_show_all(win);
    gtk_widget_hide(box_quantum_container);
}