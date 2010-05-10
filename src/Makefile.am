include_HEADERS = sn_network.h sn_stage.h sn_comparator.h

lib_LTLIBRARIES = libsortnetwork.la

bin_PROGRAMS = sn-show

libsortnetwork_la_SOURCES = sn_network.c sn_network.h       \
			    sn_stage.c sn_stage.h           \
			    sn_comparator.c sn_comparator.h \
			    sn_random.c sn_random.h
libsortnetwork_la_LDFLAGS = -version-info 0:0:0

sn_show_SOURCES = sn-show.c
sn_show_LDADD = libsortnetwork.la
