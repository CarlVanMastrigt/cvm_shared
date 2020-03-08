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

//vector_converter_data vcd;

static cvm_coherent_large_data_mp_mc_list_element * cvm_coherent_large_data_mp_mc_list_element_ini( cvm_coherent_large_data_mp_mc_list * list )
{
    uint32_t i;
    cvm_coherent_large_data_mp_mc_list_element * elem=malloc(sizeof(cvm_coherent_large_data_mp_mc_list_element));
    elem->data=malloc(list->block_size*list->type_size);

    for(i=0;i<list->block_size;i++)atomic_init(elem->data+i*list->type_size,CVM_ZERO_8);

    elem->next_element=NULL;
    return elem;
}

void cvm_coherent_large_data_mp_mc_list_ini( cvm_coherent_large_data_mp_mc_list * list , uint32_t block_size , size_t type_size )
{
    block_size--;
    block_size |= block_size >> 1;
    block_size |= block_size >> 2;
    block_size |= block_size >> 4;
    block_size |= block_size >> 8;
    block_size |= block_size >> 16;
    block_size++;

    list->type_size=type_size+sizeof(_Atomic uint8_t);
    list->block_size=block_size;
    list->active_chunks=1;
    list->total_chunks=1;
    list->min_total_chunks=4;
    list->del_denominator=3;
    list->del_numerator=1;

    cvm_coherent_large_data_mp_mc_list_element * elem1=cvm_coherent_large_data_mp_mc_list_element_ini(list);
    cvm_coherent_large_data_mp_mc_list_element * elem2=cvm_coherent_large_data_mp_mc_list_element_ini(list);
    elem1->next_element=elem2;

    list->in_element=elem1;
    list->out_element=elem1;
    list->end_element=elem2;

    atomic_init(&list->spinlock,CVM_ZERO_32);
    atomic_init(&list->in,CVM_ZERO_32);
    atomic_init(&list->out,CVM_ZERO_32);
    atomic_init(&list->in_fence,block_size);
    atomic_init(&list->in_op_count,CVM_ZERO_32);
    atomic_init(&list->out_op_count,CVM_ZERO_32);
}

void cvm_coherent_large_data_mp_mc_list_set_deletion_data( cvm_coherent_large_data_mp_mc_list * list , uint32_t numerator , uint32_t denominator , uint32_t min_total_chunks )
{
    list->del_denominator=denominator;
    list->del_numerator=numerator;
    list->min_total_chunks=min_total_chunks;
}

void cvm_coherent_large_data_mp_mc_list_add( cvm_coherent_large_data_mp_mc_list * list , void * value )
{
    cvm_coherent_large_data_mp_mc_list_element *elem,*next_elem;

    uint32_t index=atomic_fetch_add(&list->in,CVM_ONE_32);
    uint32_t lock;

    if(index > (atomic_load(&list->in_fence)+CVM_QTR_32))
    {
        while(index+CVM_MID_32>=atomic_load(&list->in_fence)+CVM_MID_32);
    }
    else
    {
        while(index>=atomic_load(&list->in_fence));
    }

    elem=list->in_element;

    uint8_t * data_ptr=elem->data+(index%list->block_size)*list->type_size;

    memcpy(data_ptr+sizeof(_Atomic uint8_t),value,list->type_size-sizeof(_Atomic uint8_t));

    atomic_store((_Atomic uint8_t*)data_ptr,CVM_ONE_8);

    if((index%list->block_size)==(list->block_size-CVM_ONE_32))
    {
        do
        {
            lock=CVM_ZERO_32;
        }
        while(!atomic_compare_exchange_weak(&list->spinlock,&lock,CVM_ONE_32));

        next_elem=elem->next_element;
        if(next_elem==NULL)
        {
            list->total_chunks++;
            next_elem=cvm_coherent_large_data_mp_mc_list_element_ini(list);
            elem->next_element=next_elem;
            list->end_element=next_elem;
        }

        while(atomic_load(&list->in_op_count)!=list->block_size-CVM_ONE_32);

        list->in_element=next_elem;

        atomic_store(&list->in_op_count,CVM_ZERO_32);
        atomic_fetch_add(&list->in_fence,list->block_size);
        list->active_chunks++;

        atomic_store(&list->spinlock,CVM_ZERO_32);
    }
    else
    {
        atomic_fetch_add(&list->in_op_count,CVM_ONE_32);
    }
}

