/**
Copyright 2021,2022 Carl van Mastrigt

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


void cvm_vk_managed_buffer_create(cvm_vk_managed_buffer * mb,uint32_t buffer_size,uint32_t min_size_factor,VkBufferUsageFlags usage,bool multithreaded,bool host_visible)
{
    uint32_t i,next;

    usage|=VK_BUFFER_USAGE_TRANSFER_DST_BIT;///for when its necessary to copy into the buffer (device only memory)

    buffer_size=(((buffer_size-1) >> min_size_factor)+1) << min_size_factor;///round to multiple of 1<<min_size_factor
    mb->total_space=buffer_size;
    mb->permanent_offset=buffer_size;
    mb->temporary_offset=0;

    mb->temporary_allocation_data_space=128;
    mb->temporary_allocation_data=malloc(mb->temporary_allocation_data_space*sizeof(cvm_vk_temporary_buffer_allocation_data));

    i=mb->temporary_allocation_data_space;
    next=CVM_VK_INVALID_TEMPORARY_ALLOCATION;
    while(i--)
    {
        mb->temporary_allocation_data[i].next=next;
        next=i;
    }

    mb->first_unused_temporary_allocation=0;///put all allocations in linked list of unused allocations
    mb->unused_temporary_allocation_count=mb->temporary_allocation_data_space;
    mb->last_used_allocation=CVM_VK_INVALID_TEMPORARY_ALLOCATION;

    for(i=0;i<32;i++)
    {
        mb->available_temporary_allocations[i].heap=malloc(sizeof(uint32_t)*16);
        mb->available_temporary_allocations[i].space=16;
        mb->available_temporary_allocations[i].count=0;
    }
    mb->available_temporary_allocation_bitmask=0;

    mb->base_temporary_allocation_size_factor=min_size_factor;

    mb->multithreaded=multithreaded;
    atomic_init(&mb->acquire_spinlock,0);

    mb->mapping=cvm_vk_create_buffer(&mb->buffer,&mb->memory,usage,buffer_size,host_visible);///if this is non-null then probably UMA and thus can circumvent staging buffer

    atomic_init(&mb->copy_spinlock,0);

    #warning should have a runtime method to check if following is necessary, will be a decent help in setting up module buffer sizes
    mb->staging_buffer=NULL;

    cvm_vk_buffer_copy_list_ini(&mb->pending_copies);

    cvm_vk_buffer_barrier_list_ini(&mb->copy_release_barriers);
    cvm_vk_buffer_barrier_list_ini(&mb->copy_acquire_barriers);

    mb->copy_update_counter=0;
    mb->copy_delay=(mb->mapping==NULL);///this shit needs serious review

    mb->copy_src_queue_family=cvm_vk_get_transfer_queue_family();
    mb->copy_dst_queue_family=cvm_vk_get_graphics_queue_family();///will have to add a way to use compute here
}

void cvm_vk_managed_buffer_destroy(cvm_vk_managed_buffer * mb)
{
    uint32_t i;

    cvm_vk_destroy_buffer(mb->buffer,mb->memory,mb->mapping);
    ///all temporary allocations should be freed at this point
    assert(mb->last_used_allocation==CVM_VK_INVALID_TEMPORARY_ALLOCATION);///ATTEMPTED TO DESTROY A BUFFER WITH ACTIVE ELEMENTS

    for(i=0;i<32;i++)free(mb->available_temporary_allocations[i].heap);

    free(mb->temporary_allocation_data);

    cvm_vk_buffer_copy_list_del(&mb->pending_copies);

    cvm_vk_buffer_barrier_list_del(&mb->copy_release_barriers);
    cvm_vk_buffer_barrier_list_del(&mb->copy_acquire_barriers);
}

bool cvm_vk_managed_buffer_acquire_temporary_allocation(cvm_vk_managed_buffer * mb,uint64_t size,uint32_t * allocation_index,uint64_t * allocation_offset)
{
    uint32_t i,offset,size_bitmask,up,down,end,size_factor,available_bitmask,additional_allocations;///up down current are for binary heap management
    uint32_t *heap;
    uint32_t prev_index,next_index;
    cvm_vk_temporary_buffer_allocation_data *allocations,*prev_data,*next_data;
    uint_fast32_t lock;

    assert(allocation_index);

    assert(size < mb->total_space);

    ///linear search should be fine as its assumed the vast majority of buffers will have size factor less than 3 (ergo better than binary search)
    for(size_factor=0;0x0000000000000001u<<(size_factor+mb->base_temporary_allocation_size_factor) < size;size_factor++);
    assert(size_factor<32);
    if(size_factor>=32)return false;

    if(mb->multithreaded)do lock=atomic_load(&mb->acquire_spinlock);
    while(lock!=0 || !atomic_compare_exchange_weak(&mb->acquire_spinlock,&lock,1));

    /// seeing as this function must be as fast and light as possible, we should track unused allocation count to determine if there are enough unused allocations left (with allowance for inaccuracy)
    if(mb->unused_temporary_allocation_count < 32)
    {
        next_index=mb->first_unused_temporary_allocation;

        additional_allocations=mb->temporary_allocation_data_space;
        additional_allocations=(additional_allocations&~(additional_allocations>>1 | additional_allocations>>2))>>2;///additional quarter of current size rounded down to power of 2
        assert(additional_allocations>=32);
        assert(!(additional_allocations&(additional_allocations-1)));

        mb->temporary_allocation_data = allocations = realloc(mb->temporary_allocation_data,sizeof(cvm_vk_temporary_buffer_allocation_data)*(mb->temporary_allocation_data_space+additional_allocations));

        for(i=mb->temporary_allocation_data_space+additional_allocations-1;i>=mb->temporary_allocation_data_space;i--)
        {
            allocations[i].next=next_index;
            next_index=i;
        }
        mb->first_unused_temporary_allocation=next_index;///same as next

        mb->unused_temporary_allocation_count+=additional_allocations;
        mb->temporary_allocation_data_space+=additional_allocations;
    }
    else allocations=mb->temporary_allocation_data;
    ///above ensures there will be enough unused allocations for any case below (is overly conservative. but is a quick, rarely failing test)

    available_bitmask=mb->available_temporary_allocation_bitmask;

    if(available_bitmask >> size_factor)///there exists an allocation of the desired size or larger
    {
        ///find lowest size with available allocation
        for(i=size_factor;!(available_bitmask>>i&1);i++);

        heap=mb->available_temporary_allocations[i].heap;

        prev_index=heap[0];///what we want, the lowest offset available allocation of the required size or smaller
        prev_data=allocations+prev_index;

        prev_data->available=false;
        prev_data->size_factor=size_factor;

        if((end= --mb->available_temporary_allocations[i].count))///after extraction patch up the heap if its nonempty
        {
            up=0;

            while((down=(up<<1)+1)<end)
            {
                down+= (down+1<end) && (allocations[heap[down+1]].offset < allocations[heap[down]].offset);///find smallest of downwards offset
                if(allocations[heap[end]].offset < allocations[heap[down]].offset) break;///if last element (old count) has smaller offset than next down then early exit
                allocations[heap[down]].heap_index=up;///need to know (new) index in heap
                heap[up]=heap[down];
                up=down;
            }

            allocations[heap[end]].heap_index=up;
            heap[up]=heap[end];
        }
        else available_bitmask&= ~(1<<i);///otherwise, if now empty mark the heap as such

        while(i-- > size_factor)
        {
            next_index=mb->first_unused_temporary_allocation;
            next_data=allocations+next_index;

            mb->first_unused_temporary_allocation=next_data->next;
            mb->unused_temporary_allocation_count--;

            ///horizontal linked list stuff
            allocations[prev_data->next].prev=next_index;///next cannot be CVM_VK_INVALID_TEMPORARY_ALLOCATION. if it were, then this allocation would have been freed back to the pool (last available allocations are recursively freed)
            next_data->next=prev_data->next;
            next_data->prev=prev_index;
            prev_data->next=next_index;

            next_data->size_factor=i;
            next_data->offset=prev_data->offset+(1u<<i);
            next_data->available=true;
            next_data->heap_index=0;///index in heap
            ///if this allocation size wasn't empty, then this code path wouldn't be taken, ergo addition to heap is super simple

            mb->available_temporary_allocations[i].heap[0]=next_index;
            mb->available_temporary_allocations[i].count=1;

            available_bitmask|=1<<i;
        }

        mb->available_temporary_allocation_bitmask=available_bitmask;

        if(mb->multithreaded) atomic_store(&mb->acquire_spinlock,0);

        *allocation_index=prev_index;///set the allocation data
        if(allocation_offset)*allocation_offset=prev_data->offset<<mb->base_temporary_allocation_size_factor;
    }
    else///try to allocate from the end of the buffer
    {
        offset=mb->temporary_offset;///store offset locally

        size_bitmask=(1u<<size_factor)-1u;

        ///rounded offset plus allocation size must be less than end of dynamic buffer space
        if( ((uint64_t)((((offset+size_bitmask)>>size_factor)<<size_factor) + (1u<<size_factor))) << mb->base_temporary_allocation_size_factor > mb->permanent_offset)
        {
            ///not enough memory left
            if(mb->multithreaded) atomic_store(&mb->acquire_spinlock,0);
            return false;
        }
        /// ^ extra brackets to make compiler shut up :sob:

        prev_index=mb->last_used_allocation;
        next_index=mb->first_unused_temporary_allocation;///rely on next chain already existing here, but next does need to be set on last used allocation
        if(prev_index!=CVM_VK_INVALID_TEMPORARY_ALLOCATION)allocations[prev_index].next=next_index;
        next_data=allocations+next_index;

        for(i=0;offset&size_bitmask;i++) if(offset & 1<<i)
        {
            mb->unused_temporary_allocation_count--;

            next_data->offset=offset;
            next_data->prev=prev_index;///right/next will get set after the fact (after this loop), prev on available linked list has not been set, so will need to set that
            next_data->size_factor=i;
            next_data->available=true;
            next_data->heap_index=mb->available_temporary_allocations[i].count++;///by virtue of being allocated from the end it will have the largest offset, thus will never need to move up the heap
            ///add to heap

            if(next_data->heap_index==mb->available_temporary_allocations[i].space)
            {
                mb->available_temporary_allocations[i].heap=realloc(mb->available_temporary_allocations[i].heap,sizeof(uint32_t)*(mb->available_temporary_allocations[i].space+=16));
            }
            mb->available_temporary_allocations[i].heap[next_data->heap_index]=next_index;

            available_bitmask|=1<<i;

            offset+=1<<i;///increment offset to align

            prev_index=next_index;
            next_index=next_data->next;
            next_data=allocations+next_index;
        }

        mb->unused_temporary_allocation_count--;
        mb->first_unused_temporary_allocation=next_data->next;

        next_data->offset=offset;
        next_data->prev=prev_index;
        next_data->next=CVM_VK_INVALID_TEMPORARY_ALLOCATION;///last used allocation now after all
        next_data->size_factor=size_factor;
        next_data->available=false;

        mb->last_used_allocation=next_index;
        mb->temporary_offset=offset+(1<<size_factor);
        mb->available_temporary_allocation_bitmask=available_bitmask;

        if(mb->multithreaded) atomic_store(&mb->acquire_spinlock,0);

        *allocation_index=next_index;///set the allocation data
        if(allocation_offset)*allocation_offset=offset<<mb->base_temporary_allocation_size_factor;
    }

    return true;
}

void cvm_vk_managed_buffer_release_temporary_allocation(cvm_vk_managed_buffer * mb,uint32_t allocation_index)
{
    uint32_t up,down,end,size_factor,offset;
    uint32_t * heap;
    uint32_t neighbour_index;///represents neighbouring/"buddy" allocation
    cvm_vk_temporary_buffer_allocation_data * allocations;
    cvm_vk_temporary_buffer_allocation_data *allocation_data,*neighbour_data;
    uint_fast32_t lock;

    if(mb->multithreaded)do lock=atomic_load(&mb->acquire_spinlock);
    while(lock!=0 || !atomic_compare_exchange_weak(&mb->acquire_spinlock,&lock,1));

    allocations=mb->temporary_allocation_data;

    if(allocation_index==mb->last_used_allocation)///could also check data.next==CVM_VK_INVALID_TEMPORARY_ALLOCATION
    {
        ///free space from end until an unavailable allocation is found
        ///need handling for only 1 alloc, which is rightmost

        allocations[allocation_index].next=mb->first_unused_temporary_allocation;///beyond this we can rely on the extant next

        mb->first_unused_temporary_allocation=allocation_index;
        allocation_index=allocations[allocation_index].prev;
        mb->unused_temporary_allocation_count++;

        while(allocation_index!=CVM_VK_INVALID_TEMPORARY_ALLOCATION && allocations[allocation_index].available)
        {
            size_factor=allocations[allocation_index].size_factor;
            ///remove from heap, index itself cannot have any children as it's the allocation with the largest offset (it's at the end), but the one we replace it with may, so no need to check down when adding
            if((end= --mb->available_temporary_allocations[size_factor].count))
            {
                heap=mb->available_temporary_allocations[size_factor].heap;

                down=allocations[allocation_index].heap_index;
                while(down && allocations[heap[end]].offset < allocations[heap[(up=(down-1)>>1)]].offset)
                {
                    allocations[heap[up]].heap_index=down;
                    heap[down]=heap[up];
                    down=up;
                }

                allocations[heap[end]].heap_index=down;
                heap[down]=heap[end];
            }
            else mb->available_temporary_allocation_bitmask&= ~(1<<size_factor);///just removed last available allocation of this size

            mb->first_unused_temporary_allocation=allocation_index;
            allocation_index=allocations[allocation_index].prev;
            mb->unused_temporary_allocation_count++;
        }

        if(allocation_index!=CVM_VK_INVALID_TEMPORARY_ALLOCATION)allocations[allocation_index].next=CVM_VK_INVALID_TEMPORARY_ALLOCATION;
        mb->last_used_allocation=allocation_index;

        mb->temporary_offset=allocations[mb->first_unused_temporary_allocation].offset;///set based on last added, which will be end of removed, a little hacky but w/e
    }
    else /// the difficult one...
    {
        allocation_data=allocations+allocation_index;
        size_factor=allocation_data->size_factor;
        offset=allocation_data->offset;

        while(size_factor+1u < 32)/// combine allocations in aligned way
        {
            ///neighbouring allocation for this alignment
            neighbour_index = offset & 1<<size_factor ? allocation_data->prev : allocation_data->next;/// will never be NULL (if a was last other branch would be taken, if first (offset is 0) n will always be set to next)
            neighbour_data=allocations+neighbour_index;

            if(!neighbour_data->available || neighbour_data->size_factor!=size_factor)break;///an size factor must be the same size or smaller, can only combine if the same size and available

            ///remove n from its availability heap
            ///n's size factor must be same as size factor here
            if((end= --mb->available_temporary_allocations[size_factor].count))
            {
                heap=mb->available_temporary_allocations[size_factor].heap;

                down=neighbour_data->heap_index;
                ///try moving up
                while(down && allocations[heap[end]].offset < allocations[heap[(up=(down-1)>>1)]].offset)
                {
                    allocations[heap[up]].heap_index=down;
                    heap[down]=heap[up];
                    down=up;
                }
                ///try moving down
                up=down;
                while((down=(up<<1)+1)<end)
                {
                    down+= (down+1<end) && (allocations[heap[down+1]].offset < allocations[heap[down]].offset);
                    if(allocations[heap[end]].offset < allocations[heap[down]].offset) break;///will break on first cycle if above loop did anything
                    allocations[heap[down]].heap_index=up;
                    heap[up]=heap[down];
                    up=down;
                }

                allocations[heap[end]].heap_index=up;
                heap[up]=heap[end];
            }
            else mb->available_temporary_allocation_bitmask&= ~(1<<size_factor);///just removed last available allocation of this size

            ///change linked list/ adjacency information
            if(offset&1<<size_factor) /// a is next/right
            {
                offset-=1<<size_factor;
                ///this should be rare enough to set next and prev during loop and still be the most efficient option
                allocation_data->prev=neighbour_data->prev;
                if(allocation_data->prev!=CVM_VK_INVALID_TEMPORARY_ALLOCATION) allocations[allocation_data->prev].next=allocation_index;///using prev set above
            }
            else allocations[(allocation_data->next=neighbour_data->next)].prev=allocation_index;///next CANNOT be NULL, as last allocation cannot be available (and thus break above would have been hit)

            size_factor++;

            neighbour_data->next=mb->first_unused_temporary_allocation;///put subsumed adjacent allocation in unused list
            mb->first_unused_temporary_allocation=neighbour_index;
            mb->unused_temporary_allocation_count++;
        }

        allocation_data->size_factor=size_factor;
        allocation_data->offset=offset;
        allocation_data->available=true;

        /// add a to appropriate availability heap
        down=mb->available_temporary_allocations[size_factor].count++;///by virtue of being allocated from the end it will have the largest offset, thus will never need to move up the heap

        heap=mb->available_temporary_allocations[size_factor].heap;
        if(down==mb->available_temporary_allocations[size_factor].space)
        {
            heap=mb->available_temporary_allocations[size_factor].heap=realloc(heap,sizeof(uint32_t)*(mb->available_temporary_allocations[size_factor].space+=16));
        }

        while(down && offset < allocations[heap[(up=(down-1)>>1)]].offset)
        {
            allocations[heap[up]].heap_index=down;
            heap[down]=heap[up];
            down=up;
        }

        allocation_data->heap_index=down;
        heap[down]=allocation_index;

        mb->available_temporary_allocation_bitmask|=1<<size_factor;
    }

    if(mb->multithreaded) atomic_store(&mb->acquire_spinlock,0);
}

uint64_t cvm_vk_managed_buffer_acquire_permanent_allocation(cvm_vk_managed_buffer * mb,uint64_t size,uint64_t alignment)
{
    uint64_t permanent_offset,temporary_offset;
    uint_fast32_t lock;

    if(mb->multithreaded)do lock=atomic_load(&mb->acquire_spinlock);
    while(lock!=0 || !atomic_compare_exchange_weak(&mb->acquire_spinlock,&lock,1));

    ///allocating at desired alignment from end of buffer, can simply "round down"
    ///don't require power of 2 alignment (useful for base vertex use cases, so can index different sections of buffer w/o binding to different slot)

    permanent_offset=mb->permanent_offset;
    ///make sure there's enough space in the buffer to begin with (and avoid underflow errors), set error otherwise
    ///also would be interpreted as an error is 0 space remained so don't allocate ALL of the remaining space
    if(permanent_offset<=size)permanent_offset=0;
    else
    {
        permanent_offset-=size;
        permanent_offset-=permanent_offset%alignment;///align allocation properly

        temporary_offset=((uint64_t)mb->temporary_offset)<<mb->base_temporary_allocation_size_factor;///the other end of the buffer contains a dynamic allocator, need to check this allocation wouldn't overlap with that
        if(permanent_offset<temporary_offset) permanent_offset=0;///ensure there's enough space available, set error otherwise
    }

    ///if there's precisely 0 space left that would be interpreted as an error so don't make that wasted/untracked allocation (as well as don't allocate upon error)
    if(permanent_offset) mb->permanent_offset=permanent_offset;///if allocation was valid make it official

    if(mb->multithreaded) atomic_store(&mb->acquire_spinlock,0);

    return permanent_offset;
}

static inline void * stage_copy_action(cvm_vk_managed_buffer * mb,uint64_t offset,uint64_t size,VkPipelineStageFlags2 stage_mask,VkAccessFlags2 access_mask)
{
    VkDeviceSize staging_offset;
    uint_fast32_t lock;
    void * ptr;

    assert(mb->staging_buffer);///ATTEMPTED TO MAP DEVICE LOCAL MEMORY WITHOUT SETTING A STAGING BUFFER
    assert(size <= mb->staging_buffer->total_space);///ATTEMPTED TO UPLOAD MORE DATA THEN ASSIGNED STAGING BUFFER CAN ACCOMODATE

    ptr=cvm_vk_staging_buffer_acquire_space(mb->staging_buffer,size,&staging_offset);

    if(!ptr) return NULL;

    if(mb->multithreaded)do lock=atomic_load(&mb->copy_spinlock);
    while(lock!=0 || !atomic_compare_exchange_weak(&mb->copy_spinlock,&lock,1));

    *cvm_vk_buffer_copy_list_new(&mb->pending_copies)=(VkBufferCopy)
    {
        .srcOffset=staging_offset,
        .dstOffset=offset,
        .size=size
    };

    *cvm_vk_buffer_barrier_list_new(&mb->copy_release_barriers)=(VkBufferMemoryBarrier2)
    {
        .sType=VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
        .pNext=NULL,
        .srcStageMask=VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .srcAccessMask=VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .dstStageMask=stage_mask,
        .dstAccessMask=access_mask,/// staging/copy only ever writes
        .srcQueueFamilyIndex=mb->copy_src_queue_family,
        .dstQueueFamilyIndex=mb->copy_dst_queue_family,
        .buffer=mb->buffer,
        .offset=offset,
        .size=size
    };

    if(mb->multithreaded) atomic_store(&mb->copy_spinlock,0);

    return ptr;
}

void * cvm_vk_managed_buffer_get_permanent_allocation_mapping(cvm_vk_managed_buffer * mb,uint64_t offset,uint64_t size,VkPipelineStageFlags2 stage_mask,VkAccessFlags2 access_mask,uint16_t * availability_token)
{
    *availability_token=mb->copy_update_counter;///could technically go after the early exit as mapped buffers (so far) don't need to be transferre

    if(mb->mapping) return mb->mapping+offset;///on UMA can access memory directly!

    return stage_copy_action(mb,offset,size,stage_mask,access_mask);
}

void * cvm_vk_managed_buffer_get_temporary_allocation_mapping(cvm_vk_managed_buffer * mb,uint64_t offset,uint64_t size,VkPipelineStageFlags2 stage_mask,VkAccessFlags2 access_mask,uint16_t * availability_token)
{
    *availability_token=mb->copy_update_counter;///could technically go after the early exit as mapped buffers (so far) don't need to be transferre

    if(mb->mapping) return mb->mapping+offset;///on UMA can access memory directly!

    return stage_copy_action(mb,offset,size,stage_mask,access_mask);
}


void cvm_vk_managed_buffer_submit_all_acquire_barriers(cvm_vk_managed_buffer * mb,VkCommandBuffer graphics_cb)
{
    if(mb->copy_acquire_barriers.count)
    {
        assert(!mb->mapping);
        VkDependencyInfo copy_aquire_dependencies=
        {
            .sType=VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext=NULL,
            .dependencyFlags=0,
            .memoryBarrierCount=0,
            .pMemoryBarriers=NULL,
            .bufferMemoryBarrierCount=mb->copy_acquire_barriers.count,
            .pBufferMemoryBarriers=mb->copy_acquire_barriers.list,
            .imageMemoryBarrierCount=0,
            .pImageMemoryBarriers=NULL
        };
        vkCmdPipelineBarrier2(graphics_cb,&copy_aquire_dependencies);

        ///rest now that we're done with it
        mb->copy_acquire_barriers.count=0;
    }
}

void cvm_vk_managed_buffer_submit_all_pending_copy_actions(cvm_vk_managed_buffer * mb,VkCommandBuffer transfer_cb)
{
    uint_fast32_t lock;
    cvm_vk_buffer_barrier_list tmp;

    mb->copy_update_counter++;

    #warning need to check VK_MEMORY_PROPERTY_HOST_COHERENT to see if flushes are actually necessary
    if(mb->mapping)/// is UMA system, no staging copies necessary
    {
        VkMappedMemoryRange flush_range=(VkMappedMemoryRange)
        {
            .sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            .pNext=NULL,
            .memory=mb->memory,
            .offset=0,
            .size=VK_WHOLE_SIZE
        };

        cvm_vk_flush_buffer_memory_range(&flush_range);
    }
    else
    {
        if(mb->pending_copies.count)
        {
            ///multithreading consideration shouldnt be necessary as this should only ever be called from the main thread relevant to this resource while no worker threads are operating on the resource

            vkCmdCopyBuffer(transfer_cb,mb->staging_buffer->buffer,mb->buffer,mb->pending_copies.count,mb->pending_copies.list);
            mb->pending_copies.count=0;

            if(mb->copy_src_queue_family!=mb->copy_dst_queue_family)///first barrier is only needed when QFOT is necessary
            {
                VkDependencyInfo copy_release_dependencies=
                {
                    .sType=VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                    .pNext=NULL,
                    .dependencyFlags=0,
                    .memoryBarrierCount=0,
                    .pMemoryBarriers=NULL,
                    .bufferMemoryBarrierCount=mb->copy_release_barriers.count,
                    .pBufferMemoryBarriers=mb->copy_release_barriers.list,
                    .imageMemoryBarrierCount=0,
                    .pImageMemoryBarriers=NULL
                };
                vkCmdPipelineBarrier2(transfer_cb,&copy_release_dependencies);
            }

            tmp=mb->copy_acquire_barriers;
            mb->copy_acquire_barriers=mb->copy_release_barriers;
            mb->copy_release_barriers=tmp;
        }
    }
}



void cvm_vk_managed_buffer_bind_as_vertex(VkCommandBuffer cmd_buf,cvm_vk_managed_buffer * mb,uint32_t binding)
{
    /// vertex offset in draw assumes all verts have same offset, so it makes no sense in this paradigm to have multiple binding points from a single buffer!
    #warning the above offset issue may become a MASSIVE hassle when isntance data gets involved, base-indexed vertex offsets may not be viable anymore
    ///     ^ actually does have instanceOffset...

    VkDeviceSize offset=0;
    vkCmdBindVertexBuffers(cmd_buf,binding,1,&mb->buffer,&offset);
}

void cvm_vk_managed_buffer_bind_as_index(VkCommandBuffer cmd_buf,cvm_vk_managed_buffer * mb,VkIndexType type)
{
    vkCmdBindIndexBuffer(cmd_buf,mb->buffer,0,type);
}











void cvm_vk_staging_buffer_create(cvm_vk_staging_buffer * sb,VkBufferUsageFlags usage)
{
    atomic_init(&sb->active_offset,0);
    sb->total_space=0;
    sb->mapping=NULL;
    sb->buffer=VK_NULL_HANDLE;
    sb->memory=VK_NULL_HANDLE;
    sb->usage=usage;

    sb->active_offset=0;
    sb->active_region.start=0;
    sb->active_region.end=0;

    sb->available_region_count=0;
    sb->acquired_regions=NULL;
    sb->available_regions=NULL;

    uint32_t required_alignment=cvm_vk_get_buffer_alignment_requirements(usage);

    for(sb->alignment_size_factor=0;1u<<sb->alignment_size_factor < required_alignment;sb->alignment_size_factor++);///alignment_size_factor expected to be small, ~6
    ///can catastrophically fail if required_alignment is greater than 2^31, but w/e

    assert(1u<<sb->alignment_size_factor == required_alignment);///NON POWER OF 2 ALIGNMENTS NOT SUPPORTED
}

void cvm_vk_staging_buffer_update(cvm_vk_staging_buffer * sb,uint32_t space_per_frame, uint32_t frame_count)
{
    assert(sb->available_region_count==0 || (sb->available_region_count==1 && sb->available_regions[0].start==0 && sb->available_regions[0].end==sb->total_space));///NOT ALL STAGING BUFFER SPACE RELINQUISHED BEFORE RECREATION
    assert(!(space_per_frame & ((1<<sb->alignment_size_factor)-1)));///REQUESTED UNALIGNED STAGING SIZE

    uint32_t i,buffer_size=space_per_frame*frame_count;

    if(frame_count)
    {
        sb->acquired_regions=realloc(sb->acquired_regions,sizeof(cvm_vk_staging_buffer_region)*frame_count);
        for(i=0;i<frame_count;i++)sb->acquired_regions[i]=(cvm_vk_staging_buffer_region){.start=0,.end=0};
        sb->available_regions=realloc(sb->available_regions,sizeof(cvm_vk_staging_buffer_region)*frame_count);
        sb->available_region_count=1;
        sb->available_regions[0].start=0;
        sb->available_regions[0].end=buffer_size;
    }

    if(buffer_size != sb->total_space)
    {
        if(sb->total_space)
        {
            cvm_vk_destroy_buffer(sb->buffer,sb->memory,sb->mapping);
        }

        sb->mapping=cvm_vk_create_buffer(&sb->buffer,&sb->memory,sb->usage,buffer_size,true);

        atomic_store(&sb->active_offset,0);
        sb->active_region.start=0;///doesn't really need to be set here
        sb->active_region.end=0;///doesn't really need to be set here
        sb->total_space=buffer_size;
    }
}

void cvm_vk_staging_buffer_destroy(cvm_vk_staging_buffer * sb)
{
    assert(sb->available_region_count==0 || (sb->available_region_count==1 && sb->available_regions[0].start==0 && sb->available_regions[0].end==sb->total_space));///NOT ALL STAGING BUFFER SPACE RELINQUISHED BEFORE RECREATION

    free(sb->acquired_regions);
    free(sb->available_regions);

    cvm_vk_destroy_buffer(sb->buffer,sb->memory,sb->mapping);
}

uint32_t cvm_vk_staging_buffer_get_rounded_allocation_size(cvm_vk_staging_buffer * sb,uint32_t allocation_size)
{
    return (((allocation_size-1)>>sb->alignment_size_factor)+1)<<sb->alignment_size_factor;
}

void cvm_vk_staging_buffer_begin(cvm_vk_staging_buffer * sb)
{
    uint32_t current_region_size,largest_region_size=0;
    cvm_vk_staging_buffer_region *largest,*current,*end;
    largest=NULL;

    for(current=sb->available_regions,end=sb->available_regions+sb->available_region_count ; current<end ; current++)
    {
        assert(current->start!=current->end);/// CANNOT HAVE NULL REGION OR LOOPING REGION THAT STARTS MIDWAY

        if(current->end > current->start) current_region_size = current->end - current->start;
        else current_region_size = sb->total_space + current->end - current->start;

        if(current_region_size > largest_region_size)
        {
            largest=current;
            largest_region_size=current_region_size;
        }
    }

    if(largest_region_size)
    {
        sb->active_region=*largest;
        *largest=sb->available_regions[--sb->available_region_count];
    }
    else
    {
        sb->active_region.start=0;
        sb->active_region.end=0;
    }

    atomic_store(&sb->active_offset,sb->active_region.start);
}

void cvm_vk_staging_buffer_end(cvm_vk_staging_buffer * sb,uint32_t frame_index)
{
    uint_fast32_t offset;
    uint32_t start;

    offset=atomic_load(&sb->active_offset);
    start=sb->active_region.start;

    if(offset < start)///if the buffer wrapped between begin and end
    {
        #warning need to check VK_MEMORY_PROPERTY_HOST_COHERENT to see if flushes are actually necessary

        VkMappedMemoryRange flush_range=(VkMappedMemoryRange)
        {
            .sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            .pNext=NULL,
            .memory=sb->memory,
            .offset=start,
            .size=sb->total_space-start
        };

        cvm_vk_flush_buffer_memory_range(&flush_range);
        start=0;
    }

    if(start<offset)///if anything was written that wasn't handled by the previous branch (i.e. we've looped around to the start o the staging buffer this frame)
    {
        #warning need to check VK_MEMORY_PROPERTY_HOST_COHERENT to see if flushes are actually necessary

        VkMappedMemoryRange flush_range=(VkMappedMemoryRange)
        {
            .sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            .pNext=NULL,
            .memory=sb->memory,
            .offset=start,
            .size=offset
        };

        cvm_vk_flush_buffer_memory_range(&flush_range);
    }


    if(offset==sb->active_region.start) sb->acquired_regions[frame_index]=(cvm_vk_staging_buffer_region){.start=0,.end=0};
    else sb->acquired_regions[frame_index]=(cvm_vk_staging_buffer_region){.start=sb->active_region.start,.end=offset};

    if(offset!=sb->active_region.end)sb->available_regions[sb->available_region_count++]=(cvm_vk_staging_buffer_region){.start=offset,.end=sb->active_region.end};

    sb->active_region.start=0;
    sb->active_region.end=0;
}

void * cvm_vk_staging_buffer_acquire_space(cvm_vk_staging_buffer * sb,uint32_t allocation_size,VkDeviceSize * acquired_offset)
{
    allocation_size=(((allocation_size-1)>>sb->alignment_size_factor)+1)<<sb->alignment_size_factor;///round as required

    uint_fast32_t old_offset,new_offset,offset;

    old_offset=atomic_load(&sb->active_offset);
    do
    {
        if(sb->active_region.end < sb->active_region.start)
        {
            if(old_offset+allocation_size > sb->total_space)
            {
                if(allocation_size>sb->active_region.end)return NULL;
                offset=0;
                new_offset=allocation_size;
            }
            else
            {
                offset=old_offset;
                new_offset=old_offset+allocation_size;
            }
        }
        else
        {
            if(old_offset+allocation_size > sb->active_region.end)return NULL;
            offset=old_offset;
            new_offset=old_offset+allocation_size;
        }
    }
    while(!atomic_compare_exchange_weak(&sb->active_offset,&old_offset,new_offset));

    *acquired_offset=offset;
    return sb->mapping+offset;
}

///should only ever be called during cleanup, not thread safe
void cvm_vk_staging_buffer_release_space(cvm_vk_staging_buffer * sb,uint32_t frame_index)
{
    cvm_vk_staging_buffer_region region;
    uint32_t i;

    region=sb->acquired_regions[frame_index];

    sb->acquired_regions[frame_index]=(cvm_vk_staging_buffer_region){.start=0,.end=0};

    if(region.end!=region.start)
    {
        i=0;
        while(i<sb->available_region_count)
        {
            if(sb->available_regions[i].start==region.end)
            {
                region.end=sb->available_regions[i].end;
                sb->available_regions[i]=sb->available_regions[--sb->available_region_count];
                i=0;
            }
            else if(sb->available_regions[i].end==region.start)
            {
                region.start=sb->available_regions[i].start;
                sb->available_regions[i]=sb->available_regions[--sb->available_region_count];
                i=0;
            }
            else i++;
        }

        ///cannot be a false complete allocation because required non-empty region to start with
        if(region.end==region.start)
        {
            assert(!sb->available_region_count);///FALSE COMPLETE REGION DETECTED
            region=(cvm_vk_staging_buffer_region){.start=0,.end=sb->total_space};
        }

        sb->available_regions[sb->available_region_count++]=region;
    }
}







void cvm_vk_transient_buffer_create(cvm_vk_transient_buffer * tb,VkBufferUsageFlags usage)
{
    atomic_init(&tb->current_offset,0);
    tb->total_space=0;
    tb->space_per_frame=0;
    tb->max_offset=0;
    tb->mapping=NULL;
    tb->buffer=VK_NULL_HANDLE;
    tb->memory=VK_NULL_HANDLE;
    tb->usage=usage;

    uint32_t required_alignment=cvm_vk_get_buffer_alignment_requirements(usage);

    for(tb->alignment_size_factor=0;1u<<tb->alignment_size_factor < required_alignment;tb->alignment_size_factor++);///alignment_size_factor expected to be small, ~6
    ///can catastrophically fail if required_alignment is greater than 2^31, but w/e

    assert(1u<<tb->alignment_size_factor == required_alignment);///NON POWER OF 2 ALIGNMENTS NOT SUPPORTED (AND SHOULDN"T BE RETURNED BY VULKAN)
}

void cvm_vk_transient_buffer_update(cvm_vk_transient_buffer * tb,uint32_t space_per_frame, uint32_t frame_count)
{
    uint32_t total_space=space_per_frame*frame_count;

    assert(!tb->max_offset);///TRYING TO RECREATE TRANSIENT BUFFER THAT IS STILL IN USE
    assert(!(space_per_frame & ((1<<tb->alignment_size_factor)-1)));///REQUESTED UNALIGNED TRANSIENT STAGING SIZE

    if(total_space != tb->total_space)///recreate buffer
    {
        if(tb->total_space)
        {
            cvm_vk_destroy_buffer(tb->buffer,tb->memory,tb->mapping);
            tb->mapping=NULL;
        }

        if(total_space)
        {
            tb->mapping=cvm_vk_create_buffer(&tb->buffer,&tb->memory,tb->usage,total_space,true);
        }
    }

    atomic_store(&tb->current_offset,0);
    tb->total_space=total_space;
    tb->space_per_frame=space_per_frame;
    tb->max_offset=0;
}

void cvm_vk_transient_buffer_destroy(cvm_vk_transient_buffer * tb)
{
    assert(!tb->max_offset);///TRYING TO DESTROY TRANSIENT BUFFER THAT IS STILL IN USE

    cvm_vk_destroy_buffer(tb->buffer,tb->memory,tb->mapping);
}

uint32_t cvm_vk_transient_buffer_get_rounded_allocation_size(cvm_vk_transient_buffer * tb,uint32_t allocation_size,uint32_t alignment)
{
    /// if aligned to base (alignment==0) then just need enough to round up as that should cover itself
    /// if aligned to own alignment, need enough to align to oneself and to align next (potentially base aligned) allocation

    ///base alignment requirements handle specific alignment requirements when base alignment is a multiple of specific alignment, and base alignment is always a Po2 so in this case specific must also be smaller Po2
    if( !(alignment&(alignment-1)) && alignment<(1u<<tb->alignment_size_factor) )alignment=0;

    return (((allocation_size+alignment-1)>>tb->alignment_size_factor)+1)<<tb->alignment_size_factor;
}

void cvm_vk_transient_buffer_begin(cvm_vk_transient_buffer * tb,uint32_t frame_index)
{
    tb->max_offset=(frame_index+1)*tb->space_per_frame;
    atomic_store(&tb->current_offset,frame_index*tb->space_per_frame);
}

void cvm_vk_transient_buffer_end(cvm_vk_transient_buffer * tb)
{
    uint32_t acquired_space=tb->space_per_frame - (tb->max_offset-atomic_load(&tb->current_offset)); /// max space - space remaining

    if(acquired_space)
    {
        #warning need to check VK_MEMORY_PROPERTY_HOST_COHERENT to see if flushes are actually necessary

        acquired_space=(((acquired_space-1)>>tb->alignment_size_factor)+1)<<tb->alignment_size_factor;

        VkMappedMemoryRange flush_range=(VkMappedMemoryRange)
        {
            .sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            .pNext=NULL,
            .memory=tb->memory,
            .offset=tb->max_offset-tb->space_per_frame,
            .size=acquired_space
        };

        cvm_vk_flush_buffer_memory_range(&flush_range);
    }

    tb->max_offset=0;///indicates this buffer is no longer in use
}

void * cvm_vk_transient_buffer_get_allocation(cvm_vk_transient_buffer * tb,uint32_t allocation_size,uint32_t alignment,VkDeviceSize * acquired_offset)
{
    VkDeviceSize offset;
    uint_fast32_t old_offset,new_offset;

    old_offset=atomic_load(&tb->current_offset);
    do
    {
        offset=(old_offset+alignment-1);
        if(alignment) offset-=offset%alignment;
        else offset=((offset>>tb->alignment_size_factor)+1)<<tb->alignment_size_factor;

        new_offset=offset+allocation_size;
        if(new_offset>tb->max_offset)
        {
            return NULL;
        }
    }
    while(!atomic_compare_exchange_weak(&tb->current_offset,&old_offset,new_offset));

    *acquired_offset=offset;
    return tb->mapping+offset;
}

void cvm_vk_transient_buffer_bind_as_vertex(VkCommandBuffer cmd_buf,cvm_vk_transient_buffer * tb,uint32_t binding,VkDeviceSize offset)
{
    vkCmdBindVertexBuffers(cmd_buf,binding,1,&tb->buffer,&offset);
}

void cvm_vk_transient_buffer_bind_as_index(VkCommandBuffer cmd_buf,cvm_vk_transient_buffer * tb,VkIndexType type,VkDeviceSize offset)
{
    vkCmdBindIndexBuffer(cmd_buf,tb->buffer,offset,type);
}

