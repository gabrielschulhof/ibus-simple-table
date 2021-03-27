#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "config-file.h"

typedef struct {
  GtkWidget* view;
  GtkWidget* spinner;
  GtkWidget* label;
  guint timeout_id;
} AppState;

#define APP_STATE_INIT \
  { NULL, NULL, NULL, 0 }

// Started with https://stackoverflow.com/questions/43115473/

#define CHANGE_FONT_SIZE "textview { font-size: 28px; }"

static void app_set_spinning(AppState* app, gboolean spinning) {
  if (spinning) {
    gtk_spinner_start(GTK_SPINNER(app->spinner));
    gtk_widget_set_sensitive(app->label, FALSE);
    gtk_label_set_text(GTK_LABEL(app->label), "Saving...");
  } else {
    gtk_spinner_stop(GTK_SPINNER(app->spinner));
    gtk_widget_set_sensitive(app->label, TRUE);
    gtk_label_set_text(GTK_LABEL(app->label), "Saved");
  }
}

static char* from_disk() {
  char* result = NULL;
  char* fname =
      g_strdup_printf("%s" G_DIR_SEPARATOR_S ".notes", g_get_home_dir());

  g_file_get_contents(fname, &result, NULL, NULL);

  g_free(fname);
  return result;
}

static gboolean to_disk(AppState* app) {
  char* fname =
      g_strdup_printf("%s" G_DIR_SEPARATOR_S ".notes", g_get_home_dir());

  GtkTextBuffer* buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->view));
  GtkTextIter beg, end;

  gtk_text_buffer_get_start_iter(buf, &beg);
  gtk_text_buffer_get_end_iter(buf, &end);

  char* text = gtk_text_buffer_get_text(buf, &beg, &end, FALSE);

  GError* err = NULL;
  g_file_set_contents(fname, text, -1, &err);

  app_set_spinning(app, FALSE);
  g_free(fname);
  g_free(text);
  app->timeout_id = 0;
  return FALSE;
}

static gboolean
key_press_event(GtkWidget* widget, GdkEventKey* ev, AppState* app) {
  if (app->timeout_id != 0) {
    g_source_remove(app->timeout_id);
    app->timeout_id = 0;
  }
  app_set_spinning(app, TRUE);
  app->timeout_id = g_timeout_add_seconds(1, G_SOURCE_FUNC(to_disk), app);
  return FALSE;
}

int main(int argc, char *argv[]) {
  GtkWidget *window, *box, *spinner_box;
  GdkDisplay *display;
  GdkScreen *screen;
  GtkCssProvider *provider;
  GError *error = NULL;
  AppState app = APP_STATE_INIT;

  gtk_init(&argc, &argv);

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "Ékezetek");
  gtk_window_maximize(GTK_WINDOW(window));
  g_signal_connect(
      G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
  gtk_window_set_default_size(GTK_WINDOW(window), 350, 300);

  box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show(box);
  gtk_container_add(GTK_CONTAINER(window), box);

  spinner_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_show(spinner_box);
  gtk_widget_set_halign(spinner_box, GTK_ALIGN_END);
  gtk_widget_set_margin_end(spinner_box, 5);
  gtk_box_pack_end(GTK_BOX(box), spinner_box, FALSE, TRUE, 0);

  app.spinner = gtk_spinner_new();
  gtk_widget_show(app.spinner);
  gtk_box_pack_start(GTK_BOX(spinner_box), app.spinner, FALSE, FALSE, 0);

  app.label = gtk_label_new("Saved");
  gtk_label_set_xalign(GTK_LABEL(app.label), 0);
  gtk_widget_show(app.label);
  gtk_box_pack_end(GTK_BOX(spinner_box), app.label, TRUE, TRUE, 0);

  app.view = gtk_text_view_new();
  display = gdk_display_get_default();
  screen = gdk_display_get_default_screen(display);
  provider = gtk_css_provider_new();
  gtk_style_context_add_provider_for_screen(
      screen, GTK_STYLE_PROVIDER(provider),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  gtk_css_provider_load_from_data(
      provider, CHANGE_FONT_SIZE, strlen(CHANGE_FONT_SIZE), &error);
  GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app.view));
  g_signal_connect(
      G_OBJECT(app.view), "key-press-event", G_CALLBACK(key_press_event),
      &app);
  gtk_box_pack_start(GTK_BOX(box), app.view, TRUE, TRUE, 0);

  char* notes = from_disk();
  gtk_text_buffer_set_text(buffer, notes, -1);
  g_free(notes);

  gtk_widget_show_all(window);

  gtk_main();

  return 0;
}
