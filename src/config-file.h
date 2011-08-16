#ifndef _CONFIG_FILE_H_
#define _CONFIG_FILE_H_

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _SimpleTableConfiguration      SimpleTableConfiguration;
typedef struct _SimpleTableConfigurationPriv  SimpleTableConfigurationPriv;
typedef struct _SimpleTableConfigurationClass SimpleTableConfigurationClass;

struct _SimpleTableConfiguration {
  GObject __parent_instance__;
  SimpleTableConfigurationPriv *priv;
};

struct _SimpleTableConfigurationClass {
  GObjectClass __parent_class__;
  void (*changed) (SimpleTableConfiguration *stc);
};

GType simple_table_configuration_get_type();
const gunichar *simple_table_configuration_get_combining(SimpleTableConfiguration *stc, gunichar c);
gunichar simple_table_configuration_get_trigger(SimpleTableConfiguration *stc);

#define SIMPLE_TABLE_CONFIGURATION_TYPE           simple_table_configuration_get_type()
#define SIMPLE_TABLE_CONFIGURATION(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj), SIMPLE_TABLE_CONFIGURATION_TYPE, SimpleTableConfiguration))
#define SIMPLE_TABLE_CONFIGURATION_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),  SIMPLE_TABLE_CONFIGURATION_TYPE, SimpleTableConfigurationClass)) 
#define IS_SIMPLE_TABLE_CONFIGURATION(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), SIMPLE_TABLE_CONFIGURATION_TYPE))
#define IS_SIMPLE_TABLE_CONFIGURATION_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE((obj),    SIMPLE_TABLE_CONFIGURATION_TYPE))
#define SIMPLE_TABLE_CONFIGURATION_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj),  SIMPLE_TABLE_CONFIGURATION_TYPE, SimpleTableConfigurationClass))

G_END_DECLS

#endif /* !_CONFIG_FILE_H_ */
