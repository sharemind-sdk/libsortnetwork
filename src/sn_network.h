#ifndef SN_NETWORK_H
#define SN_NETWORK_H 1

#include <stdio.h>

#include "sn_comparator.h"
#include "sn_stage.h"

struct sn_network_s
{
  int inputs_num;
  sn_stage_t **stages;
  int stages_num;
};
typedef struct sn_network_s sn_network_t;

#define SN_NETWORK_STAGE_NUM(n) (n)->stages_num
#define SN_NETWORK_STAGE_GET(n,i) ((n)->stages[i])

sn_network_t *sn_network_create (int inputs_num);
void sn_network_destroy (sn_network_t *n);

int sn_network_stage_add (sn_network_t *n, sn_stage_t *s);
int sn_network_stage_remove (sn_network_t *n, int s_num);

int sn_network_show (sn_network_t *n);
int sn_network_invert (sn_network_t *n);
int sn_network_compress (sn_network_t *n);

int sn_network_cut_at (sn_network_t *n, int input, enum sn_network_cut_dir_e dir);
sn_network_t *sn_network_combine (sn_network_t *n0, sn_network_t *n1);

sn_network_t *sn_network_read (FILE *fh);
sn_network_t *sn_network_read_file (const char *file);
int sn_network_write (sn_network_t *n, FILE *fh);

#endif /* SN_NETWORK_H */

/* vim: set shiftwidth=2 softtabstop=2 : */
