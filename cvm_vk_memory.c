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
    for(i=0;i<reserved_allocation_count-1;i++)mb->dynamic_allocations[i].link=i+1;
    mb->dynamic_allocations[i].link=CVM_VK_NULL_ALLOCATION_INDEX;
    mb->unused_allocation_count=reserved_allocation_count;

    mb->rightmost_allocation=CVM_VK_NULL_ALLOCATION_INDEX;///rightmost doesn't exist as it hasn't been allocated


    mb->num_dynamic_allocation_sizes=max_size_factor-min_size_factor;
    mb->available_dynamic_allocations=malloc(sizeof(cvm_vk_availablility_heap)*mb->num_dynamic_allocation_sizes);
    for(i=0;i<mb->num_dynamic_allocation_sizes;i++)
    {
        mb->available_dynamic_allocations[i].indices=malloc(sizeof(uint32_t)*16);
        mb->available_dynamic_allocations[i].space=16;
        mb->available_dynamic_allocations[i].count=0;
    }
    mb->available_dynamic_allocation_bitmask=0;

    mb->base_dynamic_allocation_size_factor=min_size_factor;

    #warning mb->mapping=cvm_vk_create_buffer(&mb->buffer,&mb->memory,usage,buffer_size,false);///if this is non-null then probably UMA and thus can circumvent staging buffer
}

void cvm_vk_destroy_managed_buffer(cvm_vk_managed_buffer * mb)
{
    #warning cvm_vk_destroy_buffer(mb->buffer,mb->memory,mb->mapping);
    uint32_t i;
    for(i=0;i<mb->num_dynamic_allocation_sizes;i++)free(mb->available_dynamic_allocations[i].indices);
    free(mb->available_dynamic_allocations);
    free(mb->dynamic_allocations);
}