int cvm_coherent_large_data_mp_mc_list_get( cvm_coherent_large_data_mp_mc_list * list , void * value )
{
    cvm_coherent_large_data_mp_mc_list_element *next_elem,*elem;
    uint32_t index=atomic_load(&list->out);
    uint32_t lock;
    int last=0;

    do
    {
        if(  (index==atomic_load(&list->in))  ||  (index==atomic_load(&list->in_fence)-CVM_ONE_32)  )
        {
            return 0;
        }
        if((index%list->block_size)==(list->block_size-CVM_ONE_32))
        {
            do
            {
                lock=CVM_ZERO_32;
            }
            while(!atomic_compare_exchange_weak(&list->spinlock,&lock,CVM_ONE_32));

            index=atomic_load(&list->out);

            if(  (index==atomic_load(&list->in))  ||  (index==atomic_load(&list->in_fence)-CVM_ONE_32)  )
            {
                atomic_store(&list->spinlock,CVM_ZERO_32);
                return 0;
            }

            if((index%list->block_size)==(list->block_size-CVM_ONE_32))
            {
                last=1;
                break;
            }
            else
            {
                atomic_store(&list->spinlock,CVM_ZERO_32);
            }
        }
    }
    while(!atomic_compare_exchange_weak(&list->out,&index,index+CVM_ONE_32));

    elem=list->out_element;

    uint8_t * data_ptr=elem->data+(index%list->block_size)*list->type_size;

    while(atomic_load((_Atomic uint8_t*)data_ptr)==CVM_ZERO_8);

    memcpy(value,data_ptr+sizeof(_Atomic uint8_t),list->type_size-sizeof(_Atomic uint8_t));

    atomic_store((_Atomic uint8_t*)data_ptr,CVM_ZERO_8);

    if(last)
    {
        next_elem=elem->next_element;

        while(atomic_load(&list->out_op_count)!=list->block_size-CVM_ONE_32);

        list->out_element=next_elem;
        atomic_store(&list->out_op_count,CVM_ZERO_32);
        atomic_fetch_add(&list->out,CVM_ONE_32);
        list->active_chunks--;

        if( ( (list->total_chunks*list->del_numerator)>(list->active_chunks*list->del_denominator) ) && (list->total_chunks>list->min_total_chunks) )
        {
            atomic_store(&list->spinlock,CVM_ZERO_32);

            list->total_chunks--;
            free(elem->data);
            free(elem);
        }
        else
        {
            elem->next_element=NULL;
            list->end_element->next_element=elem;
            list->end_element=elem;

            atomic_store(&list->spinlock,CVM_ZERO_32);
        }
    }
    else
    {
        atomic_fetch_add(&list->out_op_count,CVM_ONE_32);
    }

    return 1;
}

void cvm_coherent_large_data_mp_mc_list_del( cvm_coherent_large_data_mp_mc_list * list )
{
    cvm_coherent_large_data_mp_mc_list_element *next,*elem=list->out_element;

    do
    {
        free(elem->data);
        next=elem->next_element;
        free(elem);
        elem=next;
    }
    while(elem!=NULL);
}















static cvm_coherent_mp_mc_list_element * cvm_coherent_mp_mc_list_element_ini( cvm_coherent_mp_mc_list * list )
{
//    cvm_coherent_mp_mc_list_element * elem=malloc(sizeof(cvm_coherent_mp_mc_list_element));
//    elem->data=malloc(list->block_size*list->type_size);
//    elem->next_element=NULL;
//    return elem;

    cvm_coherent_mp_mc_list_element * elem=malloc(sizeof(cvm_coherent_mp_mc_list_element)+list->block_size*list->type_size);
    elem->next_element=NULL;
    elem->data=((uint8_t*)elem)+sizeof(uint8_t*)+sizeof(cvm_coherent_mp_mc_list_element*);
    return elem;
}





