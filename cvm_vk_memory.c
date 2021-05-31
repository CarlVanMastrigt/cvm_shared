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
    mb->static_offset=buffer_size;
    mb->dynamic_offset=0;

    mb->dynamic_allocations=malloc(sizeof(cvm_vk_dynamic_buffer_allocation)*reserved_allocation_count);
    mb->dynamic_allocation_space=reserved_allocation_count;
    mb->dynamic_allocation_count=0;
    mb->reserved_allocation_count=reserved_allocation_count;

    mb->first_unused_allocation=0;///put all allocations in linked list of unused allocations
    for(i=0;i<reserved_allocation_count-1;i++)mb->dynamic_allocations[i].next=i+1;
    mb->dynamic_allocations[i].next=CVM_VK_NULL_ALLOCATION_INDEX;
    mb->unused_allocation_count=reserved_allocation_count;

    mb->rightmost_allocation=CVM_VK_NULL_ALLOCATION_INDEX;///rightmost doesn't exist as it hasn't been allocated


    mb->num_dynamic_allocation_sizes=max_size_factor-min_size_factor;
    mb->available_dynamic_allocations=malloc(sizeof(uint32_t)*mb->num_dynamic_allocation_sizes);
    for(i=0;i<mb->num_dynamic_allocation_sizes;i++)mb->available_dynamic_allocations[i]=CVM_VK_NULL_ALLOCATION_INDEX;///no available allocations
    mb->available_dynamic_allocation_bitmask=0;

    mb->base_dynamic_allocation_size_factor=min_size_factor;

    mb->mapping=cvm_vk_create_buffer(&mb->buffer,&mb->memory,usage,buffer_size,false);///if this is non-null then probably UMA and thus can circumvent staging buffer
}

void cvm_vk_destroy_managed_buffer(cvm_vk_managed_buffer * mb)
{
    cvm_vk_destroy_buffer(mb->buffer,mb->memory,mb->mapping);

    free(mb->available_dynamic_allocations);
    free(mb->dynamic_allocations);
}

uint32_t cvm_vk_acquire_dynamic_buffer_allocation(cvm_vk_managed_buffer * mb,uint32_t size)
{
    uint32_t i,size_factor,available_bitmask;
    uint32_t l,r,o;
    cvm_vk_dynamic_buffer_allocation ar,al;

    ///linear search should be fine as its assumed the vast majority of buffers will have size factor less than 3 (ergo better than binary search)
    for(size_factor=mb->base_dynamic_allocation_size_factor;1<<size_factor > size;size_factor++);
    size_factor-=mb->base_dynamic_allocation_size_factor;

    ///should probably lock allocation here

    available_bitmask=mb->available_dynamic_allocation_bitmask;

    /// okay, seeing as this function must be as fast and light as possible, we should track unused allocation count to determine if there are enough quickly at the start (with allowance for inaccuracy)
    if(unused_allocation_count < mb->num_dynamic_allocation_sizes)return CVM_VK_NULL_ALLOCATION_INDEX;///there might not be enough unused allocations, early exit


    if(available_bitmask >> size_factor)///there exists an allocation of the desired size or larger
    {
        ///find lowest size with available allocation and simultaneously ensure there are enough unused allocations
        ///doing this check here ensures that there's no need to recombine if allocation fails due to there not being enough available allocations
        for(i=size_factor;!(available_bitmask>>i&1);i++);

        l=mb->available_dynamic_allocations[i];
        al=mb->dynamic_allocations[l];

        mb->available_dynamic_allocations[i]=al.next;///remove from linked list
        if(al.next==CVM_VK_NULL_ALLOCATION_INDEX)available_bitmask&= ~(1<<i);///if this was the last element of the linked list then remove it from the available bitmask
        else
        {
            mb->dynamic_allocations[al.next].prev=CVM_VK_NULL_ALLOCATION_INDEX;
            al.next=CVM_VK_NULL_ALLOCATION_INDEX;
        }
        ///al.prev is already CVM_VK_NULL_ALLOCATION_INDEX
        al.available=false;

        while(i-- > size_factor)
        {
            r=mb->first_unused_allocation;///need to fill out this linked list at initialisation/expansion to allow this section of code to run quickly with very few branches
            mb->first_unused_allocation=mb->dynamic_allocations[r].next;///r is guaranteed to be a valid location by checking done before starting this loop

            ///horizontal linked list stuff
            mb->dynamic_allocations[al.right].left=r;///right cannot be NULL. if it were, then this allocation would have been freed back to the pool (rightmost available allocations are recursively freed)
            ar.right=al.right;
            ar.left=l;
            al.right=r;

            ar.size_factor=al.size_factor=i;
            ar.offset=al.offset+(1<<i);
            ar.available=true;
            ///if this allocation size wasn't empty this code path wouldn't be taken, ergo no linked list checks needed; the list was empty
            ar.prev=CVM_VK_NULL_ALLOCATION_INDEX;
            ar.next=CVM_VK_NULL_ALLOCATION_INDEX;

            mb->available_dynamic_allocations[i]=r;
            mb->dynamic_allocations[r]=ar;

            available_bitmask|=1<<i;
        }
        #warning if implementing defragger then need to put this in offset ordered (and size tiered?) list of active allocations

        mb->dynamic_allocations[l]=al;
        mb->available_dynamic_allocation_bitmask=available_bitmask;

        return l;
    }
    else///try to allocate from the end of the buffer
    {
        ///ensure there is enough space and are enough unused allocations to get the desired buffer
        ///doing this check here ensures that there's no need to recombine if allocation fails due to there not being enough available allocations
        //((mb->dynamic_offset-1)|((1<<size_factor)-1))+(1<<size_factor)+1;
        //((mb->dynamic_offset-1) & (-(1<<size_factor)))+(2<<size_factor);

        //o=mb->dynamic_offset&((1<<size_factor)-1);
        r=mb->first_unused_allocation;
        o=mb->dynamic_offset&(1<<size_factor)-1;/// offset within aligned block of size 1<<size_factor
        if( (mb->dynamic_offset^o) + (2-!o<<size_factor) << mb->base_dynamic_allocation_size_factor > mb->static_offset)return CVM_VK_NULL_ALLOCATION_INDEX;
        ///not enough memory left or not enough dynamic allocatio

        o= -o & (1<<size_factor)-1;///determine bitmask of sizes that need new allocations
        while(o)
        {
            if(o&1)
            {
                //
            }
        }



    }

    //return l;
}

void cvm_vk_relinquish_dynamic_buffer_allocation(cvm_vk_managed_buffer * mb,uint32_t index)
{
    uint32_t size;
    ///should probably locak allocation
}







