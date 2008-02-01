#include <stdlib.h>
#include <stdio.h>
#include <strings.h>

#include "sn_network.h"

void exit_usage (const char *name)
{
  printf ("%s <position> <min|max>\n", name);
  exit (1);
}

int main (int argc, char **argv)
{
  sn_network_t *n;

  int pos = 0;
  enum sn_network_cut_dir_e dir = DIR_MIN;

  if (argc != 3)
    exit_usage (argv[0]);

  pos = atoi (argv[1]);
  if (strcasecmp ("max", argv[2]) == 0)
    dir = DIR_MAX;

  n = sn_network_read (stdin);
  if (n == NULL)
  {
    printf ("n == NULL!\n");
    return (1);
  }

  sn_network_cut_at (n, pos, dir);
  sn_network_compress (n);

  sn_network_write (n, stdout);

  return (0);
} /* int main */

/* vim: set shiftwidth=2 softtabstop=2 : */
