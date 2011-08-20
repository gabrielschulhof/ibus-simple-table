/* vim:set et sts=4: */
#ifndef __ENGINE_H__
#define __ENGINE_H__

#include <ibus.h>

G_BEGIN_DECLS

#define IBUS_TYPE_SIMPLE_TABLE_ENGINE	\
	(ibus_simple_table_engine_get_type ())

GType   ibus_simple_table_engine_get_type    (void);

G_END_DECLS

#endif
