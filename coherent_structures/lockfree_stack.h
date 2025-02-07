/**
Copyright 2024,2025 Carl van Mastrigt

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

#include <stddef.h>
#include <inttypes.h>
#include <stdatomic.h>
#include <stdbool.h>

#include "coherent_structures/lockfree_pool.h"

struct sol_lockfree_stack
{
    atomic_uint_fast64_t head;

    size_t entry_size;
    size_t capacity_exponent;
    char* entry_data;
    uint32_t* next_buffer;
};


/// a stack uses a pool for backing
void sol_lockfree_stack_initialise(struct sol_lockfree_stack* stack, struct sol_lockfree_pool* pool);
void sol_lockfree_stack_terminate(struct sol_lockfree_stack* stack);


void sol_lockfree_stack_push(struct sol_lockfree_stack* stack, void* entry);
void* sol_lockfree_stack_pull(struct sol_lockfree_stack* stack);// NULL returned to indicate failure
