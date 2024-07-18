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



void cvm_vk_staging_buffer_initialise(cvm_vk_staging_buffer_ * staging_buffer, cvm_vk_device * device, VkBufferUsageFlags usage, VkDeviceSize buffer_size, VkDeviceSize reserved_high_priority_space)
{
    const VkDeviceSize alignment=cvm_vk_buffer_alignment_requirements(device, usage);

    buffer_size=cvm_vk_align(buffer_size, alignment);/// round to multiple of alignment

    cvm_vk_buffer_memory_pair_setup buffer_setup=(cvm_vk_buffer_memory_pair_setup)
    {
            /// in
        .buffer_size=buffer_size,
        .usage=usage,
        .required_properties=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        .desired_properties=VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        .map_memory=true,
    };

    cvm_vk_buffer_memory_pair_create(device, &buffer_setup);

    assert(buffer_setup.buffer != VK_NULL_HANDLE && buffer_setup.memory != VK_NULL_HANDLE && buffer_setup.mapping != NULL);/// for now requre success

    staging_buffer->buffer=buffer_setup.buffer;
    staging_buffer->memory=buffer_setup.memory;
    staging_buffer->mapping=buffer_setup.mapping;
    staging_buffer->mapping_coherent=buffer_setup.mapping_coherent;

    staging_buffer->usage=usage;
    staging_buffer->alignment=alignment;

    staging_buffer->threads_waiting_on_semaphore_setup=false;

    staging_buffer->buffer_size=buffer_size;
    staging_buffer->reserved_high_priority_space=reserved_high_priority_space;
    staging_buffer->current_offset=0;
    staging_buffer->remaining_space=buffer_size;

    mtx_init(&staging_buffer->access_mutex,mtx_plain);
    cnd_init(&staging_buffer->setup_stall_condition);

    staging_buffer->segment_space=16;
    staging_buffer->active_segments=malloc(sizeof(cvm_vk_staging_buffer_segment)*staging_buffer->segment_space);
    staging_buffer->first_active_segment_index=0;
    staging_buffer->active_segment_count=0;
}

void cvm_vk_staging_buffer_terminate(cvm_vk_staging_buffer_ * staging_buffer, cvm_vk_device * device)
{
    const uint32_t segment_mask = staging_buffer->segment_space-1;/// can be const here because the access mutex is not unlocked during this function
    cvm_vk_staging_buffer_segment * oldest_active_segment;

    /// wait for all memory uses to complete, must be externally synchronised to ensure buffer is not in use elsewhere
    while(staging_buffer->active_segment_count)
    {
        oldest_active_segment = staging_buffer->active_segments+(staging_buffer->first_active_segment_index&segment_mask);

        assert(oldest_active_segment->moment_of_last_use.semaphore != VK_NULL_HANDLE);///cant try to terminate while still in use
        assert(oldest_active_segment->size);///segment must occupy space

        cvm_vk_timeline_semaphore_moment_wait(device,&oldest_active_segment->moment_of_last_use);

        assert(oldest_active_segment->offset == (staging_buffer->current_offset+staging_buffer->remaining_space) % staging_buffer->buffer_size);/// out of order free for some reason, or offset mismatch

        staging_buffer->remaining_space += oldest_active_segment->size;///relinquish this segments space space

        staging_buffer->first_active_segment_index++;
        staging_buffer->active_segment_count--;
    }

    assert(staging_buffer->remaining_space==staging_buffer->buffer_size);

    mtx_destroy(&staging_buffer->access_mutex);
    cnd_destroy(&staging_buffer->setup_stall_condition);
    free(staging_buffer->active_segments);

    cvm_vk_buffer_memory_pair_destroy(device, staging_buffer->buffer, staging_buffer->memory, true);
}


