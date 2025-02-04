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

#include "solipsix.h"

VkResult cvm_vk_staging_buffer_initialise(struct cvm_vk_staging_buffer_ * staging_buffer, cvm_vk_device * device, VkBufferUsageFlags usage, VkDeviceSize buffer_size, VkDeviceSize reserved_high_priority_space)
{
    VkResult result;
    const VkDeviceSize alignment = cvm_vk_buffer_alignment_requirements(device, usage);

    buffer_size = cvm_vk_align(buffer_size, alignment);/// round to multiple of alignment

    cvm_vk_buffer_memory_pair_setup buffer_setup=(cvm_vk_buffer_memory_pair_setup)
    {
        /// in
        .buffer_size=buffer_size,
        .usage=usage,
        .required_properties=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        .desired_properties=VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        .map_memory=true,
        /// out starts uninit
    };

    result = cvm_vk_buffer_memory_pair_create(device, &buffer_setup);

    if(result == VK_SUCCESS)
    {
        staging_buffer->buffer = buffer_setup.buffer;
        staging_buffer->memory = buffer_setup.memory;
        staging_buffer->mapping = buffer_setup.mapping;
        staging_buffer->mapping_coherent = buffer_setup.mapping_coherent;

        staging_buffer->usage = usage;
        staging_buffer->alignment = alignment;

        staging_buffer->threads_waiting_on_semaphore_setup = false;

        staging_buffer->terminating = false;

        staging_buffer->buffer_size = buffer_size;
        staging_buffer->reserved_high_priority_space = reserved_high_priority_space;
        staging_buffer->current_offset = 0;
        staging_buffer->remaining_space = buffer_size;

        mtx_init(&staging_buffer->access_mutex, mtx_plain);
        cnd_init(&staging_buffer->setup_stall_condition);

        cvm_vk_staging_buffer_segment_queue_initialise(&staging_buffer->segment_queue);
    }

    return result;
}

/// cannot acquire allocations after this has been called
void cvm_vk_staging_buffer_terminate(struct cvm_vk_staging_buffer_ * staging_buffer, cvm_vk_device * device)
{
    struct cvm_vk_staging_buffer_segment * oldest_active_segment;

    mtx_lock(&staging_buffer->access_mutex);
    staging_buffer->terminating = true;
    /// wait for all memory uses to complete, must be externally synchronised to ensure buffer is not in use elsewhere
    while((oldest_active_segment = cvm_vk_staging_buffer_segment_queue_dequeue_ptr(&staging_buffer->segment_queue)))
    {
        /// this allows cleanup to be initiated while dangling allocations have been made
        if (oldest_active_segment->moment_of_last_use.semaphore == VK_NULL_HANDLE)/// semaphore not actually set up yet, this segment has been reserved but not completed
        {
            staging_buffer->threads_waiting_on_semaphore_setup = true;
            cnd_wait(&staging_buffer->setup_stall_condition, &staging_buffer->access_mutex);
        }
        else
        {
            assert(oldest_active_segment->size);///segment must occupy space

            cvm_vk_timeline_semaphore_moment_wait(device,&oldest_active_segment->moment_of_last_use);

            /// this checks that the start of this segment is the end of the available space
            assert(oldest_active_segment->offset == (staging_buffer->current_offset+staging_buffer->remaining_space) % staging_buffer->buffer_size);/// out of order free for some reason, or offset mismatch

            staging_buffer->remaining_space += oldest_active_segment->size;///relinquish this segments space space
        }
    }

    mtx_unlock(&staging_buffer->access_mutex);

    assert(staging_buffer->remaining_space==staging_buffer->buffer_size);

    mtx_destroy(&staging_buffer->access_mutex);
    cnd_destroy(&staging_buffer->setup_stall_condition);
    cvm_vk_staging_buffer_segment_queue_terminate(&staging_buffer->segment_queue);

    cvm_vk_buffer_memory_pair_destroy(device, staging_buffer->buffer, staging_buffer->memory, true);
}


