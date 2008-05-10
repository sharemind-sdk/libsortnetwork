CC = gcc
CFLAGS = -Wall -Werror -std=c99 -O3 -pthread
#CFLAGS = -Wall -Werror -std=c99 -O0 -g -pthread

APPLICATIONS = sn-apply sn-check-bf sn-cut sn-evolution sn-merge \
	       sn-normalize sn-show sn-tex

all: $(APPLICATIONS)

clean:
	rm -f *.o
	rm -f $(APPLICATIONS)

sn_comparator.o: sn_comparator.c sn_comparator.h

sn_network.o: sn_network.c sn_network.h sn_stage.h sn_comparator.h sn_random.h

sn_population.o: sn_population.c sn_population.h sn_network.h sn_random.h

sn_random.o: sn_random.c sn_random.h

sn_stage.o: sn_stage.c sn_stage.h sn_comparator.h


sn-apply: sn-apply.c sn_network.o sn_stage.o sn_comparator.o sn_random.o

sn-check-bf: sn-check-bf.c sn_network.o sn_stage.o sn_comparator.o sn_random.o

sn-cut: sn-cut.c sn_network.o sn_stage.o sn_comparator.o sn_random.o

sn-evolution: sn-evolution.c sn_network.o sn_stage.o sn_comparator.o sn_population.o sn_random.o

sn-merge: sn-merge.c sn_network.o sn_stage.o sn_comparator.o sn_random.o

sn-normalize: sn-normalize.c sn_network.o sn_stage.o sn_comparator.o sn_random.o

sn-show: sn-show.c sn_network.o sn_stage.o sn_comparator.o sn_random.o

sn-tex: sn-tex.c sn_network.o sn_stage.o sn_comparator.o sn_random.o

.PHONY: clean

