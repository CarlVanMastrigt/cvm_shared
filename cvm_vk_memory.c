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







void cvm_vk_create_managed_buffer(cvm_vk_managed_buffer * mb,uint32_t buffer_size,uint32_t min_size_factor,uint32_t max_size_factor,uint32_t reserved_allocation_count,VkBufferUsageFlags usage,bool multithreaded)
{
    uint32_t i;
    buffer_size=(((buffer_size-1) >> min_size_factor)+1) << min_size_factor;///round to multiple (complier warns about what i want as needing braces, lol)
    mb->total_space=buffer_size;
    mb->static_offset=buffer_size;
    mb->dynamic_offset=0;

    cvm_vk_dynamic_buffer_allocation * dynamic_allocations=malloc(sizeof(cvm_vk_dynamic_buffer_allocation)*reserved_allocation_count);

    cvm_vk_dynamic_buffer_allocation * next=NULL;
    i=reserved_allocation_count;
    while(i--)
    {
        dynamic_allocations[i].link.next=next;
        dynamic_allocations[i].is_base=!i;
        next=dynamic_allocations+i;
    }

    mb->first_unused_allocation=dynamic_allocations;///put all allocations in linked list of unused allocations
    mb->unused_allocation_count=reserved_allocation_count;
    mb->reserved_allocation_count=reserved_allocation_count;
    mb->rightmost_allocation=NULL;///rightmost doesn't exist as it hasn't been allocated


    mb->num_dynamic_allocation_sizes=max_size_factor-min_size_factor;
    mb->available_dynamic_allocations=malloc(sizeof(cvm_vk_availablility_heap)*mb->num_dynamic_allocation_sizes);
    for(i=0;i<mb->num_dynamic_allocation_sizes;i++)
    {
        mb->available_dynamic_allocations[i].heap=malloc(sizeof(cvm_vk_dynamic_buffer_allocation*)*16);
        mb->available_dynamic_allocations[i].space=16;
        mb->available_dynamic_allocations[i].count=0;
    }
    mb->available_dynamic_allocation_bitmask=0;

    mb->base_dynamic_allocation_size_factor=min_size_factor;

    mb->multithreaded=multithreaded;
    atomic_init(&mb->spinlock,0);

    #warning mb->mapping=cvm_vk_create_buffer(&mb->buffer,&mb->memory,usage,buffer_size,false);///if this is non-null then probably UMA and thus can circumvent staging buffer
}

void cvm_vk_destroy_managed_buffer(cvm_vk_managed_buffer * mb)
{
    #warning cvm_vk_destroy_buffer(mb->buffer,mb->memory,mb->mapping);
    uint32_t i;
    cvm_vk_dynamic_buffer_allocation *f,*l,*current,*next;

    for(i=0;i<mb->num_dynamic_allocation_sizes;i++)free(mb->available_dynamic_allocations[i].heap);
    free(mb->available_dynamic_allocations);

    ///all dynamic allocations should be freed at this point
    if(mb->rightmost_allocation)
    {
        fprintf(stderr,"TRYING TO DESTROY A BUFFER WITH ACTIVE ELEMENTS\n");
        exit(-1);
        ///could instead just rightmost to offload all active buffers to the unused linked list
    }


    f=l=NULL;
    ///get all base allocations in their own linked list, discarding all others
    for(current=mb->first_unused_allocation;current;current=next)
    {
        next=current->link.next;
        if(current->is_base)
        {
            if(l) l=l->link.next=current;
            else f=l=current;
        }
    }
    l->link.next=NULL;

    for(current=f;current;current=next)
    {
        next=current->link.next;
        free(current);///free all base allocations (to mathch appropriate allocs)
    }
}

void cvm_vk_update_managed_buffer_reservations(cvm_vk_managed_buffer * mb)
{
    uint32_t i;
    uint_fast32_t lock;
    cvm_vk_dynamic_buffer_allocation *next,*dynamic_allocations;

    if(mb->multithreaded)do lock=atomic_load(&mb->spinlock);
    while(lock!=0 || !atomic_compare_exchange_weak(&mb->spinlock,&lock,1));
    ///probably want to call this in critical section anyway but w/e

    if(mb->unused_allocation_count<mb->reserved_allocation_count)
    {
        dynamic_allocations=malloc(sizeof(cvm_vk_dynamic_buffer_allocation)*mb->reserved_allocation_count);

        next=mb->first_unused_allocation;
        i=mb->reserved_allocation_count;
        while(i--)
        {
            dynamic_allocations[i].link.next=next;
            dynamic_allocations[i].is_base=!i;
            next=dynamic_allocations+i;
        }

        mb->first_unused_allocation=dynamic_allocations;
        mb->unused_allocation_count+=mb->reserved_allocation_count;
    }

    atomic_store(&mb->spinlock,0);
}

