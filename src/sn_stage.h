#ifndef SN_STAGE_H
#define SN_STAGE_H 1

#include <stdio.h>

#include "sn_comparator.h"

struct sn_stage_s
{
  int depth;
  sn_comparator_t *comparators;
  int comparators_num;
};
typedef struct sn_stage_s sn_stage_t;

enum sn_network_cut_dir_e
{
  DIR_MIN,
  DIR_MAX
};

#define SN_STAGE_DEPTH(s) (s)->depth
#define SN_STAGE_COMP_NUM(s) (s)->comparators_num
#define SN_STAGE_COMP_GET(s,n) ((s)->comparators + (n))

sn_stage_t *sn_stage_create (int depth);
sn_stage_t *sn_stage_clone (const sn_stage_t *s);
void sn_stage_destroy (sn_stage_t *s);

int sn_stage_comparator_add (sn_stage_t *s, const sn_comparator_t *c);
int sn_stage_comparator_remove (sn_stage_t *s, int c_num);
int sn_stage_comparator_check_conflict (sn_stage_t *s, const sn_comparator_t *c);

int sn_stage_show (sn_stage_t *s);
int sn_stage_invert (sn_stage_t *s);
int sn_stage_swap (sn_stage_t *s, int con0, int con1);
int sn_stage_cut_at (sn_stage_t *s, int input, enum sn_network_cut_dir_e dir);
int sn_stage_remove_input (sn_stage_t *s, int input);

sn_stage_t *sn_stage_read (FILE *fh);
int sn_stage_write (sn_stage_t *s, FILE *fh);

#endif /* SN_STAGE_H */

/* vim: set shiftwidth=2 softtabstop=2 : */
