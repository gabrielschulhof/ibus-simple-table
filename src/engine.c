/* vim:set et sts=4: */

#include <gtk/gtk.h>
#include "engine.h"
#include "config-file.h"
#include "debug.h"

typedef struct _IBusEnchantEngine IBusEnchantEngine;
typedef struct _IBusEnchantEngineClass IBusEnchantEngineClass;

struct _IBusEnchantEngine {
	IBusEngine parent;

    /* members */
    GArray *preedit;
    gint cursor_pos;
    SimpleTableConfiguration *stc;
    IBusPropList *prop_list;
    GtkWidget *config_file_chooser;
};

struct _IBusEnchantEngineClass {
	IBusEngineClass parent;
};

/* functions prototype */
static void	ibus_enchant_engine_class_init	(IBusEnchantEngineClass	*klass);
static void	ibus_enchant_engine_init		(IBusEnchantEngine		*engine);
static void	ibus_enchant_engine_destroy		(IBusEnchantEngine		*engine);
static gboolean 
			ibus_enchant_engine_process_key_event
                                            (IBusEngine             *engine,
                                             guint               	 keyval,
                                             guint               	 keycode,
                                             guint               	 modifiers);
#if (0)
static void ibus_enchant_engine_focus_out   (IBusEngine             *engine);
static void ibus_enchant_engine_reset       (IBusEngine             *engine);
static void ibus_enchant_engine_disable     (IBusEngine             *engine);
static void ibus_engine_set_cursor_location (IBusEngine             *engine,
                                             gint                    x,
                                             gint                    y,
                                             gint                    w,
                                             gint                    h);
static void ibus_enchant_engine_set_capabilities
                                            (IBusEngine             *engine,
                                             guint                   caps);
static void ibus_enchant_engine_page_up     (IBusEngine             *engine);
static void ibus_enchant_engine_page_down   (IBusEngine             *engine);
static void ibus_enchant_engine_cursor_up   (IBusEngine             *engine);
static void ibus_enchant_engine_cursor_down (IBusEngine             *engine);
static void ibus_enchant_engine_property_activate  (IBusEngine             *engine,
                                             const gchar            *prop_name,
                                             gint                    prop_state);
static void ibus_enchant_engine_property_show
											(IBusEngine             *engine,
                                             const gchar            *prop_name);
static void ibus_enchant_engine_property_hide
											(IBusEngine             *engine,
                                             const gchar            *prop_name);
#endif /* (0) */
static void ibus_enchant_engine_enable      (IBusEngine             *engine);
static void ibus_enchant_engine_focus_in    (IBusEngine             *engine);
static void ibus_enchant_engine_property_activate  (IBusEngine             *engine,
                                             const gchar            *prop_name,
                                             guint                    prop_state);
static void ibus_enchant_engine_commit_string
                                            (IBusEnchantEngine      *enchant,
                                             const gchar            *string);
static void ibus_enchant_engine_update      (IBusEnchantEngine      *enchant);

G_DEFINE_TYPE (IBusEnchantEngine, ibus_enchant_engine, IBUS_TYPE_ENGINE)

static void
choose_config_file_response_cb(GtkWidget *dlg, guint response, IBusEnchantEngine *enchant)
{
  if (GTK_RESPONSE_ACCEPT == response) {
    char *fname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
    char *old_fname = NULL;

    g_object_get(G_OBJECT(enchant->stc), "config-file", &old_fname, NULL);
    if (g_strcmp0(old_fname, fname)) {
      debug_print("choose_config_file_response_cb: Setting stc config-file to %s\n");
      g_object_set(G_OBJECT(enchant->stc), "config-file", fname, NULL);
    }
    g_free(old_fname);
    g_free(fname);
  }
  gtk_widget_destroy(dlg);
  enchant->config_file_chooser = NULL;
}

static void
ibus_enchant_engine_property_activate (IBusEngine *engine, const gchar *prop_name, guint prop_state)
{
  IBusEnchantEngine *enchant = G_TYPE_CHECK_INSTANCE_CAST(engine, IBUS_TYPE_ENCHANT_ENGINE, IBusEnchantEngine);
  if (!g_strcmp0(prop_name, "config-file")) {
    if (enchant->config_file_chooser)
      gtk_window_present(GTK_WINDOW(enchant->config_file_chooser));
    else {
      char *fname = NULL;

      enchant->config_file_chooser = gtk_file_chooser_dialog_new("Choose Map File", NULL, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
      g_object_set(G_OBJECT(enchant->config_file_chooser), "icon-name", GTK_STOCK_PROPERTIES, NULL);
      g_signal_connect(G_OBJECT(enchant->config_file_chooser), "response", (GCallback)choose_config_file_response_cb, engine);
      gtk_widget_show(enchant->config_file_chooser);

      g_object_get(G_OBJECT(enchant->stc), "config-file", &fname, NULL);
      debug_print("ibus_enchant_engine_property_activate: Got fname %s from stc\n", fname);
      gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(enchant->config_file_chooser), fname);
      g_free(fname);
    }
  }
}