cvm_vk_dynamic_buffer_allocation * cvm_vk_acquire_dynamic_buffer_allocation(cvm_vk_managed_buffer * mb,uint64_t size)
{
    uint32_t i,o,m,u,d,c,size_factor,available_bitmask;
    cvm_vk_dynamic_buffer_allocation ** heap;
    cvm_vk_dynamic_buffer_allocation *l,*r;
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
        atomic_store(&mb->spinlock,0);
        return NULL;///there might not be enough unused allocations, early exit with failure (this should VERY rarely, if ever happen)
    }
    ///above ensures there will be enough unused allocations for any case below (is overly conservative. but is a quick, rarely failing test)

    available_bitmask=mb->available_dynamic_allocation_bitmask;

    if(available_bitmask >> size_factor)///there exists an allocation of the desired size or larger
    {
        ///find lowest size with available allocation
        for(i=size_factor;!(available_bitmask>>i&1);i++);

        heap=mb->available_dynamic_allocations[i].heap;

        l=heap[0];

        if((c= --mb->available_dynamic_allocations[i].count))
        {
            u=0;

            while((d=(u<<1)+1)<c)
            {
                d+= (d+1<c) && (heap[d+1]->offset < heap[d]->offset);
                if(heap[c]->offset < heap[d]->offset) break;
                heap[d]->link.heap_index=u;///need to know (new) index in heap
                heap[u]=heap[d];
                u=d;
            }

            heap[c]->link.heap_index=u;
            heap[u]=heap[c];
        }
        else available_bitmask&= ~(1<<i);

        l->available=false;
        l->link.next=NULL;///not in any linked list
        l->size_factor=size_factor;

        while(i-- > size_factor)
        {
            r=mb->first_unused_allocation;
            mb->first_unused_allocation=r->link.next;
            mb->unused_allocation_count--;

            ///horizontal linked list stuff
            l->right->left=r;///right cannot be NULL. if it were, then this allocation would have been freed back to the pool (rightmost available allocations are recursively freed)
            r->right=l->right;
            r->left=l;
            l->right=r;

            r->size_factor=i;
            r->offset=l->offset+(1<<i);
            r->available=true;
            ///if this allocation size wasn't empty, then this code path wouldn't be taken, ergo addition to heap is super simple
            r->link.heap_index=0;///index in heap

            mb->available_dynamic_allocations[i].heap[0]=r;
            mb->available_dynamic_allocations[i].count=1;

            available_bitmask|=1<<i;
        }

        mb->available_dynamic_allocation_bitmask=available_bitmask;

        atomic_store(&mb->spinlock,0);

        return l;
    }
    else///try to allocate from the end of the buffer
    {
        o=mb->dynamic_offset;///store offset locally

        m=(1<<size_factor)-1;

        if( ((uint64_t)((((o+m)>>size_factor)<<size_factor) + (1<<size_factor))) << mb->base_dynamic_allocation_size_factor > mb->static_offset)return NULL;///not enough memory left
        /// ^ extra brackets to make compiler shut up :sob:

        l=mb->rightmost_allocation;
        ///i is initially set to bitmask of sizes that need new allocations


        for(i=0;o&m;i++) if(o&1<<i)/// o&m allows early exit at very little (if any) cost over i<size_favtor
        {
            r=mb->first_unused_allocation;
            mb->first_unused_allocation=r->link.next;///r is guaranteed to be a valid location by checking done before starting this loop
            mb->unused_allocation_count--;

            r->offset=o;
            r->left=l;///right will get set after the fact
            r->size_factor=i;
            r->available=true;
            ///add to heap
            r->link.heap_index=mb->available_dynamic_allocations[i].count++;///by virtue of being allocated from the end it will have the largest offset, thus will never need to move up the heap

            if(r->link.heap_index==mb->available_dynamic_allocations[i].space)
            {
                mb->available_dynamic_allocations[i].heap=realloc(mb->available_dynamic_allocations[i].heap,sizeof(cvm_vk_dynamic_buffer_allocation*)*(mb->available_dynamic_allocations[i].space*=2));
            }
            mb->available_dynamic_allocations[i].heap[r->link.heap_index]=r;

            available_bitmask|=1<<i;

            l->right=r;///l cannot be CVM_VK_NULL_ALLOCATION_INDEX as then alignment would already be satisfied

            o+=1<<i;///increment offset

            l=r;
        }

        r=mb->first_unused_allocation;
        mb->first_unused_allocation=r->link.next;///r is guaranteed to be a valid location by checking done before starting this loop
        mb->unused_allocation_count--;

        r->offset=o;
        r->left=l;
        r->right=NULL;
        r->size_factor=size_factor;
        r->link.next=NULL;/// prev/next are unnecessary for allocations in use tbh
        r->available=false;

        if(l)l->right=r;


        mb->rightmost_allocation=r;
        mb->dynamic_offset=o+(1<<size_factor);
        mb->available_dynamic_allocation_bitmask=available_bitmask;

        atomic_store(&mb->spinlock,0);

        return r;
    }
}

