/**
Copyright 2024 Carl van Mastrigt

This file is part of solipsix.

solipsix is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

solipsix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with solipsix.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

// #include <stddef.h>
// #include <inttypes.h>
// #include <stdbool.h>
// #include <stdatomic.h>

// typedef struct sol_lockfree_pool sol_lockfree_pool;
// // struct sol_lockfree_pool;

#include "coherent_structures/lockfree_pool.h"
#include "coherent_structures/lockfree_stack.h"
#include "coherent_structures/lockfree_hopper.h"
#include "coherent_structures/coherent_queue.h"
#include "coherent_structures/coherent_queue_with_counter.h"
