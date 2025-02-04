/**
Copyright 2024 Carl van Mastrigt

This file is part of cvm_shared.

cvm_shared is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

cvm_shared is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with cvm_shared.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stddef.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdatomic.h>

#ifndef CVM_COHERENT_STRUCTURES_H
#define CVM_COHERENT_STRUCTURES_H

/** lockfree_pool
 * a pool is just a stack that has it's own internal backing and starts full
 * it's used for the memory backing of other types (stacks, queues and hoppers)
 * the interplay of pool and stack can be a little complicated to understand, but they are very powerful together*/

typedef struct cvm_lockfree_pool cvm_lockfree_pool;
// struct cvm_lockfree_pool;

#include "coherent_structures/lockfree_stack.h"
#include "coherent_structures/lockfree_pool.h"
#include "coherent_structures/lockfree_hopper.h"
#include "coherent_structures/coherent_queue.h"
#include "coherent_structures/coherent_queue_with_counter.h"

#endif

