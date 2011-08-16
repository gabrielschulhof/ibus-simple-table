#include <stdio.h>
#include <gio/gio.h>
#include "config-file.h"
#include "debug.h"

struct  _SimpleTableConfigurationPriv {
  GHashTable *combining_mods;
  GHashTable *arbitrary_combos;
  gunichar trigger;
  GSettings *settings;
};

G_DEFINE_TYPE(SimpleTableConfiguration, simple_table_configuration, G_TYPE_OBJECT);

enum {
  CHANGED_SIGNAL,
  N_SIGNALS  
};
static guint signal_ids[N_SIGNALS] = { 0 };

static void
simple_table_configuration_reset(SimpleTableConfiguration *stc)
{
  if (stc->priv->combining_mods) {
    g_hash_table_unref(stc->priv->combining_mods);
    stc->priv->combining_mods = NULL;
  }

  if (stc->priv->arbitrary_combos) {
    g_hash_table_unref(stc->priv->arbitrary_combos);
    stc->priv->arbitrary_combos = NULL;
  }

  stc->priv->trigger = (gunichar)0;
}

static void
simple_table_configuration_dump_foreach_cb(gunichar key, gunichar *value, GString *foreach_str)
{
  int Nix;
  g_string_assign(foreach_str, "");
  g_string_append_unichar(foreach_str, key);
  g_string_append_printf(foreach_str, ": ");
  if (value[0]) {
    g_string_append_unichar(foreach_str, value[0]);
    for (Nix = 1 ; value[Nix] ; Nix++) {
      g_string_append_printf(foreach_str, ", ");
      g_string_append_unichar(foreach_str, value[Nix]);
    }
  }
  debug_print("simple_table_configuration_dump: combining-mods: %s\n", foreach_str->str);
}

static void
simple_table_configuration_dump(SimpleTableConfiguration *stc)
{
  GString *foreach_str = g_string_new("");
  char *utf8;
  gunichar c[2] = {0, 0};

  c[0] = stc->priv->trigger;
  utf8 = g_ucs4_to_utf8(c, -1, NULL, NULL, NULL);
  debug_print("simple_table_configuration_dump: trigger: %s\n", utf8);
  g_free(utf8);

  g_hash_table_foreach(stc->priv->combining_mods, (GHFunc)simple_table_configuration_dump_foreach_cb, foreach_str);
  g_string_free(foreach_str, TRUE);
}

static void
simple_table_configuration_load_from_file(SimpleTableConfiguration *stc, const char *fname)
{
  stc->priv->combining_mods = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)g_free);
  GKeyFile *file = g_key_file_new();
  if (file) {
    if (g_key_file_load_from_file(file, fname, 0, NULL)) {
      gsize n_keys = 0;
      char *trigger = g_key_file_get_string(file, "trigger", "trigger", NULL);
      char **keys = g_key_file_get_keys(file, "combining-map", &n_keys, NULL);

      /* Populate the combining-map */
      if (n_keys && keys) {
        int Nix, Nix1;
        char **values = NULL;
        gsize n_values;
        gunichar key;
        gunichar value;
        GArray *values_ar;

        for (Nix = 0 ; keys[Nix] ; Nix++) {
          key = g_utf8_get_char_validated(keys[Nix], -1);
          n_values = 0;
          if (key > 0) {
            values = g_key_file_get_string_list(file, "combining-map", keys[Nix], &n_values, NULL);
            if (n_values && values) {
              values_ar = g_array_new(TRUE, TRUE, sizeof(gunichar));
              for (Nix1 = 0 ; values[Nix1] ; Nix1++) {
                const char *fmt = "U+%06X";
                sscanf(values[Nix1], fmt, &value);
                if (value > 0) {
                  gunichar debug_string[5] = {key, (gunichar)':', value, (gunichar)' ', 0};
                  char *utf8 = g_ucs4_to_utf8(debug_string, 3, NULL, NULL, NULL);
                  debug_print("simple_table_configuration_load_from_file: %s: %s: %x: %s\n", fmt, values[Nix1], value, utf8);
                  g_free(utf8);
                  g_array_append_vals(values_ar, &value, 1);
                }
              }
              
              g_hash_table_insert(stc->priv->combining_mods, GINT_TO_POINTER(key), g_array_free(values_ar, FALSE));
              g_strfreev(values);
            }
          }
        }
        g_strfreev(keys);
      }

      /* Set the trigger */
      if (trigger) {
        gunichar trigger_c = g_utf8_get_char_validated(trigger, -1);
        if (trigger_c > 0) {
          gunichar ucs4[2] = { trigger_c, 0};
          char *utf8 = g_ucs4_to_utf8(ucs4, 2, NULL, NULL, NULL);
          debug_print("simple_table_configuration_load_from_file: trigger is %s\n", utf8);
          g_free(utf8);
          stc->priv->trigger = trigger_c;
        }
        else
          debug_print("simple_table_configuration_load_from_file: trigger is %d\n", trigger_c);
        g_free(trigger);
      }
      else
        debug_print("simple_table_configuration_load_from_file: trigger is NULL!\n");
    }

    g_key_file_free(file);
  }

  debug_print("simple_table_configuration_load_from_file: Dumping\n");
  simple_table_configuration_dump(stc);
}

