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


void cvm_vk_managed_buffer_create(cvm_vk_managed_buffer * mb,uint32_t buffer_size,uint32_t min_size_factor,uint32_t max_size_factor,VkBufferUsageFlags usage,bool multithreaded,bool host_visible)
{
    uint32_t i;
    buffer_size=(((buffer_size-1) >> min_size_factor)+1) << min_size_factor;///round to multiple (complier warns about what i want as needing braces, lol)
    mb->total_space=buffer_size;
    mb->static_offset=buffer_size;
    mb->dynamic_offset=0;

    cvm_vk_dynamic_buffer_allocation * dynamic_allocations=malloc(CVM_VK_RESERVED_DYNAMIC_BUFFER_ALLOCATION_COUNT*sizeof(cvm_vk_dynamic_buffer_allocation));

    cvm_vk_dynamic_buffer_allocation * next=NULL;
    i=CVM_VK_RESERVED_DYNAMIC_BUFFER_ALLOCATION_COUNT;
    while(i--)
    {
        dynamic_allocations[i].next=next;
        dynamic_allocations[i].is_base=!i;
        next=dynamic_allocations+i;
    }

    mb->first_unused_allocation=dynamic_allocations;///put all allocations in linked list of unused allocations
    mb->unused_allocation_count=CVM_VK_RESERVED_DYNAMIC_BUFFER_ALLOCATION_COUNT;
    mb->last_used_allocation=NULL;///rightmost doesn't exist as it hasn't been allocated


    mb->num_dynamic_allocation_sizes=max_size_factor-min_size_factor;
    mb->available_dynamic_allocations=malloc(mb->num_dynamic_allocation_sizes*sizeof(cvm_vk_available_dynamic_allocation_heap));
    for(i=0;i<mb->num_dynamic_allocation_sizes;i++)
    {
        mb->available_dynamic_allocations[i].heap=malloc(16*sizeof(cvm_vk_dynamic_buffer_allocation*));
        mb->available_dynamic_allocations[i].space=16;
        mb->available_dynamic_allocations[i].count=0;
    }
    mb->available_dynamic_allocation_bitmask=0;

    mb->base_dynamic_allocation_size_factor=min_size_factor;

    mb->multithreaded=multithreaded;
    atomic_init(&mb->acquire_spinlock,0);

    mb->mapping=cvm_vk_create_buffer(&mb->buffer,&mb->memory,usage,buffer_size,host_visible);///if this is non-null then probably UMA and thus can circumvent staging buffer

    atomic_init(&mb->copy_spinlock,0);

    mb->staging_buffer=NULL;
    mb->pending_copy_actions=malloc(sizeof(VkBufferCopy)*16);
    mb->pending_copy_space=16;
    mb->pending_copy_count=0;

    mb->copy_release_barriers.barriers=malloc(sizeof(VkBufferMemoryBarrier2)*16);
    mb->copy_release_barriers.space=16;
    mb->copy_release_barriers.count=0;

    mb->copy_acquire_barriers.barriers=malloc(sizeof(VkBufferMemoryBarrier2)*16);
    mb->copy_acquire_barriers.space=16;
    mb->copy_acquire_barriers.count=0;

    mb->copy_update_counter=0;
    mb->copy_delay=mb->mapping==NULL;

    mb->copy_dst_queue_family=cvm_vk_get_graphics_queue_family();///will have to add a way to use compute here
    mb->copy_dst_queue_family=cvm_vk_get_transfer_queue_family();
}

void cvm_vk_managed_buffer_destroy(cvm_vk_managed_buffer * mb)
{
    cvm_vk_destroy_buffer(mb->buffer,mb->memory,mb->mapping);
    uint32_t i;
    cvm_vk_dynamic_buffer_allocation *first,*last,*current,*next;

    ///all dynamic allocations should be freed at this point
    assert(!mb->last_used_allocation);///ATTEMPTED TO DESTROY A BUFFER WITH ACTIVE ELEMENTS

    for(i=0;i<mb->num_dynamic_allocation_sizes;i++)free(mb->available_dynamic_allocations[i].heap);
    free(mb->available_dynamic_allocations);

    first=last=NULL;
    ///get all base allocations in their own linked list, discarding all others
    for(current=mb->first_unused_allocation;current;current=next)
    {
        next=current->next;
        if(current->is_base)
        {
            if(last) last=last->next=current;
            else first=last=current;
        }
    }
    last->next=NULL;

    for(current=first;current;current=next)
    {
        next=current->next;
        free(current);///free all base allocations (to match appropriate allocs)
    }

    free(mb->pending_copy_actions);

    free(mb->copy_release_barriers.barriers);
    free(mb->copy_acquire_barriers.barriers);
}