void cvm_vk_relinquish_dynamic_buffer_allocation(cvm_vk_managed_buffer * mb,cvm_vk_dynamic_buffer_allocation * a)
{
    uint32_t u,d,c,available_bitmask;
    cvm_vk_dynamic_buffer_allocation ** heap;
    cvm_vk_dynamic_buffer_allocation *n;///represents next or neighbour
    uint_fast32_t lock;

    if(mb->multithreaded)do lock=atomic_load(&mb->spinlock);
    while(lock!=0 || !atomic_compare_exchange_weak(&mb->spinlock,&lock,1));

    available_bitmask=mb->available_dynamic_allocation_bitmask;

    if(a==mb->rightmost_allocation)
    {
        ///free space from right until an unavailable allocation is found
        ///need handling for only 1 alloc, which is rightmost

        a->link.next=mb->first_unused_allocation;

        n=a;
        a=a->left;
        mb->unused_allocation_count++;

        while(a && a->available)
        {
            ///remove from heap, index cannot have any children as it's the allocation with the largest offset
            if((c= --mb->available_dynamic_allocations[a->size_factor].count))
            {
                heap=mb->available_dynamic_allocations[a->size_factor].heap;

                d=a->link.heap_index;
                while(d && heap[c]->offset < heap[(u=(d-1)>>1)]->offset)
                {
                    heap[u]->link.heap_index=d;
                    heap[d]=heap[u];
                    d=u;
                }

                heap[c]->link.heap_index=d;
                heap[d]=heap[c];
            }
            else available_bitmask&= ~(1<<a->size_factor);///just removed last available allocation of this size

            ///put into unused linked list
            a->link.next=n;

            n=a;
            a=a->left;
            mb->unused_allocation_count++;
        }

        mb->first_unused_allocation=n;
        mb->rightmost_allocation=a;
        if(a)a->right=NULL;
        mb->dynamic_offset=n->offset;///despite having been conceptually moved this data is still available and valid
        mb->available_dynamic_allocation_bitmask=available_bitmask;
    }
    else /// the difficult one...
    {
        while(a->size_factor+1 < mb->num_dynamic_allocation_sizes)/// combine allocations in aligned way
        {
            ///neighbouring allocation for this alignment
            n = a->offset&1<<a->size_factor ? a->left : a->right;/// will never be CVM_VK_NULL_ALLOCATION_INDEX (if was rightmost other branch would be take, if leftmost (offset is 0), right will always be taken)

            if(!n->available || n->size_factor!=a->size_factor)break;///an size factor must be the same size or smaller, can only combine if the same size and available

            ///remove n from its availability heap
            if((c= --mb->available_dynamic_allocations[n->size_factor].count))
            {
                heap=mb->available_dynamic_allocations[n->size_factor].heap;

                d=n->link.heap_index;
                while(d && heap[c]->offset < heap[(u=(d-1)>>1)]->offset)///try moving up
                {
                    heap[u]->link.heap_index=d;
                    heap[d]=heap[u];
                    d=u;
                }
                u=d;
                while((d=(u<<1)+1)<c)///try moving down
                {
                    d+= (d+1<c) && (heap[d+1]->offset < heap[d]->offset);
                    if(heap[c]->offset < heap[d]->offset) break;
                    heap[d]->link.heap_index=u;
                    heap[u]=heap[d];
                    u=d;
                }

                heap[c]->link.heap_index=u;
                heap[u]=heap[c];
            }
            else available_bitmask&= ~(1<<n->size_factor);///just removed last available allocation of this size

            ///change left-right bindings
            if(a->offset&1<<a->size_factor) /// a is right
            {
                a->offset=n->offset;
                ///this should be rare enough to set next and prev during loop and still be the most efficient option
                a->left=n->left;
                if(a->left) a->left->right=a;
            }
            else (a->right=n->right)->left=a;///right CANNOT be NULL, as rightmost allocation cannot be available

            a->size_factor++;

            n->link.next=mb->first_unused_allocation;///put subsumed adjacent allocation in unused list, not worth copying all an back when only 1 value changes
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
            heap[u]->link.heap_index=d;
            heap[d]=heap[u];
            d=u;
        }

        a->link.heap_index=d;
        heap[d]=a;

        available_bitmask|=1<<a->size_factor;

        mb->available_dynamic_allocation_bitmask=available_bitmask;
    }

    atomic_store(&mb->spinlock,0);
}

uint64_t cvm_vk_acquire_static_buffer_allocation(cvm_vk_managed_buffer * mb,uint64_t size,uint64_t alignment)///it is assumed alignment is power of 2, may want to ensure this is the case
{
    uint64_t dynamic_offset,s,e,offset;
    uint_fast32_t lock;

    size=((size+(alignment-1))& ~(alignment-1));///round up size t multiple of alignment (not strictly necessary but w/e)

    if(mb->multithreaded)do lock=atomic_load(&mb->spinlock);
    while(lock!=0 || !atomic_compare_exchange_weak(&mb->spinlock,&lock,1));

    e=mb->static_offset& ~(alignment-1);
    s=((uint64_t)mb->dynamic_offset)<<mb->base_dynamic_allocation_size_factor;

    if(s+size > e || e==size)///must have enough space and cannot use whole buffer for static allocations (returning 0 is reserved for errors)
    {
        atomic_store(&mb->spinlock,0);
        return 0;
    }

    mb->static_offset=e-size;

    atomic_store(&mb->spinlock,0);

    return e-size;
}




