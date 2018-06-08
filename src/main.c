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

#include <ibus.h>
#include <gtk/gtk.h>
#include "engine.h"

static IBusBus *bus = NULL;
static IBusFactory *factory = NULL;

/* Command line */
static gboolean ibus = FALSE;

static const GOptionEntry cmdline_entries[] = 
{
  { "ibus", 'i', 0, G_OPTION_ARG_NONE, &ibus, "component is executed by ibus", NULL },
  { NULL }
};

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

  if (ibus)
    ibus_bus_request_name (bus, "org.freedesktop.IBus.SimpleTableEngine", 0);
  else {
    IBusComponent *component;

    component = ibus_component_new ("org.freedesktop.IBus.Enchant",
                                   "Simple Table",
                                   "0.1.0",
                                   "GPL",
                                   "Gabriel Schulhof <gabrielschulhof@gmail.com>",
                                   "http://code.google.com/p/ibus/",
                                   "",
                                   "simple-table");

    ibus_component_add_engine (component,
      ibus_engine_desc_new ("simple-table",
                           "Simple Table",
                           "Simple Table",
                           "",
                           "GPL",
                           "Gabriel Schulhof <gabrielschulhof@gmail.com>",
                           PKGDATADIR"/icons/ibus-engine-simple-table.svg",
                           "us"));
    ibus_bus_register_component (bus, component);
  }
}

int
main(int argc, char **argv)
{
  GError *error = NULL;
  GOptionContext *context;

  /* Parse command line */
  context = g_option_context_new("- ibus simple table engine");
  g_option_context_add_main_entries(context, cmdline_entries, "simple-table");

  if (!g_option_context_parse(context, &argc, &argv, &error)) {
    g_error("Option parsing failed: %s\n", error->message);
    g_error_free(error);
    return -1;
  }

  gtk_init(&argc, &argv);
  init ();
  ibus_main ();
}
