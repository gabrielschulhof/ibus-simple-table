/*
 * Simple IBus Table
 * Copyright (C) 2016  Gabriel Schulhof <gabrielschulhof@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/* vim:set et sts=4: */

#include <gtk/gtk.h>
#include "engine.h"
#include "config-file.h"
#include "debug.h"

typedef struct _IBusSimpleTableEngine IBusSimpleTableEngine;
typedef struct _IBusSimpleTableEngineClass IBusSimpleTableEngineClass;

struct _IBusSimpleTableEngine {
	IBusEngine parent;

    /* members */
    GArray *preedit;
    gint cursor_pos;
    SimpleTableConfiguration *stc;
    IBusPropList *prop_list;
    GtkWidget *config_file_chooser;
};

struct _IBusSimpleTableEngineClass {
	IBusEngineClass parent;
};

/* functions prototype */
static void	ibus_simple_table_engine_class_init	(IBusSimpleTableEngineClass	*klass);
static void	ibus_simple_table_engine_init		(IBusSimpleTableEngine		*engine);
static void	ibus_simple_table_engine_destroy		(IBusSimpleTableEngine		*engine);
static gboolean 
			ibus_simple_table_engine_process_key_event
                                            (IBusEngine             *engine,
                                             guint               	 keyval,
                                             guint               	 keycode,
                                             guint               	 modifiers);
#if (0)
static void ibus_simple_table_engine_focus_out   (IBusEngine             *engine);
static void ibus_simple_table_engine_reset       (IBusEngine             *engine);
static void ibus_simple_table_engine_disable     (IBusEngine             *engine);
static void ibus_engine_set_cursor_location (IBusEngine             *engine,
                                             gint                    x,
                                             gint                    y,
                                             gint                    w,
                                             gint                    h);
static void ibus_simple_table_engine_set_capabilities
                                            (IBusEngine             *engine,
                                             guint                   caps);
static void ibus_simple_table_engine_page_up     (IBusEngine             *engine);
static void ibus_simple_table_engine_page_down   (IBusEngine             *engine);
static void ibus_simple_table_engine_cursor_up   (IBusEngine             *engine);
static void ibus_simple_table_engine_cursor_down (IBusEngine             *engine);
static void ibus_simple_table_engine_property_activate  (IBusEngine             *engine,
                                             const gchar            *prop_name,
                                             gint                    prop_state);
static void ibus_simple_table_engine_property_show
											(IBusEngine             *engine,
                                             const gchar            *prop_name);
static void ibus_simple_table_engine_property_hide
											(IBusEngine             *engine,
                                             const gchar            *prop_name);
#endif /* (0) */
static void ibus_simple_table_engine_enable      (IBusEngine             *engine);
static void ibus_simple_table_engine_focus_in    (IBusEngine             *engine);
static void ibus_simple_table_engine_property_activate  (IBusEngine             *engine,
                                             const gchar            *prop_name,
                                             guint                    prop_state);
static void ibus_simple_table_engine_commit_string
                                            (IBusSimpleTableEngine      *ste,
                                             const gchar            *string);
static void ibus_simple_table_engine_update      (IBusSimpleTableEngine      *ste);

G_DEFINE_TYPE (IBusSimpleTableEngine, ibus_simple_table_engine, IBUS_TYPE_ENGINE)

static void
choose_config_file_response_cb(GtkWidget *dlg, guint response, IBusSimpleTableEngine *ste)
{
  if (GTK_RESPONSE_ACCEPT == response) {
    char *fname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
    char *old_fname = NULL;

    g_object_get(G_OBJECT(ste->stc), "config-file", &old_fname, NULL);
    if (g_strcmp0(old_fname, fname)) {
      debug_print("choose_config_file_response_cb: Setting stc config-file to %s\n");
      g_object_set(G_OBJECT(ste->stc), "config-file", fname, NULL);
    }
    g_free(old_fname);
    g_free(fname);
  }
  gtk_widget_destroy(dlg);
  ste->config_file_chooser = NULL;
}

