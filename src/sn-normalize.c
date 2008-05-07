#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sn_network.h"

void exit_usage (const char *name)
{
  printf ("%s [file]\n", name);
  exit (1);
}

int main (int argc, char **argv)
{
  sn_network_t *n;

  if (argc > 2)
  {
    exit_usage (argv[0]);
  }
  else if (argc == 2)
  {
    if ((strcmp ("-h", argv[1]) == 0)
	|| (strcmp ("--help", argv[1]) == 0)
	|| (strcmp ("-help", argv[1]) == 0))
      exit_usage (argv[0]);

    n = sn_network_read_file (argv[1]);
  }
  else
  {
    n = sn_network_read (stdin);
  }

  if (n == NULL)
  {
    printf ("n == NULL!\n");
    return (1);
  }

  sn_network_normalize (n);
  sn_network_compress (n);

  sn_network_write (n, stdout);

  return (0);
} /* int main */

/* vim: set shiftwidth=2 softtabstop=2 : */