cvm_vk_dynamic_buffer_allocation * cvm_vk_managed_buffer_acquire_dynamic_allocation(cvm_vk_managed_buffer * mb,uint64_t size)
{
    uint32_t i,o,m,u,d,c,size_factor,available_bitmask;
    cvm_vk_dynamic_buffer_allocation ** heap;
    cvm_vk_dynamic_buffer_allocation *p,*n;
    uint_fast32_t lock;

    ///linear search should be fine as its assumed the vast majority of buffers will have size factor less than 3 (ergo better than binary search)
    for(size_factor=mb->base_dynamic_allocation_size_factor;0x0000000000000001u<<size_factor < size;size_factor++);

    size_factor-=mb->base_dynamic_allocation_size_factor;

    if(size_factor>=mb->num_dynamic_allocation_sizes)return NULL;

    if(mb->multithreaded)do lock=atomic_load(&mb->acquire_spinlock);
    while(lock!=0 || !atomic_compare_exchange_weak(&mb->acquire_spinlock,&lock,1));

    /// seeing as this function must be as fast and light as possible, we should track unused allocation count to determine if there are enough unused allocations left (with allowance for inaccuracy)
    if(mb->unused_allocation_count < mb->num_dynamic_allocation_sizes)
    {
        p=malloc(CVM_VK_RESERVED_DYNAMIC_BUFFER_ALLOCATION_COUNT*sizeof(cvm_vk_dynamic_buffer_allocation));

        n=mb->first_unused_allocation;
        i=CVM_VK_RESERVED_DYNAMIC_BUFFER_ALLOCATION_COUNT;
        while(i--)
        {
            p[i].next=n;
            p[i].is_base=!i;
            n=p+i;
        }

        mb->first_unused_allocation=p;
        mb->unused_allocation_count+=CVM_VK_RESERVED_DYNAMIC_BUFFER_ALLOCATION_COUNT;
    }
    ///above ensures there will be enough unused allocations for any case below (is overly conservative. but is a quick, rarely failing test)

    available_bitmask=mb->available_dynamic_allocation_bitmask;

    if(available_bitmask >> size_factor)///there exists an allocation of the desired size or larger
    {
        ///find lowest size with available allocation
        for(i=size_factor;!(available_bitmask>>i&1);i++);

        heap=mb->available_dynamic_allocations[i].heap;

        p=heap[0];

        if((c= --mb->available_dynamic_allocations[i].count))
        {
            u=0;

            while((d=(u<<1)+1)<c)
            {
                d+= (d+1<c) && (heap[d+1]->offset < heap[d]->offset);
                if(heap[c]->offset < heap[d]->offset) break;
                heap[d]->heap_index=u;///need to know (new) index in heap
                heap[u]=heap[d];
                u=d;
            }

            heap[c]->heap_index=u;
            heap[u]=heap[c];
        }
        else available_bitmask&= ~(1<<i);

        p->available=false;
        p->size_factor=size_factor;

        while(i-- > size_factor)
        {
            n=mb->first_unused_allocation;
            mb->first_unused_allocation=n->next;
            mb->unused_allocation_count--;

            ///horizontal linked list stuff
            p->next->prev=n;///next cannot be NULL. if it were, then this allocation would have been freed back to the pool (last available allocations are recursively freed)
            n->next=p->next;
            n->prev=p;
            p->next=n;

            n->size_factor=i;
            n->offset=p->offset+(1<<i);
            n->available=true;
            ///if this allocation size wasn't empty, then this code path wouldn't be taken, ergo addition to heap is super simple
            n->heap_index=0;///index in heap

            mb->available_dynamic_allocations[i].heap[0]=n;
            mb->available_dynamic_allocations[i].count=1;

            available_bitmask|=1<<i;
        }

        mb->available_dynamic_allocation_bitmask=available_bitmask;

        if(mb->multithreaded) atomic_store(&mb->acquire_spinlock,0);

        return p;
    }
    else///try to allocate from the end of the buffer
    {
        o=mb->dynamic_offset;///store offset locally

        m=(1<<size_factor)-1;

        if( ((uint64_t)((((o+m)>>size_factor)<<size_factor) + (1<<size_factor))) << mb->base_dynamic_allocation_size_factor > mb->static_offset)
        {
            if(mb->multithreaded) atomic_store(&mb->acquire_spinlock,0);
            return NULL;///not enough memory left
        }
        /// ^ extra brackets to make compiler shut up :sob:

        p=mb->last_used_allocation;

        for(i=0;o&m;i++) if(o&1<<i)
        {
            n=mb->first_unused_allocation;
            mb->first_unused_allocation=n->next;///r is guaranteed to be a valid location by checking done before starting this loop
            mb->unused_allocation_count--;

            n->offset=o;
            n->prev=p;///right will get set after the fact
            n->size_factor=i;
            n->available=true;
            ///add to heap
            n->heap_index=mb->available_dynamic_allocations[i].count++;///by virtue of being allocated from the end it will have the largest offset, thus will never need to move up the heap

            if(n->heap_index==mb->available_dynamic_allocations[i].space)
            {
                mb->available_dynamic_allocations[i].heap=realloc(mb->available_dynamic_allocations[i].heap,sizeof(cvm_vk_dynamic_buffer_allocation*)*(mb->available_dynamic_allocations[i].space*=2));
            }
            mb->available_dynamic_allocations[i].heap[n->heap_index]=n;

            available_bitmask|=1<<i;

            p->next=n;///p cannot be NULL as then alignment would already be satisfied

            o+=1<<i;///increment offset

            p=n;
        }

        n=mb->first_unused_allocation;
        mb->first_unused_allocation=n->next;
        mb->unused_allocation_count--;

        n->offset=o;
        n->prev=p;
        n->next=NULL;
        n->size_factor=size_factor;
        n->available=false;

        if(p)p->next=n;

        mb->last_used_allocation=n;
        mb->dynamic_offset=o+(1<<size_factor);
        mb->available_dynamic_allocation_bitmask=available_bitmask;

        if(mb->multithreaded) atomic_store(&mb->acquire_spinlock,0);

        return n;
    }
}

