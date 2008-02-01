#include <stdlib.h>
#include <string.h>

#include "sn_comparator.h"

sn_comparator_t *sn_comparator_create (int min, int max)
{
  sn_comparator_t *c;

  c = (sn_comparator_t *) malloc (sizeof (sn_comparator_t));
  if (c == NULL)
    return (NULL);
  memset (c, '\0', sizeof (sn_comparator_t));

  c->min = min;
  c->max = max;

  return (c);
} /* sn_comparator_t *sn_comparator_create */

void sn_comparator_destroy (sn_comparator_t *c)
{
  if (c != NULL)
    free (c);
} /* void sn_comparator_destroy */

void sn_comparator_invert (sn_comparator_t *c)
{
  int max = c->min;
  int min = c->max;

  c->min = min;
  c->max = max;
} /* void sn_comparator_invert */

void sn_comparator_swap (sn_comparator_t *c, int con0, int con1)
{
  if (c->min == con0)
  {
    c->min = con1;
  }
  else if (c->min == con1)
  {
    c->min = con0;
  }

  if (c->max == con0)
  {
    c->max = con1;
  }
  else if (c->max == con1)
  {
    c->max = con0;
  }
} /* void sn_comparator_swap */

int sn_comparator_compare (const void *v0, const void *v1)
{
  sn_comparator_t *c0 = (sn_comparator_t *) v0;
  sn_comparator_t *c1 = (sn_comparator_t *) v1;

  if (SN_COMP_LEFT (c0) < SN_COMP_LEFT (c1))
    return (-1);
  else if (SN_COMP_LEFT (c0) > SN_COMP_LEFT (c1))
    return (1);
  else if (SN_COMP_RIGHT (c0) < SN_COMP_RIGHT (c1))
    return (-1);
  else if (SN_COMP_RIGHT (c0) > SN_COMP_RIGHT (c1))
    return (1);
  else
    return (0);
} /* int sn_comparator_compare */

/* vim: set shiftwidth=2 softtabstop=2 : */
