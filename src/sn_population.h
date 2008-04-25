#ifndef SN_POPULATION_H
#define SN_POPULATION_H 1

#include <stdint.h>
#include "sn_network.h"

/*
 * sn_population_t
 *
 * Opaque data type for the population.
 */
struct sn_population_s;
typedef struct sn_population_s sn_population_t;

sn_population_t *sn_population_create (uint32_t size);
void sn_population_destroy (sn_population_t *p);

/*
 * push
 *
 * Puts a new network into the population. The network is _copied_ and may be
 * modified by the caller after inserting the network. This won'thave an effect
 * on the copy in the population, though. Of course, the caller must free his
 * own copy himself!
 */
int sn_population_push (sn_population_t *p, sn_network_t *n);

/*
 * pop
 *
 * Returns a _copy_ of an individuum in the population. The network is NOT
 * removed from the population by this operation! The caller must free this
 * copy himself. Returns NULL upon failure.
 */
sn_network_t *sn_population_pop (sn_population_t *p);

sn_network_t *sn_population_best (sn_population_t *p);

#endif /* SN_POPULATION_H */