#warning consider moving this back into the main function?
static inline void cvm_vk_staging_buffer_query_allocations(cvm_vk_staging_buffer_ * staging_buffer, cvm_vk_device * device)
{
    const uint32_t segment_mask = staging_buffer->segment_space-1;/// can be const here because the access mutex is not unlocked during this function

    cvm_vk_staging_buffer_segment * oldest_active_segment;

    while(staging_buffer->active_segment_count)
    {
        oldest_active_segment = staging_buffer->active_segments+(staging_buffer->first_active_segment_index&segment_mask);

        if(oldest_active_segment->moment_of_last_use.semaphore == VK_NULL_HANDLE) return; /// oldest segment has not been "completed"

        assert(oldest_active_segment->size);///segment must occupy space

        if(!cvm_vk_timeline_semaphore_moment_query(device,&oldest_active_segment->moment_of_last_use)) return; /// oldest segment is still in use by command_buffer/queue

        assert(oldest_active_segment->offset == (staging_buffer->current_offset+staging_buffer->remaining_space) % staging_buffer->buffer_size);/// out of order free for some reason, or offset mismatch

        staging_buffer->remaining_space += oldest_active_segment->size;///relinquish this segments space space

        staging_buffer->first_active_segment_index++;
        staging_buffer->active_segment_count--;

        if(staging_buffer->remaining_space==staging_buffer->buffer_size)
        {
            assert(staging_buffer->active_segment_count==0);
            /// should the whole buffer become available, reset offset to 0 so that we can use the buffer more efficiently (try to avoid wrap)
            staging_buffer->current_offset=0;
        }
    }
}

cvm_vk_staging_buffer_allocation cvm_vk_staging_buffer_reserve_allocation(cvm_vk_staging_buffer_ * staging_buffer, cvm_vk_device * device, VkDeviceSize requested_space, bool high_priority)
{
    cvm_vk_staging_buffer_allocation allocation;
    VkDeviceSize required_space;
    bool wrap;
    cvm_vk_staging_buffer_segment * oldest_active_segment;
    cvm_vk_staging_buffer_segment *src;
    cvm_vk_timeline_semaphore_moment oldest_moment;
    uint32_t segment_index,segment_count,masked_first_segment_index;

    /// this design may allow continuous small allocations to effectively stall a large allocation until the whole buffer is full...
    requested_space = cvm_vk_align(requested_space, staging_buffer->alignment);

    assert(requested_space < staging_buffer->buffer_size);/// REALLY want make max allocation 1/4 total space...

    mtx_lock(&staging_buffer->access_mutex);

    while(true)
    {
        cvm_vk_staging_buffer_query_allocations(staging_buffer,device);

        wrap = staging_buffer->current_offset+requested_space > staging_buffer->buffer_size;

        /// if this request must wrap then we need to consume the unused section of the buffer that remains
        required_space = requested_space + wrap * (staging_buffer->buffer_size - staging_buffer->current_offset);

        assert(required_space < staging_buffer->buffer_size);

        if(required_space + high_priority * staging_buffer->reserved_high_priority_space <= staging_buffer->remaining_space)
        {
            break;/// request will fit, we're done
        }

        /// otherwise; more space required
        assert(staging_buffer->active_segment_count);///should not need more space if there are no active segments

        oldest_active_segment = staging_buffer->active_segments + (staging_buffer->first_active_segment_index & (staging_buffer->segment_space-1));

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

            mtx_unlock(&staging_buffer->access_mutex);
            /// wait for semaphore outside of mutex so as to not block inside the mutex
            cvm_vk_timeline_semaphore_moment_wait(device,&oldest_moment);

            mtx_lock(&staging_buffer->access_mutex);
        }
    }

    ///make space for new segment as required
    if(staging_buffer->active_segment_count==staging_buffer->segment_space)
    {
        staging_buffer->active_segments=realloc(staging_buffer->active_segments, sizeof(cvm_vk_staging_buffer_segment) * staging_buffer->segment_space * 2);
        /**
        need to move the correct part of the buffer to maintain (modulo) indices after resizing:
        |+++o+++|
        |---o+++++++----|
        vs
                |+++o+++|
        |+++--------o+++|
        ^ realloced segment array with alignment of relative (intended) indices/offsets
        */

        masked_first_segment_index = staging_buffer->first_active_segment_index & (staging_buffer->segment_space-1);
        /// if (relative) top bit set, then should end up in newly allocated part of the buffer
        if(staging_buffer->first_active_segment_index & staging_buffer->segment_space)///first needs to end up in second half
        {
            src = staging_buffer->active_segments + masked_first_segment_index;
            segment_count = staging_buffer->segment_space - masked_first_segment_index;
        }
        else
        {
            ///copy 0 to first to old_space
            src = staging_buffer->active_segments;
            segment_count = masked_first_segment_index;
        }

        ///destination is always src + old_array_size
        memcpy(src + staging_buffer->segment_space, src, sizeof(cvm_vk_staging_buffer_segment) * segment_count);

        staging_buffer->segment_space*=2;
    }

    /// note active segment count is incremented, also importantly this index is UNWRAPPED, so that if the segment buffer gets expanded this index will still be valid
    segment_index = staging_buffer->first_active_segment_index + staging_buffer->active_segment_count++;

    allocation.acquired_offset = wrap ? 0 : staging_buffer->current_offset;
    allocation.mapping = staging_buffer->mapping + allocation.acquired_offset;
    allocation.segment_index = segment_index;

    staging_buffer->active_segments[segment_index&(staging_buffer->segment_space-1)]=(cvm_vk_staging_buffer_segment)
    {
        .moment_of_last_use = cvm_vk_timeline_semaphore_moment_null,
        .offset = staging_buffer->current_offset,
        .size = required_space,
    };

    staging_buffer->remaining_space-=requested_space;
    staging_buffer->current_offset+=requested_space;

    if(staging_buffer->current_offset > staging_buffer->buffer_size)
    {
        /// wrap the current offset if necessary
        staging_buffer->current_offset -= staging_buffer->buffer_size;
    }

    mtx_unlock(&staging_buffer->access_mutex);

    return allocation;
}