uint32_t cvm_vk_acquire_dynamic_buffer_allocation(cvm_vk_managed_buffer * mb,uint32_t size)
{
    uint32_t i,l,r,o,m,u,d,c,size_factor,available_bitmask;
    uint32_t * indices;
    cvm_vk_dynamic_buffer_allocation ar,al;
    cvm_vk_dynamic_buffer_allocation * allocations=mb->dynamic_allocations;

    ///linear search should be fine as its assumed the vast majority of buffers will have size factor less than 3 (ergo better than binary search)
    for(size_factor=mb->base_dynamic_allocation_size_factor;1<<size_factor < size;size_factor++);
    size_factor-=mb->base_dynamic_allocation_size_factor;

    if(size_factor>=mb->num_dynamic_allocation_sizes)return CVM_VK_NULL_ALLOCATION_INDEX;

    /// ~~ lock here ~~

    /// okay, seeing as this function must be as fast and light as possible, we should track unused allocation count to determine if there are enough unused allocations left (with allowance for inaccuracy)
    if(mb->unused_allocation_count < mb->num_dynamic_allocation_sizes) return CVM_VK_NULL_ALLOCATION_INDEX;///there might not be enough unused allocations, early exit with failure (this should VERY rarely, if ever happen)
    ///above ensures there will be enough unused allocations for any case below (is overly conservative. but is a quick, rarely failing test)

    available_bitmask=mb->available_dynamic_allocation_bitmask;

    if(available_bitmask >> size_factor)///there exists an allocation of the desired size or larger
    {
        ///find lowest size with available allocation
        for(i=size_factor;!(available_bitmask>>i&1);i++);

        indices=mb->available_dynamic_allocations[i].indices;

        l=indices[0];
        al=allocations[l];

        if((c= --mb->available_dynamic_allocations[i].count))///as we need
        {
            u=0;

            while((d=(u<<1)+1)<c)
            {
                d+= (d+1<c) && (allocations[indices[d+1]].offset < allocations[indices[d]].offset);
                if(allocations[indices[c]].offset < allocations[indices[d]].offset) break;
                allocations[indices[d]].link=u;///need to know (new) index in heap
                indices[u]=indices[d];
                u=d;
            }

            allocations[indices[c]].link=u;
            indices[u]=indices[c];
        }
        else available_bitmask&= ~(1<<i);

        al.available=false;
        al.link=CVM_VK_NULL_ALLOCATION_INDEX;///not in any linked list
        al.size_factor=size_factor;

        while(i-- > size_factor)
        {
            r=mb->first_unused_allocation;
            mb->first_unused_allocation=allocations[r].link;
            mb->unused_allocation_count--;

            ///horizontal linked list stuff
            allocations[al.right].left=r;///right cannot be NULL. if it were, then this allocation would have been freed back to the pool (rightmost available allocations are recursively freed)
            ar.right=al.right;
            ar.left=l;
            al.right=r;

            ar.size_factor=i;
            ar.offset=al.offset+(1<<i);
            ar.available=true;
            ///if this allocation size wasn't empty, then this code path wouldn't be taken, ergo addition to heap is super simple
            ar.link=0;///index in heap

            mb->available_dynamic_allocations[i].indices[0]=r;
            mb->available_dynamic_allocations[i].count=1;

            allocations[r]=ar;

            available_bitmask|=1<<i;
        }
        #warning if implementing defragger then need to put this in offset ordered (and size tiered?) list of active allocations

        allocations[l]=al;
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
            mb->first_unused_allocation=allocations[r].link;///r is guaranteed to be a valid location by checking done before starting this loop
            mb->unused_allocation_count--;

            ar.offset=o;
            ar.left=l;///right will get set after the fact
            ar.size_factor=i;
            ar.available=true;
            ///add to heap
            ar.link=mb->available_dynamic_allocations[i].count++;///by virtue of being allocated from the end it will have the largest offset, thus will never need to move up the heap

            if(ar.link==mb->available_dynamic_allocations[i].space)
            {
                mb->available_dynamic_allocations[i].indices=realloc(mb->available_dynamic_allocations[i].indices,sizeof(uint32_t)*(mb->available_dynamic_allocations[i].space*=2));
            }
            mb->available_dynamic_allocations[i].indices[ar.link]=r;

            available_bitmask|=1<<i;

            allocations[l].right=r;///l cannot be CVM_VK_NULL_ALLOCATION_INDEX as then alignment would already be satisfied

            allocations[r]=ar;

            o+=1<<i;///increment offset

            l=r;
        }

        r=mb->first_unused_allocation;
        mb->first_unused_allocation=allocations[r].link;///r is guaranteed to be a valid location by checking done before starting this loop
        mb->unused_allocation_count--;

        ar.offset=o;
        ar.left=l;
        ar.right=CVM_VK_NULL_ALLOCATION_INDEX;
        ar.size_factor=size_factor;
        ar.link=CVM_VK_NULL_ALLOCATION_INDEX;/// prev/next are unnecessary for allocations in use tbh
        ar.available=false;

        if(l!=CVM_VK_NULL_ALLOCATION_INDEX)allocations[l].right=r;

        allocations[r]=ar;

        mb->rightmost_allocation=r;
        mb->dynamic_offset=o+(1<<size_factor);
        mb->available_dynamic_allocation_bitmask=available_bitmask;

        /// ~~ unlock here ~~

        return r;
    }
}

