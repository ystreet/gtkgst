DOC_MODULE = gtkgstwidget
DOC_MAIN_SGML_FILE = gtkgstwidget-docs.xml
DOC_SOURCE_DIR = $(top_srcdir)/src

SCAN_OPTIONS = --rebuild-types
SCANGOBJ_OPTIONS =
MKDB_OPTIONS = --output-format=xml --name-space=gtkgst
FIXXREF_OPTIONS =

HFILE_GLOB = $(top_srcdir)/src/*.h $(top_builddir)/src/*.h
CFILE_GLOB = $(top_srcdir)/src/*.c

IGNORE_HFILES = \
	$(top_builddir)/src/config.h \
	$(top_builddir)/src/gtkgst-config.h \
	$(top_builddir)/src/gtkgst-version.h \
	$(top_builddir)/src/gstgtksink.h \
	$(top_builddir)/src/gstgtkglsink.h \
	$(top_srcdir)/src/gstgtksink.h \
	$(top_srcdir)/src/gstgtkglsink.h

EXTRA_HFILES =

HTML_IMAGES =

content_files =
expand_content_files = 

API_VERSION = $(GTK_GST_WIDGET_VERSION).$(GTK_GST_WIDGET_MINOR_VERSION)

AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src $(GTK_GST_WIDGET_CFLAGS)
GTKDOC_CFLAGS = $(GTK_GST_WIDGET_CFLAGS)
GTKDOC_LIBS = $(top_builddir)/src/libgtkgst-1.0.la \
	$(GTK_GST_WIDGET_LIBS)

include $(top_srcdir)/gtk-doc.make