static void
ibus_enchant_engine_class_init (IBusEnchantEngineClass *klass)
{
    IBusObjectClass *ibus_object_class = IBUS_OBJECT_CLASS (klass);
    IBusEngineClass *engine_class = IBUS_ENGINE_CLASS (klass);

    ibus_object_class->destroy = (IBusObjectDestroyFunc) ibus_enchant_engine_destroy;

    engine_class->process_key_event = ibus_enchant_engine_process_key_event;
    engine_class->focus_in = ibus_enchant_engine_focus_in;
    engine_class->enable = ibus_enchant_engine_enable;
    engine_class->property_activate = ibus_enchant_engine_property_activate;

    debug_print("ibus_enchant_engine_class_init: Exiting\n");
}

static void
ibus_enchant_engine_focus_in(IBusEngine *engine)
{
  IBusEnchantEngine *enchant = G_TYPE_CHECK_INSTANCE_CAST(engine, IBUS_TYPE_ENCHANT_ENGINE, IBusEnchantEngine);
  ibus_engine_register_properties(engine, enchant->prop_list);
}

static void
ibus_enchant_engine_enable (IBusEngine *engine)
{
  IBusEnchantEngine *enchant = G_TYPE_CHECK_INSTANCE_CAST(engine, IBUS_TYPE_ENCHANT_ENGINE, IBusEnchantEngine);
  if (enchant->preedit->len > 0)
    g_array_remove_range(enchant->preedit, 0, enchant->preedit->len);
  enchant->cursor_pos = 0;
}

static void
ibus_enchant_engine_config_changed(IBusEnchantEngine *enchant, GParamSpec *pspec, SimpleTableConfiguration *stc)
{
  if (enchant->preedit->len > 0)
    g_array_remove_range(enchant->preedit, 0, enchant->preedit->len);
  enchant->cursor_pos = 0;
  ibus_enchant_engine_update(enchant);
}

static void
ibus_enchant_engine_init (IBusEnchantEngine *enchant)
{
  IBusProperty *config_file_prop = ibus_property_new(
    "config-file", 
    PROP_TYPE_NORMAL, 
    ibus_text_new_from_static_string("Choose Map File"),
    GTK_STOCK_PROPERTIES,
    ibus_text_new_from_static_string("Click to choose the map file containing the trigger, combining map, and arbitrary combinations."),
    TRUE, TRUE, 0, NULL);

  debug_print("ibus_enchant_engine_init: Entering\n");

  enchant->config_file_chooser = NULL;
  enchant->preedit = g_array_new(TRUE, TRUE, sizeof(gunichar));
  enchant->cursor_pos = 0;

  enchant->stc = g_object_new(SIMPLE_TABLE_CONFIGURATION_TYPE, NULL);
  g_signal_connect_swapped(G_OBJECT(enchant->stc), "notify::config-file", (GCallback)ibus_enchant_engine_config_changed, enchant);

  debug_print("ibus_enchant_engine_init: Created config_file_prop: %p\n", config_file_prop);

  enchant->prop_list = g_object_ref_sink(ibus_prop_list_new());

  debug_print("ibus_enchant_engine_init: Created prop_list: %p\n", enchant->prop_list);

  ibus_prop_list_append(enchant->prop_list, config_file_prop);

  debug_print("ibus_enchant_engine_init: Exiting\n");
}

static void
ibus_enchant_engine_destroy (IBusEnchantEngine *enchant)
{
  debug_print("ibus_enchant_engine_destroy: Entering\n");
    if (enchant->preedit) {
        g_array_free (enchant->preedit, TRUE);
        enchant->preedit = NULL;
    }

    if (enchant->stc) {
      g_object_unref(enchant->stc);
      enchant->stc = NULL;
    }

    if (enchant->prop_list) {
      g_object_unref(enchant->prop_list);
      enchant->prop_list = NULL;
    }

	((IBusObjectClass *) ibus_enchant_engine_parent_class)->destroy ((IBusObject *)enchant);
  debug_print("ibus_enchant_engine_destroy: Entering\n");
}

