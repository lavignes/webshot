#include <gtk/gtk.h>
#include <cairo/cairo.h>
#include <webkit/webkit.h>

void goto_address(GtkEntry*, gpointer);
void goto_back(GtkToolButton*, gpointer);
void goto_forward(GtkToolButton*, gpointer);
void load_status(WebKitWebView*, gpointer);
void webshot(gchar*, gint, gint);
gboolean console_message(WebKitWebView*, gchar*, gint, gchar*, gpointer);
gboolean progress_update(gpointer);

void take_snapshot(GtkToolButton*, gpointer);

static GtkWidget* window;
static GtkWidget* urlbar;
static GtkWidget* view;
static GtkWidget* scrollbars;
static GtkToolItem* backbutton;
static GtkToolItem* forwardbutton;

gint main(gint argc, gchar** argv) {

  gtk_init(&argc, &argv);

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

  gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
  gtk_window_set_title(GTK_WINDOW(window), "webshot");

  view = webkit_web_view_new();
  GtkWidget* layout = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget* toolbar = gtk_toolbar_new();
  GtkToolItem* urltoolitem = gtk_tool_item_new();
  gtk_tool_item_set_expand(urltoolitem, TRUE);
  urlbar = gtk_entry_new();

  backbutton = gtk_tool_button_new_from_stock(GTK_STOCK_GO_BACK);
  gtk_widget_set_sensitive(GTK_WIDGET(backbutton), FALSE);

  forwardbutton = gtk_tool_button_new_from_stock(GTK_STOCK_GO_FORWARD);
  gtk_widget_set_sensitive(GTK_WIDGET(forwardbutton), FALSE);

  GtkToolItem* snapbutton = gtk_tool_button_new_from_stock(GTK_STOCK_SAVE);

  scrollbars = gtk_scrolled_window_new(NULL, NULL);

  gtk_style_context_add_class(gtk_widget_get_style_context(toolbar),
                              GTK_STYLE_CLASS_PRIMARY_TOOLBAR);

  gtk_container_add(GTK_CONTAINER(urltoolitem), urlbar);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), backbutton, 0);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), forwardbutton, 1);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), urltoolitem, 2);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar), snapbutton, 3);

  gtk_container_add(GTK_CONTAINER(scrollbars), view);

  gtk_box_pack_start(GTK_BOX(layout), toolbar, FALSE, TRUE, 0);
  gtk_box_pack_end(GTK_BOX(layout), scrollbars, TRUE, TRUE, 0);

  gtk_container_add(GTK_CONTAINER(window), layout);

  g_signal_connect(urlbar, "activate", G_CALLBACK(goto_address), NULL);
  g_signal_connect(view, "notify::load-status", G_CALLBACK(load_status), NULL);
  g_signal_connect(view, "console-message", G_CALLBACK(console_message), NULL);
  g_signal_connect(backbutton, "clicked", G_CALLBACK(goto_back), NULL);
  g_signal_connect(forwardbutton, "clicked", G_CALLBACK(goto_forward), NULL);
  g_signal_connect(snapbutton, "clicked", G_CALLBACK(take_snapshot), NULL);

  gtk_widget_show_all(window);

  gtk_main();

  return 0;
}

void goto_address(GtkEntry* urlbar, gpointer data) {

  GString* uri = g_string_new(gtk_entry_get_text(urlbar));

  gchar* scheme = g_uri_parse_scheme(uri->str);

  if (scheme == NULL) {
    g_message("Bad uri scheme provided, assuming http\n");
    g_string_prepend(uri, "http://");
  }

  g_free(scheme);
  
  // Ask webkit to load page
  webkit_web_view_load_uri(WEBKIT_WEB_VIEW(view), uri->str);

  g_string_free(uri, TRUE);
}

void goto_back(GtkToolButton* backbutton, gpointer data) {

  webkit_web_view_go_back(WEBKIT_WEB_VIEW(view));
}

void goto_forward(GtkToolButton* forwardbutton, gpointer data) {

  webkit_web_view_go_forward(WEBKIT_WEB_VIEW(view));
}

