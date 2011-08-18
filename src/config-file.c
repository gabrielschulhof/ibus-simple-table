#include <stdio.h>
#include <gio/gio.h>
#include "config-file.h"
#include "debug.h"

struct  _SimpleTableConfigurationPriv {
  GHashTable *combining_mods;
  GHashTable *arbitrary_combos;
  gunichar trigger;
  GSettings *settings;
  char *fname;
  int max_length;
};

G_DEFINE_TYPE(SimpleTableConfiguration, simple_table_configuration, G_TYPE_OBJECT);

enum {
  CONFIG_FILENAME_PROPERTY = 1
};

static void
simple_table_configuration_reset(SimpleTableConfiguration *stc)
{
  if (stc->priv->fname) {
    g_free(stc->priv->fname);
    stc->priv->fname = NULL;
  }

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
  stc->priv->fname = g_strdup(fname);
  stc->priv->combining_mods = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)g_free);
  stc->priv->arbitrary_combos = g_hash_table_new_full(g_str_hash, g_str_equal, (GDestroyNotify)g_free, NULL);
  GKeyFile *file = g_key_file_new();
  if (file) {
    if (g_key_file_load_from_file(file, fname, 0, NULL)) {
      gsize n_combining_keys = 0;
      char **combining_keys = g_key_file_get_keys(file, "combining-map", &n_combining_keys, NULL);
      gsize n_arbitrary_keys = 0;
      char **arbitrary_keys = g_key_file_get_keys(file, "arbitrary", &n_arbitrary_keys, NULL);
      char *trigger = g_key_file_get_string(file, "trigger", "trigger", NULL);
      int Nix;
      gunichar value;
      const char *fmt = "U+%06X";

      /* Populate the combining-map */
      if (n_combining_keys && combining_keys) {
        int Nix1;
        char **values = NULL;
        gsize n_values;
        gunichar key;
        GArray *values_ar;

        for (Nix = 0 ; combining_keys[Nix] ; Nix++) {
          key = g_utf8_get_char_validated(combining_keys[Nix], -1);
          n_values = 0;
          if (key > 0) {
            values = g_key_file_get_string_list(file, "combining-map", combining_keys[Nix], &n_values, NULL);
            if (n_values && values) {
              values_ar = g_array_new(TRUE, TRUE, sizeof(gunichar));
              for (Nix1 = 0 ; values[Nix1] ; Nix1++) {
                sscanf(values[Nix1], fmt, &value);
                if (value > 0) {
                  gunichar debug_string[5] = {key, (gunichar)':', value, (gunichar)' ', 0};
                  char *utf8 = g_ucs4_to_utf8(debug_string, 3, NULL, NULL, NULL);
                  debug_print("simple_table_configuration_load_from_file: combining: %s: %s: %06x: %s\n", fmt, values[Nix1], value, utf8);
                  g_free(utf8);
                  g_array_append_vals(values_ar, &value, 1);
                }
              }

              g_hash_table_insert(stc->priv->combining_mods, GINT_TO_POINTER(key), g_array_free(values_ar, FALSE));
              stc->priv->max_length = MAX(stc->priv->max_length, 3);
              g_strfreev(values);
            }
          }
        }
        g_strfreev(combining_keys);
      }

      /* Populate the arbitrary combos hash */
      if (n_arbitrary_keys && arbitrary_keys) {
        int key_length;
        char *value_str;
        for (Nix = 0 ; arbitrary_keys[Nix] ; Nix++) {
          value_str = g_key_file_get_string(file, "arbitrary", arbitrary_keys[Nix], NULL);
          if (value_str) {
            sscanf(value_str, fmt, &value);
            if (value > 0) {
              gunichar debug_string[2] = {value, 0};
              char *utf8 = g_ucs4_to_utf8(debug_string, 3, NULL, NULL, NULL);
              debug_print("simple_table_configuration_load_from_file: arbitrary: %s: %s: %06x: %s : %s\n", fmt, value_str, value, arbitrary_keys[Nix], utf8);
              g_free(utf8);
              key_length = g_utf8_strlen(arbitrary_keys[Nix], -1) + 1;
              stc->priv->max_length = MAX(stc->priv->max_length, key_length);
              g_hash_table_insert(stc->priv->arbitrary_combos, arbitrary_keys[Nix], GINT_TO_POINTER(value));
            }
            else
              debug_print("simple_table_configuration_load_from_file: value is %d\n", value);
            g_free(value_str);
          }
          else
            debug_print("simple_table_configuration_load_from_file: value_str is NULL\n");
        }
      }
      else
        debug_print("simple_table_configuration_load_from_file: No arbitrary keys (n_arbitrary_keys = %d and arbitrary_keys = %p)\n",
          n_arbitrary_keys, arbitrary_keys);

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
//  simple_table_configuration_dump(stc);
}

static void
simple_table_configuration_file_changed_cb(GSettings *settings, gchar *key, SimpleTableConfiguration *stc)
{
  const char *fname = g_settings_get_string(settings, key);
  if (fname) {
    simple_table_configuration_reset(stc);
    simple_table_configuration_load_from_file(stc, fname);
    g_object_notify(G_OBJECT(stc), "config-file");
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
simple_table_configuration_get_property(GObject *obj, guint prop_id, GValue *val, GParamSpec *pspec)
{
  SimpleTableConfiguration *stc = SIMPLE_TABLE_CONFIGURATION(obj);
  switch(prop_id) {
    case CONFIG_FILENAME_PROPERTY:
      g_value_set_string(val, stc->priv->fname);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
      break;
  }
}

static void
simple_table_configuration_set_property(GObject *obj, guint prop_id, const GValue *val, GParamSpec *pspec)
{
  switch(prop_id) {
    case CONFIG_FILENAME_PROPERTY:
      g_settings_set_string(SIMPLE_TABLE_CONFIGURATION(obj)->priv->settings, "config-file", g_value_get_string(val));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
      break;
  }
}

static void
simple_table_configuration_class_init(SimpleTableConfigurationClass *klass)
{
  g_type_class_add_private(klass, sizeof(SimpleTableConfigurationPriv));

  G_OBJECT_CLASS(klass)->finalize = simple_table_configuration_finalize;
  G_OBJECT_CLASS(klass)->get_property = simple_table_configuration_get_property;
  G_OBJECT_CLASS(klass)->set_property = simple_table_configuration_set_property;

  g_object_class_install_property(G_OBJECT_CLASS(klass), CONFIG_FILENAME_PROPERTY,
    g_param_spec_string("config-file", "Configuration Filename", "Name of current configuration file",
      NULL, G_PARAM_READWRITE));
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

gunichar
simple_table_configuration_get_arbitrary(SimpleTableConfiguration *stc, const char *str)
{
  return GPOINTER_TO_INT(g_hash_table_lookup(stc->priv->arbitrary_combos, str));
}

int
simple_table_configuration_get_max_length(SimpleTableConfiguration *stc)
{
  return stc->priv->max_length;
}