void cvm_vk_relinquish_dynamic_buffer_allocation(cvm_vk_managed_buffer * mb,uint32_t index)
{
    uint32_t n,u,d,c,available_bitmask;
    uint32_t * indices;
    cvm_vk_dynamic_buffer_allocation a,an;
    cvm_vk_dynamic_buffer_allocation * allocations=mb->dynamic_allocations;

    /// ~~ lock here ~~

    available_bitmask=mb->available_dynamic_allocation_bitmask;

    if(index==mb->rightmost_allocation)
    {
        ///free space from right until an unavailable allocation is found
        ///need handling for only 1 alloc, which is rightmost

        n=mb->first_unused_allocation;

        allocations[index].link=n;

        n=index;
        index=allocations[index].left;
        mb->unused_allocation_count++;

        while(index!=CVM_VK_NULL_ALLOCATION_INDEX && (a=allocations[index]).available)
        {
            ///remove from heap, index cannot have any children as it's the allocation with the largest offset
            if((c= --mb->available_dynamic_allocations[a.size_factor].count))
            {
                indices=mb->available_dynamic_allocations[a.size_factor].indices;

                d=a.link;
                while(d && allocations[indices[c]].offset < allocations[indices[(u=(d-1)>>1)]].offset)
                {
                    allocations[indices[u]].link=d;
                    indices[d]=indices[u];
                    d=u;
                }

                allocations[indices[c]].link=d;
                indices[d]=indices[c];
            }
            else available_bitmask&= ~(1<<a.size_factor);///just removed last available allocation of this size

            ///put into unused linked list
            allocations[index].link=n;

            n=index;
            index=allocations[index].left;
            mb->unused_allocation_count++;
        }

        mb->first_unused_allocation=n;
        mb->rightmost_allocation=index;
        if(index!=CVM_VK_NULL_ALLOCATION_INDEX)allocations[index].right=CVM_VK_NULL_ALLOCATION_INDEX;
        mb->dynamic_offset=allocations[n].offset;///despite having been conceptually moved this data is still available and valid
        mb->available_dynamic_allocation_bitmask=available_bitmask;
    }
    else /// the difficult one...
    {
        a=allocations[index];

        while(a.size_factor+1 < mb->num_dynamic_allocation_sizes)/// combine allocations in aligned way
        {
            ///neighbouring allocation for this alignment
            n = a.offset&1<<a.size_factor ? a.left : a.right;/// will never be CVM_VK_NULL_ALLOCATION_INDEX (if was rightmost other branch would be take, if leftmost (offset is 0), right will always be taken)

            an=allocations[n];

            if(!an.available || an.size_factor!=a.size_factor)break;///an size factor must be the same size or smaller, can only combine if the same size and available

            ///remove an from its availability heap
            if((c= --mb->available_dynamic_allocations[an.size_factor].count))
            {
                indices=mb->available_dynamic_allocations[an.size_factor].indices;

                d=an.link;
                ///first try moving up the ranks
                while(d && allocations[indices[c]].offset < allocations[indices[(u=(d-1)>>1)]].offset)///if indices[c]==index then this is never true and branch does basically nothing
                {
                    allocations[indices[u]].link=d;
                    indices[d]=indices[u];
                    d=u;
                }
                u=d;///swaping these for clarity...
                ///then try moving down the ranks
                while((d=(u<<1)+1)<c)
                {
                    d+= (d+1<c) && (allocations[indices[d+1]].offset < allocations[indices[d]].offset);
                    if(allocations[indices[c]].offset < allocations[indices[d]].offset) break;
                    allocations[indices[d]].link=u;
                    indices[u]=indices[d];
                    u=d;
                }

                allocations[indices[c]].link=u;
                indices[u]=indices[c];
            }
            else available_bitmask&= ~(1<<an.size_factor);///just removed last available allocation of this size

            ///change left-right bindings
            if(a.offset&1<<a.size_factor) /// a is right
            {
                a.offset=an.offset;
                ///this should be rare enough to set next and prev during loop and still be the most efficient option
                a.left=an.left;
                if(a.left!=CVM_VK_NULL_ALLOCATION_INDEX) allocations[a.left].right=index;
            }
            else /// a is left
            {
                a.right=an.right;
                mb->dynamic_allocations[a.right].left=index;///right CANNOT be CVM_VK_NULL_ALLOCATION_INDEX, as rightmost allocation cannot be available
            }

            a.size_factor++;

            allocations[n].link=mb->first_unused_allocation;///put subsumed adjacent allocation in unused list, not worth copying all an back when only 1 value changes
            mb->first_unused_allocation=n;
            mb->unused_allocation_count++;
        }

        /// add a to appropriate availability heap
        a.available=true;

        d=mb->available_dynamic_allocations[a.size_factor].count++;///by virtue of being allocated from the end it will have the largest offset, thus will never need to move up the heap
        indices=mb->available_dynamic_allocations[a.size_factor].indices;

        if(d==mb->available_dynamic_allocations[a.size_factor].space)
        {
            indices=mb->available_dynamic_allocations[a.size_factor].indices=realloc(indices,sizeof(uint32_t)*(mb->available_dynamic_allocations[a.size_factor].space*=2));
        }

        while(d && a.offset < allocations[indices[(u=(d-1)>>1)]].offset)
        {
            allocations[indices[u]].link=d;
            indices[d]=indices[u];
            d=u;
        }

        a.link=d;
        indices[d]=index;

        available_bitmask|=1<<a.size_factor;

        mb->available_dynamic_allocation_bitmask=available_bitmask;
        allocations[index]=a;
    }

    /// ~~ unlock here ~~
}







