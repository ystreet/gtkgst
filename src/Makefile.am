# preamble
NULL =
BUILT_SOURCES =
CLEANFILES =
EXTRA_DIST =
DISTCLEANFILES =
lib_LTLIBRARIES =

# source
source_h = \
	gtkgstwidget.h \
	gtkgst.h \
	$(NULL)
source_c = \
	gtkgstwidget.c \
	$(NULL)

source_h_priv =
source_c_priv =

# version header
DISTCLEANFILES += gtkgst-version.h gtkgst-config.h
EXTRA_DIST += gtkgst-version.h.in

shared_cflags = $(GTK_GST_WIDGET_CFLAGS) $(MAINTAINER_CFLAGS)
shared_libadd = $(GTK_GST_WIDGET_LIBS)

# main library
libgtkgst_1_0_la_CPPFLAGS = \
	-DGTK_GST_WIDGET_COMPILATION=1 \
	-DG_LOG_DOMAIN=\"GtkGst\" \
	-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_30 \
	-DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_32 \
	-I$(top_srcdir)/src \
	-I$(top_builddir)/src \
	$(GTK_GST_WIDGET_DEBUG_CFLAGS)
libgtkgst_1_0_la_CFLAGS = $(shared_cflags)
libgtkgst_1_0_la_LIBADD = $(shared_libadd)
libgtkgst_1_0_la_LDFLAGS = $(GTK_GST_WIDGET_LT_LDFLAGS) -export-dynamic
libgtkgst_1_0_la_SOURCES = $(source_h) $(source_h_priv) $(source_c) $(source_c_priv) $(BUILT_SOURCES)

if ENABLE_GL
libgtkgst_1_0_la_SOURCES += gtkgstglwidget.c gtkgstglwidget.h
endif

lib_LTLIBRARIES += libgtkgst-1.0.la

# headers
gtkgstincludedir = $(includedir)/gtkgst-1.0
gtkgstinclude_HEADERS = $(source_h)
nodist_gtkgstinclude_HEADERS = \
	$(builddir)/gtkgst-version.h \
	$(NULL)

# pkg-config files
gtkgst-1.0.pc: gtkgst.pc
	$(AM_V_GEN)cp -f $< $@

pkgconfigdir = $(libdir)/pkgconfig

pkgconfig_DATA = gtkgst-1.0.pc
CLEANFILES += gtkgst-1.0.pc
EXTRA_DIST += gtkgst.pc.in

# gst plugin
libgstgtksink_la_CPPFLAGS = \
	-I$(top_srcdir)/src \
	-I$(top_builddir)/src \
	$(GTK_GST_WIDGET_DEBUG_CFLAGS)
libgstgtksink_la_CFLAGS = $(shared_cflags)
libgstgtksink_la_LIBADD = $(builddir)/libgtkgst-1.0.la $(shared_libadd)
libgstgtksink_la_LDFLAGS = $(GTK_GST_PLUGIN_LDFLAGS)
libgstgtksink_la_SOURCES = gstgtksink.h gstgtksink.c gstplugin.c

if ENABLE_GL
libgstgtksink_la_SOURCES += gstgtkglsink.c gstgtkglsink.h
endif

plugin_LTLIBRARIES = libgstgtksink.la

-include $(INTROSPECTION_MAKEFILE)

if HAVE_INTROSPECTION

INTROSPECTION_GIRS = GtkGst-1.0.gir

introspection_source_h = $(source_h)
introspection_source_c = $(source_c)

GtkGst-1.0.gir: libgtkgst-1.0.la Makefile

GtkGst_1_0_gir_VERSION = 1.0
GtkGst_1_0_gir_LIBS = libgtkgst-1.0.la
GtkGst_1_0_gir_SCANNERFLAGS = \
	    --identifier-filter-cmd="$(SED) -e 's|\(.*\)_t\b|\1|' -e 's|^\([a-z]\)|\u\1|' -e 's|_\([a-z]\)|\u\1|g' -e 's|\([0-9]\)\([a-z]\)|\1\u\2|g'" \
	    --accept-unprefixed \
	    --symbol-prefix=gtk_gst \
            --c-include="gtkgst.h" \
	    --warn-all \
	    --pkg-export gtkgst-1.0

GtkGst_1_0_gir_PACKAGES =
GtkGst_1_0_gir_INCLUDES = GObject-2.0 Gtk-3.0 Gst-1.0
GtkGst_1_0_gir_CFLAGS = -DGTK_GST_WIDGET_COMPILATION -I$(top_srcdir)/src -I$(top_builddir)/src
GtkGst_1_0_gir_FILES = $(introspection_source_h) $(introspection_source_c)

girdir = $(datadir)/gir-1.0
nodist_gir_DATA = $(INTROSPECTION_GIRS)
CLEANFILES += $(nodist_gir_DATA)

typelibdir = $(libdir)/girepository-1.0
nodist_typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)
CLEANFILES += $(nodist_typelib_DATA)

endif # HAVE_INTROSPECTION
