#ifndef SN_COMPARATOR_H
#define SN_COMPARATOR_H 1

struct sn_comparator_s
{
  int min;
  int max;
};
typedef struct sn_comparator_s sn_comparator_t;

#define SN_COMP_LEFT(c)  (((c)->min < (c)->max) ? (c)->min : (c)->max)
#define SN_COMP_RIGHT(c) (((c)->min > (c)->max) ? (c)->min : (c)->max)
#define SN_COMP_MIN(c) (c)->min
#define SN_COMP_MAX(c) (c)->max

sn_comparator_t *sn_comparator_create (int min, int max);
void sn_comparator_destroy (sn_comparator_t *c);
void sn_comparator_invert (sn_comparator_t *c);
void sn_comparator_swap (sn_comparator_t *c, int con0, int con1);

int sn_comparator_compare (const void *, const void *);

#endif /* SN_COMPARATOR_H */

/* vim: set shiftwidth=2 softtabstop=2 : */
