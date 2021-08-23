##
## Our Makefile.am template for Makefile.in (and eventually, Makefile)
##
## Process this with automake to generate Makefile.in (or better yet,
## use autoreconf)
##

AM_CFLAGS = -Wall

bin_SCRIPTS = cac4teams
CLEANFILES = $(bin_SCRIPTS)

EXTRA_DIST = src/cac4teams.in

lib_LTLIBRARIES = libcac4teams.la

libcac4teams_la_SOURCES = \
		src/cac4teams.c \
		#

libcac4teams_la_LDFLAGS = \
		-shared \
		-avoid-version \
		-framework Security \
		#

do_subst = ( sed \
	-e 's,[@]libdir[@],$(libdir),g' \
	)

cac4teams: $(srcdir)/src/cac4teams.in Makefile
	rm -f $@ $@-t
	$(do_subst) < $(srcdir)/src/$@.in > $@-t
	chmod +x $@-t
	mv -f $@-t $@