static void
ibus_enchant_engine_update_preedit (IBusEnchantEngine *enchant)
{
    IBusText *text;

    debug_print("ibus_enchant_engine_update_preedit: Entering\n");
    text = ibus_text_new_from_ucs4 ((const gunichar *)enchant->preedit->data);
    text->attrs = ibus_attr_list_new ();
    
    ibus_attr_list_append (text->attrs,
                           ibus_attr_underline_new (IBUS_ATTR_UNDERLINE_SINGLE, 0, enchant->preedit->len));

    ibus_engine_update_preedit_text ((IBusEngine *)enchant,
                                     text,
                                     enchant->cursor_pos,
                                     TRUE);

    debug_print("ibus_enchant_engine_update_preedit: Exiting\n");
}

/* commit preedit to client and update preedit */
static gboolean
ibus_enchant_engine_commit_preedit (IBusEnchantEngine *enchant)
{
    char *utf8;
    debug_print("ibus_enchant_engine_commit_preedit: Entering\n");
    if (enchant->preedit->len == 0) {
        debug_print("ibus_enchant_engine_commit_preedit: Returning FALSE\n");
        return FALSE;
    }

    utf8 = g_ucs4_to_utf8(((const gunichar *)(enchant->preedit->data)), -1, NULL, NULL, NULL);

    ibus_enchant_engine_commit_string (enchant, utf8);
    if (enchant->preedit->len > 0)
      g_array_remove_range(enchant->preedit, 0, enchant->preedit->len);
    enchant->cursor_pos = 0;

    ibus_enchant_engine_update (enchant);

    g_free(utf8);

    debug_print("ibus_enchant_engine_commit_preedit: Returning TRUE\n");
    return TRUE;
}


static void
ibus_enchant_engine_commit_string (IBusEnchantEngine *enchant,
                                   const gchar       *string)
{
    IBusText *text;
    debug_print("ibus_enchant_engine_commit_string: Entering\n");
    text = ibus_text_new_from_static_string (string);
    ibus_engine_commit_text ((IBusEngine *)enchant, text);
    debug_print("ibus_enchant_engine_commit_string: Entering\n");
}

static void
ibus_enchant_engine_update (IBusEnchantEngine *enchant)
{
    gunichar result = 0;
    char *utf8 = g_ucs4_to_utf8(((const gunichar *)(enchant->preedit->data)), -1, NULL, NULL, NULL);
    debug_print("ibus_enchant_engine_update: Entering (preedit is %s) -> len = %d\n", utf8, enchant->preedit->len);
    g_free(utf8);

    if (enchant->preedit->len <= simple_table_configuration_get_max_length(enchant->stc)) {
      if (enchant->preedit->len > 0) {
        utf8 = g_ucs4_to_utf8((const gunichar *)&g_array_index(enchant->preedit, gunichar, 1), -1, NULL, NULL, NULL);
        debug_print("ibus_enchant_engine_update: Looking up %s in arbitrary\n", utf8);
        result = simple_table_configuration_get_arbitrary(enchant->stc, utf8);
        g_free(utf8);
      }

      if (3 == enchant->preedit->len && result <= 0) {
        utf8 = g_ucs4_to_utf8(&g_array_index(enchant->preedit, gunichar, 2), 1, NULL, NULL, NULL);
        debug_print("ibus_enchant_engine_update: Looking up %s in hash\n", utf8);
        g_free(utf8);

        const gunichar *possible_modifiers = simple_table_configuration_get_combining(enchant->stc, g_array_index(enchant->preedit, gunichar, 2));

        if (possible_modifiers) {
          int Nix;

          for (Nix = 0 ; possible_modifiers[Nix] ; Nix++) {
            utf8 = g_ucs4_to_utf8(&possible_modifiers[Nix], 1, NULL, NULL, NULL);
            debug_print("ibus_enchant_engine_update: Found %s in hash\n", utf8);
            g_free(utf8);

            if (g_unichar_compose(g_array_index(enchant->preedit, gunichar, 1), possible_modifiers[Nix], &result))
              break;
            else {
              result = 0;
              debug_print("ibus_enchant_engine_update: Characters fail to compose\n");
            }
          }
        }
        else
          debug_print("ibus_enchant_engine_update: possible_modifiers not found in hash\n");
      }

      if (result > 0) {
        if (enchant->preedit->len > 0)
          g_array_remove_range(enchant->preedit, 0, enchant->preedit->len);
        g_array_append_vals(enchant->preedit, &result, 1);
        enchant->cursor_pos = 1;
        ibus_enchant_engine_commit_preedit(enchant);
      }
    }
    else
      ibus_enchant_engine_commit_preedit(enchant);

    ibus_enchant_engine_update_preedit (enchant);

    ibus_engine_hide_lookup_table ((IBusEngine *)enchant);
    debug_print("ibus_enchant_engine_update: Exiting\n");
}

