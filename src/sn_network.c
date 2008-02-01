#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <assert.h>

#include "sn_network.h"

sn_network_t *sn_network_create (int inputs_num)
{
  sn_network_t *n;

  n = (sn_network_t *) malloc (sizeof (sn_network_t));
  if (n == NULL)
    return (NULL);
  memset (n, '\0', sizeof (sn_network_t));

  n->inputs_num = inputs_num;

  return (n);
} /* sn_network_t *sn_network_create */

void sn_network_destroy (sn_network_t *n)
{
  if (n == NULL)
    return;

  if (n->stages != NULL)
  {
    int i;
    for (i = 0; i < n->stages_num; i++)
    {
      sn_stage_destroy (n->stages[i]);
      n->stages[i] = NULL;
    }
    n->stages = NULL;
  }

  free (n);
} /* void sn_network_destroy */

int sn_network_stage_add (sn_network_t *n, sn_stage_t *s)
{
  sn_stage_t **temp;

  temp = (sn_stage_t **) realloc (n->stages, (n->stages_num + 1)
      * sizeof (sn_stage_t *));
  if (temp == NULL)
    return (-1);

  n->stages = temp;
  SN_STAGE_DEPTH (s) = n->stages_num;
  n->stages[n->stages_num] = s;
  n->stages_num++;

  return (0);
} /* int sn_network_stage_add */

int sn_network_stage_remove (sn_network_t *n, int s_num)
{
  int nmemb = n->stages_num - (s_num + 1);
  sn_stage_t **temp;

  assert (s_num < n->stages_num);

  sn_stage_destroy (n->stages[s_num]);
  n->stages[s_num] = NULL;

  if (nmemb > 0)
    memmove (n->stages + s_num, n->stages + (s_num + 1),
	nmemb * sizeof (sn_stage_t *));
  n->stages_num--;

  /* Free the unused memory */
  temp = (sn_stage_t **) realloc (n->stages,
      n->stages_num * sizeof (sn_stage_t *));
  if (temp == NULL)
    return (-1);
  n->stages = temp;

  return (0);
} /* int sn_network_stage_remove */

int sn_network_show (sn_network_t *n)
{
  int i;

  for (i = 0; i < n->stages_num; i++)
    sn_stage_show (n->stages[i]);

  return (0);
} /* int sn_network_show */

int sn_network_invert (sn_network_t *n)
{
  int i;

  for (i = 0; i < n->stages_num; i++)
    sn_stage_invert (n->stages[i]);

  return (0);
} /* int sn_network_invert */

int sn_network_compress (sn_network_t *n)
{
  int i;
  int j;
  int k;

  for (i = 1; i < n->stages_num; i++)
  {
    sn_stage_t *s;
    
    s = n->stages[i];
    
    for (j = 0; j < SN_STAGE_COMP_NUM (s); j++)
    {
      sn_comparator_t *c = SN_STAGE_COMP_GET (s, j);
      int move_to = i;

      for (k = i - 1; k >= 0; k--)
      {
	int conflict;

	conflict = sn_stage_comparator_check_conflict (n->stages[k], c);
	if (conflict == 0)
	{
	  move_to = k;
	  continue;
	}

	if (conflict == 2)
	  move_to = -1;
	break;
      }

      if (move_to < i)
      {
	if (move_to >= 0)
	  sn_stage_comparator_add (n->stages[move_to], c);
	sn_stage_comparator_remove (s, j);
	j--;
      }
    }
  }

  return (0);
} /* int sn_network_compress */

int sn_network_cut_at (sn_network_t *n, int input, enum sn_network_cut_dir_e dir)
{
  int i;
  int position = input;

  for (i = 0; i < n->stages_num; i++)
  {
    sn_stage_t *s;
    int new_position;
    
    s = n->stages[i];
    new_position = sn_stage_cut_at (s, position, dir);
    
    if (position != new_position)
    {
      int j;

      for (j = 0; j < i; j++)
	sn_stage_swap (n->stages[j], position, new_position);
    }

    position = new_position;
  }

  assert (((dir == DIR_MIN) && (position == 0))
      || ((dir == DIR_MAX) && (position == (n->inputs_num - 1))));


  for (i = 0; i < n->stages_num; i++)
    sn_stage_remove_input (n->stages[i], position);

  n->inputs_num--;

  return (0);
} /* int sn_network_cut_at */

static int sn_network_add_bitonic_merger_recursive (sn_network_t *n,
    int low, int num)
{
  sn_stage_t *s;
  int m;
  int i;

  s = sn_stage_create (n->stages_num);
  if (s == NULL)
    return (-1);

  if (num == 1)
    return (0);

  m = num / 2;

  for (i = low; i < (low + m); i++)
  {
    sn_comparator_t c;

    c.min = i;
    c.max = i + m;

    sn_stage_comparator_add (s, &c);
  }

  sn_network_stage_add (n, s);

  sn_network_add_bitonic_merger_recursive (n, low, m);
  sn_network_add_bitonic_merger_recursive (n, low + m, m);

  return (0);
} /* int sn_network_add_bitonic_merger_recursive */

