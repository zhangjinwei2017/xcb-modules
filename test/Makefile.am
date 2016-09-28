noinst_PROGRAMS = test-amer
test_amer_SOURCES = $(top_srcdir)/src/mem.c $(top_srcdir)/src/utils.c          \
$(top_srcdir)/src/brent.c $(top_srcdir)/src/norms.c $(top_srcdir)/src/bs.c     \
$(top_srcdir)/src/baw.c $(top_srcdir)/src/binomial.c                           \
$(top_srcdir)/src/trinomial.c $(top_srcdir)/src/fd.c $(top_srcdir)/src/lsm.c   \
test-amer.c
test_amer_CPPFLAGS = -I$(top_srcdir)/include
test_amer_CFLAGS = -Wall
test_amer_LDFLAGS = -lm -lgsl -lgslcblas

