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

#include <inttypes.h>
#include <stdatomic.h>
#include <threads.h>

#include "coherent_structures/lockfree_pool.h"

#include "sync/primitive.h"

struct sol_gate_pool
{
    struct sol_lockfree_pool available_gates;
};

void sol_gate_pool_initialise(struct sol_gate_pool* pool, size_t capacity_exponent);
void sol_gate_pool_terminate(struct sol_gate_pool* pool);

struct sol_gate
{
    struct sol_sync_primitive primitive;

    struct sol_gate_pool* pool;

    atomic_uint_fast32_t status;

    mtx_t* mutex;
    cnd_t* condition;
};

struct sol_gate* sol_gate_prepare(struct sol_gate_pool* pool);

/// MUST be called at some point as this will clean up the acquired gate
void sol_gate_wait(struct sol_gate* gate);


/// used to set up dependencies (calls to sol_gate_signal_conditions) to wait on (dont return from wait until all signals happen)
/// must know that an gate has at least one outstanding dependency and/or that the gate hasn't been waited on for calling this to be legal
void sol_gate_impose_conditions(struct sol_gate* gate, uint_fast32_t count);

/// must be called once for every dependency added by a call to sol_gate_impose_conditions
void sol_gate_signal_conditions(struct sol_gate* gate, uint_fast32_t count);