void cvm_vk_staging_buffer_flush_allocation(const cvm_vk_staging_buffer_ * staging_buffer, const cvm_vk_device * device, const cvm_vk_staging_buffer_allocation * allocation, VkDeviceSize relative_offset, VkDeviceSize size)
{
    VkResult flush_result;

    if( ! staging_buffer->mapping_coherent)
    {
        VkMappedMemoryRange flush_range=(VkMappedMemoryRange)
        {
            .sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            .pNext=NULL,
            .memory=staging_buffer->memory,
            .offset=allocation->acquired_offset + relative_offset,
            .size=size,
        };

        flush_result=vkFlushMappedMemoryRanges(device->device, 1, &flush_range);
    }

    #warning if debug ensure mapping is called, even if it isn't necessary on this platform
}

void cvm_vk_staging_buffer_complete_allocation(cvm_vk_staging_buffer_ * staging_buffer, uint32_t allocation_segment_index, cvm_vk_timeline_semaphore_moment moment_of_last_use)
{
    mtx_lock(&staging_buffer->access_mutex);

    staging_buffer->active_segments[allocation_segment_index & (staging_buffer->segment_space-1)].moment_of_last_use=moment_of_last_use;

    if(staging_buffer->threads_waiting_on_semaphore_setup)
    {
        staging_buffer->threads_waiting_on_semaphore_setup=false;
        cnd_broadcast(&staging_buffer->setup_stall_condition);
    }

    mtx_unlock(&staging_buffer->access_mutex);
}

VkDeviceSize cvm_vk_staging_buffer_allocation_align_offset(cvm_vk_staging_buffer_ * staging_buffer, VkDeviceSize offset)
{
    #warning have base VK variant that takes usage as input
    return cvm_vk_align(offset, staging_buffer->alignment);
}





void cvm_vk_staging_shunt_buffer_initialise(cvm_vk_staging_shunt_buffer * buffer, VkDeviceSize alignment)
{
    buffer->size=CVM_MAX(4096, alignment);
    buffer->backing=malloc(buffer->size);
    buffer->alignment=alignment;
    buffer->offset=0;
}

void cvm_vk_staging_shunt_buffer_terminate(cvm_vk_staging_shunt_buffer * buffer)
{
    free(buffer->backing);
}


void cvm_vk_staging_shunt_buffer_reset(cvm_vk_staging_shunt_buffer * buffer)
{
    buffer->offset=0;
}

void * cvm_vk_staging_shunt_buffer_add_bytes(cvm_vk_staging_shunt_buffer * buffer, VkDeviceSize byte_count)
{
    #warning have max viable size and assert that we are under it!
    void * ptr;

    if(buffer->offset+byte_count > buffer->size)
    {
        buffer->backing = realloc(buffer->backing, (buffer->size*=2));
    }
    ptr=buffer->backing+buffer->offset;
    buffer->offset+=byte_count;

    return ptr;
}

VkDeviceSize cvm_vk_staging_shunt_buffer_new_segment(cvm_vk_staging_shunt_buffer * buffer)
{
    buffer->offset = cvm_vk_align(buffer->offset, buffer->alignment);
    return buffer->offset;
}

void cvm_vk_staging_shunt_buffer_copy(cvm_vk_staging_shunt_buffer * buffer, void * dst)
{
    memcpy(dst,buffer->backing,buffer->offset);
}



