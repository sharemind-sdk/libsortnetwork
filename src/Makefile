CC = gcc
CFLAGS = -Wall -Werror -std=c99 -O3
#CFLAGS = -Wall -Werror -std=c99 -O0 -g

all: sn-cut sn-evolution sn-merge sn-show

sn_comparator.o: sn_comparator.c sn_comparator.h

sn_stage.o: sn_stage.c sn_stage.h sn_comparator.h

sn_network.o: sn_network.c sn_network.h sn_stage.h sn_comparator.h

sn-cut: sn-cut.c sn_network.o sn_stage.o sn_comparator.o

sn-evolution: sn-evolution.c sn_network.o sn_stage.o sn_comparator.o

sn-merge: sn-merge.c sn_network.o sn_stage.o sn_comparator.o

sn-show: sn-show.c sn_network.o sn_stage.o sn_comparator.o