static int sn_network_add_bitonic_merger (sn_network_t *n)
{
  sn_stage_t *s;
  int m;
  int i;

  s = sn_stage_create (n->stages_num);
  if (s == NULL)
    return (-1);

  m = n->inputs_num / 2;

  for (i = 0; i < m; i++)
  {
    sn_comparator_t c;

    c.min = i;
    c.max = n->inputs_num - (i + 1);

    sn_stage_comparator_add (s, &c);
  }

  sn_network_stage_add (n, s);

  sn_network_add_bitonic_merger_recursive (n, 0, m);
  sn_network_add_bitonic_merger_recursive (n, m, m);

  return (0);
} /* int sn_network_add_bitonic_merger */

sn_network_t *sn_network_combine (sn_network_t *n0, sn_network_t *n1)
{
  sn_network_t *n;
  int stages_num;
  int i;
  int j;

  stages_num = (n0->stages_num > n1->stages_num)
    ? n0->stages_num
    : n1->stages_num;

  n = sn_network_create (n0->inputs_num + n1->inputs_num);
  if (n == NULL)
    return (NULL);

  for (i = 0; i < stages_num; i++)
  {
    sn_stage_t *s = sn_stage_create (i);

    if (i < n0->stages_num)
      for (j = 0; j < SN_STAGE_COMP_NUM (n0->stages[i]); j++)
      {
	sn_comparator_t *c = SN_STAGE_COMP_GET (n0->stages[i], j);
	sn_stage_comparator_add (s, c);
      }

    if (i < n1->stages_num)
      for (j = 0; j < SN_STAGE_COMP_NUM (n1->stages[i]); j++)
      {
	sn_comparator_t *c_orig = SN_STAGE_COMP_GET (n1->stages[i], j);
	sn_comparator_t  c_copy;

	SN_COMP_MIN(&c_copy) = SN_COMP_MIN(c_orig) + n0->inputs_num;
	SN_COMP_MAX(&c_copy) = SN_COMP_MAX(c_orig) + n0->inputs_num;

	sn_stage_comparator_add (s, &c_copy);
      }

    sn_network_stage_add (n, s);
  }

  sn_network_add_bitonic_merger (n);
  sn_network_compress (n);

  return (n);
} /* sn_network_t *sn_network_combine */

sn_network_t *sn_network_read (FILE *fh)
{
  sn_network_t *n;
  char buffer[64];

  int opt_inputs = 0;

  while (fgets (buffer, sizeof (buffer), fh) != NULL)
  {
    char *str_key = buffer;
    char *str_value = NULL;
    int   buffer_len = strlen (buffer);

    while ((buffer_len > 0) && ((buffer[buffer_len - 1] == '\n')
	  || (buffer[buffer_len - 1] == '\r')))
    {
      buffer_len--;
      buffer[buffer_len] = '\0';
    }
    if (buffer_len == 0)
      break;

    str_value = strchr (buffer, ':');
    if (str_value == NULL)
    {
      printf ("Cannot parse line: %s\n", buffer);
      continue;
    }

    *str_value = '\0'; str_value++;
    while ((*str_value != '\0') && (isspace (*str_value) != 0))
      str_value++;

    if (strcasecmp ("Inputs", str_key) == 0)
      opt_inputs = atoi (str_value);
    else
      printf ("Unknown key: %s\n", str_key);
  } /* while (fgets) */

  if (opt_inputs < 2)
    return (NULL);

  n = sn_network_create (opt_inputs);

  while (42)
  {
    sn_stage_t *s;

    s = sn_stage_read (fh);
    if (s == NULL)
      break;

    sn_network_stage_add (n, s);
  }

  if (SN_NETWORK_STAGE_NUM (n) < 1)
  {
    sn_network_destroy (n);
    return (NULL);
  }

  return (n);
} /* sn_network_t *sn_network_read */

sn_network_t *sn_network_read_file (const char *file)
{
  sn_network_t *n;
  FILE *fh;

  fh = fopen (file, "r");
  if (fh == NULL)
    return (NULL);

  n = sn_network_read (fh);

  fclose (fh);

  return (n);
} /* sn_network_t *sn_network_read_file */

int sn_network_write (sn_network_t *n, FILE *fh)
{
  int i;

  fprintf (fh, "Inputs: %i\n", n->inputs_num);
  fprintf (fh, "\n");

  for (i = 0; i < n->stages_num; i++)
    sn_stage_write (n->stages[i], fh);

  return (0);
} /* int sn_network_write */

/* vim: set shiftwidth=2 softtabstop=2 : */
