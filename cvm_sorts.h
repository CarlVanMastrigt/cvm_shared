/**
Copyright 2025 Carl van Mastrigt

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



struct cvm_quicksort_node
{
    void* start;
    void* end;
};


/**
 * must define CVM_COMPARE_LT macro before using this function which should be of the form:
 * CVM_COMPARE_LT(const type* a, const TYPE* b) and return `a < b` in desired order (must NOT return `a <= b`)
 *
 * CHUNK_SIZE is the threshold below which to use bubble sort, this alters performance characteristics, but 16, 32 & 64 are all reasonable
*/

#define CVM_QUICKSORT(TYPE, FUNCTION_NAME, CHUNK_SIZE)                                                                 \
void FUNCTION_NAME(TYPE * data, size_t count)                                                                          \
{                                                                                                                      \
    const size_t chunk_size=CHUNK_SIZE;                                                                                \
                                                                                                                       \
    struct cvm_quicksort_node stack[64];                                                                               \
    size_t stack_size;                                                                                                 \
                                                                                                                       \
    TYPE tmp;                                                                                                          \
    TYPE pivot;                                                                                                        \
                                                                                                                       \
    TYPE* start;                                                                                                       \
    TYPE* end;                                                                                                         \
    TYPE* iter_forwards;                                                                                               \
    TYPE* iter_backwards;                                                                                              \
    TYPE* middle;                                                                                                      \
    TYPE* smallest;                                                                                                    \
                                                                                                                       \
    stack_size=0;                                                                                                      \
                                                                                                                       \
    start=data;                                                                                                        \
    end=data+count-1;                                                                                                  \
                                                                                                                       \
    if(count>chunk_size) while(1)                                                                                      \
    {                                                                                                                  \
        /* pre-sort start and end of range */                                                                          \
        if(CVM_COMPARE_LT(end, start))                                                                                 \
        {                                                                                                              \
            tmp = *start;                                                                                              \
            *start = *end;                                                                                             \
            *end = tmp;                                                                                                \
        }                                                                                                              \
                                                                                                                       \
        middle=start + ((end - start) >> 1);                                                                           \
                                                                                                                       \
        /* sort middle relative to start and end, this also also sets the middle(valued) of the 3 as pivot */          \
        if(CVM_COMPARE_LT(end, middle))                                                                                \
        {                                                                                                              \
            pivot = *end;                                                                                              \
            *end = *middle;                                                                                            \
            *middle = pivot;                                                                                           \
        }                                                                                                              \
        else if(CVM_COMPARE_LT(middle, start))                                                                         \
        {                                                                                                              \
            pivot = *start;                                                                                            \
            *start = *middle;                                                                                          \
            *middle = pivot;                                                                                           \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            pivot=*middle;                                                                                             \
        }                                                                                                              \
                                                                                                                       \
        iter_forwards = start;                                                                                         \
        iter_backwards = end;                                                                                          \
                                                                                                                       \
        while(1)                                                                                                       \
        {                                                                                                              \
            /* prefix increment/decrement b/c current iterators have already been sorted */                            \
            /* we want the iters after these while loops to be the first positions that violate pivot sorting */       \
            /* note: we don't specially handle the pivot, so it may end up on either side of range */                  \
            while(CVM_COMPARE_LT((++iter_forwards), (&pivot)));                                                        \
            while(CVM_COMPARE_LT((&pivot), (--iter_backwards)));                                                       \
                                                                                                                       \
            if(iter_backwards<iter_forwards)                                                                           \
            {                                                                                                          \
                break;                                                                                                 \
            }                                                                                                          \
                                                                                                                       \
            tmp = *iter_forwards;                                                                                      \
            *iter_forwards = *iter_backwards;                                                                          \
            *iter_backwards = tmp;                                                                                     \
        }                                                                                                              \
                                                                                                                       \
                                                                                                                       \
                                                                                                                       \
        /* any chunk (either side of pivot after above sort) that is smaller than the `chunk_size` is "sorted" */      \
        /* otherwise one or both of these ranges need processing */                                                    \
        if((iter_backwards - start) < chunk_size)                                                                      \
        {                                                                                                              \
            if((end - iter_forwards) < chunk_size)                                                                     \
            {                                                                                                          \
                /* both parts of this range sorted, get another range to sort (if any are left) */                     \
                if(stack_size==0)                                                                                      \
                {                                                                                                      \
                    break;                                                                                             \
                }                                                                                                      \
                stack_size--;                                                                                          \
                start=stack[stack_size].start;                                                                         \
                end=stack[stack_size].end;                                                                             \
            }                                                                                                          \
            else                                                                                                       \
            {                                                                                                          \
                /* after pivot sorted, before pivot unsorted so sort that range */                                     \
                start = iter_forwards;                                                                                 \
            }                                                                                                          \
        }                                                                                                              \
        else if((end - iter_forwards) < chunk_size)                                                                    \
        {                                                                                                              \
            /* before pivot sorted, after pivot unsorted so sort that range next */                                    \
            end = iter_backwards;                                                                                      \
        }                                                                                                              \
        /* otherwise both chunks (before and after the pivot) are unsorted and must be sufficiently sorted */          \
        /* to avoid overflow of the stack record the larger side to the stack and sort the smaller immediately */      \
        else if((iter_backwards - start) > (end - iter_forwards))                                                      \
        {                                                                                                              \
            stack[stack_size++] = (struct cvm_quicksort_node){.start=start,.end=iter_backwards};                       \
            start=iter_forwards;                                                                                       \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            stack[stack_size++] = (struct cvm_quicksort_node){.start=iter_forwards,.end=end};                          \
            end=iter_backwards;                                                                                        \
        }                                                                                                              \
    }                                                                                                                  \
                                                                                                                       \
    /* find smallest in (at least) first chunk */                                                                      \
    smallest = data;                                                                                                   \
    /* non-inclusive end */                                                                                            \
    end = data + ((chunk_size > count) ? count : chunk_size);                                                          \
    for(iter_forwards = data + 1; iter_forwards < end ; iter_forwards++)                                               \
    {                                                                                                                  \
        if(CVM_COMPARE_LT(iter_forwards, smallest))                                                                    \
        {                                                                                                              \
            smallest = iter_forwards;                                                                                  \
        }                                                                                                              \
    }                                                                                                                  \
    /* move smallest to start */                                                                                       \
    if(data!=smallest)                                                                                                 \
    {                                                                                                                  \
        tmp = *smallest;                                                                                               \
        *smallest = *data;                                                                                             \
        *data = tmp;                                                                                                   \
    }                                                                                                                  \
                                                                                                                       \
    /* bubble sort the whole list, shouldnt have to move any element more than `chunk_size` elements back */           \
    /* inclusive end */                                                                                                \
    end = data + count - 1;                                                                                            \
    for(iter_forwards = data+2; iter_forwards <= end; iter_forwards++)                                                 \
    {                                                                                                                  \
        iter_backwards = iter_forwards - 1;                                                                            \
        if(CVM_COMPARE_LT(iter_forwards, iter_backwards))                                                              \
        {                                                                                                              \
            tmp = *iter_forwards;                                                                                      \
            do                                                                                                         \
            {                                                                                                          \
                iter_backwards[1]=iter_backwards[0];                                                                   \
            }                                                                                                          \
            while(CVM_COMPARE_LT((&tmp), (--iter_backwards)));                                                         \
            iter_backwards[1] = tmp;                                                                                   \
        }                                                                                                              \
    }                                                                                                                  \
}                                                                                                                      \

