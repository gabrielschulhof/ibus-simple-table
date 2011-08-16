#include <stdio.h>
#include "trigger-string-hash.h"

typedef struct {
  int indent;
  GString *str;
  gboolean recurse;
} ForeachParams;

static void
trigger_string_hash_dump_foreach_cb(gunichar key, GHashTable *tbl, ForeachParams *p)
{
  g_string_printf(p->str, "%*s%p: ", p->indent, " ", tbl);
  g_string_append_unichar(p->str, key);
  g_string_append(p->str, " ->\n");
  fprintf(stderr, p->str->str);
  if (p->recurse) {
    p->indent += 2;
    g_hash_table_foreach(tbl, (GHFunc)trigger_string_hash_dump_foreach_cb, p);
    p->indent -= 2;
  }
}

static void
trigger_string_hash_dump_priv(GHashTable *trigger_string_hash, gboolean recurse)
{
  ForeachParams p = { 0, g_string_new(""), recurse };
  g_hash_table_foreach(trigger_string_hash, (GHFunc)trigger_string_hash_dump_foreach_cb, &p);
  g_string_free(p.str, TRUE);
}

GHashTable *
trigger_string_hash_from_file(const char *fname)
{
  GHashTable *ret = g_hash_table_new(NULL, NULL);

  FILE *pfile = fopen(fname, "r");
  if (pfile) {
    GHashTable *dst = ret;
    GHashTable *tbl = NULL;
    GString *str = g_string_new("");
    char c;
    gunichar result;

    while(fread(&c, 1, 1, pfile)) {
      g_string_append_c(str, c);
      result = g_utf8_get_char_validated(str->str, -1);
      if (((gunichar)-2) != result) {
        if (result >= 0) {
          if ('\n' == result)
            dst = ret;
          else
          if ('\t' != result) {
            tbl = g_hash_table_lookup(dst, GINT_TO_POINTER(result));
            if (NULL == tbl) {
              tbl = g_hash_table_new(NULL, NULL);
              g_hash_table_insert(dst, GINT_TO_POINTER(result), tbl);
            }
            dst = tbl;
          }
        }
        g_string_assign(str, "");
      }
    }
    g_string_free(str, TRUE);
  }

  return ret;
}

void
trigger_string_hash_destroy_foreach_cb(gunichar key, GHashTable *tbl, gpointer null)
{
  trigger_string_hash_destroy(tbl);
}

void
trigger_string_hash_destroy(GHashTable *trigger_string_hash)
{
  g_hash_table_foreach(trigger_string_hash, (GHFunc)trigger_string_hash_destroy_foreach_cb, NULL);
  g_hash_table_unref(trigger_string_hash);
}

void
trigger_string_hash_dump(GHashTable *trigger_string_hash)
{
  trigger_string_hash_dump_priv(trigger_string_hash, TRUE);
}
