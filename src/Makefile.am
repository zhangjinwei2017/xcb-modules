lib_LTLIBRARIES = app_impv.la app_impvbaw.la app_greeks.la app_greeksbaw.la    \
app_quote.la
app_impv_la_SOURCES = norms.c bs.c impv.c app_impv.c
app_impv_la_CPPFLAGS = -I$(top_srcdir)/include
app_impv_la_CFLAGS = -fPIC -Wall
app_impv_la_LDFLAGS = -module -avoid-version -ltcmalloc
app_impvbaw_la_SOURCES = norms.c bs.c baw.c impv.c app_impvbaw.c
app_impvbaw_la_CPPFLAGS = -I$(top_srcdir)/include
app_impvbaw_la_CFLAGS = -fPIC -Wall
app_impvbaw_la_LDFLAGS = -module -avoid-version -ltcmalloc
app_greeks_la_SOURCES = norms.c bs.c app_greeks.c
app_greeks_la_CPPFLAGS = -I$(top_srcdir)/include
app_greeks_la_CFLAGS = -fPIC -Wall
app_greeks_la_LDFLAGS = -module -avoid-version -ltcmalloc
app_greeksbaw_la_SOURCES = norms.c bs.c baw.c app_greeksbaw.c
app_greeksbaw_la_CPPFLAGS = -I$(top_srcdir)/include
app_greeksbaw_la_CFLAGS = -fPIC -Wall
app_greeksbaw_la_LDFLAGS = -module -avoid-version -ltcmalloc
app_quote_la_SOURCES = app_quote.c
app_quote_la_CPPFLAGS = -I$(top_srcdir)/include
app_quote_la_CFLAGS = -fPIC -Wall
app_quote_la_LDFLAGS = -module -avoid-version -ltcmalloc

