# bump micro_version to the next even number for each point release
# bump micro_version to the next odd number after each release
m4_define([gtk_gst_widget_major_version], [0])
m4_define([gtk_gst_widget_minor_version], [1])
m4_define([gtk_gst_widget_micro_version], [1])

m4_define([gtk_gst_widget_version], [gtk_gst_widget_major_version.gtk_gst_widget_minor_version.gtk_gst_widget_micro_version])

m4_define([gtk_gst_widget_release_status],
          [m4_if(m4_eval(gtk_gst_widget_micro_version % 2), [1], [git],
                 [m4_if(m4_eval(gtk_gst_widget_minor_version % 2), [1], [snapshot], [release])])])

# bump up by 1 for every micro release with no API changes, otherwise
# set to 0. after release, bump up by 1
m4_define([gtk_gst_widget_interface_age], [1])
m4_define([gtk_gst_widget_binary_age], [m4_eval(100 * gtk_gst_widget_minor_version + gtk_gst_widget_micro_version)])

m4_define([lt_current], [m4_eval(100 * gtk_gst_widget_minor_version + gtk_gst_widget_micro_version - gtk_gst_widget_interface_age)])
m4_define([lt_revision], [gtk_gst_widget_interface_age])
m4_define([lt_age], [m4_eval(gtk_gst_widget_binary_age - gtk_gst_widget_interface_age)])

m4_define([glib_req_version], [2.30.0])
m4_define([gstreamer_req_version], [1.4.0])
m4_define([gtk_req_version], [3.12.0])
m4_define([gi_req_version], [1.41.0])

AC_PREREQ([2.63])

