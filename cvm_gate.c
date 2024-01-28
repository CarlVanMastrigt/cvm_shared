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

#include "cvm_shared.h"

static void cvm_gate_init(cvm_gate * gate)
{
    cnd_init(&gate->condition);
    mtx_init(&gate->mutex,mtx_plain);
    atomic_init(&gate->status,0);
}

static void cvm_gate_destroy(cvm_gate * gate)
{
    cnd_destroy(&gate->condition);
    mtx_destroy(&gate->mutex);
}

#define CVM_LOCKFREE_POOL_ENTRY_TYPE cvm_gate

#define CVM_LOCKFREE_POOL_TYPE cvm_gate_pool
#define CVM_LOCKFREE_POOL_FUNCTION_PREFIX cvm_gate_pool

#define CVM_LOCKFREE_LIST_TYPE cvm_gate_list
#define CVM_LOCKFREE_LIST_FUNCTION_PREFIX cvm_gate_list

#define CVM_LOCKFREE_POOL_BATCH_COUNT_EXPONENT CVM_GATE_POOL_BATCH_COUNT_EXPONENT
#define CVM_LOCKFREE_POOL_BATCH_SIZE_EXPONENT  CVM_GATE_POOL_BATCH_SIZE_EXPONENT

#define CVM_LOCKFREE_POOL_ENTRY_INIT_FUNCTION cvm_gate_init
#define CVM_LOCKFREE_POOL_ENTRY_DESTROY_FUNCTION cvm_gate_destroy

#include "cvm_lockfree_list.c"



#define CVM_GATE_WAITING_FLAG   0x80000000
#define CVM_GATE_SIGNAL_STATUS  0x80000001 /**would be last signal and thread is waiting*/
#define CVM_GATE_DEP_COUNT_MASK 0x7FFFFFFF

cvm_gate * cvm_gate_acquire(cvm_gate_pool * pool)
{
    cvm_gate * gate;

    gate=cvm_gate_pool_acquire_entry(pool);
    atomic_store(&gate->status,0);/// this is 0 dependencies and in the non-waiting state, this could be made one with an fetch sub in `cvm_gate_wait_and_relinquish` if error checking is desirable

    return gate;
}

void cvm_gate_signal(cvm_gate * gate)
{
    uint_fast32_t current_status, replacement_status;
    bool locked;

    current_status=atomic_load_explicit(&gate->status, memory_order_relaxed);
    locked = false;

    do
    {
        assert((current_status&CVM_GATE_DEP_COUNT_MASK)>0);/// you have signalled more times than dependencies were added! this is incredibly bad and unsafe

        if(current_status==CVM_GATE_SIGNAL_STATUS)///it's only possible for 1 thread to see this value. and if locked before deps hitting 0: exactly 1 thread MUST see this value
        {
            if(!locked)
            {
                mtx_lock(&gate->mutex);/// must lock before going from waiting to "complete" (status==0)
                locked = true;/// don't relock if compare exchange fails
            }
            replacement_status=0;/// this indicates the waiting thread may proceed
        }
        else
        {
            replacement_status=current_status-1;
        }

    }
    while(!atomic_compare_exchange_weak_explicit(&gate->status, &current_status, replacement_status, memory_order_release, memory_order_relaxed));

    if(current_status==CVM_GATE_SIGNAL_STATUS)
    {
        assert(locked);///must have locked if we will need to signal
        cnd_signal(&gate->condition);
        mtx_unlock(&gate->mutex);
    }
}

void cvm_gate_wait_and_relinquish(cvm_gate * gate)
{
    uint_fast32_t current_status, replacement_status;

    current_status=atomic_load_explicit(&gate->status, memory_order_acquire);

    if(current_status)/// has dependencies remaining
    {
        mtx_lock(&gate->mutex);///must lock before making assertions regarding status being 0

        do
        {
            assert(!(current_status&CVM_GATE_WAITING_FLAG));/// must NOT already have waiting flag set, did you try and wait on this gate twice?
            replacement_status=current_status|CVM_GATE_WAITING_FLAG;
        }
        while(current_status && !atomic_compare_exchange_weak_explicit(&gate->status, &current_status, replacement_status, memory_order_release, memory_order_acquire));/// please not the sequence point
        ///must either have nothing waiting or have flagged the status as waiting by this point
        /// release to ensure ordering of mutex lock (not 100% on whether this is actually necessary)

        while(current_status)///if we did flag the status as waiting
        {
            cnd_wait(&gate->condition, &gate->mutex);
            current_status=atomic_load_explicit(&gate->status, memory_order_acquire);/// load to double check signal actually happened in case of spurrious wake up
        }

        mtx_unlock(&gate->mutex);
    }

    /// at this point anything else that would use (signal) this gate must have completed, so we're safe to recycle
    cvm_gate_pool_relinquish_entry(gate->pool,gate);
}

void cvm_gate_add_dependencies(cvm_gate * gate, uint32_t dependency_count)
{
    atomic_fetch_add_explicit(&gate->status,dependency_count,memory_order_relaxed);
    /// ADDING dependencies shouldn't incurr memory ordering restrictions
    /// though error checking (initial dep count is 1) to check that this is a valid operation to perform
}
