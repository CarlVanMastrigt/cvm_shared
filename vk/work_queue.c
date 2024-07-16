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


void cvm_vk_work_queue_initialise(cvm_vk_work_queue * queue, const cvm_vk_work_queue_setup * setup)
{
    uint32_t i;
    queue->shared_data=setup->shared_data;
    queue->entry_size=setup->entry_size;
    queue->entry_count=setup->entry_count;
    queue->available_index=0;
    queue->available_count=setup->entry_count;
    queue->entry_in_use=false;

    queue->entry_terminate_func=setup->entry_terminate_func;
    queue->entry_reset_func=setup->entry_reset_func;

    queue->entry_data=malloc(setup->entry_size*setup->entry_count);
    queue->entry_completion_moments=malloc(sizeof(cvm_vk_timeline_semaphore)*setup->entry_count);
    for(i=0;i<setup->entry_count;i++)
    {
        queue->entry_completion_moments[i]=cvm_vk_timeline_semaphore_moment_null;
        setup->entry_initialise_func(setup->shared_data,queue->entry_data+i*setup->entry_size);
    }
}

void cvm_vk_work_queue_terminate(cvm_vk_device * device, cvm_vk_work_queue * queue)
{
    /// stalls on all entries?
    uint32_t i;

    while(queue->available_count < queue->entry_count)
    {
        i = (queue->available_index+queue->available_count) % queue->entry_count;
        assert(queue->entry_completion_moments[i].semaphore!=VK_NULL_HANDLE);///must have a valid point in time to query

        cvm_vk_timeline_semaphore_moment_wait(device,queue->entry_completion_moments+i);///can stall!

        /// reset and add back to available set
        queue->entry_reset_func(queue->shared_data,queue->entry_data+i*queue->entry_size);
        queue->available_count++;
    }

    for(i=0;i<queue->entry_count;i++)
    {
        queue->entry_terminate_func(queue->shared_data,queue->entry_data+i*queue->entry_size);
    }

    free(queue->entry_data);
    free(queue->entry_completion_moments);
}

#warning have debug logging for all the time points that can stall
void * cvm_vk_work_queue_acquire_entry(cvm_vk_device * device, cvm_vk_work_queue * queue)
{
    uint32_t i;

    /// THIS IS NOT ENOUGH IN A MULTITHREADED CONTEXT!
    /// is it even desirable for this to be multithreaded!? out of order sumbission of work doesn't seem very desirable...

    assert(queue->available_count <= queue->entry_count);
    assert(!queue->entry_in_use);/// should not get another command_queue_entry before the already acquired one is submitted
    while(queue->available_count < queue->entry_count)
    {
        i = (queue->available_index+queue->available_count) % queue->entry_count;
        assert(queue->entry_completion_moments[i].semaphore!=VK_NULL_HANDLE);///must have a valid point in time to query

        if(!cvm_vk_timeline_semaphore_moment_query(device,queue->entry_completion_moments+i)) break;

        /// reset and add back to available set
        queue->entry_reset_func(queue->shared_data,queue->entry_data+i*queue->entry_size);
        queue->available_count++;
    }


    i = queue->available_index;
    /// this goes second b/c its assumed wait is slower than query
    if(queue->available_count==0)
    {
        assert(queue->entry_completion_moments[i].semaphore!=VK_NULL_HANDLE);///must have a valid point in time to query

        cvm_vk_timeline_semaphore_moment_wait(device,queue->entry_completion_moments+i);///can stall!

        /// reset and add back to available set
        queue->entry_reset_func(queue->shared_data,queue->entry_data+i*queue->entry_size);
        queue->available_count++;
    }

    queue->entry_in_use=true;

    return queue->entry_data+i*queue->entry_size;
}

void cvm_vk_work_queue_release_entry(cvm_vk_work_queue * queue, void * entry, const cvm_vk_timeline_semaphore_moment * completion_moment)
{
    assert((((char*)entry-queue->entry_data)%queue->entry_size)==0);///entry was not an acquired value
    assert((((char*)entry-queue->entry_data)/queue->entry_size)==queue->available_index);///should be releasing the get index
    assert(queue->entry_in_use);/// entry should be in use if its going to be released
    assert(completion_moment && completion_moment->semaphore!=VK_NULL_HANDLE);///a valid completion moment is required

    queue->entry_in_use=false;

    queue->entry_completion_moments[queue->available_index]=*completion_moment;

    queue->available_index++;///officially comes off the available set
    queue->available_count--;

    if(queue->available_index == queue->entry_count)
    {
        ///needed to avoid eventual overflow (wrap locally)
        queue->available_index=0;
    }
}








