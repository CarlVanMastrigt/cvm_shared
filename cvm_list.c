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

//static cvm_coherent_large_data_mp_mc_list_element * cvm_coherent_large_data_mp_mc_list_element_ini( cvm_coherent_large_data_mp_mc_list * list )
//{
//    uint32_t i;
//    cvm_coherent_large_data_mp_mc_list_element * elem=malloc(sizeof(cvm_coherent_large_data_mp_mc_list_element));
//    elem->data=malloc(list->block_size*(list->type_size+sizeof(_Atomic uint8_t)));
//
//    for(i=0;i<list->block_size;i++)atomic_init(elem->data+i*(list->type_size+sizeof(_Atomic uint8_t)),0);
//
//    elem->next_element=NULL;
//    return elem;
//}
//
//void cvm_coherent_large_data_mp_mc_list_ini( cvm_coherent_large_data_mp_mc_list * list , uint32_t block_size , size_t type_size )
//{
//    block_size--;
//    block_size |= block_size >> 1;
//    block_size |= block_size >> 2;
//    block_size |= block_size >> 4;
//    block_size |= block_size >> 8;
//    block_size |= block_size >> 16;
//    block_size++;
//
//    ///type size will generally form memory pad for the atomic lock
//    ///should really round this data to ensure memory alignment of contained data, but as only memcpy is used ~should~ be okay (as memcpy operates on single bytes)
//    list->type_size=type_size;
//    list->block_size=block_size;
//    list->active_chunks=1;
//    list->total_chunks=1;
//    ///by default ensure there are at least 4 chunks and remove chunks when less than 1/3 are used
//    list->min_total_chunks=4;
//    list->del_denominator=3;
//    list->del_numerator=1;
//
//    cvm_coherent_large_data_mp_mc_list_element * elem1=cvm_coherent_large_data_mp_mc_list_element_ini(list);
//    cvm_coherent_large_data_mp_mc_list_element * elem2=cvm_coherent_large_data_mp_mc_list_element_ini(list);
//    elem1->next_element=elem2;
//
//    list->in_element=elem1;
//    list->out_element=elem1;
//    list->end_element=elem2;
//
//    atomic_init(&list->spinlock,0);
//    atomic_init(&list->in,0);
//    atomic_init(&list->out,0);
//    atomic_init(&list->in_fence ,block_size);
//    atomic_init(&list->in_op_count,0);
//    atomic_init(&list->out_op_count,0);
//}
//
//void cvm_coherent_large_data_mp_mc_list_set_deletion_data( cvm_coherent_large_data_mp_mc_list * list , uint32_t numerator , uint32_t denominator , uint32_t min_total_chunks )
//{
//    list->del_denominator=denominator;
//    list->del_numerator=numerator;
//    list->min_total_chunks=min_total_chunks;
//}
//
//void cvm_coherent_large_data_mp_mc_list_add( cvm_coherent_large_data_mp_mc_list * list , void * value )
//{
//    cvm_coherent_large_data_mp_mc_list_element *elem,*next_elem;
//
//    uint32_t index=atomic_fetch_add(&list->in,1);
//    uint32_t lock;
//    uint32_t index_in_block=index&(list->block_size-1);///this works b/c list->block_size is always power of 2
//
//    /// if in fence has overflowed(past uint_max) need way to properly test if index is an within "in_element", add 1/2 range to force index to oveflow as well, allows up to 2^30 threads contesting resource
//    if(index > (atomic_load(&list->in_fence)+0x40000000)) while(index+0x80000000 >= atomic_load(&list->in_fence)+0x80000000);
//    else while(index>=atomic_load(&list->in_fence));///regular version when neither would overflow
//
//    elem=list->in_element;///above ensures in element is set properly, (in_fence forms memory barrier)
//
//    uint8_t * data_ptr=elem->data+index_in_block*(list->type_size+sizeof(_Atomic uint8_t));
//
//    memcpy(data_ptr+sizeof(_Atomic uint8_t),value,list->type_size);
//
//    atomic_store((_Atomic uint8_t*)data_ptr,1);///mark entry as having been written (data_ptr atomic is memory barrier)
//
//    if(index_in_block == list->block_size-1)///last element in block
//    {
//        do lock=0;
//        while(!atomic_compare_exchange_weak(&list->spinlock,&lock,1));
//
//        next_elem=elem->next_element;
//
//        if(next_elem==NULL)
//        {
//            list->total_chunks++;
//            next_elem=cvm_coherent_large_data_mp_mc_list_element_ini(list);
//            elem->next_element=next_elem;
//            list->end_element=next_elem;
//        }
//
//        #warning could change to atomic_compare_exchange_weak for in_op_count
//        while(atomic_load(&list->in_op_count) != list->block_size-1);///how much of block has finished writing
//
//        list->in_element=next_elem;
//
//
//        #warning atomic ops inside spinlock is not pretty
//        atomic_store(&list->in_op_count,0);
//        atomic_fetch_add(&list->in_fence,list->block_size);
//        list->active_chunks++;
//
//        atomic_store(&list->spinlock,0);
//    }
//    else atomic_fetch_add(&list->in_op_count,1);
//}
//
//bool cvm_coherent_large_data_mp_mc_list_get( cvm_coherent_large_data_mp_mc_list * list , void * value )
//{
//    cvm_coherent_large_data_mp_mc_list_element *next_elem,*elem;
//    uint32_t index=atomic_load(&list->out);
//    uint32_t lock;
//    uint32_t index_in_block=index&(list->block_size-1);///this works b/c list->block_size is always power of 2
//    bool last=false;
//
//    do
//    {
//        if(  (index==atomic_load(&list->in))  ||  (index == atomic_load(&list->in_fence)-1)  )///dont think in_fence test is necessary
//        {
//            return false;
//        }
//        if((index%list->block_size)==(list->block_size-1))
//        {
//            do lock=0;
//            while(!atomic_compare_exchange_weak(&list->spinlock,&lock,1));
//
//            index=atomic_load(&list->out);
//
//            if(  (index==atomic_load(&list->in))  ||  (index==atomic_load(&list->in_fence)-1)  )
//            {
//                atomic_store(&list->spinlock,0);
//                return false;
//            }
//
//            if((index%list->block_size)==(list->block_size-1))
//            {
//                last=true;
//                break;
//            }
//            else
//            {
//                atomic_store(&list->spinlock,0);
//            }
//        }
//    }
//    while(!atomic_compare_exchange_weak(&list->out,&index,index+1));
//
//    elem=list->out_element;
//
//    uint8_t * data_ptr=elem->data+index_in_block*(list->type_size+sizeof(_Atomic uint8_t));
//
//    while(atomic_load((_Atomic uint8_t*)data_ptr)==0);///wait for data to finish writing when necessary
//
//    memcpy(value,data_ptr+sizeof(_Atomic uint8_t),list->type_size);
//
//    atomic_store((_Atomic uint8_t*)data_ptr,0);
//
//    if(last)
//    {
//        next_elem=elem->next_element;
//
//        while(atomic_load(&list->out_op_count)!=list->block_size-1);
//
//        list->out_element=next_elem;
//        atomic_store(&list->out_op_count,0);
//        atomic_fetch_add(&list->out,1);
//        list->active_chunks--;
//
//        if( ( (list->total_chunks*list->del_numerator)>(list->active_chunks*list->del_denominator) ) && (list->total_chunks>list->min_total_chunks) )
//        {
//            atomic_store(&list->spinlock,0);
//
//            list->total_chunks--;
//            free(elem->data);
//            free(elem);
//        }
//        else
//        {
//            elem->next_element=NULL;
//            list->end_element->next_element=elem;
//            list->end_element=elem;
//
//            atomic_store(&list->spinlock,0);
//        }
//    }
//    else
//    {
//        atomic_fetch_add(&list->out_op_count,0);
//    }
//
//    return 1;
//}
//
//void cvm_coherent_large_data_mp_mc_list_del( cvm_coherent_large_data_mp_mc_list * list )
//{
//    cvm_coherent_large_data_mp_mc_list_element *next,*elem=list->out_element;
//
//    do
//    {
//        free(elem->data);
//        next=elem->next_element;
//        free(elem);
//        elem=next;
//    }
//    while(elem!=NULL);
//}
















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

    ///ERROR HERE, in fence not correct
    ///is + max_entry_count actually correct offset, check w/ 4 buffer, should be though
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
    num_units--;
    num_units |= num_units >> 1;
    num_units |= num_units >> 2;
    num_units |= num_units >> 4;
    num_units |= num_units >> 8;
    num_units |= num_units >> 16;
    num_units++;


    stack->type_size=type_size;

    type_size+=sizeof(void*);

    char * ptr=malloc(num_units*type_size);

    stack->unit_allocation=ptr;

    uint32_t i;

    atomic_init(&stack->allocated,((cvm_lockfree_mp_mc_stack_head){.count=0,.top=NULL}));
    atomic_init(&stack->available,((cvm_lockfree_mp_mc_stack_head){.count=0,.top=ptr}));

    for(i=1;i<num_units;i++)
    {
        *((void**)(ptr+(i-1)*type_size))=ptr+i*type_size;
    }
    *((void**)(ptr+(i-1)*type_size))=NULL;
}

