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

void cvm_vk_create_managed_buffer(cvm_vk_managed_buffer * mb,uint32_t buffer_size,uint32_t min_size_factor,uint32_t max_size_factor,uint32_t reserved_allocation_count,VkBufferUsageFlags usage)
{
    uint32_t i;
    buffer_size=(((buffer_size-1) >> min_size_factor)+1) << min_size_factor;///round to multiple (complier warns about what i want as needing braces, lol)
    mb->total_space=buffer_size;
    mb->static_offset=buffer_size;
    mb->dynamic_offset=0;

    mb->dynamic_allocations=malloc(sizeof(cvm_vk_dynamic_buffer_allocation)*reserved_allocation_count);
    mb->dynamic_allocation_space=reserved_allocation_count;
    mb->reserved_allocation_count=reserved_allocation_count;

    mb->first_unused_allocation=0;///put all allocations in linked list of unused allocations
    for(i=0;i<reserved_allocation_count-1;i++)mb->dynamic_allocations[i].next=i+1;
    mb->dynamic_allocations[i].next=CVM_VK_NULL_ALLOCATION_INDEX;
    mb->unused_allocation_count=reserved_allocation_count;

    mb->rightmost_allocation=CVM_VK_NULL_ALLOCATION_INDEX;///rightmost doesn't exist as it hasn't been allocated


    mb->num_dynamic_allocation_sizes=max_size_factor-min_size_factor;
    mb->available_dynamic_allocations_start=malloc(sizeof(uint32_t)*mb->num_dynamic_allocation_sizes);
    mb->available_dynamic_allocations_end=malloc(sizeof(uint32_t)*mb->num_dynamic_allocation_sizes);
    for(i=0;i<mb->num_dynamic_allocation_sizes;i++)mb->available_dynamic_allocations_start[i]=mb->available_dynamic_allocations_end[i]=CVM_VK_NULL_ALLOCATION_INDEX;///no available allocations
    mb->available_dynamic_allocation_bitmask=0;

    mb->base_dynamic_allocation_size_factor=min_size_factor;

    #warning mb->mapping=cvm_vk_create_buffer(&mb->buffer,&mb->memory,usage,buffer_size,false);///if this is non-null then probably UMA and thus can circumvent staging buffer
}

void cvm_vk_destroy_managed_buffer(cvm_vk_managed_buffer * mb)
{
    #warning cvm_vk_destroy_buffer(mb->buffer,mb->memory,mb->mapping);

    free(mb->available_dynamic_allocations_start);
    free(mb->available_dynamic_allocations_end);
    free(mb->dynamic_allocations);
}