void cvm_coherent_mp_mc_list_ini( cvm_coherent_mp_mc_list * list , uint32_t block_size , size_t type_size )
{
    block_size--;
    block_size |= block_size >> 1;
    block_size |= block_size >> 2;
    block_size |= block_size >> 4;
    block_size |= block_size >> 8;
    block_size |= block_size >> 16;
    block_size++;


    list->type_size=type_size;
    list->block_size=block_size;

    cvm_coherent_mp_mc_list_element * elem1=cvm_coherent_mp_mc_list_element_ini(list);
    cvm_coherent_mp_mc_list_element * elem2=cvm_coherent_mp_mc_list_element_ini(list);
    elem1->next_element=elem2;

    list->in_element=elem1;
    list->out_element=elem1;
    list->end_element=elem2;

    atomic_init(&list->spinlock,CVM_ZERO_32);
    atomic_init(&list->in,CVM_ZERO_32);
    atomic_init(&list->out,CVM_ZERO_32);
    atomic_init(&list->in_buffer_fence,block_size);
    atomic_init(&list->out_buffer_fence,block_size);
    atomic_init(&list->out_fence,CVM_ZERO_32);
    atomic_init(&list->out_completions,CVM_ZERO_32);
    //atomic_init(&list->in_op_count,CVM_ZERO_32);
    //atomic_init(&list->out_op_count,CVM_ZERO_32);
}

void cvm_coherent_mp_mc_list_add( cvm_coherent_mp_mc_list * list , void * value )
{
    cvm_coherent_mp_mc_list_element *elem,*next_elem;

    uint32_t lock,fence,index=atomic_fetch_add(&list->in,CVM_ONE_32);

    if(index > ((uint32_t)(atomic_load(&list->in_buffer_fence)+CVM_QTR_32)))
    {
        while(((uint32_t)(index+CVM_MID_32)) >= ((uint32_t)(atomic_load(&list->in_buffer_fence)+CVM_MID_32)));
    }
    else
    {
        while(index >= ((uint32_t)atomic_load(&list->in_buffer_fence)));
    }



    elem=list->in_element;

    uint8_t * data_ptr=elem->data+(index%list->block_size)*list->type_size;

    memcpy(data_ptr,value,list->type_size);



    if((index%list->block_size) == (list->block_size-CVM_ONE_32))
    {
        do
        {
            lock=CVM_ZERO_32;
        }
        while(!atomic_compare_exchange_weak(&list->spinlock,&lock,CVM_ONE_32));

        next_elem=elem->next_element;
        if(next_elem == NULL)
        {
            next_elem=cvm_coherent_mp_mc_list_element_ini(list);
            list->end_element->next_element=next_elem;
            list->end_element=next_elem;
        }

        while(((uint32_t)(atomic_load(&list->out_fence))) != index);

        list->in_element=next_elem;

        atomic_store(&list->spinlock,CVM_ZERO_32);

        atomic_fetch_add(&list->in_buffer_fence,list->block_size);

        atomic_fetch_add(&list->out_fence,CVM_ONE_32);
    }
    else
    {
        do
        {
            fence=index;
        }
        while(!atomic_compare_exchange_weak(&list->out_fence,&fence,((uint32_t)(index+CVM_ONE_32))));
    }
}

int  cvm_coherent_mp_mc_list_get( cvm_coherent_mp_mc_list * list , void * value )
{
    cvm_coherent_mp_mc_list_element *next_elem,*elem;
    uint32_t lock,fence,index=atomic_load(&list->out);

    do
    {
        if(index == ((uint32_t)atomic_load(&list->out_fence)))
        {
            return 0;
        }
    }
    while(!atomic_compare_exchange_weak(&list->out,&index,((uint32_t)(index+CVM_ONE_32))));

    if(index > ((uint32_t)(atomic_load(&list->out_buffer_fence)+CVM_QTR_32)))
    {
        while(((uint32_t)(index+CVM_MID_32)) >= ((uint32_t)(atomic_load(&list->out_buffer_fence)+CVM_MID_32)));
    }
    else
    {
        while(index >= ((uint32_t)atomic_load(&list->out_buffer_fence)));
    }




    elem=list->out_element;

    uint8_t * data_ptr=elem->data+(index%list->block_size)*list->type_size;

    memcpy(value,data_ptr,list->type_size);





    if((index%list->block_size) == (list->block_size-CVM_ONE_32))
    {
        do
        {
            lock=CVM_ZERO_32;
        }
        while(!atomic_compare_exchange_weak(&list->spinlock,&lock,CVM_ONE_32));

        next_elem=elem->next_element;

        list->end_element->next_element=elem;
        list->end_element=elem;
        elem->next_element=NULL;

        while(((uint32_t)atomic_load(&list->out_completions))!=index);

        list->out_element=next_elem;

        atomic_store(&list->spinlock,CVM_ZERO_32);

        atomic_fetch_add(&list->out_buffer_fence,list->block_size);

        atomic_fetch_add(&list->out_completions,CVM_ONE_32);
    }
    else
    {
        do
        {
            fence=index;
        }
        while(!atomic_compare_exchange_weak(&list->out_completions,&fence,((uint32_t)(index+CVM_ONE_32))));
    }

    return 1;
}