int cvm_lockfree_mp_mc_stack_add( cvm_lockfree_mp_mc_stack * stack , void * value )
{
    cvm_lockfree_mp_mc_stack_head current,replacement;
    void * ptr;

    current=atomic_load(&stack->available);

    do
    {
        if(current.top==NULL)
        {
            return 0;
        }
        ptr=current.top;
        replacement.count=current.count+1;
        replacement.top=*((void**)ptr);
    }
    while(!atomic_compare_exchange_weak(&stack->available,&current,replacement));


    memcpy(((char*)ptr)+sizeof(void*),value,stack->type_size);


    current=atomic_load(&stack->allocated);

    do
    {
        replacement.count=current.count;
        replacement.top=ptr;
        *((void**)ptr)=current.top;
    }
    while(!atomic_compare_exchange_weak(&stack->allocated,&current,replacement));

    return 1;
}

int cvm_lockfree_mp_mc_stack_get( cvm_lockfree_mp_mc_stack * stack , void * value )
{
    cvm_lockfree_mp_mc_stack_head current,replacement;
    void * ptr;

    current=atomic_load(&stack->allocated);

    do
    {
        if(current.top==NULL)
        {
            return 0;
        }
        ptr=current.top;
        replacement.count=current.count+1;
        replacement.top=*((void**)ptr);
    }
    while(!atomic_compare_exchange_weak(&stack->allocated,&current,replacement));


    memcpy(value,((char*)ptr)+sizeof(void*),stack->type_size);


    current=atomic_load(&stack->available);

    do
    {
        replacement.count=current.count;
        replacement.top=ptr;
        *((void**)ptr)=current.top;
    }
    while(!atomic_compare_exchange_weak(&stack->available,&current,replacement));

    return 1;
}

