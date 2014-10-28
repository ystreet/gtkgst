DISTCHECK_CONFIGURE_FLAGS=--enable-gtk-doc --enable-introspection

SUBDIRS = \
	src docs

DIST_SUBDIRS = \
	src docs

EXTRA_DIST = \
	AUTHORS COPYING NEWS README RELEASE REQUIREMENTS \
	ChangeLog autogen.sh

DISTCLEANFILES =

ACLOCAL_AMFLAGS = -I build/autotools

check-valgrind:
	cd tests/check && make check-valgrind

dist-hook:
	@if test -d "$(srcdir)/.git"; then \
	  echo Generating ChangeLog... ; \
	  ( cd "$(srcdir)" \
	    && $(top_srcdir)/build/missing --run git log --stat ) > ChangeLog.tmp \
	  && mv -f ChangeLog.tmp $(top_distdir)/ChangeLog \
	  || ( rm -f ChangeLog.tmp; \
	       echo Failed to generate ChangeLog >&2 ); \
	else \
	  echo A git checkout is required to generate a ChangeLog >&2; \
	fi

if ENABLE_GCOV
# use recursive makes in order to ignore errors during check/perf
lcov:
	-$(MAKE) $(AM_MAKEFLAGS) check
	$(MAKE) $(AM_MAKEFLAGS) genlcov

# we have to massage the lcov.info file slightly to hide the effect of libtool
# placing the objects files in the .libs/ directory separate from the *.c
genlcov:
	$(LTP) --directory $(top_builddir) --capture --output-file gtkgstwidget-lcov.info --test-name GTK_GST_WIDGET_TEST --no-checksum
	$(SED) -e 's#.libs/##' < gtkgstwidget-lcov.info > gtkgstwidget-lcov.info.tmp
	LANG=C $(LTP_GENHTML) --prefix $(top_builddir) --output-directory graphene-lcov --title "GtkGstWidget Code Coverage" --show-details gtkgstwidget-lcov.info.tmp
	rm -f gtkgstwidget-lcov.info.tmp

lcov-clean:
	-$(LTP) --directory $(top_builddir) -z
	-$(RM) -rf gtkgstwidget-lcov.info gtkgstwidget-lcov
else
lcov genlcov lcov-clean:
	@echo You need to configure Graphene with --enable-code-coverage
endif

.PHONY: lcov genlcov lcov-clean
