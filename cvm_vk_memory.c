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







void cvm_vk_create_managed_buffer(cvm_vk_managed_buffer * mb,uint32_t buffer_size,uint32_t min_size_factor,uint32_t max_size_factor,VkBufferUsageFlags usage,bool multithreaded,bool host_visible)
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
    atomic_init(&mb->spinlock,0);

    mb->mapping=cvm_vk_create_buffer(&mb->buffer,&mb->memory,usage,buffer_size,host_visible);///if this is non-null then probably UMA and thus can circumvent staging buffer
}

void cvm_vk_destroy_managed_buffer(cvm_vk_managed_buffer * mb)
{
    cvm_vk_destroy_buffer(mb->buffer,mb->memory,mb->mapping);
    uint32_t i;
    cvm_vk_dynamic_buffer_allocation *first,*last,*current,*next;

    for(i=0;i<mb->num_dynamic_allocation_sizes;i++)free(mb->available_dynamic_allocations[i].heap);
    free(mb->available_dynamic_allocations);

    ///all dynamic allocations should be freed at this point
    if(mb->last_used_allocation)
    {
        fprintf(stderr,"TRYING TO DESTROY A BUFFER WITH ACTIVE ELEMENTS\n");
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
}

cvm_vk_dynamic_buffer_allocation * cvm_vk_acquire_dynamic_buffer_allocation(cvm_vk_managed_buffer * mb,uint64_t size)
{
    uint32_t i,o,m,u,d,c,size_factor,available_bitmask;
    cvm_vk_dynamic_buffer_allocation ** heap;
    cvm_vk_dynamic_buffer_allocation *p,*n;
    uint_fast32_t lock;

    ///linear search should be fine as its assumed the vast majority of buffers will have size factor less than 3 (ergo better than binary search)
    for(size_factor=mb->base_dynamic_allocation_size_factor;0x0000000000000001<<size_factor < size;size_factor++);

    size_factor-=mb->base_dynamic_allocation_size_factor;

    if(size_factor>=mb->num_dynamic_allocation_sizes)return NULL;

    if(mb->multithreaded)do lock=atomic_load(&mb->spinlock);
    while(lock!=0 || !atomic_compare_exchange_weak(&mb->spinlock,&lock,1));

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

        if(mb->multithreaded) atomic_store(&mb->spinlock,0);

        return p;
    }
    else///try to allocate from the end of the buffer
    {
        o=mb->dynamic_offset;///store offset locally

        m=(1<<size_factor)-1;

        if( ((uint64_t)((((o+m)>>size_factor)<<size_factor) + (1<<size_factor))) << mb->base_dynamic_allocation_size_factor > mb->static_offset)
        {
            if(mb->multithreaded) atomic_store(&mb->spinlock,0);
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

        if(mb->multithreaded) atomic_store(&mb->spinlock,0);

        return n;
    }
}

void cvm_vk_relinquish_dynamic_buffer_allocation(cvm_vk_managed_buffer * mb,cvm_vk_dynamic_buffer_allocation * a)
{
    uint32_t u,d,c;
    cvm_vk_dynamic_buffer_allocation ** heap;
    cvm_vk_dynamic_buffer_allocation *n;///represents next or neighbour
    uint_fast32_t lock;

    if(mb->multithreaded)do lock=atomic_load(&mb->spinlock);
    while(lock!=0 || !atomic_compare_exchange_weak(&mb->spinlock,&lock,1));

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

    if(mb->multithreaded) atomic_store(&mb->spinlock,0);
}

uint64_t cvm_vk_acquire_static_buffer_allocation(cvm_vk_managed_buffer * mb,uint64_t size,uint64_t alignment)///it is assumed alignment is power of 2, may want to ensure this is the case
{
    uint64_t s,e;
    uint_fast32_t lock;

    size=((size+(alignment-1))& ~(alignment-1));///round up size t multiple of alignment (not strictly necessary but w/e), assumes alignment is power of 2, should really check this is the case before using

    if(mb->multithreaded)do lock=atomic_load(&mb->spinlock);
    while(lock!=0 || !atomic_compare_exchange_weak(&mb->spinlock,&lock,1));

    e=mb->static_offset& ~(alignment-1);
    s=((uint64_t)mb->dynamic_offset)<<mb->base_dynamic_allocation_size_factor;

    if(s+size > e || e==size)///must have enough space and cannot use whole buffer for static allocations (returning 0 is reserved for errors)
    {
        if(mb->multithreaded) atomic_store(&mb->spinlock,0);
        return 0;
    }

    mb->static_offset=e-size;

    if(mb->multithreaded) atomic_store(&mb->spinlock,0);

    return e-size;
}

void * cvm_vk_get_dynamic_buffer_allocation_mapping(cvm_vk_managed_buffer * mb,cvm_vk_dynamic_buffer_allocation * allocation)
{
    return mb->mapping ? mb->mapping+(allocation->offset<<mb->base_dynamic_allocation_size_factor) : NULL;
}

void * cvm_vk_get_static_buffer_allocation_mapping(cvm_vk_managed_buffer * mb,uint64_t offset)
{
    return mb->mapping ? mb->mapping+offset : NULL;
}

void cvm_vk_bind_dymanic_allocation_vertex(VkCommandBuffer cmd_buf,cvm_vk_managed_buffer * mb,cvm_vk_dynamic_buffer_allocation * allocation,uint32_t binding)
{
    VkDeviceSize offset=allocation->offset << mb->base_dynamic_allocation_size_factor;
    vkCmdBindVertexBuffers(cmd_buf,binding,1,&mb->buffer,&offset);
}

void cvm_vk_bind_dymanic_allocation_index(VkCommandBuffer cmd_buf,cvm_vk_managed_buffer * mb,cvm_vk_dynamic_buffer_allocation * allocation,VkIndexType type)
{
    VkDeviceSize offset=allocation->offset << mb->base_dynamic_allocation_size_factor;
    vkCmdBindIndexBuffer(cmd_buf,mb->buffer,offset,type);
}

void cvm_vk_flush_buffer_allocation_mapping(cvm_vk_managed_buffer * mb,cvm_vk_dynamic_buffer_allocation * allocation)
{
    VkMappedMemoryRange flush_range=(VkMappedMemoryRange)
    {
        .sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
        .pNext=NULL,
        .memory=mb->memory,
        .offset=allocation->offset,
        .size=1<<(mb->base_dynamic_allocation_size_factor+allocation->size_factor)
    };

    cvm_vk_flush_buffer_memory_range(&flush_range);
}

void cvm_vk_flush_managed_buffer(cvm_vk_managed_buffer * mb)
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






void cvm_vk_create_ring_buffer(cvm_vk_ring_buffer * rb,VkBufferUsageFlags usage,uint32_t buffer_size)
{
    ///size really needn't be a PO2, when overrunning we need to detect that anyway, ergo can probably just allocate exact amount desired
    /// if taking above approach probably want to round everything in buffer to some base allocation offset to make amount of mem available constant across acquisition order
    /// will have to pass requests for size through per buffer filter or generalised "across the board" filter that rounds to required alignment sizes

    atomic_init(&rb->space_remaining,0);
    rb->initial_space_remaining=0;///doesn't really need to be set here
    rb->max_offset=0;
    rb->total_space=0;
    rb->usage=usage;

    uint32_t required_alignment=cvm_vk_get_buffer_alignment_requirements(usage);

    for(rb->alignment_size_factor=0;1<<rb->alignment_size_factor < required_alignment;rb->alignment_size_factor++);///alignment_size_factor expected to be small, ~6
    ///can catastrophically fail if required_alignment is greater than 2^31, but w/e

    if(1<<rb->alignment_size_factor != required_alignment)
    {
        fprintf(stderr,"NON POWER OF 2 ALIGNMENTS NOT SUPPORTED!\n");
        exit(-1);
    }

    if(buffer_size) cvm_vk_update_ring_buffer(rb,buffer_size);
}

void cvm_vk_update_ring_buffer(cvm_vk_ring_buffer * rb,uint32_t buffer_size)
{
    if(atomic_load(&rb->space_remaining)!=rb->total_space)
    {
        fprintf(stderr,"NOT ALL RING_BUFFER SPACE_RELINQUISHED BEFORE RECREATION!\n");
        exit(-1);
    }

    if(buffer_size != rb->total_space)
    {
        if(rb->total_space)
        {
            cvm_vk_destroy_buffer(rb->buffer,rb->memory,rb->mapping);
        }

        rb->mapping=cvm_vk_create_buffer(&rb->buffer,&rb->memory,rb->usage,buffer_size,true);

        atomic_store(&rb->space_remaining,buffer_size);
        rb->initial_space_remaining=0;///doesn't really need to be set here
        rb->max_offset=0;
        rb->total_space=buffer_size;
    }
}

void cvm_vk_destroy_ring_buffer(cvm_vk_ring_buffer * rb)
{
    if(atomic_load(&rb->space_remaining)!=rb->total_space)
    {
        fprintf(stderr,"NOT ALL RING_BUFFER SPACE_RELINQUISHED BEFORE DESTRUCTION!\n");
        exit(-1);
    }

    cvm_vk_destroy_buffer(rb->buffer,rb->memory,rb->mapping);
}

uint32_t cvm_vk_ring_buffer_get_rounded_allocation_size(cvm_vk_ring_buffer * rb,uint32_t allocation_size)
{
    return (((allocation_size-1)>>rb->alignment_size_factor)+1)<<rb->alignment_size_factor;
}

void cvm_vk_begin_ring_buffer(cvm_vk_ring_buffer * rb)
{
    rb->initial_space_remaining=atomic_load(&rb->space_remaining);
}

uint32_t cvm_vk_end_ring_buffer(cvm_vk_ring_buffer * rb)
{
    uint32_t acquired_space,offset,remaining_space;

    acquired_space=rb->initial_space_remaining-atomic_load(&rb->space_remaining);
    offset=rb->max_offset-rb->initial_space_remaining + rb->total_space*(rb->initial_space_remaining > rb->max_offset);
    remaining_space=acquired_space;


    if(offset+remaining_space > rb->total_space)///if the buffer wrapped between begin and end
    {
        VkMappedMemoryRange flush_range=(VkMappedMemoryRange)
        {
            .sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            .pNext=NULL,
            .memory=rb->memory,
            .offset=offset,
            .size=rb->total_space-offset
        };

        cvm_vk_flush_buffer_memory_range(&flush_range);

        remaining_space-=rb->total_space-offset;
        offset=0;
    }

    if(remaining_space)///if anything was written that wasnt handled by the previous branch
    {
        VkMappedMemoryRange flush_range=(VkMappedMemoryRange)
        {
            .sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            .pNext=NULL,
            .memory=rb->memory,
            .offset=offset,
            .size=remaining_space
        };

        cvm_vk_flush_buffer_memory_range(&flush_range);
    }

    return acquired_space;
}

void * cvm_vk_get_ring_buffer_allocation(cvm_vk_ring_buffer * rb,uint32_t allocation_size,VkDeviceSize * acquired_offset)
{
    allocation_size=(((allocation_size-1)>>rb->alignment_size_factor)+1)<<rb->alignment_size_factor;///round as required

    uint32_t offset,new_remaining;
    uint_fast32_t old_remaining;

    old_remaining=atomic_load(&rb->space_remaining);
    do
    {
        new_remaining=old_remaining;

        offset=rb->max_offset-old_remaining + rb->total_space*(old_remaining > rb->max_offset);

        if(offset+allocation_size > rb->total_space)
        {
            new_remaining-=rb->total_space-offset;
            offset=0;
        }

        if(allocation_size>new_remaining) return NULL;

        new_remaining-=allocation_size;
    }
    while(!atomic_compare_exchange_weak(&rb->space_remaining,&old_remaining,new_remaining));

    *acquired_offset=offset;
    return rb->mapping+offset;
}

///should only ever be called during cleanup, not thread safe
void cvm_vk_relinquish_ring_buffer_space(cvm_vk_ring_buffer * rb,uint32_t * relinquished_space)
{
    rb->max_offset+=*relinquished_space;
    rb->max_offset-=rb->total_space*(rb->max_offset>=rb->total_space);

    atomic_fetch_add(&rb->space_remaining,*relinquished_space);
    *relinquished_space=0;
}