#define is_alpha(c) (((c) >= IBUS_a && (c) <= IBUS_z) || ((c) >= IBUS_A && (c) <= IBUS_Z))

static gboolean 
ibus_enchant_engine_process_key_event (IBusEngine *engine,
                                       guint       keyval,
                                       guint       keycode,
                                       guint       modifiers)
{
    IBusEnchantEngine *enchant = (IBusEnchantEngine *)engine;

    debug_print("ibus_enchant_engine_process_key_event: Entering\n");
    if (modifiers & IBUS_RELEASE_MASK) {
        debug_print("ibus_enchant_engine_process_key_event: Exiting FALSE because (modifiers & IBUS_RELEASE_MASK)\n");
        return FALSE;
    }

    modifiers &= (IBUS_CONTROL_MASK | IBUS_MOD1_MASK);

    if (modifiers != 0) {
        debug_print("ibus_enchant_engine_process_key_event: Exiting %s\n", (enchant->preedit->len == 0) ? "FALSE" : "TRUE");
        if (enchant->preedit->len == 0)
            return FALSE;
        else
            return TRUE;
    }


    switch (keyval) {
    case IBUS_space: {
        gunichar s = ((gunichar)' ');
        g_array_append_vals(enchant->preedit, &s, 1);
        return ibus_enchant_engine_commit_preedit (enchant);
    }
    case IBUS_Return:
        return ibus_enchant_engine_commit_preedit (enchant);

    case IBUS_Escape:
        if (enchant->preedit->len == 0)
            return FALSE;

        if (enchant->preedit->len > 0)
          g_array_remove_range(enchant->preedit, 0, enchant->preedit->len);
        enchant->cursor_pos = 0;
        ibus_enchant_engine_update (enchant);
        return TRUE;        

    case IBUS_Left:
        if (enchant->preedit->len == 0)
            return FALSE;
        if (enchant->cursor_pos > 0) {
            enchant->cursor_pos --;
            ibus_enchant_engine_update (enchant);
        }
        return TRUE;

    case IBUS_Right:
        if (enchant->preedit->len == 0)
            return FALSE;
        if (enchant->cursor_pos < enchant->preedit->len) {
            enchant->cursor_pos ++;
            ibus_enchant_engine_update (enchant);
        }
        return TRUE;
    
    case IBUS_Up:
        if (enchant->preedit->len == 0)
            return FALSE;
        if (enchant->cursor_pos != 0) {
            enchant->cursor_pos = 0;
            ibus_enchant_engine_update (enchant);
        }
        return TRUE;

    case IBUS_Down:
        if (enchant->preedit->len == 0)
            return FALSE;
        
        if (enchant->cursor_pos != enchant->preedit->len) {
            enchant->cursor_pos = enchant->preedit->len;
            ibus_enchant_engine_update (enchant);
        }
        
        return TRUE;
    
    case IBUS_BackSpace:
        if (enchant->preedit->len == 0)
            return FALSE;
        if (enchant->cursor_pos > 0) {
            enchant->cursor_pos --;
            g_array_remove_index(enchant->preedit, enchant->cursor_pos);
            ibus_enchant_engine_update (enchant);
        }
        return TRUE;
    
    case IBUS_Delete:
        if (enchant->preedit->len == 0)
            return FALSE;
        if (enchant->cursor_pos < enchant->preedit->len) {
            g_array_remove_index(enchant->preedit, enchant->cursor_pos);
            ibus_enchant_engine_update (enchant);
        }
        return TRUE;
    }

    debug_print("ibus_enchant_engine_process_key_event: Considering %x|%c|\n", keyval, (char)keyval);

    /* 0xf--- seem to be modifier keys that do not result in symbols */
    if ((((keyval >> 12) & 0xf) != 0xf) &&
        ((enchant->preedit->len > 0) ||
         (0 == enchant->preedit->len &&
          keyval == simple_table_configuration_get_trigger(enchant->stc)))) {
      g_array_insert_vals(enchant->preedit, enchant->cursor_pos, &keyval, 1);
      enchant->cursor_pos ++;
      ibus_enchant_engine_update (enchant);

      return TRUE;
    }

    return FALSE;
}
