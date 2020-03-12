/**
Copyright 2020 Carl van Mastrigt

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

void cvm_expanding_mp_mc_list_ini( cvm_expanding_mp_mc_list * list , uint_fast32_t block_size , size_t type_size )
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

void cvm_expanding_mp_mc_list_add( cvm_expanding_mp_mc_list * list , void * value )
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

bool cvm_expanding_mp_mc_list_get( cvm_expanding_mp_mc_list * list , void * value )
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

void cvm_expanding_mp_mc_list_del( cvm_expanding_mp_mc_list * list )
{
    char *block,*next_block;

    for(block=list->out_block;block;block=next_block)
    {
        next_block= *((char**)block);
        free(block);
        block=next_block;
    }
}




///slower than expanding due to 2x CAS needed in both add/get, rather than just ADD & CAS in add
void cvm_fixed_size_mp_mc_list_ini( cvm_fixed_size_mp_mc_list * list , uint_fast32_t max_entry_count , size_t type_size )
{
    ///block size must always be power of 2

    uint_fast32_t i;

    max_entry_count--;
    for(i=1;i<sizeof(uint_fast32_t)*8;i<<=1)max_entry_count |= max_entry_count >> i;
    max_entry_count++;

    list->type_size=type_size;
    list->max_entry_count=max_entry_count;
    list->data=malloc(max_entry_count*type_size);

    atomic_init(&list->in,0);
    atomic_init(&list->out,0);
    atomic_init(&list->in_fence,max_entry_count);
    atomic_init(&list->out_fence,0);
}

bool cvm_fixed_size_mp_mc_list_add( cvm_fixed_size_mp_mc_list * list , void * value )
{
    uint_fast32_t index,fence;

    index=atomic_load(&list->in);
    do if(index == atomic_load(&list->in_fence)) return false;
    while( ! atomic_compare_exchange_weak(&list->in,&index,index+1));

    memcpy(list->data+list->type_size*(index&(list->max_entry_count-1)),value,list->type_size);

    do fence=index;
    while( ! atomic_compare_exchange_weak(&list->out_fence,&fence,index+1));

    return true;
}

bool cvm_fixed_size_mp_mc_list_get( cvm_fixed_size_mp_mc_list * list , void * value )
{
    uint_fast32_t index,fence;

    index=atomic_load(&list->out);
    do if(index == atomic_load(&list->out_fence)) return false;
    while( ! atomic_compare_exchange_weak(&list->out,&index,index+1));

    memcpy(value,list->data+list->type_size*(index&(list->max_entry_count-1)),list->type_size);

    do fence=index+list->max_entry_count;
    while( ! atomic_compare_exchange_weak(&list->in_fence,&fence,index+list->max_entry_count+1));

    return true;
}

void cvm_fixed_size_mp_mc_list_del( cvm_fixed_size_mp_mc_list * list )
{
    free(list->data);
}






void cvm_lockfree_mp_mc_stack_ini( cvm_lockfree_mp_mc_stack * stack , uint32_t num_units , size_t type_size )
{
    stack->type_size=type_size;

    type_size+=sizeof(char*);

    char * ptr=stack->unit_allocation=malloc(num_units*type_size);

    uint32_t i;

    atomic_init(&stack->allocated,((cvm_lockfree_mp_mc_stack_head){.change_count=0,.first=NULL}));
    atomic_init(&stack->available,((cvm_lockfree_mp_mc_stack_head){.change_count=0,.first=ptr}));

    for(i=1;i<num_units;i++)
    {
        *((char**)(ptr+(i-1)*type_size))=ptr+i*type_size;
    }
    *((char**)(ptr+(i-1)*type_size))=NULL;
}

bool cvm_lockfree_mp_mc_stack_add( cvm_lockfree_mp_mc_stack * stack , void * value )
{
    cvm_lockfree_mp_mc_stack_head current,replacement;
    char * ptr;

    current=atomic_load(&stack->available);
    do
    {
        if(current.first==NULL) return false;
        ptr=current.first;
        replacement.change_count=current.change_count+1;
        replacement.first=*((char**)ptr);
    }
    while(!atomic_compare_exchange_weak(&stack->available,&current,replacement));

    memcpy(ptr+sizeof(char*),value,stack->type_size);

    current=atomic_load(&stack->allocated);
    do
    {
        ///+1 technically not needed here as pointer would change anyway and change count would be forced to increment before extracting and re-inserting same pointer
        replacement.change_count=current.change_count+1;
        replacement.first=ptr;
        *((char**)ptr)=current.first;
    }
    while(!atomic_compare_exchange_weak(&stack->allocated,&current,replacement));


    return true;
}

bool cvm_lockfree_mp_mc_stack_get( cvm_lockfree_mp_mc_stack * stack , void * value )
{
    cvm_lockfree_mp_mc_stack_head current,replacement;
    char * ptr;

    current=atomic_load(&stack->allocated);
    do
    {
        if(current.first==NULL) return false;
        ptr=current.first;
        replacement.change_count=current.change_count+1;
        replacement.first=*((char**)ptr);
    }
    while(!atomic_compare_exchange_weak(&stack->allocated,&current,replacement));

    memcpy(value,ptr+sizeof(char*),stack->type_size);

    current=atomic_load(&stack->available);
    do
    {
        ///+1 technically not needed here as pointer would change anyway and change count would be forced to increment before extracting and re-inserting same pointer
        replacement.change_count=current.change_count+1;
        replacement.first=ptr;
        *((void**)ptr)=current.first;
    }
    while(!atomic_compare_exchange_weak(&stack->available,&current,replacement));

    return true;
}

void cvm_lockfree_mp_mc_stack_del( cvm_lockfree_mp_mc_stack * stack )
{
    free(stack->unit_allocation);
}






void cvm_fast_unsafe_mp_mc_list_ini( cvm_fast_unsafe_mp_mc_list * list , uint_fast32_t max_entry_count , size_t type_size )
{
    ///max_entry_count must always be power of 2

    uint_fast32_t i;

    max_entry_count--;
    for(i=1;i<sizeof(uint_fast32_t)*8;i<<=1)max_entry_count |= max_entry_count >> i;
    max_entry_count++;

    list->data=malloc(max_entry_count*type_size);
    atomic_init(&list->fence,0);
    atomic_init(&list->in,0);
    atomic_init(&list->out,0);
    list->max_entry_count=max_entry_count;
    list->type_size=type_size;
}

void cvm_fast_unsafe_mp_mc_list_add( cvm_fast_unsafe_mp_mc_list * list , void * value )
{
    uint_fast32_t index,fence;
    index=atomic_fetch_add(&list->in,1);

    memcpy(list->data+(index&(list->max_entry_count-1))*list->type_size,value,list->type_size);

    do fence=index;
    while( ! atomic_compare_exchange_weak(&list->fence,&fence,index+1));
}

bool cvm_fast_unsafe_mp_mc_list_get( cvm_fast_unsafe_mp_mc_list * list , void * value )
{
    uint_fast32_t index=atomic_load(&list->out);

    do if(index==atomic_load(&list->fence)) return false;
    while(!atomic_compare_exchange_weak(&list->out,&index,index+1));

    memcpy(value,list->data+(index&(list->max_entry_count-1))*list->type_size,list->type_size);

    return true;
}

void cvm_fast_unsafe_mp_mc_list_del( cvm_fast_unsafe_mp_mc_list * list )
{
    free(list->data);
}







//typedef struct coherent_list_test_data
//{
//    cvm_fast_unsafe_mp_mc_list list;
//    _Atomic uint32_t test_total;
//
//    bool running;
//    uint32_t cycle_count;
//}
//coherent_list_test_data;
//
//int test_thread_in(void * in)///writes
//{
//    uint32_t i,tmp;
//
//    coherent_list_test_data * data=in;
//
//    for(i=0;i < 83*data->cycle_count;i++)
//    {
//        tmp=i%83;
//        //while( ! cvm_fast_unsafe_mp_mc_list_add(&data->list,&tmp));
//        cvm_fast_unsafe_mp_mc_list_add(&data->list,&tmp);
//    }
//
//    return 0;
//}
//
//int test_thread_out(void * in)///reads
//{
//    uint32_t j,k=0;
//
//    coherent_list_test_data * data=in;
//
//    for(;;)
//    {
//        if(data->running)
//        {
//            if(cvm_fast_unsafe_mp_mc_list_get(&data->list,&j))atomic_fetch_add(&data->test_total,j);
//            else k++;
//        }
//        else
//        {
//            if(cvm_fast_unsafe_mp_mc_list_get(&data->list,&j))atomic_fetch_add(&data->test_total,j);
//            else break;
//        }
//    }
//
//    return k;
//}
//
//void test_coherent_data_structures()
//{
//    uint32_t i,in_tc,out_tc;
//    int t,t_tot=0;
//
//    in_tc=3;
//    out_tc=3;
//
//    coherent_list_test_data data;
//
//
//    cvm_fast_unsafe_mp_mc_list_ini(&data.list,50000000,sizeof(uint32_t));
//    atomic_init(&data.test_total,0);
//    data.running=true;
//    data.cycle_count=60000/in_tc;
//
//    SDL_Thread * in_threads[8];
//    SDL_Thread * out_threads[8];
//
//    for(i=0;i<in_tc;i++)in_threads[i]=SDL_CreateThread(test_thread_in,"in thread",&data);
//    for(i=0;i<out_tc;i++)out_threads[i]=SDL_CreateThread(test_thread_out,"out thread",&data);
//
//    for(i=0;i<in_tc;i++)SDL_WaitThread(in_threads[i],&t);
//
//    data.running=false;
//
//
//    for(i=0;i<out_tc;i++)
//    {
//        SDL_WaitThread(out_threads[i],&t);
//        t_tot+=t;
//    }
//
//    printf("thread test: %d  %u\n",t_tot,atomic_load(&data.test_total));
//
//    cvm_fast_unsafe_mp_mc_list_del(&data.list);
//}


//typedef struct coherent_stack_test_data
//{
//    cvm_lockfree_mp_mc_stack stack;
//    _Atomic uint32_t test_total;
//
//    bool running;
//    uint32_t cycle_count;
//}
//coherent_stack_test_data;
//
//int test_thread_in(void * in)///writes
//{
//    uint32_t i,tmp;
//
//    coherent_stack_test_data * data=in;
//
//    for(i=0;i < 83*data->cycle_count;i++)
//    {
//        tmp=i%83;
//        while( ! cvm_lockfree_mp_mc_stack_add(&data->stack,&tmp));
//        //cvm_lockfree_mp_mc_stack_add(&data->stack,&tmp);
//    }
//
//    return 0;
//}
//
//int test_thread_out(void * in)///reads
//{
//    uint32_t j,k=0;
//
//    coherent_stack_test_data * data=in;
//
//    for(;;)
//    {
//        if(data->running)
//        {
//            if(cvm_lockfree_mp_mc_stack_get(&data->stack,&j))atomic_fetch_add(&data->test_total,j);
//            else k++;
//        }
//        else
//        {
//            if(cvm_lockfree_mp_mc_stack_get(&data->stack,&j))atomic_fetch_add(&data->test_total,j);
//            else break;
//        }
//    }
//
//    return k;
//}
//
//void test_coherent_data_structures()
//{
//    uint32_t i,in_tc,out_tc;
//    int t,t_tot=0;
//
//    in_tc=3;
//    out_tc=3;
//
//    coherent_stack_test_data data;
//
//
//    cvm_lockfree_mp_mc_stack_ini(&data.stack,500000,sizeof(uint32_t));
//    atomic_init(&data.test_total,0);
//    data.running=true;
//    data.cycle_count=60000/in_tc;
//
//    SDL_Thread * in_threads[8];
//    SDL_Thread * out_threads[8];
//
//    for(i=0;i<in_tc;i++)in_threads[i]=SDL_CreateThread(test_thread_in,"in thread",&data);
//    for(i=0;i<out_tc;i++)out_threads[i]=SDL_CreateThread(test_thread_out,"out thread",&data);
//
//    for(i=0;i<in_tc;i++)SDL_WaitThread(in_threads[i],&t);
//
//    data.running=false;
//
//
//    for(i=0;i<out_tc;i++)
//    {
//        SDL_WaitThread(out_threads[i],&t);
//        t_tot+=t;
//    }
//
//    printf("thread test: %d  %u\n",t_tot,atomic_load(&data.test_total));
//
//    cvm_lockfree_mp_mc_stack_del(&data.stack);
//}
