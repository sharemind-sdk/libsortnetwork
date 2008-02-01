#include <stdlib.h>
#include <stdio.h>

#include "sn_network.h"

void exit_usage (const char *name)
{
  printf ("%s <file0> <file1>\n", name);
  exit (1);
} /* void exit_usage */

int main (int argc, char **argv)
{
  sn_network_t *n0;
  sn_network_t *n1;
  sn_network_t *n;

  if (argc != 3)
    exit_usage (argv[0]);

  n0 = sn_network_read_file (argv[1]);
  if (n0 == NULL)
  {
    printf ("n0 == NULL\n");
    return (1);
  }

  n1 = sn_network_read_file (argv[2]);
  if (n1 == NULL)
  {
    printf ("n1 == NULL\n");
    return (1);
  }

  n = sn_network_combine (n0, n1);
  sn_network_destroy (n0);
  sn_network_destroy (n1);

  sn_network_write (n, stdout);

  return (0);
} /* int main */

/* vim: set shiftwidth=2 softtabstop=2 : */