void cvm_coherent_mp_mc_list_del( cvm_coherent_mp_mc_list * list )
{
    cvm_coherent_mp_mc_list_element *next_elem,*elem=list->out_element;

    while(elem!=NULL)
    {
        next_elem=elem->next_element;
        free(elem);
        elem=next_elem;
    }
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

















void cvm_fast_unsafe_mp_mc_list_ini( cvm_fast_unsafe_mp_mc_list * list , uint32_t list_size , size_t type_size )
{
    list_size--;
    list_size |= list_size >> 1;
    list_size |= list_size >> 2;
    list_size |= list_size >> 4;
    list_size |= list_size >> 8;
    list_size |= list_size >> 16;
    list_size++;

    uint32_t i;

    type_size+=sizeof(_Atomic uint8_t);
    list->data=malloc(list_size*type_size);
    for(i=0;i<list_size;i++)atomic_init(list->data+i*type_size,CVM_ZERO_8);
    atomic_init(&list->in,CVM_ZERO_32);
    atomic_init(&list->out,CVM_ZERO_32);
    list->list_size=list_size;
    list->type_size=type_size;
}

void cvm_fast_unsafe_mp_mc_list_add( cvm_fast_unsafe_mp_mc_list * list , void * value )
{
    uint32_t index=atomic_fetch_add(&list->in,CVM_ONE_32);

    uint8_t * data_ptr=list->data+(index%list->list_size)*list->type_size;

    memcpy(data_ptr+sizeof(_Atomic uint8_t),value,list->type_size-sizeof(_Atomic uint8_t));

    atomic_store((_Atomic uint8_t*)data_ptr,CVM_ONE_8);
}

int cvm_fast_unsafe_mp_mc_list_get( cvm_fast_unsafe_mp_mc_list * list , void * value )
{
    uint32_t index=atomic_load(&list->out);

    do
    {
        if(index==atomic_load(&list->in))
        {
            return 0;
        }
    }
    while(!atomic_compare_exchange_weak(&list->out,&index,index+CVM_ONE_32));

    uint8_t * data_ptr=list->data+(index%list->list_size)*list->type_size;

    while(atomic_load((_Atomic uint8_t*)data_ptr)==CVM_ZERO_8);

    memcpy(value,data_ptr+sizeof(_Atomic uint8_t),list->type_size-sizeof(_Atomic uint8_t));

    atomic_store((_Atomic uint8_t*)data_ptr,CVM_ZERO_8);

    return 1;
}

void cvm_fast_unsafe_mp_mc_list_del( cvm_fast_unsafe_mp_mc_list * list )
{
    free(list->data);
}




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
    atomic_init(&list->in,CVM_ZERO_32);
    atomic_init(&list->fence,CVM_ZERO_32);
    atomic_init(&list->out,CVM_ZERO_32);
    list->list_size=list_size;
    list->type_size=type_size;
}

void cvm_fast_unsafe_sp_mc_list_add( cvm_fast_unsafe_sp_mc_list * list , void * value )
{
    uint32_t index=atomic_fetch_add(&list->in,CVM_ONE_32);

    memcpy(list->data+(index%list->list_size)*list->type_size,value,list->type_size);

    atomic_fetch_add(&list->fence,CVM_ONE_32);
}

int  cvm_fast_unsafe_sp_mc_list_get( cvm_fast_unsafe_sp_mc_list * list , void * value )
{
    uint32_t index=atomic_load(&list->out);

    do
    {
        if(index==atomic_load(&list->fence))
        {
            return 0;
        }
    }
    while(!atomic_compare_exchange_weak(&list->out,&index,index+CVM_ONE_32));

    memcpy(value,list->data+(index%list->list_size)*list->type_size,list->type_size);

    return 1;
}

void cvm_fast_unsafe_sp_mc_list_del( cvm_fast_unsafe_sp_mc_list * list )
{
    free(list->data);
}

