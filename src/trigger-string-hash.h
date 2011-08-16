#ifndef _TRIGGER_STRING_HASH_H_
#define _TRIGGER_STRING_HASH_H_

#include <glib.h>

G_BEGIN_DECLS

GHashTable *trigger_string_hash_from_file(const char *fname);

void trigger_string_hash_dump(GHashTable *trigger_string_hash);

void trigger_string_hash_destroy(GHashTable *trigger_string_hash);

G_END_DECLS

#endif /* !_TRIGGER_STRING_HASH_H_ */