uint32_t cvm_vk_acquire_dynamic_buffer_allocation(cvm_vk_managed_buffer * mb,uint32_t size)
{
    uint32_t i,size_factor,available_bitmask;
    uint32_t l,r,o,m;
    cvm_vk_dynamic_buffer_allocation ar,al;

    ///linear search should be fine as its assumed the vast majority of buffers will have size factor less than 3 (ergo better than binary search)
    for(size_factor=mb->base_dynamic_allocation_size_factor;1<<size_factor < size;size_factor++);
    size_factor-=mb->base_dynamic_allocation_size_factor;

    /// ~~ lock here ~~

    /// okay, seeing as this function must be as fast and light as possible, we should track unused allocation count to determine if there are enough unused allocations left (with allowance for inaccuracy)
    if(mb->unused_allocation_count < mb->num_dynamic_allocation_sizes) return CVM_VK_NULL_ALLOCATION_INDEX;///there might not be enough unused allocations, early exit with failure (this should VERY rarely, if ever happen)
    ///above ensures there will be enough unused allocations for any case below (is overly conservative. but is a quick, rarely failing test)

    available_bitmask=mb->available_dynamic_allocation_bitmask;

    if(available_bitmask >> size_factor)///there exists an allocation of the desired size or larger
    {
        ///find lowest size with available allocation
        for(i=size_factor;!(available_bitmask>>i&1);i++);

        l=mb->available_dynamic_allocations_start[i];
        al=mb->dynamic_allocations[l];

        mb->available_dynamic_allocations_start[i]=al.next;///remove from linked list

        if(al.next==CVM_VK_NULL_ALLOCATION_INDEX)
        {
            available_bitmask&= ~(1<<i);///if this was the last element of the linked list then remove it from the available bitmask
            mb->available_dynamic_allocations_end[i]=CVM_VK_NULL_ALLOCATION_INDEX;
        }
        else
        {
            mb->dynamic_allocations[al.next].prev=CVM_VK_NULL_ALLOCATION_INDEX;
            al.next=CVM_VK_NULL_ALLOCATION_INDEX;/// prev/next are unnecessary for allocations in use tbh (tho prev is already CVM_VK_NULL_ALLOCATION_INDEX here)
        }

        al.available=false;

        while(i-- > size_factor)
        {
            r=mb->first_unused_allocation;
            mb->first_unused_allocation=mb->dynamic_allocations[r].next;
            mb->unused_allocation_count--;

            ///horizontal linked list stuff
            mb->dynamic_allocations[al.right].left=r;///right cannot be NULL. if it were, then this allocation would have been freed back to the pool (rightmost available allocations are recursively freed)
            ar.right=al.right;
            ar.left=l;
            al.right=r;

            ar.size_factor=al.size_factor=i;
            ar.offset=al.offset+(1<<i);
            ar.available=true;
            ///if this allocation size wasn't empty this code path wouldn't be taken, ergo no linked list checks needed; the list was empty
            ar.prev=ar.next=CVM_VK_NULL_ALLOCATION_INDEX;

            mb->available_dynamic_allocations_start[i]=mb->available_dynamic_allocations_end[i]=r;
            mb->dynamic_allocations[r]=ar;

            available_bitmask|=1<<i;
        }
        #warning if implementing defragger then need to put this in offset ordered (and size tiered?) list of active allocations

        mb->dynamic_allocations[l]=al;
        mb->available_dynamic_allocation_bitmask=available_bitmask;

        /// ~~ unlock here ~~

        return l;
    }
    else///try to allocate from the end of the buffer
    {
        o=mb->dynamic_offset;///store offset locally

        m=(1<<size_factor)-1;

        if( ((((o+m)>>size_factor)<<size_factor) + (1<<size_factor)) << mb->base_dynamic_allocation_size_factor > mb->static_offset)return CVM_VK_NULL_ALLOCATION_INDEX;///not enough memory left
        /// ^ extra brackets to make compiler shut up :sob:

        l=mb->rightmost_allocation;///could have some dummy allocations to avoid need for testing against CVM_VK_NULL_ALLOCATION_INDEX
        ///i is initially set to bitmask of sizes that need new allocations


        for(i=0;o&m;i++) if(o&1<<i)/// o&m allows early exit at very little (if any) cost over i<size_favtor
        {
            r=mb->first_unused_allocation;
            mb->first_unused_allocation=mb->dynamic_allocations[r].next;///r is guaranteed to be a valid location by checking done before starting this loop
            mb->unused_allocation_count--;

            ar.offset=o;
            ar.left=l;///right will get set after the fact
            ar.size_factor=i;
            ar.prev=mb->available_dynamic_allocations_end[i];
            ar.next=CVM_VK_NULL_ALLOCATION_INDEX;///must go at end of available, as by virtue of being allocated from the end it will have the largest offset
            ar.available=true;

            ///we want to sort by available allocations by offset, this one will always go last, ergo need to find last elem of linked list
            ///should be reasonably short so dont store index of last elem (as this adds to complexity and cost elsewhere), just use linear search (at lest for now)

            mb->dynamic_allocations[l].right=r;///l cannot be CVM_VK_NULL_ALLOCATION_INDEX as then alignment would already be satisfied

            ///put in appropriate available allocation linked list
            if(ar.prev==CVM_VK_NULL_ALLOCATION_INDEX) mb->available_dynamic_allocations_start[i]=r;///if none before then this is only entry in list
            else mb->dynamic_allocations[ar.prev].next=r;
            mb->available_dynamic_allocations_end[i]=r;
            available_bitmask|=1<<i;

            mb->dynamic_allocations[r]=ar;

            o+=1<<i;///increment offset

            l=r;
        }

        r=mb->first_unused_allocation;
        mb->first_unused_allocation=mb->dynamic_allocations[r].next;///r is guaranteed to be a valid location by checking done before starting this loop
        mb->unused_allocation_count--;

        ar.offset=o;
        ar.left=l;
        ar.right=CVM_VK_NULL_ALLOCATION_INDEX;
        ar.size_factor=size_factor;
        ar.prev=ar.next=CVM_VK_NULL_ALLOCATION_INDEX;/// prev/next are unnecessary for allocations in use tbh
        ar.available=false;

        if(l!=CVM_VK_NULL_ALLOCATION_INDEX)mb->dynamic_allocations[l].right=r;

        mb->dynamic_allocations[r]=ar;
        mb->rightmost_allocation=r;
        mb->dynamic_offset=o+(1<<size_factor);
        mb->available_dynamic_allocation_bitmask=available_bitmask;

        /// ~~ unlock here ~~

        return r;
    }
}