void cvm_vk_managed_buffer_relinquish_dynamic_allocation(cvm_vk_managed_buffer * mb,cvm_vk_dynamic_buffer_allocation * a)
{
    uint32_t u,d,c;
    cvm_vk_dynamic_buffer_allocation ** heap;
    cvm_vk_dynamic_buffer_allocation *n;///represents next or neighbour
    uint_fast32_t lock;

    if(mb->multithreaded)do lock=atomic_load(&mb->acquire_spinlock);
    while(lock!=0 || !atomic_compare_exchange_weak(&mb->acquire_spinlock,&lock,1));

    if(a==mb->last_used_allocation)///could also check a->right==NULL
    {
        ///free space from end until an unavailable allocation is found
        ///need handling for only 1 alloc, which is rightmost

        a->next=mb->first_unused_allocation;

        n=a;
        a=a->prev;
        mb->unused_allocation_count++;

        while(a && a->available)
        {
            ///remove from heap, index cannot have any children as it's the allocation with the largest offset
            if((c= --mb->available_dynamic_allocations[a->size_factor].count))
            {
                heap=mb->available_dynamic_allocations[a->size_factor].heap;

                d=a->heap_index;
                while(d && heap[c]->offset < heap[(u=(d-1)>>1)]->offset)
                {
                    heap[u]->heap_index=d;
                    heap[d]=heap[u];
                    d=u;
                }

                heap[c]->heap_index=d;
                heap[d]=heap[c];
            }
            else mb->available_dynamic_allocation_bitmask&= ~(1<<a->size_factor);///just removed last available allocation of this size

            ///put into unused linked list
            a->next=n;

            n=a;
            a=a->prev;
            mb->unused_allocation_count++;
        }

        mb->first_unused_allocation=n;
        mb->last_used_allocation=a;
        if(a)a->next=NULL;
        mb->dynamic_offset=n->offset;///despite having been conceptually moved this data is still available and valid
    }
    else /// the difficult one...
    {
        while(a->size_factor+1u < mb->num_dynamic_allocation_sizes)/// combine allocations in aligned way
        {
            ///neighbouring allocation for this alignment
            n = a->offset&1<<a->size_factor ? a->prev : a->next;/// will never be NULL (if a was last other branch would be taken, if first (offset is 0) n will always be set to next)

            if(!n->available || n->size_factor!=a->size_factor)break;///an size factor must be the same size or smaller, can only combine if the same size and available

            ///remove n from its availability heap
            if((c= --mb->available_dynamic_allocations[n->size_factor].count))
            {
                heap=mb->available_dynamic_allocations[n->size_factor].heap;

                d=n->heap_index;
                while(d && heap[c]->offset < heap[(u=(d-1)>>1)]->offset)///try moving up
                {
                    heap[u]->heap_index=d;
                    heap[d]=heap[u];
                    d=u;
                }
                u=d;
                while((d=(u<<1)+1)<c)///try moving down
                {
                    d+= (d+1<c) && (heap[d+1]->offset < heap[d]->offset);
                    if(heap[c]->offset < heap[d]->offset) break;
                    heap[d]->heap_index=u;
                    heap[u]=heap[d];
                    u=d;
                }

                heap[c]->heap_index=u;
                heap[u]=heap[c];
            }
            else mb->available_dynamic_allocation_bitmask&= ~(1<<n->size_factor);///just removed last available allocation of this size

            ///change bindings
            if(a->offset&1<<a->size_factor) /// a is next/right
            {
                a->offset=n->offset;
                ///this should be rare enough to set next and prev during loop and still be the most efficient option
                a->prev=n->prev;
                if(a->prev) a->prev->next=a;
            }
            else (a->next=n->next)->prev=a;///next CANNOT be NULL, as last allocation cannot be available (and thus break above would have been hit)

            a->size_factor++;

            n->next=mb->first_unused_allocation;///put subsumed adjacent allocation in unused list
            mb->first_unused_allocation=n;
            mb->unused_allocation_count++;
        }

        /// add a to appropriate availability heap
        a->available=true;

        d=mb->available_dynamic_allocations[a->size_factor].count++;///by virtue of being allocated from the end it will have the largest offset, thus will never need to move up the heap
        heap=mb->available_dynamic_allocations[a->size_factor].heap;

        if(d==mb->available_dynamic_allocations[a->size_factor].space)
        {
            heap=mb->available_dynamic_allocations[a->size_factor].heap=realloc(heap,sizeof(cvm_vk_dynamic_buffer_allocation*)*(mb->available_dynamic_allocations[a->size_factor].space*=2));
        }

        while(d && a->offset < heap[(u=(d-1)>>1)]->offset)
        {
            heap[u]->heap_index=d;
            heap[d]=heap[u];
            d=u;
        }

        a->heap_index=d;
        heap[d]=a;

        mb->available_dynamic_allocation_bitmask|=1<<a->size_factor;
    }

    if(mb->multithreaded) atomic_store(&mb->acquire_spinlock,0);
}

