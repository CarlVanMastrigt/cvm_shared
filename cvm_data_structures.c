/**
Copyright 2020,2021,2022 Carl van Mastrigt

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

void cvm_thread_safe_expanding_queue_ini( cvm_thread_safe_expanding_queue * list , uint_fast32_t block_size , size_t type_size )
{
    ///block size must always be power of 2

    uint_fast32_t i;

    block_size--;
    for(i=1;i<sizeof(uint_fast32_t)*8;i<<=1)block_size |= block_size >> i;
    block_size++;

    list->type_size=type_size;
    list->block_size=block_size;

    char * b1=malloc(sizeof(char*)+block_size*type_size);
    char * b2=malloc(sizeof(char*)+block_size*type_size);
    *((char**)b1)=b2;
    *((char**)b2)=NULL;

    list->in_block=b1;
    list->out_block=b1;
    list->end_block=b2;

    atomic_init(&list->spinlock,0);
    atomic_init(&list->in,0);
    atomic_init(&list->out,0);
    atomic_init(&list->in_buffer_fence,block_size);
    atomic_init(&list->out_buffer_fence,block_size);
    atomic_init(&list->in_completions,0);
    atomic_init(&list->out_completions,0);
}

void cvm_thread_safe_expanding_queue_add( cvm_thread_safe_expanding_queue * list , void * value )
{
    char *block,*next_block;

    uint_fast32_t lock,fence,index,index_in_block;

    index=atomic_fetch_add(&list->in,1);
    index_in_block=index&(list->block_size-1);///this works b/c list->block_size is always power of 2

    ///investigate if its possible to use index to do following test, avoiding need for atomic_load
    /// if in_buffer_fence has overflowed need way to properly test if index is an within "in_element", thus add 1/2 range to force index to oveflow as well, allows up to 2^30 threads contesting resource
    ///wait for fence condition to be met (buffer that contains index becomes active)

    if(index > atomic_load(&list->in_buffer_fence)+0x40000000) while(index+0x80000000 >= atomic_load(&list->in_buffer_fence)+0x80000000);
    else while(index >= atomic_load(&list->in_buffer_fence));

    block=list->in_block;
    memcpy(((char*)block)+sizeof(char*)+index_in_block*list->type_size,value,list->type_size);

    if(index_in_block == list->block_size-1)
    {
        while(atomic_load(&list->in_completions) != index);/// wait for all other writes to this element to complete

        do lock=0;
        while( ! atomic_compare_exchange_weak(&list->spinlock,&lock,1));

        next_block= *((char**)block);
        if(next_block == NULL)
        {
            next_block=malloc(sizeof(char*)+list->block_size*list->type_size);
            *((char**)list->end_block)=next_block;
            list->end_block=next_block;
            *((char**)next_block)=NULL;
        }
        list->in_block=next_block;

        atomic_store(&list->spinlock,0);

        atomic_fetch_add(&list->in_buffer_fence,list->block_size);

        atomic_fetch_add(&list->in_completions,1);
    }
    else
    {
        do fence=index;
        while( ! atomic_compare_exchange_weak(&list->in_completions,&fence,index+1));
    }
}

bool cvm_thread_safe_expanding_queue_get( cvm_thread_safe_expanding_queue * list , void * value )
{
    char *block,*next_block;
    uint_fast32_t lock,fence,index,index_in_block;

    index=atomic_load(&list->out);

    do if(index == atomic_load(&list->in_completions)) return false;///next element to read from not available yet, (either because that entry doesn't exist or it hasn't finished being written yet)
    while( ! atomic_compare_exchange_weak(&list->out,&index,index+1));

    index_in_block=index&(list->block_size-1);///this works b/c list->block_size is always power of 2

    ///investigate if its possible to use index to do following test, avoiding need for atomic_load
    /// if out_buffer_fence has overflowed need way to properly test if index is an within "out_element", thus add 1/2 range to force index to oveflow as well, allows up to 2^30 threads contesting resource
    ///wait for fence condition to be met (buffer that contains index becomes active)

    if(index > atomic_load(&list->out_buffer_fence)+0x40000000) while(index+0x80000000 >= atomic_load(&list->out_buffer_fence)+0x80000000);
    else while(index >= atomic_load(&list->out_buffer_fence));


    block=list->out_block;
    memcpy(value,((char*)block)+sizeof(char*)+index_in_block*list->type_size,list->type_size);


    if(index_in_block == list->block_size-1)
    {
        while(atomic_load(&list->out_completions) != index);///wait for all other reads from this element to complete

        do lock=0;
        while( ! atomic_compare_exchange_weak(&list->spinlock,&lock,1));

        next_block= *((char**)block);
        *((char**)list->end_block)=block;
        list->end_block=block;
        *((char**)block)=NULL;
        list->out_block=next_block;

        atomic_store(&list->spinlock,0);

        atomic_fetch_add(&list->out_buffer_fence,list->block_size);

        atomic_fetch_add(&list->out_completions,1);
    }
    else
    {
        do fence=index;
        while( ! atomic_compare_exchange_weak(&list->out_completions,&fence,index+1));
    }

    return true;
}

void cvm_thread_safe_expanding_queue_del( cvm_thread_safe_expanding_queue * list )
{
    char *block,*next_block;

    for(block=list->out_block;block;block=next_block)
    {
        next_block= *((char**)block);
        free(block);
        block=next_block;
    }
}





