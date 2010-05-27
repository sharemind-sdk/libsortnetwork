include_HEADERS = sn_network.h sn_stage.h sn_comparator.h

lib_LTLIBRARIES = libsortnetwork.la

bin_PROGRAMS = sn-apply sn-batcher sn-check-bf sn-cut sn-info sn-merge \
	       sn-normalize sn-oddevenmerge sn-shmoo sn-show sn-svg sn-tex

libsortnetwork_la_SOURCES = sn_network.c sn_network.h       \
			    sn_stage.c sn_stage.h           \
			    sn_comparator.c sn_comparator.h \
			    sn_random.c sn_random.h
libsortnetwork_la_LDFLAGS = -version-info 0:0:0

sn_apply_SOURCES = sn-apply.c
sn_apply_LDADD = libsortnetwork.la

sn_batcher_SOURCES = sn-batcher.c
sn_batcher_LDADD = libsortnetwork.la

sn_check_bf_SOURCES = sn-check-bf.c
sn_check_bf_LDADD = libsortnetwork.la

sn_cut_SOURCES = sn-cut.c
sn_cut_LDADD = libsortnetwork.la

sn_info_SOURCES = sn-info.c
sn_info_LDADD = libsortnetwork.la

sn_merge_SOURCES = sn-merge.c
sn_merge_LDADD = libsortnetwork.la

sn_normalize_SOURCES = sn-normalize.c
sn_normalize_LDADD = libsortnetwork.la

sn_oddevenmerge_SOURCES = sn-oddevenmerge.c
sn_oddevenmerge_LDADD = libsortnetwork.la

sn_shmoo_SOURCES = sn-shmoo.c
sn_shmoo_LDADD = libsortnetwork.la

sn_show_SOURCES = sn-show.c
sn_show_LDADD = libsortnetwork.la

sn_svg_SOURCES = sn-svg.c
sn_svg_LDADD = libsortnetwork.la

sn_tex_SOURCES = sn-tex.c
sn_tex_LDADD = libsortnetwork.la

if BUILD_WITH_LIBPOPULATION
bin_PROGRAMS += sn-evolution sn-evolution2 sn-evolution-cut

sn_evolution_SOURCES = sn-evolution.c
sn_evolution_CPPFLAGS = $(AM_CPPFLAGS) $(LIBPOPULATION_CPPFLAGS)
sn_evolution_LDFLAGS = $(AM_LDFLAGS) $(LIBPOPULATION_LDFLAGS)
sn_evolution_LDADD = libsortnetwork.la $(LIBPOPULATION_LIBS)

sn_evolution2_SOURCES = sn-evolution2.c
sn_evolution2_CPPFLAGS = $(AM_CPPFLAGS) $(LIBPOPULATION_CPPFLAGS)
sn_evolution2_LDFLAGS = $(AM_LDFLAGS) $(LIBPOPULATION_LDFLAGS)
sn_evolution2_LDADD = libsortnetwork.la $(LIBPOPULATION_LIBS)

sn_evolution_cut_SOURCES = sn-evolution-cut.c
sn_evolution_cut_CPPFLAGS = $(AM_CPPFLAGS) $(LIBPOPULATION_CPPFLAGS)
sn_evolution_cut_LDFLAGS = $(AM_LDFLAGS) $(LIBPOPULATION_LDFLAGS)
sn_evolution_cut_LDADD = libsortnetwork.la $(LIBPOPULATION_LIBS)
endif