static void
ibus_simple_table_engine_property_activate (IBusEngine *engine, const gchar *prop_name, guint prop_state)
{
  IBusSimpleTableEngine *ste = G_TYPE_CHECK_INSTANCE_CAST(engine, IBUS_TYPE_SIMPLE_TABLE_ENGINE, IBusSimpleTableEngine);
  if (!g_strcmp0(prop_name, "config-file")) {
    if (ste->config_file_chooser)
      gtk_window_present(GTK_WINDOW(ste->config_file_chooser));
    else {
      char *fname = NULL;

      ste->config_file_chooser = gtk_file_chooser_dialog_new("Choose Map File", NULL, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
      g_object_set(G_OBJECT(ste->config_file_chooser), "icon-name", GTK_STOCK_PROPERTIES, NULL);
      g_signal_connect(G_OBJECT(ste->config_file_chooser), "response", (GCallback)choose_config_file_response_cb, engine);
      gtk_widget_show(ste->config_file_chooser);

      g_object_get(G_OBJECT(ste->stc), "config-file", &fname, NULL);
      debug_print("ibus_simple_table_engine_property_activate: Got fname %s from stc\n", fname);
      gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(ste->config_file_chooser), fname);
      g_free(fname);
    }
  }
}

static void
ibus_simple_table_engine_class_init (IBusSimpleTableEngineClass *klass)
{
    IBusObjectClass *ibus_object_class = IBUS_OBJECT_CLASS (klass);
    IBusEngineClass *engine_class = IBUS_ENGINE_CLASS (klass);

    ibus_object_class->destroy = (IBusObjectDestroyFunc) ibus_simple_table_engine_destroy;

    engine_class->process_key_event = ibus_simple_table_engine_process_key_event;
    engine_class->focus_in = ibus_simple_table_engine_focus_in;
    engine_class->enable = ibus_simple_table_engine_enable;
    engine_class->property_activate = ibus_simple_table_engine_property_activate;

    debug_print("ibus_simple_table_engine_class_init: Exiting\n");
}

static void
ibus_simple_table_engine_focus_in(IBusEngine *engine)
{
  IBusSimpleTableEngine *ste = G_TYPE_CHECK_INSTANCE_CAST(engine, IBUS_TYPE_SIMPLE_TABLE_ENGINE, IBusSimpleTableEngine);
  ibus_engine_register_properties(engine, ste->prop_list);
}

static void
ibus_simple_table_engine_enable (IBusEngine *engine)
{
  IBusSimpleTableEngine *ste = G_TYPE_CHECK_INSTANCE_CAST(engine, IBUS_TYPE_SIMPLE_TABLE_ENGINE, IBusSimpleTableEngine);
  if (ste->preedit->len > 0)
    g_array_remove_range(ste->preedit, 0, ste->preedit->len);
  ste->cursor_pos = 0;
}

static void
ibus_simple_table_engine_config_changed(IBusSimpleTableEngine *ste, GParamSpec *pspec, SimpleTableConfiguration *stc)
{
  if (ste->preedit->len > 0)
    g_array_remove_range(ste->preedit, 0, ste->preedit->len);
  ste->cursor_pos = 0;
  ibus_simple_table_engine_update(ste);
}

static void
ibus_simple_table_engine_init (IBusSimpleTableEngine *ste)
{
  IBusProperty *config_file_prop = ibus_property_new(
    "config-file", 
    PROP_TYPE_NORMAL, 
    ibus_text_new_from_static_string("Choose Map File"),
    GTK_STOCK_PROPERTIES,
    ibus_text_new_from_static_string("Click to choose the map file containing the trigger, combining map, and arbitrary combinations."),
    TRUE, TRUE, 0, NULL);

  debug_print("ibus_simple_table_engine_init: Entering\n");

  ste->config_file_chooser = NULL;
  ste->preedit = g_array_new(TRUE, TRUE, sizeof(gunichar));
  ste->cursor_pos = 0;

  ste->stc = g_object_new(SIMPLE_TABLE_CONFIGURATION_TYPE, NULL);
  g_signal_connect_swapped(G_OBJECT(ste->stc), "notify::config-file", (GCallback)ibus_simple_table_engine_config_changed, ste);

  debug_print("ibus_simple_table_engine_init: Created config_file_prop: %p\n", config_file_prop);

  ste->prop_list = g_object_ref_sink(ibus_prop_list_new());

  debug_print("ibus_simple_table_engine_init: Created prop_list: %p\n", ste->prop_list);

  ibus_prop_list_append(ste->prop_list, config_file_prop);

  debug_print("ibus_simple_table_engine_init: Exiting\n");
}

static void
ibus_simple_table_engine_destroy (IBusSimpleTableEngine *ste)
{
  debug_print("ibus_simple_table_engine_destroy: Entering\n");
    if (ste->preedit) {
        g_array_free (ste->preedit, TRUE);
        ste->preedit = NULL;
    }

    if (ste->stc) {
      g_object_unref(ste->stc);
      ste->stc = NULL;
    }

    if (ste->prop_list) {
      g_object_unref(ste->prop_list);
      ste->prop_list = NULL;
    }

	((IBusObjectClass *) ibus_simple_table_engine_parent_class)->destroy ((IBusObject *)ste);
  debug_print("ibus_simple_table_engine_destroy: Entering\n");
}