#warning consider moving this back into the main function?
static inline void cvm_vk_staging_buffer_query_allocations(struct cvm_vk_staging_buffer_ * staging_buffer, const cvm_vk_device * device)
{
    struct cvm_vk_staging_buffer_segment * oldest_active_segment;

    while((oldest_active_segment = cvm_vk_staging_buffer_segment_queue_get_front_ptr(&staging_buffer->segment_queue)))
    {
        if(oldest_active_segment->moment_of_last_use.semaphore == VK_NULL_HANDLE) return; /// oldest segment has not been "completed"

        assert(oldest_active_segment->size);///segment must occupy space

        if(!cvm_vk_timeline_semaphore_moment_query(device,&oldest_active_segment->moment_of_last_use)) return; /// oldest segment is still in use by some command_buffer/queue

        /// this checks that the start of this segment is the end of the available space
        assert(oldest_active_segment->offset == (staging_buffer->current_offset+staging_buffer->remaining_space) % staging_buffer->buffer_size);/// out of order free for some reason, or offset mismatch

        staging_buffer->remaining_space += oldest_active_segment->size;///relinquish this segments space space

        cvm_vk_staging_buffer_segment_queue_dequeue(&staging_buffer->segment_queue, NULL);/// remove oldest_active_segment from the queue

        if(staging_buffer->remaining_space == staging_buffer->buffer_size)
        {
            assert(staging_buffer->segment_queue.count==0);
            /// should the whole buffer become available, reset offset to 0 so that we can use the buffer more efficiently (try to avoid wrap)
            staging_buffer->current_offset=0;
        }
    }
}

struct cvm_vk_staging_buffer_allocation cvm_vk_staging_buffer_allocation_acquire(struct cvm_vk_staging_buffer_ * staging_buffer, const cvm_vk_device * device, VkDeviceSize requested_space, bool high_priority)
{
    VkDeviceSize required_space;
    bool wrap;
    struct cvm_vk_staging_buffer_segment* oldest_active_segment;
    cvm_vk_timeline_semaphore_moment oldest_moment;
    uint32_t segment_index,segment_count,masked_first_segment_index;
    VkDeviceSize acquired_offset;
    char* mapping;

    /// this design may allow continuous small allocations to effectively stall a large allocation until the whole buffer is full...
    requested_space = cvm_vk_align(requested_space, staging_buffer->alignment);

    assert(requested_space < staging_buffer->buffer_size);/// REALLY want make max allocation 1/4 total space...

    mtx_lock(&staging_buffer->access_mutex);

    while(true)
    {
        assert(!staging_buffer->terminating);
        /// try to free up space
        cvm_vk_staging_buffer_query_allocations(staging_buffer,device);

        wrap = staging_buffer->current_offset+requested_space > staging_buffer->buffer_size;

        /// if this request must wrap then we need to consume the unused section of the buffer that remains
        required_space = requested_space + wrap * (staging_buffer->buffer_size - staging_buffer->current_offset);

        assert(required_space < staging_buffer->buffer_size);

        /// check that the reserved high priority space will be preserved when processing low priority requests
        if(required_space + (high_priority ? 0 : staging_buffer->reserved_high_priority_space) <= staging_buffer->remaining_space)
        {
            break;/// request will fit, we're done
        }

        /// otherwise; more space required
        assert(staging_buffer->segment_queue.count > 0);///should not need more space if there are no active segments

        oldest_active_segment = cvm_vk_staging_buffer_segment_queue_get_front_ptr(&staging_buffer->segment_queue);

        if (oldest_active_segment->moment_of_last_use.semaphore == VK_NULL_HANDLE)/// semaphore not actually set up yet, this segment has been reserved but not completed
        {
            /// wait on semaphore to be set up (if necessary) -- this is a particularly bad situation
            /// must be while loop to handle spurrious wakeup
            /// must be signalled by thread that sets this semaphore moment
            staging_buffer->threads_waiting_on_semaphore_setup = true;
            cnd_wait(&staging_buffer->setup_stall_condition, &staging_buffer->access_mutex);
            /// when this actually regains the lock the actual first segment may have progressed, thus we need to start again (don't try to wait here)
        }
        else
        {
            /// wait on semaphore, many threads may actually do this, but that should be fine as long as timeline semaphore waiting on many threads isnt an issue
            ///need to retrieve all mutex controlled data used outside mutex, before unlocking mutex
            oldest_moment=oldest_active_segment->moment_of_last_use;

            /// isn't ideal b/c we now have to check this moment again inside the mutex
            mtx_unlock(&staging_buffer->access_mutex);
            /// wait for semaphore outside of mutex so as to not block inside the mutex
            cvm_vk_timeline_semaphore_moment_wait(device,&oldest_moment);

            mtx_lock(&staging_buffer->access_mutex);
        }
    }

