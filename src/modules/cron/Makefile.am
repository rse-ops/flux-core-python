AM_CFLAGS = \
	$(WARNING_CFLAGS) \
	$(CODE_COVERAGE_CFLAGS)

AM_LDFLAGS = \
	$(CODE_COVERAGE_LIBS) \
	$(JANSSON_CFLAGS)

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src/include \
	-I$(top_builddir)/src/common/libflux

fluxmod_LTLIBRARIES = cron.la

cron_la_SOURCES = \
	task.h \
	task.c \
	entry.h \
	types.h \
	types.c \
	interval.c \
	event.c \
	datetime.c \
	cron.c

cron_la_LDFLAGS = $(fluxmod_ldflags) -module
cron_la_LIBADD = $(top_builddir)/src/common/libflux-internal.la \
		 $(top_builddir)/src/common/libflux-core.la \
		 $(JANSSON_LIBS)