AC_INIT([gtk_gst_widget],
        [gtk_gst_widget_version],
        [https://github.com/ystreet/gtk_gst_widget/issues],
        [gtk_gst_widget],
        [https://github.com/ystreet/gtk_gst_widget])

AC_CONFIG_HEADER([src/config.h])
AC_CONFIG_SRCDIR([src/gtkgstwidget.h])
AC_CONFIG_AUX_DIR([build])
AC_CONFIG_MACRO_DIR([build/autotools])

# Honor aclocal flags
ACLOCAL="$ACLOCAL $ACLOCAL_FLAGS"

AM_INIT_AUTOMAKE([1.11 no-define foreign -Wno-portability dist-xz no-dist-gzip tar-ustar])

AM_SILENT_RULES([yes])

AC_USE_SYSTEM_EXTENSIONS

GST_API_VERSION=1.0
AC_SUBST(GST_API_VERSION)
AC_DEFINE_UNQUOTED(GST_API_VERSION, "$GST_API_VERSION",
  [GStreamer API Version])

dnl === Base compiler flags ===================================================

BASE_CFLAGS=""

dnl === Libtool ===============================================================

LT_PREREQ([2.2.6])
LT_INIT([disable-static])
LT_LIB_M

dnl === Versioning ============================================================

GTK_GST_WIDGET_MAJOR_VERSION=gtk_gst_widget_major_version
GTK_GST_WIDGET_MINOR_VERSION=gtk_gst_widget_minor_version
GTK_GST_WIDGET_MICRO_VERSION=gtk_gst_widget_micro_version
GTK_GST_WIDGET_VERSION=gtk_gst_widget_version
GTK_GST_WIDGET_RELEASE_STATUS=gtk_gst_widget_release_status
AC_SUBST(GTK_GST_WIDGET_MAJOR_VERSION)
AC_SUBST(GTK_GST_WIDGET_MICRO_VERSION)
AC_SUBST(GTK_GST_WIDGET_MINOR_VERSION)
AC_SUBST(GTK_GST_WIDGET_VERSION)
AC_SUBST(GTK_GST_WIDGET_RELEASE_STATUS)

GTK_GST_WIDGET_LT_CURRENT=lt_current
GTK_GST_WIDGET_LT_REVISION=lt_revision
GTK_GST_WIDGET_LT_AGE=lt_age
GTK_GST_WIDGET_LT_VERSION="$GTK_GST_WIDGET_LT_CURRENT:$GTK_GST_WIDGET_LT_REVISION:$GTK_GST_WIDGET_LT_AGE"
GTK_GST_WIDGET_LT_LDFLAGS="-version-info $GTK_GST_WIDGET_LT_VERSION"

dnl === Dependencies ==========================================================

GTK_GST_WIDGET_REQS="gtk+-3.0 gstreamer-1.0 gstreamer-video-1.0 gstreamer-app-1.0"

PKG_PROG_PKG_CONFIG

dnl === Platform checks =======================================================

platform_linux=no
platform_win32=no

AC_CANONICAL_HOST
AC_MSG_CHECKING([if building for some Win32 platform])
AS_CASE([$host],

        [*-*-mingw*|*-*-cygwin*],
        [
          GTK_GST_WIDGET_LT_LDFLAGS="$GTK_GST_WIDGET_LT_LDFLAGS -no-undefined"
          platform_win32=yes
        ],

        [*-*-linux*],
        [
          platform_linux=yes
        ],

        []
)
AC_MSG_RESULT([$platform_win32])

AM_CONDITIONAL(OS_LINUX, [test "$platform_linux" = "yes"])
AM_CONDITIONAL(OS_WIN32, [test "$platform_win32" = "yes"])

AC_CHECK_FUNCS([aligned_alloc posix_memalign memalign])

# Check for -Bsymbolic-functions to avoid intra-library PLT jumps
AC_ARG_ENABLE([Bsymbolic],
              [AS_HELP_STRING([--disable-Bsymbolic], [Avoid linking with -Bsymbolic])],
              [],
              [
                saved_LDFLAGS="${LDFLAGS}"
                AC_MSG_CHECKING([for -Bsymbolic-functions linker flag])
                LDFLAGS=-Wl,-Bsymbolic-functions
                AC_TRY_LINK([], [int main (void) { return 0; }],
                            [
                              AC_MSG_RESULT([yes])
                              enable_Bsymbolic=yes
                            ],
                            [
                              AC_MSG_RESULT([no])
                              enable_Bsymbolic=no
                            ])
                LDFLAGS="${saved_LDFLAGS}"
              ])

AS_IF([test "x$enable_Bsymbolic" = "xyes"], [GTK_GST_WIDGET_LINK_FLAGS=-Wl[,]-Bsymbolic-functions])

dnl === Enable debug level ====================================================

m4_define([debug_default], [m4_if(m4_eval(gtk_gst_widget_minor_version % 2), [1], [yes], [minimum])])

AC_ARG_ENABLE([debug],
              [AS_HELP_STRING([--enable-debug=@<:@no/minimum/yes@:>@],
                              [turn on debugging @<:@default=]debug_default[@:>@])],
              [],
              [enable_debug=debug_default])

AS_CASE([$enable_debug],
        [yes],
        [
          BASE_CFLAGS="-g $BASE_CFLAGS"
          GTK_GST_WIDGET_DEBUG_CFLAGS="-DGTK_GST_WIDGET_ENABLE_DEBUG"
        ],

        [minimum],
        [
          GTK_GST_WIDGET_DEBUG_CFLAGS="-DGTK_GST_WIDGET_ENABLE_DEBUG -DG_DISABLE_CAST_CHECKS"
        ],

        [no],
        [
          GTK_GST_WIDGET_DEBUG_CFLAGS="-DG_DISABLE_ASSERT -DG_DISABLE_CHECKS -DG_DISABLE_CAST_CHECKS"
        ],

        [AC_MSG_ERROR([Unknown argument to --enable-debug])]
)

AC_SUBST(GTK_GST_WIDGET_DEBUG_CFLAGS)

dnl === GLib ==================================================================

PKG_CHECK_MODULES(GLIB, [glib-2.0 >= glib_req_version],
                        [HAVE_GLIB=yes],
                        [HAVE_GLIB=no])

AS_IF([test "x$HAVE_GLIB" = xno],
      [AC_MSG_ERROR([GLib >= $glib_req_version is required])])

AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

dnl === GStreamer =============================================================

PKG_CHECK_MODULES(GSTREAMER, [gstreamer-1.0 >= gstreamer_req_version gstreamer-video-1.0 >= gstreamer_req_version gstreamer-app-1.0 >= gstreamer_req_version],
                        [HAVE_GSTREAMER=yes],
                        [HAVE_GSTREAMER=no])

AS_IF([test "x$HAVE_GSTREAMER" = xno],
      [AC_MSG_ERROR([GStreamer >= $gstreamer_req_version is required])])

AC_SUBST(GSTREAMER_CFLAGS)
AC_SUBST(GSTREAMER_LIBS)

dnl === Gtk ===================================================================

PKG_CHECK_MODULES(GTK, [gtk+-3.0 >= gtk_req_version],
                        [HAVE_GTK=yes],
                        [HAVE_GTK=no])

AS_IF([test "x$HAVE_GTK" = xno],
      [AC_MSG_ERROR([GTK+ >= $gtk_req_version is required])])

AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

dnl === Gtk GL ================================================================

PKG_CHECK_MODULES(GTK_GL, [gtk+-3.0 >= 3.15.0],
                          [HAVE_GTK_GL=yes],
                          [HAVE_GTK_GL=no])

AC_SUBST(GTK_GL_CFLAGS)
AC_SUBST(GTK_GL_LIBS)

dnl === GStreamer GL ==========================================================

PKG_CHECK_MODULES(GST_GL, [gstreamer-gl-1.0 >= 1.5.0],
                          [HAVE_GST_GL=yes],
                          [HAVE_GST_GL=no])

AC_SUBST(GST_GL_CFLAGS)
AC_SUBST(GST_GL_LIBS)

AM_CONDITIONAL(ENABLE_GL, [test "x$HAVE_GTK_GL" = xyes -a "x$HAVE_GST_GL" = xyes])

GTK_GST_HAVE_GL=0
if test "x$HAVE_GTK_GL" = xyes -a "x$HAVE_GST_GL" = xyes; then
  AC_DEFINE(HAVE_GL, 1, [Whether you have OpenGL])
  GTK_GST_HAVE_GL=1
fi

dnl === Platform specifics - x11 ==============================================

PKG_CHECK_MODULES(GTK_X11, [gtk+-x11-3.0 >= gtk_req_version],
                           [HAVE_GTK_X11=yes],
                           [HAVE_GTK_X11=no])

HAVE_GST_GL_X11=no
save_CFLAGS="$CFLAGS"
CFLAGS="$GST_GL_CFLAGS"
AC_MSG_CHECKING([for GStreamer GL library x11 support])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <gst/gl/gl.h>]], [[
        #if !GST_GL_HAVE_WINDOW_X11
        #error unsupported
        #endif
    ]])], [
      AC_MSG_RESULT(yes)
      HAVE_GST_GL_X11=yes
    ], [
      AC_MSG_RESULT(no)
    ])