void cvm_lockfree_mp_mc_stack_del( cvm_lockfree_mp_mc_stack * stack )
{
    free(stack->unit_allocation);
}








//void cvm_fast_unsafe_mp_mc_list_ini( cvm_fast_unsafe_mp_mc_list * list , uint32_t list_size , size_t type_size )
//{
//    list_size--;
//    list_size |= list_size >> 1;
//    list_size |= list_size >> 2;
//    list_size |= list_size >> 4;
//    list_size |= list_size >> 8;
//    list_size |= list_size >> 16;
//    list_size++;
//
//    uint32_t i;
//
//    type_size+=sizeof(_Atomic uint8_t);
//    list->data=malloc(list_size*type_size);
//    for(i=0;i<list_size;i++)atomic_init(list->data+i*type_size,CVM_ZERO_8);
//    atomic_init(&list->in,CVM_ZERO_32);
//    atomic_init(&list->out,CVM_ZERO_32);
//    list->list_size=list_size;
//    list->type_size=type_size;
//}
//
//void cvm_fast_unsafe_mp_mc_list_add( cvm_fast_unsafe_mp_mc_list * list , void * value )
//{
//    uint32_t index=atomic_fetch_add(&list->in,CVM_ONE_32);
//
//    uint8_t * data_ptr=list->data+(index%list->list_size)*list->type_size;
//
//    memcpy(data_ptr+sizeof(_Atomic uint8_t),value,list->type_size-sizeof(_Atomic uint8_t));
//
//    atomic_store((_Atomic uint8_t*)data_ptr,CVM_ONE_8);
//}
//
//int cvm_fast_unsafe_mp_mc_list_get( cvm_fast_unsafe_mp_mc_list * list , void * value )
//{
//    uint32_t index=atomic_load(&list->out);
//
//    do
//    {
//        if(index==atomic_load(&list->in))
//        {
//            return 0;
//        }
//    }
//    while(!atomic_compare_exchange_weak(&list->out,&index,index+CVM_ONE_32));
//
//    uint8_t * data_ptr=list->data+(index%list->list_size)*list->type_size;
//
//    while(atomic_load((_Atomic uint8_t*)data_ptr)==CVM_ZERO_8);
//
//    memcpy(value,data_ptr+sizeof(_Atomic uint8_t),list->type_size-sizeof(_Atomic uint8_t));
//
//    atomic_store((_Atomic uint8_t*)data_ptr,CVM_ZERO_8);
//
//    return 1;
//}
//
//void cvm_fast_unsafe_mp_mc_list_del( cvm_fast_unsafe_mp_mc_list * list )
//{
//    free(list->data);
//}




