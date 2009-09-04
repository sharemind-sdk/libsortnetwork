CC = gcc
CFLAGS = -Wall -Werror -std=c99 -O3 -pthread
#CFLAGS = -Wall -Werror -std=c99 -O0 -g -pthread

APPLICATIONS = sn-apply sn-batcher sn-check-bf sn-cut \
	       sn-evolution sn-evolution2 sn-find-9 sn-merge \
	       sn-normalize sn-oddevenmerge sn-shmoo sn-show sn-tex

POPULATION_CFLAGS = -I/tmp/libpopulation/include

POPULATION_LDFLAGS = -L/tmp/libpopulation/lib -Wl,--rpath -Wl,/tmp/libpopulation/lib -lpopulation

all: $(APPLICATIONS)

clean:
	rm -f *.o
	rm -f $(APPLICATIONS)

sn-batcher: sn-batcher.c sn_network.o sn_stage.o sn_comparator.o sn_random.o

sn_comparator.o: sn_comparator.c sn_comparator.h

sn_network.o: sn_network.c sn_network.h sn_stage.h sn_comparator.h sn_random.h

sn_population.o: CFLAGS = $(POPULATION_CFLAGS)
sn_population.o: sn_population.c sn_population.h sn_network.h sn_random.h

sn_random.o: sn_random.c sn_random.h

sn_stage.o: sn_stage.c sn_stage.h sn_comparator.h

pop_stats.o: pop_stats.c pop_stats.h

sn-apply: sn-apply.c sn_network.o sn_stage.o sn_comparator.o sn_random.o

sn-check-bf: sn-check-bf.c sn_network.o sn_stage.o sn_comparator.o sn_random.o

sn-cut: sn-cut.c sn_network.o sn_stage.o sn_comparator.o sn_random.o

sn-evolution: CFLAGS += $(POPULATION_CFLAGS)
sn-evolution: LDFLAGS += $(POPULATION_LDFLAGS)
sn-evolution: sn-evolution.c sn_network.o sn_stage.o sn_comparator.o sn_population.o sn_random.o pop_stats.o

sn-evolution2: CFLAGS += $(POPULATION_CFLAGS)
sn-evolution2: LDFLAGS += $(POPULATION_LDFLAGS) -lm
sn-evolution2: sn-evolution2.c sn_network.o sn_stage.o sn_comparator.o sn_population.o sn_random.o

sn-find-9: sn-find-9.c sn_network.o sn_stage.o sn_comparator.o sn_random.o

sn-merge: sn-merge.c sn_network.o sn_stage.o sn_comparator.o sn_random.o

sn-normalize: sn-normalize.c sn_network.o sn_stage.o sn_comparator.o sn_random.o

sn-oddevenmerge: sn-oddevenmerge.c sn_network.o sn_stage.o sn_comparator.o sn_random.o

sn-shmoo: sn-shmoo.c sn_network.o sn_stage.o sn_comparator.o sn_random.o

sn-show: sn-show.c sn_network.o sn_stage.o sn_comparator.o sn_random.o

sn-tex: sn-tex.c sn_network.o sn_stage.o sn_comparator.o sn_random.o

.PHONY: clean

