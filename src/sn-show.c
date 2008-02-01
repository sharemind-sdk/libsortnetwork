#include <stdlib.h>
#include <stdio.h>

#include "sn_network.h"

int main (int argc, char **argv)
{
  sn_network_t *n;
  FILE *fh = NULL;

  if (argc == 1)
    fh = stdin;
  else if (argc == 2)
    fh = fopen (argv[1], "r");

  if (fh == NULL)
  {
    printf ("fh == NULL!\n");
    return (1);
  }

  n = sn_network_read (fh);

  if (n == NULL)
  {
    printf ("n == NULL!\n");
    return (1);
  }

  sn_network_show (n);

  return (0);
} /* int main */

/* vim: set shiftwidth=2 softtabstop=2 : */
