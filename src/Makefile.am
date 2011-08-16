# vim:set noet ts=4:
#
# ibus-anthy - The Anthy engine for IBus
#
# Copyright (c) 2007-2008 Huang Peng <shawn.p.huang@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


libexec_PROGRAMS = ibus-engine-enchant

ibus_engine_enchant_SOURCES = \
	main.c \
	engine.c \
	engine.h \
	config-file.c \
	config-file.h \
	debug.c \
	debug.h \
	$(NULL)
ibus_engine_enchant_CFLAGS = \
	@IBUS_CFLAGS@ \
	-DPKGDATADIR=\"$(pkgdatadir)\"
	$(NULL)
ibus_engine_enchant_LDFLAGS = \
	@IBUS_LIBS@ \
	$(NULL)

component_DATA = \
	enchant.xml \
	$(NULL)
componentdir = @datadir@/ibus/component

pkgdata_DATA = config.txt

gsettings_SCHEMAS = ca.go-nix.IBusSimpleTable.gschema.xml
@GSETTINGS_RULES@

EXTRA_DIST = \
	ca.go-nix.IBusSimpleTable.gschema.xml.in \
	config.txt \
	enchant.xml.in \
	$(NULL)

CLEANFILES = \
	enchant.xml \
	$(NULL)

enchant.xml: enchant.xml.in
	$(AM_V_GEN) \
	( \
		libexecdir=${libexecdir}; \
		pkgdatadir=${pkgdatadir}; \
		s=`cat $<`; \
		eval "echo \"$${s}\""; \
	) > $@

ca.go-nix.IBusSimpleTable.gschema.xml: ca.go-nix.IBusSimpleTable.gschema.xml.in
	cat $< | sed 's!@DEFAULT_CONFIG_FILE@!$(pkgdatadir)/config.txt!g' > $@

test: ibus-engine-enchant
	$(builddir)/ibus-engine-enchant