CFLAGS="$save_CFLAGS"

GTK_GST_HAVE_X11=0
if test "x$HAVE_GTK_X11" = xyes -a "x$HAVE_GST_GL_X11" = xyes; then
  GTK_GST_HAVE_X11=1
fi

dnl === Library Configuration =================================================

gtk_gst_config_defines="
#define GTK_GST_HAVE_GL $GTK_GST_HAVE_GL
#define GTK_GST_HAVE_X11 $GTK_GST_HAVE_X11
"

AC_CONFIG_COMMANDS([src/gtkgst-config.h], [
	outfile=gtkgst-config.h-tmp
	cat > $outfile <<\_______EOF
/* gtkgst-config.h
 *
 * This is a generated file.  Please modify `configure.ac'
 */

#ifndef __GTK_GST_CONFIG_H__
#define __GTK_GST_CONFIG_H__

#include <gst/gst.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

_______EOF

	cat >>$outfile <<_______EOF
$config_defines
_______EOF

	cat >>$outfile <<_______EOF

G_END_DECLS

#endif  /* __GTK_GST_CONFIG_H__ */
_______EOF


	if cmp -s $outfile src/gtkgst-config.h; then
          AC_MSG_NOTICE([src/gtkgst-config.h is unchanged])
	  rm -f $outfile
	else
	  mv $outfile src/gtkgst-config.h
	fi
],[
config_defines='$gtk_gst_config_defines'
])

dnl === GObject introspection =================================================

GOBJECT_INTROSPECTION_CHECK([gi_req_version])

dnl === Test suite ============================================================

AC_ARG_ENABLE([tests],
              [AS_HELP_STRING([--enable-tests], [Enable test suite])],
              [enable_tests=$enableval],
              [enable_tests=yes])

dnl === Code coverage =========================================================

AC_ARG_ENABLE([code-coverage],
              [AS_HELP_STRING([--enable-code-coverage], [Enable test suite code coverage])],
              [use_gcov=$enableval],
              [use_gcov=no])

dnl Disable code coverage if tests were disabled
AS_IF([test "x$build_tests" = disabled ], [ use_gcov=disabled ])