    /// note active segment count is incremented, also importantly this index is UNWRAPPED, so that if the segment buffer gets expanded this index will still be valid
    segment_index = cvm_vk_staging_buffer_segment_queue_enqueue(&staging_buffer->segment_queue, (struct cvm_vk_staging_buffer_segment)
    {
        .moment_of_last_use = CVM_VK_TIMELINE_SEMAPHORE_MOMENT_NULL,
        .offset = staging_buffer->current_offset,
        .size = required_space,
    });

    acquired_offset = wrap ? 0 : staging_buffer->current_offset;
    mapping = staging_buffer->mapping + acquired_offset;

    staging_buffer->remaining_space-=required_space;
    staging_buffer->current_offset+=required_space;

    if(staging_buffer->current_offset > staging_buffer->buffer_size)
    {
        /// wrap the current offset if necessary
        staging_buffer->current_offset -= staging_buffer->buffer_size;
    }

    mtx_unlock(&staging_buffer->access_mutex);

    return (struct cvm_vk_staging_buffer_allocation)
    {
        .parent = staging_buffer,
        .acquired_offset = acquired_offset,
        .mapping = mapping,
        .segment_index = segment_index,
        .flushed = false,
    };
}

void cvm_vk_staging_buffer_allocation_flush_range(const struct cvm_vk_staging_buffer_ * staging_buffer, const cvm_vk_device * device, struct cvm_vk_staging_buffer_allocation* allocation, VkDeviceSize relative_offset, VkDeviceSize size)
{
    VkResult flush_result;

    if( ! staging_buffer->mapping_coherent)
    {
        VkMappedMemoryRange flush_range=(VkMappedMemoryRange)
        {
            .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            .pNext = NULL,
            .memory = staging_buffer->memory,
            .offset = allocation->acquired_offset + relative_offset,
            .size = size,
        };

        flush_result=vkFlushMappedMemoryRanges(device->device, 1, &flush_range);
    }

    assert(!allocation->flushed);/// only want to flush once
    allocation->flushed = true;
}

void cvm_vk_staging_buffer_allocation_release(struct cvm_vk_staging_buffer_allocation* allocation, cvm_vk_timeline_semaphore_moment moment_of_last_use)
{
    struct cvm_vk_staging_buffer_ * staging_buffer = allocation->parent;
    assert(allocation->flushed);
    mtx_lock(&staging_buffer->access_mutex);

    cvm_vk_staging_buffer_segment_queue_get_ptr(&staging_buffer->segment_queue, allocation->segment_index)->moment_of_last_use = moment_of_last_use;

    if(staging_buffer->threads_waiting_on_semaphore_setup)
    {
        staging_buffer->threads_waiting_on_semaphore_setup=false;
        cnd_broadcast(&staging_buffer->setup_stall_condition);
    }

    mtx_unlock(&staging_buffer->access_mutex);
}

VkDeviceSize cvm_vk_staging_buffer_allocation_align_offset(const struct cvm_vk_staging_buffer_ * staging_buffer, VkDeviceSize offset)
{
    return cvm_vk_align(offset, staging_buffer->alignment);
}