static void
ibus_simple_table_engine_update_preedit (IBusSimpleTableEngine *ste)
{
    IBusText *text;

    debug_print("ibus_simple_table_engine_update_preedit: Entering\n");
    text = ibus_text_new_from_ucs4 ((const gunichar *)ste->preedit->data);
    text->attrs = ibus_attr_list_new ();
    
    ibus_attr_list_append (text->attrs,
                           ibus_attr_underline_new (IBUS_ATTR_UNDERLINE_SINGLE, 0, ste->preedit->len));

    ibus_engine_update_preedit_text ((IBusEngine *)ste,
                                     text,
                                     ste->cursor_pos,
                                     TRUE);

    if (ste->preedit->len == 0) {
        ibus_engine_hide_preedit_text((IBusEngine*)ste);
    }

    debug_print("ibus_simple_table_engine_update_preedit: Exiting\n");
}

/* commit preedit to client and update preedit */
static gboolean
ibus_simple_table_engine_commit_preedit (IBusSimpleTableEngine *ste)
{
    char *utf8;
    debug_print("ibus_simple_table_engine_commit_preedit: Entering\n");
    if (ste->preedit->len == 0) {
        debug_print("ibus_simple_table_engine_commit_preedit: Returning FALSE\n");
        return FALSE;
    }

    utf8 = g_ucs4_to_utf8(((const gunichar *)(ste->preedit->data)), -1, NULL, NULL, NULL);

    ibus_simple_table_engine_commit_string (ste, utf8);
    if (ste->preedit->len > 0)
      g_array_remove_range(ste->preedit, 0, ste->preedit->len);
    ste->cursor_pos = 0;

    ibus_simple_table_engine_update (ste);

    g_free(utf8);

    debug_print("ibus_simple_table_engine_commit_preedit: Returning TRUE\n");
    return TRUE;
}


static void
ibus_simple_table_engine_commit_string (IBusSimpleTableEngine *ste,
                                   const gchar       *string)
{
    IBusText *text;
    debug_print("ibus_simple_table_engine_commit_string: Entering\n");
    text = ibus_text_new_from_static_string (string);
    ibus_engine_commit_text ((IBusEngine *)ste, text);
    debug_print("ibus_simple_table_engine_commit_string: Entering\n");
}

static void
ibus_simple_table_engine_update (IBusSimpleTableEngine *ste)
{
    gunichar result = 0;
    char *utf8 = g_ucs4_to_utf8(((const gunichar *)(ste->preedit->data)), -1, NULL, NULL, NULL);
    debug_print("ibus_simple_table_engine_update: Entering (preedit is %s) -> len = %d\n", utf8, ste->preedit->len);
    g_free(utf8);

    if (ste->preedit->len <= simple_table_configuration_get_max_length(ste->stc)) {
      if (ste->preedit->len > 0) {
        utf8 = g_ucs4_to_utf8((const gunichar *)&g_array_index(ste->preedit, gunichar, 1), -1, NULL, NULL, NULL);
        debug_print("ibus_simple_table_engine_update: Looking up %s in arbitrary\n", utf8);
        result = simple_table_configuration_get_arbitrary(ste->stc, utf8);
        g_free(utf8);
      }

      if (3 == ste->preedit->len && result <= 0) {
        utf8 = g_ucs4_to_utf8(&g_array_index(ste->preedit, gunichar, 2), 1, NULL, NULL, NULL);
        debug_print("ibus_simple_table_engine_update: Looking up %s in hash\n", utf8);
        g_free(utf8);

        const gunichar *possible_modifiers = simple_table_configuration_get_combining(ste->stc, g_array_index(ste->preedit, gunichar, 2));

        if (possible_modifiers) {
          int Nix;

          for (Nix = 0 ; possible_modifiers[Nix] ; Nix++) {
            utf8 = g_ucs4_to_utf8(&possible_modifiers[Nix], 1, NULL, NULL, NULL);
            debug_print("ibus_simple_table_engine_update: Found %s in hash\n", utf8);
            g_free(utf8);

            if (g_unichar_compose(g_array_index(ste->preedit, gunichar, 1), possible_modifiers[Nix], &result))
              break;
            else {
              result = 0;
              debug_print("ibus_simple_table_engine_update: Characters fail to compose\n");
            }
          }
        }
        else
          debug_print("ibus_simple_table_engine_update: possible_modifiers not found in hash\n");
      }

      if (result > 0) {
        if (ste->preedit->len > 0)
          g_array_remove_range(ste->preedit, 0, ste->preedit->len);
        g_array_append_vals(ste->preedit, &result, 1);
        ste->cursor_pos = 1;
        ibus_simple_table_engine_commit_preedit(ste);
      }
    }
    else
      ibus_simple_table_engine_commit_preedit(ste);

    ibus_simple_table_engine_update_preedit (ste);

    ibus_engine_hide_lookup_table ((IBusEngine *)ste);
    debug_print("ibus_simple_table_engine_update: Exiting\n");
}

