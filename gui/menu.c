#include <gtk/gtk.h>

/* --- déclaration de show_main_window() venant de gui.c --- */
void show_main_window(GtkApplication *app);

static void on_activate(GtkApplication *app, gpointer user_data)
{
    show_main_window(app);
}

void gui_init(int argc, char **argv)
{
    GtkApplication *app = gtk_application_new(
        "org.ordonnanceur.gui",
        G_APPLICATION_FLAGS_NONE
    );

    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);
}
