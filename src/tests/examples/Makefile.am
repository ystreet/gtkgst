
noinst_PROGRAMS = gstwidget gtksink

gstwidget_SOURCES = gstwidget.c
gstwidget_CFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src \
	$(GTK_GST_WIDGET_CFLAGS)
gstwidget_LDADD = $(top_builddir)/src/libgtkgst-1.0.la

gtksink_SOURCES = gtksink.c
gtksink_CFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src \
	$(GTK_GST_WIDGET_CFLAGS)
gtksink_LDADD = $(top_builddir)/src/libgtkgst-1.0.la

if ENABLE_GL
noinst_PROGRAMS += gstglwidget gtkglsink

gstglwidget_SOURCES = gstglwidget.c
gstglwidget_CFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src \
	$(GTK_GST_WIDGET_CFLAGS)
gstglwidget_LDADD = $(top_builddir)/src/libgtkgst-1.0.la

gtkglsink_SOURCES = gtkglsink.c
gtkglsink_CFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src \
	$(GTK_GST_WIDGET_CFLAGS)
gtkglsink_LDADD = $(top_builddir)/src/libgtkgst-1.0.la
endif