#define is_alpha(c) (((c) >= IBUS_a && (c) <= IBUS_z) || ((c) >= IBUS_A && (c) <= IBUS_Z))

static gboolean 
ibus_simple_table_engine_process_key_event (IBusEngine *engine,
                                       guint       keyval,
                                       guint       keycode,
                                       guint       modifiers)
{
    IBusSimpleTableEngine *ste = (IBusSimpleTableEngine *)engine;

    debug_print("ibus_simple_table_engine_process_key_event: Entering\n");
    if (modifiers & IBUS_RELEASE_MASK) {
        debug_print("ibus_simple_table_engine_process_key_event: Exiting FALSE because (modifiers & IBUS_RELEASE_MASK)\n");
        return FALSE;
    }

    modifiers &= (IBUS_CONTROL_MASK | IBUS_MOD1_MASK);

    if (modifiers != 0) {
        debug_print("ibus_simple_table_engine_process_key_event: Exiting %s\n", (ste->preedit->len == 0) ? "FALSE" : "TRUE");
        if (ste->preedit->len == 0)
            return FALSE;
        else
            return TRUE;
    }


    switch (keyval) {
    case IBUS_space: {
        if (ste->preedit->len == 0) {
            return FALSE;
        }
        gunichar s = ((gunichar)' ');
        g_array_append_vals(ste->preedit, &s, 1);
        return ibus_simple_table_engine_commit_preedit (ste);
    }
    case IBUS_Return:
        return ibus_simple_table_engine_commit_preedit (ste);

    case IBUS_Escape:
        if (ste->preedit->len == 0)
            return FALSE;

        if (ste->preedit->len > 0)
          g_array_remove_range(ste->preedit, 0, ste->preedit->len);
        ste->cursor_pos = 0;
        ibus_simple_table_engine_update (ste);
        return TRUE;        

    case IBUS_Left:
        if (ste->preedit->len == 0)
            return FALSE;
        if (ste->cursor_pos > 0) {
            ste->cursor_pos --;
            ibus_simple_table_engine_update (ste);
        }
        return TRUE;

    case IBUS_Right:
        if (ste->preedit->len == 0)
            return FALSE;
        if (ste->cursor_pos < ste->preedit->len) {
            ste->cursor_pos ++;
            ibus_simple_table_engine_update (ste);
        }
        return TRUE;
    
    case IBUS_Up:
        if (ste->preedit->len == 0)
            return FALSE;
        if (ste->cursor_pos != 0) {
            ste->cursor_pos = 0;
            ibus_simple_table_engine_update (ste);
        }
        return TRUE;

    case IBUS_Down:
        if (ste->preedit->len == 0)
            return FALSE;
        
        if (ste->cursor_pos != ste->preedit->len) {
            ste->cursor_pos = ste->preedit->len;
            ibus_simple_table_engine_update (ste);
        }
        
        return TRUE;
    
    case IBUS_BackSpace:
        if (ste->preedit->len == 0)
            return FALSE;
        if (ste->cursor_pos > 0) {
            ste->cursor_pos --;
            g_array_remove_index(ste->preedit, ste->cursor_pos);
            ibus_simple_table_engine_update (ste);
        }
        return TRUE;
    
    case IBUS_Delete:
        if (ste->preedit->len == 0)
            return FALSE;
        if (ste->cursor_pos < ste->preedit->len) {
            g_array_remove_index(ste->preedit, ste->cursor_pos);
            ibus_simple_table_engine_update (ste);
        }
        return TRUE;
    }

    debug_print("ibus_simple_table_engine_process_key_event: Considering %x|%c|\n", keyval, (char)keyval);

    /* 0xf--- seem to be modifier keys that do not result in symbols */
    if ((((keyval >> 12) & 0xf) != 0xf) &&
        ((ste->preedit->len > 0) ||
         (0 == ste->preedit->len &&
          keyval == simple_table_configuration_get_trigger(ste->stc)))) {
      g_array_insert_vals(ste->preedit, ste->cursor_pos, &keyval, 1);
      ste->cursor_pos ++;
      ibus_simple_table_engine_update (ste);

      return TRUE;
    }

    return FALSE;
}
