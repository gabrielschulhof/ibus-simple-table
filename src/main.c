/* vim:set et sts=4: */

#include <ibus.h>
#include <gtk/gtk.h>
#include "engine.h"

static IBusBus *bus = NULL;
static IBusFactory *factory = NULL;

static void
ibus_disconnected_cb (IBusBus  *bus,
                      gpointer  user_data)
{
    ibus_quit ();
}


static void
init (void)
{
    IBusComponent *component;

    ibus_init ();

    bus = ibus_bus_new ();
    g_object_ref_sink (bus);
    g_signal_connect (bus, "disconnected", G_CALLBACK (ibus_disconnected_cb), NULL);
	
    factory = ibus_factory_new (ibus_bus_get_connection (bus));
    g_object_ref_sink (factory);
    ibus_factory_add_engine (factory, "simple-table", IBUS_TYPE_SIMPLE_TABLE_ENGINE);

    ibus_bus_request_name (bus, "org.freedesktop.IBus.SimpleTableEngine", 0);
}

int
main(int argc, char **argv)
{
    gtk_init(&argc, &argv);
    init ();
    ibus_main ();
}
