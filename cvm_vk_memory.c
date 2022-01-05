/**
Copyright 2021 Carl van Mastrigt

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

///this whole file/concept is still WIP, don't use yet

/// better to have 1 allocation per module (for viable types), or 1 set of allocations (static and dynamic, creating more of either as necessary) shared between all modules?

///only using extern for variables in same category of files (e.g. vulkan memory is permitted to have extern accesses of the vulkan base)







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

    mb->copy_release_barriers.stage_mask=0;
    mb->copy_release_barriers.barriers=malloc(sizeof(VkBufferMemoryBarrier)*16);
    mb->copy_release_barriers.space=16;
    mb->copy_release_barriers.count=0;

    mb->copy_acquire_barriers.stage_mask=0;
    mb->copy_acquire_barriers.barriers=malloc(sizeof(VkBufferMemoryBarrier)*16);
    mb->copy_acquire_barriers.space=16;
    mb->copy_acquire_barriers.count=0;

    mb->copy_update_counter=0;

    mb->copy_dst_queue_family=cvm_vk_get_graphics_queue_family();///will have to add a way to use compute here
    mb->copy_dst_queue_family=cvm_vk_get_transfer_queue_family();
}

void cvm_vk_managed_buffer_destroy(cvm_vk_managed_buffer * mb)
{
    cvm_vk_destroy_buffer(mb->buffer,mb->memory,mb->mapping);
    uint32_t i;
    cvm_vk_dynamic_buffer_allocation *first,*last,*current,*next;

    for(i=0;i<mb->num_dynamic_allocation_sizes;i++)free(mb->available_dynamic_allocations[i].heap);
    free(mb->available_dynamic_allocations);

    ///all dynamic allocations should be freed at this point
    if(mb->last_used_allocation)
    {
        fprintf(stderr,"ATTEMPTED TO DESTROY A BUFFER WITH ACTIVE ELEMENTS\n");
        exit(-1);
        ///could instead just rightmost to offload all active buffers to the unused linked list
    }


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
        free(current);///free all base allocations (to mathch appropriate allocs)
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
    for(size_factor=mb->base_dynamic_allocation_size_factor;0x0000000000000001<<size_factor < size;size_factor++);

    size_factor-=mb->base_dynamic_allocation_size_factor;

    if(size_factor>=mb->num_dynamic_allocation_sizes)return NULL;

    if(mb->multithreaded)do lock=atomic_load(&mb->acquire_spinlock);
    while(lock!=0 || !atomic_compare_exchange_weak(&mb->acquire_spinlock,&lock,1));

    /// okay, seeing as this function must be as fast and light as possible, we should track unused allocation count to determine if there are enough unused allocations left (with allowance for inaccuracy)
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
        while(a->size_factor+1 < mb->num_dynamic_allocation_sizes)/// combine allocations in aligned way
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

static inline void * stage_copy_action(cvm_vk_managed_buffer * mb,uint64_t offset,uint64_t size,VkPipelineStageFlags stage_mask,VkAccessFlags access_mask)
{
    VkDeviceSize staging_offset;
    uint_fast32_t lock;
    void * ptr;

    if(!mb->staging_buffer)
    {
        fprintf(stderr,"ATTEMPTED TO MAP DEVICE LOCAL MEMORY WITHOUT SETTING A STAGING BUFFER\n");
        exit(-1);
    }

    if(size>mb->staging_buffer->total_space)
    {
        fprintf(stderr,"ATTEMPTED TO UPLOAD MORE DATA THEN ASSIGNED STAGING BUFFER CAN ACCOMODATE\n");
        exit(-1);
    }

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

    if(mb->copy_release_barriers.count==mb->copy_release_barriers.space)mb->copy_release_barriers.barriers=realloc(mb->copy_release_barriers.barriers,sizeof(VkBufferMemoryBarrier)*(mb->copy_release_barriers.space*=2));
    mb->copy_release_barriers.barriers[mb->copy_release_barriers.count++]=(VkBufferMemoryBarrier)
    {
        .sType=VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .pNext=NULL,
        .srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask=access_mask,/// staging/copy only ever writes
        .srcQueueFamilyIndex=mb->copy_src_queue_family,
        .dstQueueFamilyIndex=mb->copy_dst_queue_family,
        .buffer=mb->buffer,
        .offset=offset,
        .size=size
    };
    mb->copy_release_barriers.stage_mask|=stage_mask;

    if(mb->multithreaded) atomic_store(&mb->copy_spinlock,0);

    return ptr;
}

void * cvm_vk_managed_buffer_get_dynamic_allocation_mapping(cvm_vk_managed_buffer * mb,cvm_vk_dynamic_buffer_allocation * allocation,uint64_t size,VkPipelineStageFlags stage_mask,VkAccessFlags access_mask,cvm_vk_availability_token * availability_token)
{
    uint64_t offset=allocation->offset<<mb->base_dynamic_allocation_size_factor;
    if(mb->mapping)///on UMA can access memory directly!
    {
        availability_token->delay=0;
        return mb->mapping+offset;
    }

    if(!size) size=1<<(allocation->size_factor+mb->base_dynamic_allocation_size_factor);///if size not specified use whole region
    availability_token->delay=1;
    availability_token->counter=mb->copy_update_counter;

    return stage_copy_action(mb,offset,size,stage_mask,access_mask);
}

void * cvm_vk_managed_buffer_get_static_allocation_mapping(cvm_vk_managed_buffer * mb,uint64_t offset,uint64_t size,VkPipelineStageFlags stage_mask,VkAccessFlags access_mask,cvm_vk_availability_token * availability_token)
{
    if(mb->mapping)///on UMA can access memory directly!
    {
        availability_token->delay=0;
        return mb->mapping+offset;
    }

    availability_token->delay=1;
    availability_token->counter=mb->copy_update_counter;

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
            //puts("BARRIER");
            vkCmdPipelineBarrier
            (
                graphics_cb,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                mb->copy_acquire_barriers.stage_mask,
                0,
                0,NULL,
                mb->copy_acquire_barriers.count,mb->copy_acquire_barriers.barriers,
                0,NULL
            );

            ///rest now that we're done with it
            mb->copy_acquire_barriers.count=0;
            mb->copy_acquire_barriers.stage_mask=0;
        }

        if(mb->pending_copy_count) /// mb->pending_copy_count should be the same as mb->copy_release_barriers.count
        {
//            if(mb->multithreaded)do lock=atomic_load(&mb->copy_spinlock);
//            while(lock!=0 || !atomic_compare_exchange_weak(&mb->copy_spinlock,&lock,1));
            ///multithreading consideration shouldnt be necessary as this should only ever be called from the main thread relevant to this resource while no worker threads are operating on the resource

            vkCmdCopyBuffer(transfer_cb,mb->staging_buffer->buffer,mb->buffer,mb->pending_copy_count,mb->pending_copy_actions);
            mb->pending_copy_count=0;

            if(mb->copy_src_queue_family!=mb->copy_dst_queue_family)///first barrier is only needed when QFOT is necessary
            {
                vkCmdPipelineBarrier
                (
                    transfer_cb,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    mb->copy_release_barriers.stage_mask,
                    0,
                    0,NULL,
                    mb->copy_release_barriers.count,mb->copy_release_barriers.barriers,
                    0,NULL
                );
            }

            tmp=mb->copy_acquire_barriers;
            mb->copy_acquire_barriers=mb->copy_release_barriers;
            mb->copy_release_barriers=tmp;

//            if(mb->multithreaded) atomic_store(&mb->copy_spinlock,0);
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
    ///size really needn't be a PO2, when overrunning we need to detect that anyway, ergo can probably just allocate exact amount desired
    /// if taking above approach probably want to round everything in buffer to some base allocation offset to make amount of mem available constant across acquisition order
    /// will have to pass requests for size through per buffer filter or generalised "across the board" filter that rounds to required alignment sizes

    atomic_init(&sb->space_remaining,0);
    sb->initial_space_remaining=0;///doesn't really need to be set here
    sb->max_offset=0;
    sb->total_space=0;
    sb->mapping=NULL;
    sb->acquisitions=NULL;
    sb->buffer=VK_NULL_HANDLE;
    sb->memory=VK_NULL_HANDLE;
    sb->usage=usage;

    uint32_t required_alignment=cvm_vk_get_buffer_alignment_requirements(usage);

    for(sb->alignment_size_factor=0;1<<sb->alignment_size_factor < required_alignment;sb->alignment_size_factor++);///alignment_size_factor expected to be small, ~6
    ///can catastrophically fail if required_alignment is greater than 2^31, but w/e

    if(1<<sb->alignment_size_factor != required_alignment)
    {
        fprintf(stderr,"NON POWER OF 2 ALIGNMENTS NOT SUPPORTED!\n");
        exit(-1);
    }
}

void cvm_vk_staging_buffer_update(cvm_vk_staging_buffer * sb,uint32_t space_per_frame, uint32_t frame_count)
{
    if(atomic_load(&sb->space_remaining)!=sb->total_space)
    {
        fprintf(stderr,"NOT ALL STAGING BUFFER SPACE RELINQUISHED BEFORE RECREATION!\n");
        exit(-1);
    }

    if(space_per_frame & ((1<<sb->alignment_size_factor)-1))
    {
        fprintf(stderr,"REQUESTED UNALIGNED STAGING SIZE!\n");
        exit(-1);
    }

    if(frame_count)
    {
        sb->acquisitions=realloc(sb->acquisitions,sizeof(uint32_t)*frame_count);
        memset(sb->acquisitions,0,sizeof(uint32_t)*frame_count);
    }

    uint32_t buffer_size=space_per_frame*frame_count;

    if(buffer_size != sb->total_space)
    {
        if(sb->total_space)
        {
            cvm_vk_destroy_buffer(sb->buffer,sb->memory,sb->mapping);
        }

        sb->mapping=cvm_vk_create_buffer(&sb->buffer,&sb->memory,sb->usage,buffer_size,true);

        atomic_store(&sb->space_remaining,buffer_size);
        sb->initial_space_remaining=0;///doesn't really need to be set here
        sb->max_offset=0;
        sb->total_space=buffer_size;
    }
}

void cvm_vk_staging_buffer_destroy(cvm_vk_staging_buffer * sb)
{
    if(atomic_load(&sb->space_remaining)!=sb->total_space)
    {
        fprintf(stderr,"NOT ALL STAGING BUFFER SPACE RELINQUISHED BEFORE DESTRUCTION!\n");
        exit(-1);
    }

    free(sb->acquisitions);

    cvm_vk_destroy_buffer(sb->buffer,sb->memory,sb->mapping);
}

uint32_t cvm_vk_staging_buffer_get_rounded_allocation_size(cvm_vk_staging_buffer * sb,uint32_t allocation_size)
{
    return (((allocation_size-1)>>sb->alignment_size_factor)+1)<<sb->alignment_size_factor;
}

void cvm_vk_staging_buffer_begin(cvm_vk_staging_buffer * sb)
{
    sb->initial_space_remaining=atomic_load(&sb->space_remaining);
}

void cvm_vk_staging_buffer_end(cvm_vk_staging_buffer * sb,uint32_t frame_index)
{
    uint32_t acquired_space,offset;

    acquired_space=sb->initial_space_remaining-atomic_load(&sb->space_remaining);
    sb->acquisitions[frame_index]=acquired_space;
    offset=sb->max_offset-sb->initial_space_remaining + sb->total_space*(sb->initial_space_remaining > sb->max_offset);


    if(offset+acquired_space > sb->total_space)///if the buffer wrapped between begin and end
    {
        VkMappedMemoryRange flush_range=(VkMappedMemoryRange)
        {
            .sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            .pNext=NULL,
            .memory=sb->memory,
            .offset=offset,
            .size=sb->total_space-offset
        };

        cvm_vk_flush_buffer_memory_range(&flush_range);

        acquired_space-=sb->total_space-offset;
        offset=0;
    }

    if(acquired_space)///if anything was written that wasnt handled by the previous branch
    {
        VkMappedMemoryRange flush_range=(VkMappedMemoryRange)
        {
            .sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            .pNext=NULL,
            .memory=sb->memory,
            .offset=offset,
            .size=acquired_space
        };

        cvm_vk_flush_buffer_memory_range(&flush_range);
    }
}

void * cvm_vk_staging_buffer_get_allocation(cvm_vk_staging_buffer * sb,uint32_t allocation_size,VkDeviceSize * acquired_offset)
{
    allocation_size=(((allocation_size-1)>>sb->alignment_size_factor)+1)<<sb->alignment_size_factor;///round as required

    uint32_t offset,new_remaining;
    uint_fast32_t old_remaining;

    old_remaining=atomic_load(&sb->space_remaining);
    do
    {
        if(allocation_size>old_remaining)return NULL;

        new_remaining=old_remaining;

        offset=sb->max_offset-old_remaining + sb->total_space*(old_remaining > sb->max_offset);

        if(offset+allocation_size > sb->total_space)
        {
            new_remaining-=sb->total_space-offset;
            offset=0;
        }

        if(allocation_size>new_remaining) return NULL;

        new_remaining-=allocation_size;
    }
    while(!atomic_compare_exchange_weak(&sb->space_remaining,&old_remaining,new_remaining));

    *acquired_offset=offset;
    return sb->mapping+offset;
}

///should only ever be called during cleanup, not thread safe
void cvm_vk_staging_buffer_relinquish_space(cvm_vk_staging_buffer * sb,uint32_t frame_index)
{
    sb->max_offset+=sb->acquisitions[frame_index];
    sb->max_offset-=sb->total_space*(sb->max_offset>=sb->total_space);

    atomic_fetch_add(&sb->space_remaining,sb->acquisitions[frame_index]);
    sb->acquisitions[frame_index]=0;
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

    for(tb->alignment_size_factor=0;1<<tb->alignment_size_factor < required_alignment;tb->alignment_size_factor++);///alignment_size_factor expected to be small, ~6
    ///can catastrophically fail if required_alignment is greater than 2^31, but w/e

    if(1<<tb->alignment_size_factor != required_alignment)
    {
        fprintf(stderr,"NON POWER OF 2 ALIGNMENTS NOT SUPPORTED!\n");
        exit(-1);
    }
}

void cvm_vk_transient_buffer_update(cvm_vk_transient_buffer * tb,uint32_t space_per_frame, uint32_t frame_count)
{
    uint32_t total_space=space_per_frame*frame_count;

    if(tb->max_offset)
    {
        fprintf(stderr,"TRYING TO RECREATE TRANSIENT BUFFER THAT IS STILL IN USE\n");
        exit(-1);
    }

    if(space_per_frame & ((1<<tb->alignment_size_factor)-1))
    {
        fprintf(stderr,"REQUESTED UNALIGNED TRANSIENT STAGING SIZE!\n");
        exit(-1);
    }

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
    if(tb->max_offset)
    {
        fprintf(stderr,"TRYING TO DESTROY TRANSIENT BUFFER THAT IS STILL IN USE\n");
        exit(-1);
    }

    cvm_vk_destroy_buffer(tb->buffer,tb->memory,tb->mapping);
}

uint32_t cvm_vk_transient_buffer_get_rounded_allocation_size(cvm_vk_transient_buffer * tb,uint32_t allocation_size)
{
    return (((allocation_size-1)>>tb->alignment_size_factor)+1)<<tb->alignment_size_factor;
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

void * cvm_vk_transient_buffer_get_allocation(cvm_vk_transient_buffer * tb,uint32_t allocation_size,VkDeviceSize * acquired_offset)
{
    allocation_size=(((allocation_size-1)>>tb->alignment_size_factor)+1)<<tb->alignment_size_factor;///round as required

    uint32_t new_remaining,offset;
    uint_fast32_t old_remaining;

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




/*void cvm_vk_create_upload_buffer(cvm_vk_upload_buffer * ub,VkBufferUsageFlags usage)
{
    ///size really needn't be a PO2, when overrunning we need to detect that anyway, ergo can probably just allocate exact amount desired
    /// if taking above approach probably want to round everything in buffer to some base allocation offset to make amount of mem available constant across acquisition order
    /// will have to pass requests for size through per buffer filter or generalised "across the board" filter that rounds to required alignment sizes

    ub->usage=usage;

    ub->frame_count=0;
    ub->current_frame=0;

    uint32_t required_alignment=cvm_vk_get_buffer_alignment_requirements(usage);

    for(ub->alignment_size_factor=0;1<<ub->alignment_size_factor < required_alignment;ub->alignment_size_factor++);///alignment_size_factor expected to be small, ~6
    ///can catastrophically fail if required_alignment is greater than 2^31, but w/e

    if(1<<ub->alignment_size_factor != required_alignment)
    {
        fprintf(stderr,"NON POWER OF 2 ALIGNMENTS NOT SUPPORTED!\n");
        exit(-1);
    }

    ub->mapping=NULL;


    ub->transient_space_per_frame=0;
    atomic_init(&ub->transient_space_remaining,0);
    ub->transient_offsets=NULL;



    ub->staging_space_per_frame=0;
    ub->staging_space=0;
    ub->max_staging_offset=0;
    atomic_init(&ub->staging_space_remaining,0);
    ub->initial_staging_space_remaining=0;
    ub->staging_buffer_acquisitions=NULL;
}

void cvm_vk_update_upload_buffer(cvm_vk_upload_buffer * ub,uint32_t staging_space_per_frame,uint32_t transient_space_per_frame,uint32_t frame_count)
{
    uint32_t i;

    if(atomic_load(&ub->staging_space_remaining)!=ub->staging_space)
    {
        fprintf(stderr,"NOT ALL UPLOAD BUFFER STAGNG SPACE RELINQUISHED BEFORE RECREATION!\n");
        exit(-1);
    }

    if(staging_space_per_frame & ((1<<ub->alignment_size_factor)-1))
    {
        fprintf(stderr,"REQUESTED UNALIGNED UPLOAD STAGING SIZE!\n");
        exit(-1);
    }

    if(transient_space_per_frame & ((1<<ub->alignment_size_factor)-1))
    {
        fprintf(stderr,"REQUESTED UNALIGNED TRANSIENT STAGING SIZE!\n");
        exit(-1);
    }

    if(frame_count!=ub->frame_count)
    {
        ub->transient_offsets=realloc(ub->transient_offsets,frame_count*sizeof(uint32_t));
        ub->staging_buffer_acquisitions=realloc(ub->staging_buffer_acquisitions,frame_count*sizeof(uint32_t));
    }



    if(staging_space_per_frame!=ub->staging_space_per_frame || transient_space_per_frame!=ub->transient_space_per_frame || frame_count!=ub->frame_count)///recreate buffer
    {
        if((ub->staging_space_per_frame || ub->transient_space_per_frame) && ub->frame_count)
        {
            cvm_vk_destroy_buffer(ub->buffer,ub->memory,ub->mapping);
            ub->mapping=NULL;
        }

        if((staging_space_per_frame || transient_space_per_frame) && frame_count)
        {
            ub->mapping=cvm_vk_create_buffer(&ub->buffer,&ub->memory,ub->usage,(staging_space_per_frame+transient_space_per_frame)*frame_count,true);
        }
    }

    ub->frame_count=frame_count;
    ub->current_frame=0;

    ub->transient_space_per_frame=transient_space_per_frame;
    atomic_store(&ub->transient_space_remaining,0);

    ub->staging_space_per_frame=staging_space_per_frame;
    ub->staging_space=staging_space_per_frame*frame_count;
    ub->max_staging_offset=0;
    atomic_store(&ub->staging_space_remaining,staging_space_per_frame*frame_count);
    ub->initial_staging_space_remaining=0;

    for(i=0;i<frame_count;i++)
    {
        ub->staging_buffer_acquisitions[i]=0;
        ub->transient_offsets[i]=staging_space_per_frame*frame_count + transient_space_per_frame*i;
    }
}

void cvm_vk_destroy_upload_buffer(cvm_vk_upload_buffer * ub)
{
    if(atomic_load(&ub->staging_space_remaining)!=ub->staging_space)
    {
        fprintf(stderr,"NOT ALL UPLOAD BUFFER STAGNG SPACE RELINQUISHED BEFORE DESTRUCTION!\n");
        exit(-1);
    }

    cvm_vk_destroy_buffer(ub->buffer,ub->memory,ub->mapping);
}

uint32_t cvm_vk_upload_buffer_get_rounded_allocation_size(cvm_vk_upload_buffer * ub,uint32_t allocation_size)
{
    return (((allocation_size-1)>>ub->alignment_size_factor)+1)<<ub->alignment_size_factor;
}

void cvm_vk_begin_upload_buffer_frame(cvm_vk_upload_buffer * ub,uint32_t frame_index)
{
    ub->initial_staging_space_remaining=atomic_load(&ub->staging_space_remaining);

    atomic_store(&ub->transient_space_remaining,ub->transient_space_per_frame);

    ub->current_frame=frame_index;
}

void cvm_vk_end_upload_buffer_frame(cvm_vk_upload_buffer * ub)
{
    uint32_t acquired_staging_space,current_staging_offset,acquired_transient_space;

    ///flush transient space
    acquired_transient_space=ub->transient_space_per_frame - atomic_load(&ub->transient_space_remaining);
    if(acquired_transient_space)
    {
        VkMappedMemoryRange flush_range=(VkMappedMemoryRange)
        {
            .sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            .pNext=NULL,
            .memory=ub->memory,
            .offset=ub->transient_offsets[ub->current_frame],
            .size=acquired_transient_space
        };

        cvm_vk_flush_buffer_memory_range(&flush_range);
        atomic_store(&ub->transient_space_remaining,0);
    }

    ///flush staging space
    acquired_staging_space = ub->initial_staging_space_remaining - atomic_load(&ub->staging_space_remaining);
    ub->staging_buffer_acquisitions[ub->current_frame] = acquired_staging_space;

    current_staging_offset = ub->max_staging_offset - ub->initial_staging_space_remaining + ub->staging_space * (ub->initial_staging_space_remaining > ub->max_staging_offset);///offset before this frame

    if(current_staging_offset+acquired_staging_space >= ub->staging_space)///if the buffer wrapped between begin and end
    {
        VkMappedMemoryRange flush_range=(VkMappedMemoryRange)
        {
            .sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            .pNext=NULL,
            .memory=ub->memory,
            .offset=current_staging_offset,
            .size=ub->staging_space-current_staging_offset
        };

        cvm_vk_flush_buffer_memory_range(&flush_range);

        acquired_staging_space-=ub->staging_space-current_staging_offset;
        current_staging_offset=0;
    }

    if(acquired_staging_space)///if anything was written that wasnt handled by the previous branch
    {
        VkMappedMemoryRange flush_range=(VkMappedMemoryRange)
        {
            .sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            .pNext=NULL,
            .memory=ub->memory,
            .offset=current_staging_offset,
            .size=acquired_staging_space
        };

        cvm_vk_flush_buffer_memory_range(&flush_range);
    }
}

///should only ever be called during cleanup, not thread safe
void cvm_vk_relinquish_upload_buffer_space(cvm_vk_upload_buffer * ub,uint32_t frame_index)
{
    ub->max_staging_offset+=ub->staging_buffer_acquisitions[frame_index];
    ub->max_staging_offset-=ub->staging_space*(ub->max_staging_offset>=ub->staging_space);

    atomic_fetch_add(&ub->staging_space_remaining,ub->staging_buffer_acquisitions[frame_index]);
    ub->staging_buffer_acquisitions[frame_index]=0;
}

void * cvm_vk_get_upload_buffer_staging_allocation(cvm_vk_upload_buffer * ub,uint32_t allocation_size,VkDeviceSize * acquired_offset)
{
    allocation_size=(((allocation_size-1)>>ub->alignment_size_factor)+1)<<ub->alignment_size_factor;///round as required

    uint32_t offset,new_remaining;
    uint_fast32_t old_remaining;

    old_remaining=atomic_load(&ub->staging_space_remaining);
    do
    {
        if(allocation_size>old_remaining)return NULL;

        new_remaining=old_remaining;

        offset=ub->max_staging_offset-old_remaining + ub->staging_space*(old_remaining > ub->max_staging_offset);

        if(offset+allocation_size > ub->staging_space)
        {
            new_remaining-=ub->staging_space-offset;
            offset=0;
        }

        if(allocation_size>new_remaining) return NULL;

        new_remaining-=allocation_size;
    }
    while(!atomic_compare_exchange_weak(&ub->staging_space_remaining,&old_remaining,new_remaining));

    *acquired_offset=offset;
    return ub->mapping+offset;
}


void * cvm_vk_get_upload_buffer_transient_allocation(cvm_vk_upload_buffer * ub,uint32_t allocation_size,VkDeviceSize * acquired_offset)
{
    allocation_size=(((allocation_size-1)>>ub->alignment_size_factor)+1)<<ub->alignment_size_factor;///round as required

    uint32_t new_remaining,offset;
    uint_fast32_t old_remaining;

    old_remaining=atomic_load(&ub->transient_space_remaining);
    do
    {
        if(allocation_size>old_remaining)return NULL;

        new_remaining=old_remaining-allocation_size;
    }
    while(!atomic_compare_exchange_weak(&ub->transient_space_remaining,&old_remaining,new_remaining));

    offset=ub->transient_offsets[ub->current_frame]+ub->transient_space_per_frame-old_remaining;

    *acquired_offset=offset;
    return ub->mapping+offset;
}*/