uint64_t cvm_vk_managed_buffer_acquire_static_allocation(cvm_vk_managed_buffer * mb,uint64_t size,uint64_t alignment)///it is assumed alignment is power of 2, may want to ensure this is the case
{
    uint64_t s,e;
    uint_fast32_t lock;

    size=((size+(alignment-1))& ~(alignment-1));///round up size t multiple of alignment (not strictly necessary but w/e), assumes alignment is power of 2, should really check this is the case before using

    if(mb->multithreaded)do lock=atomic_load(&mb->acquire_spinlock);
    while(lock!=0 || !atomic_compare_exchange_weak(&mb->acquire_spinlock,&lock,1));

    e=mb->static_offset& ~(alignment-1);
    s=((uint64_t)mb->dynamic_offset)<<mb->base_dynamic_allocation_size_factor;

    if(s+size > e || e==size)///must have enough space and cannot use whole buffer for static allocations (returning 0 is reserved for errors)
    {
        if(mb->multithreaded) atomic_store(&mb->acquire_spinlock,0);
        return 0;
    }

    mb->static_offset=e-size;

    if(mb->multithreaded) atomic_store(&mb->acquire_spinlock,0);

    return e-size;
}

static inline void * stage_copy_action(cvm_vk_managed_buffer * mb,uint64_t offset,uint64_t size,VkPipelineStageFlags2 stage_mask,VkAccessFlags2 access_mask)
{
    VkDeviceSize staging_offset;
    uint_fast32_t lock;
    void * ptr;

    assert(mb->staging_buffer);///ATTEMPTED TO MAP DEVICE LOCAL MEMORY WITHOUT SETTING A STAGING BUFFER
    assert(size <= mb->staging_buffer->total_space);///ATTEMPTED TO UPLOAD MORE DATA THEN ASSIGNED STAGING BUFFER CAN ACCOMODATE

    ptr=cvm_vk_staging_buffer_get_allocation(mb->staging_buffer,size,&staging_offset);

    if(!ptr) return NULL;

    if(mb->multithreaded)do lock=atomic_load(&mb->copy_spinlock);
    while(lock!=0 || !atomic_compare_exchange_weak(&mb->copy_spinlock,&lock,1));


    if(mb->pending_copy_count==mb->pending_copy_space)mb->pending_copy_actions=realloc(mb->pending_copy_actions,sizeof(VkBufferCopy)*(mb->pending_copy_space*=2));
    mb->pending_copy_actions[mb->pending_copy_count++]=(VkBufferCopy)
    {
        .srcOffset=staging_offset,
        .dstOffset=offset,
        .size=size
    };

    #warning this probably doesnt work... needs another look (i believe the image variant does work)
    ///will probably need src and dst component swapped in pass-back variant
    if(mb->copy_release_barriers.count==mb->copy_release_barriers.space)mb->copy_release_barriers.barriers=realloc(mb->copy_release_barriers.barriers,sizeof(VkBufferMemoryBarrier2)*(mb->copy_release_barriers.space*=2));
    mb->copy_release_barriers.barriers[mb->copy_release_barriers.count++]=(VkBufferMemoryBarrier2)
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

void * cvm_vk_managed_buffer_get_dynamic_allocation_mapping(cvm_vk_managed_buffer * mb,cvm_vk_dynamic_buffer_allocation * allocation,uint64_t size,VkPipelineStageFlags2 stage_mask,VkAccessFlags2 access_mask,uint16_t * availability_token)
{
    *availability_token=mb->copy_update_counter;///could technically go after the early exit as mapped buffers (so far) don't need to be transferred

    uint64_t offset=allocation->offset<<mb->base_dynamic_allocation_size_factor;
    if(mb->mapping) return mb->mapping+offset;///on UMA can access memory directly!

    if(!size) size=1<<(allocation->size_factor+mb->base_dynamic_allocation_size_factor);///if size not specified use whole region

    return stage_copy_action(mb,offset,size,stage_mask,access_mask);
}

void * cvm_vk_managed_buffer_get_static_allocation_mapping(cvm_vk_managed_buffer * mb,uint64_t offset,uint64_t size,VkPipelineStageFlags2 stage_mask,VkAccessFlags2 access_mask,uint16_t * availability_token)
{
    *availability_token=mb->copy_update_counter;///could technically go after the early exit as mapped buffers (so far) don't need to be transferre

    if(mb->mapping) return mb->mapping+offset;///on UMA can access memory directly!

    return stage_copy_action(mb,offset,size,stage_mask,access_mask);
}

void cvm_vk_managed_buffer_submit_all_pending_copy_actions(cvm_vk_managed_buffer * mb,VkCommandBuffer transfer_cb,VkCommandBuffer graphics_cb)
{
    uint_fast32_t lock;
    cvm_vk_managed_buffer_barrier_list tmp;

    mb->copy_update_counter++;

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
        ///this design isnt enough if using delay more than 1, but this should do for now

        if(mb->copy_acquire_barriers.count)
        {

            VkDependencyInfo copy_aquire_dependencies=
            {
                .sType=VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .pNext=NULL,
                .dependencyFlags=0,
                .memoryBarrierCount=0,
                .pMemoryBarriers=NULL,
                .bufferMemoryBarrierCount=mb->copy_acquire_barriers.count,
                .pBufferMemoryBarriers=mb->copy_acquire_barriers.barriers,
                .imageMemoryBarrierCount=0,
                .pImageMemoryBarriers=NULL
            };
            //puts("BARRIER");
            vkCmdPipelineBarrier2(graphics_cb,&copy_aquire_dependencies);

            ///rest now that we're done with it
            mb->copy_acquire_barriers.count=0;
        }

        if(mb->pending_copy_count) /// mb->pending_copy_count should be the same as mb->copy_release_barriers.count
        {
            ///multithreading consideration shouldnt be necessary as this should only ever be called from the main thread relevant to this resource while no worker threads are operating on the resource

            vkCmdCopyBuffer(transfer_cb,mb->staging_buffer->buffer,mb->buffer,mb->pending_copy_count,mb->pending_copy_actions);
            mb->pending_copy_count=0;

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
                    .pBufferMemoryBarriers=mb->copy_release_barriers.barriers,
                    .imageMemoryBarrierCount=0,
                    .pImageMemoryBarriers=NULL
                };
                vkCmdPipelineBarrier2(graphics_cb,&copy_release_dependencies);
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

    if(start<offset)///if anything was written that wasnt handled by the previous branch
    {
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

void * cvm_vk_staging_buffer_get_allocation(cvm_vk_staging_buffer * sb,uint32_t allocation_size,VkDeviceSize * acquired_offset)
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
void cvm_vk_staging_buffer_relinquish_space(cvm_vk_staging_buffer * sb,uint32_t frame_index)
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
    atomic_init(&tb->space_remaining,0);
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

    assert(1u<<tb->alignment_size_factor == required_alignment);///NON POWER OF 2 ALIGNMENTS NOT SUPPORTED
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

    atomic_store(&tb->space_remaining,0);
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
    return (((allocation_size+alignment-1)>>tb->alignment_size_factor)+1)<<tb->alignment_size_factor;
}

void cvm_vk_transient_buffer_begin(cvm_vk_transient_buffer * tb,uint32_t frame_index)
{
    tb->max_offset=(frame_index+1)*tb->space_per_frame;
    atomic_store(&tb->space_remaining,tb->space_per_frame);
}

void cvm_vk_transient_buffer_end(cvm_vk_transient_buffer * tb)
{
    uint32_t acquired_space=tb->space_per_frame-atomic_load(&tb->space_remaining);
    if(acquired_space)
    {
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

    atomic_store(&tb->space_remaining,0);
    tb->max_offset=0;
}

void * cvm_vk_transient_buffer_get_allocation(cvm_vk_transient_buffer * tb,uint32_t allocation_size,uint32_t alignment,VkDeviceSize * acquired_offset)
{
    uint32_t new_remaining,offset;
    uint_fast32_t old_remaining;

    allocation_size=(((allocation_size+alignment-1)>>tb->alignment_size_factor)+1)<<tb->alignment_size_factor;///round as required

    old_remaining=atomic_load(&tb->space_remaining);
    do
    {
        if(allocation_size>old_remaining)return NULL;

        new_remaining=old_remaining-allocation_size;
    }
    while(!atomic_compare_exchange_weak(&tb->space_remaining,&old_remaining,new_remaining));

    offset=tb->max_offset-old_remaining;

    *acquired_offset=offset;
    return tb->mapping+offset;
}

void cvm_vk_transient_buffer_bind_as_vertex(VkCommandBuffer cmd_buf,cvm_vk_transient_buffer * tb,uint32_t binding)
{
    /// vertex offset in draw assumes all verts have same offset, so it makes no sense in this paradigm to have multiple binding points from a single buffer!
    #warning the above offset issue may become a MASSIVE hassle when isntance data gets involved, base-indexed vertex offsets may not be viable anymore
    ///     ^ actually does have instanceOffset...

    VkDeviceSize offset=0;
    vkCmdBindVertexBuffers(cmd_buf,binding,1,&tb->buffer,&offset);
}

void cvm_vk_transient_buffer_bind_as_index(VkCommandBuffer cmd_buf,cvm_vk_transient_buffer * tb,VkIndexType type)
{
    vkCmdBindIndexBuffer(cmd_buf,tb->buffer,0,type);
}