AS_IF([test "x$use_gcov" = "xyes"],
      [
        dnl we need a test suite
        AS_IF([test "x$build_tests" = xno],
              [AC_MSG_ERROR([Cannot enable code coverage when the test suite is disabled])])

        dnl we need gcc
        AS_IF([test "$GCC" != "yes"], [AC_MSG_ERROR([GCC is required for --enable-code-coverage])])

        dnl Check if ccache is being used
        AC_CHECK_PROG(SHTOOL, shtool, shtool)
        AS_CASE([`$SHTOOL path $CC`],
                [*ccache*], [gcc_ccache=yes],
                [gcc_ccache=no])

        if test "$gcc_ccache" = "yes" && (test -z "$CCACHE_DISABLE" || test "$CCACHE_DISABLE" != "1"); then
          AC_MSG_ERROR([ccache must be disabled when --enable-code-coverage option is used. You can disable ccache by setting environment variable CCACHE_DISABLE=1.])
        fi

        ltp_version_list="1.6 1.7 1.8 1.9 1.10"
        AC_CHECK_PROG(LTP, lcov, lcov)
        AC_CHECK_PROG(LTP_GENHTML, genhtml, genhtml)

        if test "$LTP"; then
          AC_CACHE_CHECK([for ltp version], gtk_gst_widget_cv_ltp_version,
                         [
                           gtk_gst_widget_cv_ltp_version=invalid
                           ltp_version=`$LTP -v 2>/dev/null | $SED -e 's/^.* //'`
                           for ltp_check_version in $ltp_version_list; do
                             if test "$ltp_version" = "$ltp_check_version"; then
                               gtk_gst_widget_cv_ltp_version="$ltp_check_version (ok)"
                             fi
                           done
                         ])
        else
          ltp_msg="To enable code coverage reporting you must have one of the following LTP versions installed: $ltp_version_list"
          AC_MSG_ERROR([$ltp_msg])
        fi

        case $gtk_gst_widget_cv_ltp_version in
          ""|invalid[)]
          ltp_msg="You must have one of the following versions of LTP: $ltp_version_list (found: $ltp_version)."
          AC_MSG_ERROR([$ltp_msg])
          LTP="exit 0;"
          ;;
        esac

        if test -z "$LTP_GENHTML"; then
          AC_MSG_ERROR([Could not find genhtml from the LTP package])
        fi

        AC_DEFINE(HAVE_GCOV, 1, [Whether you have gcov])

        dnl Define the special gcc flags
        GTK_GST_WIDGET_GCOV_CFLAGS="-O0 -fprofile-arcs -ftest-coverage"
        GTK_GST_WIDGET_GCOV_LDADD="-lgcov"
      ])

AM_CONDITIONAL(ENABLE_GCOV, [test "x$use_gcov" = xyes])

dnl === API Reference =========================================================

GLIB_PREFIX="`$PKG_CONFIG --variable=prefix glib-2.0`"
GTK_PREFIX="`$PKG_CONFIG --variable=prefix gtk+-3.0`"
GST_PREFIX="`$PKG_CONFIG --variable=prefix gstreamer-1.0`"
GSTPB_PREFIX="`$PKG_CONFIG --variable=prefix gstreamer-plugins-base-1.0`"
AC_SUBST(GLIB_PREFIX)
AC_SUBST(GTK_PREFIX)
AC_SUBST(GST_PREFIX)
AC_SUBST(GSTPB_PREFIX)

GTK_DOC_CHECK([1.20], [--flavour no-tmpl])

dnl === GStreamer plugin ======================================================

AG_GST_SET_PLUGINDIR

dnl === Output ================================================================

GTK_GST_WIDGET_CFLAGS="$BASE_CFLAGS $GTK_GST_WIDGET_VISIBILITY_CFLAGS $GTK_GST_WIDGET_GCOV_CFLAGS $GTK_CFLAGS $GSTREAMER_CFLAGS $GST_GL_CFLAGS $GTK_GL_CFLAGS"
GTK_GST_WIDGET_LIBS="$LIBM $GTK_GST_WIDGET_GCOV_LDADD $GTK_LIBS $GSTREAMER_LIBS $GST_GL_LIBS $GTK_GL_LIBS"
GTK_GST_WIDGET_LT_LDFLAGS="$GTK_GST_WIDGET_LT_LDFLAGS $GTK_GST_WIDGET_LINK_FLAGS -no-undefined -export-symbols-regex ^_?\(gtk_gst_\|GtkGst\|GTK_GST_\).*"
GTK_GST_PLUGIN_LDFLAGS="-no-undefined -module -avoid-version -export-symbols-regex '^[_]*gst_plugin_.*'"
AC_SUBST(GTK_GST_WIDGET_CFLAGS)
AC_SUBST(GTK_GST_WIDGET_LIBS)
AC_SUBST(GTK_GST_WIDGET_LT_LDFLAGS)
AC_SUBST(GTK_GST_PLUGIN_LDFLAGS)
AC_SUBST(GTK_GST_WIDGET_REQS)

AC_CONFIG_FILES([
        Makefile

        src/Makefile
        src/gtkgst.pc
        src/gtkgst-version.h

        tests/Makefile
        tests/examples/Makefile
        docs/Makefile
])

AC_OUTPUT