void cvm_vk_relinquish_dynamic_buffer_allocation(cvm_vk_managed_buffer * mb,uint32_t index)
{
    uint32_t n,available_bitmask;

    cvm_vk_dynamic_buffer_allocation a,an;

    /// ~~ lock here ~~

    available_bitmask=mb->available_dynamic_allocation_bitmask;

    if(index==mb->rightmost_allocation)
    {
        ///free space from right until an unavailable allocation is found
        ///need handling for only 1 alloc, which is rightmost

        n=mb->first_unused_allocation;

        mb->dynamic_allocations[index].next=n;

        n=index;
        index=mb->dynamic_allocations[index].left;
        mb->unused_allocation_count++;

        while(index!=CVM_VK_NULL_ALLOCATION_INDEX && (a=mb->dynamic_allocations[index]).available)
        {
            ///remove from linked list, will always be last (as ll is sorted by offset and this is righmost buffer) so code is simpler
            mb->available_dynamic_allocations_end[a.size_factor]=a.prev;

            if(a.prev==CVM_VK_NULL_ALLOCATION_INDEX)
            {
                mb->available_dynamic_allocations_start[a.size_factor]=CVM_VK_NULL_ALLOCATION_INDEX;
                available_bitmask&= ~(1<<a.size_factor);///if last in LL for this size then clear bitmask accordingly
            }
            else mb->dynamic_allocations[a.prev].next=CVM_VK_NULL_ALLOCATION_INDEX;

            ///put into unused linked list
            mb->dynamic_allocations[index].next=n;

            n=index;
            index=mb->dynamic_allocations[index].left;
            mb->unused_allocation_count++;
        }

        mb->first_unused_allocation=n;
        mb->rightmost_allocation=index;
        if(index!=CVM_VK_NULL_ALLOCATION_INDEX)mb->dynamic_allocations[index].right=CVM_VK_NULL_ALLOCATION_INDEX;
        mb->dynamic_offset=mb->dynamic_allocations[n].offset;
        mb->available_dynamic_allocation_bitmask=available_bitmask;
    }
    else
    {
        a=mb->dynamic_allocations[index];

        while(1)/// combine allocations in aligned way
        {
            ///neighbouring allocation for this alignment
            n = a.offset&1<<a.size_factor ? a.left : a.right;/// will never be CVM_VK_NULL_ALLOCATION_INDEX (if was rightmost other branch would be take, if leftmost (offset is 0), right will always be taken)

            an=mb->dynamic_allocations[n];

            if(!an.available || an.size_factor!=a.size_factor)break;///an size factor must be the same size or smaller, can only combine if the same size and available

            ///remove an from available linked list
            if(an.next==CVM_VK_NULL_ALLOCATION_INDEX) mb->available_dynamic_allocations_end[an.size_factor]=an.prev;
            else mb->dynamic_allocations[an.next].prev=an.prev;

            if(an.prev==CVM_VK_NULL_ALLOCATION_INDEX)
            {
                mb->available_dynamic_allocations_start[an.size_factor]=an.next;
                available_bitmask&= ~((an.next==CVM_VK_NULL_ALLOCATION_INDEX)<<an.size_factor);///if last in LL for this size then clear bitmask accordingly
            }
            else mb->dynamic_allocations[an.prev].next=an.next;

            ///change left-right bindings
            if(a.offset&1<<a.size_factor) /// a is right
            {
                a.offset=an.offset;
                ///this should be rare enough to set next and prev during loop and still be the most efficient option
                a.left=an.left;
                if(a.left!=CVM_VK_NULL_ALLOCATION_INDEX) mb->dynamic_allocations[a.left].right=index;
            }
            else /// a is left
            {
                a.right=an.right;
                mb->dynamic_allocations[a.right].left=index;///right CANNOT be CVM_VK_NULL_ALLOCATION_INDEX, as rightmost allocation cannot be available
            }

            a.size_factor++;

            mb->dynamic_allocations[n].next=mb->first_unused_allocation;///not worth copying all an back when only 1 value changes
            mb->first_unused_allocation=n;
            mb->unused_allocation_count++;
        }

        ///put a in appropriately sized available linked list
        a.available=true;
        a.prev=CVM_VK_NULL_ALLOCATION_INDEX;///should already be the case b/c all unavailable allocations have prev=next=null but w/e
        a.next=mb->available_dynamic_allocations_start[a.size_factor];
        while(a.next!=CVM_VK_NULL_ALLOCATION_INDEX && a.offset > mb->dynamic_allocations[a.next].offset)
        {
            a.prev=a.next;
            a.next=mb->dynamic_allocations[a.next].next;
        }

        if(a.prev==CVM_VK_NULL_ALLOCATION_INDEX) mb->available_dynamic_allocations_start[a.size_factor]=index;
        else mb->dynamic_allocations[a.prev].next=index;

        if(a.next==CVM_VK_NULL_ALLOCATION_INDEX) mb->available_dynamic_allocations_end[a.size_factor]=index;
        else mb->dynamic_allocations[a.next].prev=index;

        available_bitmask|=1<<a.size_factor;

        mb->available_dynamic_allocation_bitmask=available_bitmask;
        mb->dynamic_allocations[index]=a;
    }

    /// ~~ unlock here ~~
}