void cvm_fast_unsafe_sp_mc_list_ini( cvm_fast_unsafe_sp_mc_list * list , uint32_t list_size , size_t type_size )
{
    list_size--;
    list_size |= list_size >> 1;
    list_size |= list_size >> 2;
    list_size |= list_size >> 4;
    list_size |= list_size >> 8;
    list_size |= list_size >> 16;
    list_size++;

    list->data=malloc(list_size*type_size);
    atomic_init(&list->in,0);
    atomic_init(&list->fence,0);
    atomic_init(&list->out,0);
    list->list_size=list_size;
    list->type_size=type_size;
}

void cvm_fast_unsafe_sp_mc_list_add( cvm_fast_unsafe_sp_mc_list * list , void * value )
{
    uint32_t index=atomic_fetch_add(&list->in,1);
    memcpy(list->data+(index%list->list_size)*list->type_size,value,list->type_size);
    atomic_fetch_add(&list->fence,1);
}

bool cvm_fast_unsafe_sp_mc_list_get( cvm_fast_unsafe_sp_mc_list * list , void * value )
{
    uint32_t index=atomic_load(&list->out);

    do if(index==atomic_load(&list->fence)) return false;
    while(!atomic_compare_exchange_weak(&list->out,&index,index+1));

    memcpy(value,list->data+(index%list->list_size)*list->type_size,list->type_size);

    return true;
}

void cvm_fast_unsafe_sp_mc_list_del( cvm_fast_unsafe_sp_mc_list * list )
{
    free(list->data);
}











//typedef struct coherent_list_test_data
//{
//    cvm_expanding_mp_mc_list list;
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
//        cvm_expanding_mp_mc_list_add(&data->list,&tmp);
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
//            if(cvm_expanding_mp_mc_list_get(&data->list,&j))atomic_fetch_add(&data->test_total,j);
//            else k++;
//        }
//        else
//        {
//            if(cvm_expanding_mp_mc_list_get(&data->list,&j))atomic_fetch_add(&data->test_total,j);
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
//    cvm_expanding_mp_mc_list_ini(&data.list,1024,sizeof(uint32_t));
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
//    for(i=0;i<out_tc;i++)
//    {
//        SDL_WaitThread(out_threads[i],&t);
//        t_tot+=t;
//    }
//
//    printf("thread test: %d  %u\n",t_tot,atomic_load(&data.test_total));
//
//    cvm_expanding_mp_mc_list_del(&data.list);
//}



typedef struct coherent_list_test_data
{
    cvm_fixed_size_mp_mc_list list;
    _Atomic uint32_t test_total;

    bool running;
    uint32_t cycle_count;
}
coherent_list_test_data;

int test_thread_in(void * in)///writes
{
    uint32_t i,tmp;

    coherent_list_test_data * data=in;

    for(i=0;i < 83*data->cycle_count;i++)
    {
        tmp=i%83;
        while( ! cvm_fixed_size_mp_mc_list_add(&data->list,&tmp));
    }

    return 0;
}

int test_thread_out(void * in)///reads
{
    uint32_t j,k=0;

    coherent_list_test_data * data=in;

    for(;;)
    {
        if(data->running)
        {
            if(cvm_fixed_size_mp_mc_list_get(&data->list,&j))atomic_fetch_add(&data->test_total,j);
            else k++;
        }
        else
        {
            if(cvm_fixed_size_mp_mc_list_get(&data->list,&j))atomic_fetch_add(&data->test_total,j);
            else break;
        }
    }

    return k;
}

void test_coherent_data_structures()
{
    uint32_t i,in_tc,out_tc;
    int t,t_tot=0;

    in_tc=3;
    out_tc=3;

    coherent_list_test_data data;


    cvm_fixed_size_mp_mc_list_ini(&data.list,65536,sizeof(uint32_t));
    atomic_init(&data.test_total,0);
    data.running=true;
    data.cycle_count=60000/in_tc;

    SDL_Thread * in_threads[8];
    SDL_Thread * out_threads[8];

    for(i=0;i<in_tc;i++)in_threads[i]=SDL_CreateThread(test_thread_in,"in thread",&data);
    for(i=0;i<out_tc;i++)out_threads[i]=SDL_CreateThread(test_thread_out,"out thread",&data);

    for(i=0;i<in_tc;i++)SDL_WaitThread(in_threads[i],&t);

    data.running=false;


    for(i=0;i<out_tc;i++)
    {
        SDL_WaitThread(out_threads[i],&t);
        t_tot+=t;
    }

    printf("thread test: %d  %u\n",t_tot,atomic_load(&data.test_total));

    cvm_fixed_size_mp_mc_list_del(&data.list);
}