void load_status(WebKitWebView* view, gpointer data) {

  WebKitLoadStatus status = webkit_web_view_get_load_status(view);

  switch (status) {

    // The page is about to load
    case WEBKIT_LOAD_COMMITTED:
      g_timeout_add(100, progress_update, NULL);
      gtk_entry_set_text(GTK_ENTRY(urlbar), webkit_web_view_get_uri(view));
    break;

    case WEBKIT_LOAD_FINISHED:
      if (webkit_web_view_can_go_back(view))
        gtk_widget_set_sensitive(GTK_WIDGET(backbutton), TRUE);
      else
        gtk_widget_set_sensitive(GTK_WIDGET(backbutton), FALSE);

      if (webkit_web_view_can_go_forward(view))
        gtk_widget_set_sensitive(GTK_WIDGET(forwardbutton), TRUE);
      else
        gtk_widget_set_sensitive(GTK_WIDGET(forwardbutton), FALSE);

    break;
  }
}

gboolean progress_update(gpointer data) {

  gdouble progress = webkit_web_view_get_progress(WEBKIT_WEB_VIEW(view));

  gtk_entry_set_progress_fraction(GTK_ENTRY(urlbar), progress);

  if (progress == 1.0)
    return false;

  return true;
}

void take_snapshot(GtkToolButton* snapbutton, gpointer data) {

  GtkWidget* snap_dialog = gtk_dialog_new_with_buttons(
    "Take Snapshot",
    GTK_WINDOW(window),
    GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
    GTK_STOCK_CANCEL,
    GTK_RESPONSE_CANCEL,
    GTK_STOCK_SAVE,
    GTK_RESPONSE_OK,
    NULL
  );

  GtkAllocation allocation;
  gtk_widget_get_allocation(scrollbars, &allocation);

  GtkWidget* dialog_area = gtk_dialog_get_content_area(GTK_DIALOG(snap_dialog));

  GtkWidget* image_width = gtk_spin_button_new_with_range(allocation.width, 10000.0, 1.0);
  gtk_entry_set_icon_from_stock(
    GTK_ENTRY(image_width), GTK_ENTRY_ICON_PRIMARY, GTK_STOCK_ORIENTATION_LANDSCAPE);

  GtkWidget* image_height = gtk_spin_button_new_with_range(allocation.height, 10000.0, 1.0);
  gtk_entry_set_icon_from_stock(
    GTK_ENTRY(image_height), GTK_ENTRY_ICON_PRIMARY, GTK_STOCK_ORIENTATION_PORTRAIT);

  gtk_box_pack_start(GTK_BOX(dialog_area), image_width, FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(dialog_area), image_height, FALSE, FALSE, 0);

  gtk_widget_show_all(dialog_area);
  gint result = gtk_dialog_run(GTK_DIALOG(snap_dialog));

  if (result == GTK_RESPONSE_OK) {

    GtkWidget* save_dialog = gtk_file_chooser_dialog_new(
      "Save As",
      GTK_WINDOW(window),
      GTK_FILE_CHOOSER_ACTION_SAVE,
      GTK_STOCK_CANCEL,
      GTK_RESPONSE_CANCEL,
      GTK_STOCK_SAVE,
      GTK_RESPONSE_OK,
      NULL
    );

    gint result = gtk_dialog_run(GTK_DIALOG(save_dialog));

    if (result == GTK_RESPONSE_OK) {

      gchar* path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(save_dialog));

      webshot(path,
              gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(image_width)),
              gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(image_height)));

      g_free(path);
    }

    gtk_widget_destroy(save_dialog);
  } 

  gtk_widget_destroy(snap_dialog);
}

gboolean console_message(WebKitWebView* view, gchar* message,
                         gint line, gchar* source_id, gpointer data) {
  webkit_web_view_stop_loading(view);
  return FALSE;
}

void webshot(gchar* path, gint width, gint height) {

  GtkAllocation allocation;
  gtk_widget_get_allocation(scrollbars, &allocation);

  cairo_surface_t* surface = cairo_image_surface_create(
    CAIRO_FORMAT_ARGB32, width, height);

  cairo_t* cr = cairo_create(surface);

  cairo_save(cr);

  gdouble scalex = (gdouble) width / (gdouble) allocation.width;
  gdouble scaley = (gdouble) height / (gdouble) allocation.height;

  if (allocation.width < allocation.height)
    cairo_scale(cr, scalex, scalex);
  else
    cairo_scale(cr, scalex, scaley);

  // Draw main window
  //WebKitWebFrame* frame = webkit_web_view_get_main_frame(WEBKIT_WEB_VIEW(view));

  cairo_set_antialias(cr, CAIRO_ANTIALIAS_SUBPIXEL);
  gtk_widget_draw(scrollbars, cr);
  cairo_restore(cr);

  cairo_surface_write_to_png(surface, path);

  cairo_destroy(cr);
  cairo_surface_destroy(surface);
}