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

    mb->last_allocation_index=CVM_VK_NULL_ALLOCATION_INDEX;


    mb->num_dynamic_allocation_sizes=max_size_factor-min_size_factor;
    mb->available_dynamic_allocations=malloc(sizeof(cvm_vk_dynamic_buffer_index)*mb->num_dynamic_allocation_sizes);
    for(i=0;i<mb->num_dynamic_allocation_sizes;i++)mb->available_dynamic_allocations[i]=CVM_VK_NULL_ALLOCATION_INDEX;
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

cvm_vk_dynamic_buffer_index cvm_vk_acquire_dynamic_buffer_allocation(cvm_vk_managed_buffer * mb,uint32_t size)
{
    uint32_t i,size_factor,bitmask;
    cvm_vk_dynamic_buffer_index l,r;
    cvm_vk_dynamic_buffer_allocation ar,al;

    ///linear search should be fine as its assumed the vast majority of buffers will have size factor less than 3 (ergo better than binary search)
    for(size_factor=mb->base_dynamic_allocation_size_factor;1<<size_factor > size;size_factor++);
    size_factor-=mb->base_dynamic_allocation_size_factor;

    ///should probably lock allocation here


    if((bitmask=mb->available_dynamic_allocation_bitmask)>>size_factor)///there exists an allocation of the desired size or larger
    {
        for(i=size_factor;!(bitmask>>i&1);i++);///find lowest size with available allocation

        l=mb->available_dynamic_allocations[i];
        al=mb->dynamic_allocations[l];

        mb->available_dynamic_allocations[i]=al.next;///remove from linked list
        if(al.next==CVM_VK_NULL_ALLOCATION_INDEX)bitmask&= ~(1<<i);///if this was the last element of the linked list then remove it from the bitmask
        else mb->dynamic_allocations[al.next].prev=CVM_VK_NULL_ALLOCATION_INDEX;

        ///previous allocation stays left, this way left can be returned at the end and its offset never needs to be changed!
        while(i-- > size_factor)
        {
            r=;///


            //i--;

            ///horizontal linked list stuff
            mb->dynamic_allocations[al.right].left=r;///right cannot be NULL. if it were, then this allocation would have been freed back to the pool (rightmost relinquished allocations are recursively freed)
            ar.right=al.right;
            ar.left=l;
            al.right=r;

            ar.size_factor=al.size_factor=i;///probably faster than extracting bitfield values
            ar.offset=al.offset+(1<<i);
            ar.relinquished=true;///other bitfield bools need only be set when they are used
            ///if this allocation size wasn't empty this code path wouldn't be taken, ergo no linked list checks needed.
            ar.prev=CVM_VK_NULL_ALLOCATION_INDEX;
            ar.next=CVM_VK_NULL_ALLOCATION_INDEX;
            mb->available_dynamic_allocations[i]=r;
            bitmask|=1<<i;
        }

        al.relinquished=false;
        #warning if implementing defragger then need to put this in offset ordered (and size tiered?) list of active allocations
        al.next=CVM_VK_NULL_ALLOCATION_INDEX;
        ///prev is already CVM_VK_NULL_ALLOCATION_INDEX

        mb->dynamic_allocations[l]=al;
        mb->available_dynamic_allocation_bitmask=bitmask;

        return l;
    }
    else///try to allocate from the end of the buffer
    {
        //
    }

    //for(i=size_factor;)

    return CVM_VK_NULL_ALLOCATION_INDEX;
}

void cvm_vk_relinquish_dynamic_buffer_allocation(cvm_vk_managed_buffer * mb,cvm_vk_dynamic_buffer_index index)
{
    uint32_t size;
    ///should probably locak allocation

    //for()

    return CVM_VK_NULL_ALLOCATION_INDEX;
}