static void
simple_table_configuration_file_changed_cb(GSettings *settings, gchar *key, SimpleTableConfiguration *stc)
{
  const char *fname = g_settings_get_string(settings, key);
  if (fname) {
    simple_table_configuration_reset(stc);
    simple_table_configuration_load_from_file(stc, fname);
    g_signal_emit(stc, signal_ids[CHANGED_SIGNAL], 0);
  }
}

static void
simple_table_configuration_init(SimpleTableConfiguration *stc)
{
  GSettings *settings = g_settings_new("ca.go-nix.IBusSimpleTable");

  debug_print("simple_table_configuration_init: Entering\n");

  stc->priv =
    G_TYPE_INSTANCE_GET_PRIVATE(stc, SIMPLE_TABLE_CONFIGURATION_TYPE, SimpleTableConfigurationPriv);
  g_signal_connect(G_OBJECT(settings), "changed::config-file", (GCallback)simple_table_configuration_file_changed_cb, stc);
  simple_table_configuration_file_changed_cb(settings, "config-file", stc);
}

static void 
simple_table_configuration_finalize(GObject *obj)
{
  SimpleTableConfiguration *stc = SIMPLE_TABLE_CONFIGURATION(obj);
  g_object_unref(stc->priv->settings);

  simple_table_configuration_reset(stc);
}

static void
simple_table_configuration_class_init(SimpleTableConfigurationClass *klass)
{
  signal_ids[0] = 
    g_signal_new("changed", SIMPLE_TABLE_CONFIGURATION_TYPE, G_SIGNAL_RUN_LAST,
      G_STRUCT_OFFSET(SimpleTableConfigurationClass, changed), NULL, NULL,
      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
  g_type_class_add_private(klass, sizeof(SimpleTableConfigurationPriv));
  G_OBJECT_CLASS(klass)->finalize = simple_table_configuration_finalize;
}

const gunichar *
simple_table_configuration_get_combining(SimpleTableConfiguration *stc, gunichar c)
{
  gunichar ret = 0;

  g_return_val_if_fail(stc != NULL, 0);
  g_return_val_if_fail(stc->priv != NULL, 0);
  g_return_val_if_fail(stc->priv->combining_mods != NULL, 0);

  return (const gunichar *)g_hash_table_lookup(stc->priv->combining_mods, GINT_TO_POINTER(c));
}

gunichar
simple_table_configuration_get_trigger(SimpleTableConfiguration *stc)
{
  g_return_val_if_fail(stc != NULL, 0);
  g_return_val_if_fail(stc->priv != NULL, 0);

  return stc->priv->trigger;
}
