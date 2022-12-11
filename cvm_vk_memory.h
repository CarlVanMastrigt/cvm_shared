/**
Copyright 2021,2022 Carl van Mastrigt

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

#ifndef CVM_SHARED_H
#include "cvm_shared.h"
#endif


#ifndef CVM_VK_MEMORY_H
#define CVM_VK_MEMORY_H

typedef struct cvm_vk_staging_buffer_region
{
    uint32_t start;
    uint32_t end;
}
cvm_vk_staging_buffer_region;

typedef struct cvm_vk_staging_buffer
{
    VkBuffer buffer;
    VkDeviceMemory memory;

    VkBufferUsageFlags usage;

    uint32_t total_space;
    uint32_t alignment_size_factor;

    cvm_vk_staging_buffer_region active_region;///region selected for this frame
    atomic_uint_fast32_t active_offset;///need to test atomic version isn't (significatly) slower than non-atomic for single threaded applications

    cvm_vk_staging_buffer_region * acquired_regions;///one per swapchain image
    cvm_vk_staging_buffer_region * available_regions;///will always be less than swapchain image count of these
    uint32_t available_region_count;

    uint8_t * mapping;
}
cvm_vk_staging_buffer;

void cvm_vk_staging_buffer_create(cvm_vk_staging_buffer * sb,VkBufferUsageFlags usage);
void cvm_vk_staging_buffer_update(cvm_vk_staging_buffer * sb,uint32_t space_per_frame,uint32_t frame_count);
void cvm_vk_staging_buffer_destroy(cvm_vk_staging_buffer * sb);

uint32_t cvm_vk_staging_buffer_get_rounded_allocation_size(cvm_vk_staging_buffer * sb,uint32_t allocation_size);

void cvm_vk_staging_buffer_begin(cvm_vk_staging_buffer * sb);
void cvm_vk_staging_buffer_end(cvm_vk_staging_buffer * sb,uint32_t frame_index);

void * cvm_vk_staging_buffer_acquire_space(cvm_vk_staging_buffer * sb,uint32_t allocation_size,VkDeviceSize * acquired_offset);

void cvm_vk_staging_buffer_release_space(cvm_vk_staging_buffer * sb,uint32_t frame_index);






///hmmm, we dont know base_size so deref is needed anyway, and also is pointer so deref needed for that as well...


typedef uint32_t cvm_vk_temporary_buffer_allocation_index;
#define CVM_VK_INVALID_TEMPORARY_ALLOCATION 0xFFFFFFFF


///16 bytes, 4 to a cacheline
typedef struct cvm_vk_temporary_buffer_allocation_data
{
    ///doubt I'll need to support more than 64M allocations, but will fill structure space as efficiently as possible for now, and respect typing
    cvm_vk_temporary_buffer_allocation_index prev;
    cvm_vk_temporary_buffer_allocation_index next;

    uint64_t offset:36;/// actual offset = base_size<<offset_factor
    uint64_t heap_index:22;///4M frees supported should be more than enough
    uint64_t size_factor:5;///32 different sizes is definitely enough,
    uint64_t available:1;///does not have contents, can be retrieved by new allocation or recombined into
    ///get bit for "is_right_buffer" by 1 & (offset >> size_factor) during recombination calculations (256M max buffer size and 256 byte min gives the required size of this)
}
cvm_vk_temporary_buffer_allocation_data;


/// ^ can return offset alongside allocation for efficient use purposes

/// could use more complicated allocator that can recombine sections of arbitrary size with no power of 2 superstructure, may work reasonably well, especially if memory is grouped by expected lifetime
///     ^ this will definitely require a defragger!
///     ^ use expected lifetime to prioritise ordering of sections? will that happen naturally as chunks are allocated/deallocated
///     ^ profile an implementation and only use it if its substantially better than the extant implementation in terms of alloc/dealloc time
///     ^ require all chunks to meet the largest supported types alignment requirements


///need to ensure heap ALWAYS has enough space for this test,create heap with max number of allocations! (2^16)
typedef struct cvm_vk_available_temporary_allocation_heap
{
    cvm_vk_temporary_buffer_allocation_index * heap;///potentially worth testing the effect of storing in a struct with the pointer to the allocation (avoids double indirection in about half of cases)
    uint32_t space;
    uint32_t count;
}
cvm_vk_available_temporary_allocation_heap;

typedef struct cvm_vk_managed_buffer_barrier_list
{
    VkBufferMemoryBarrier2 * barriers;
    uint32_t space;
    uint32_t count;
}
cvm_vk_managed_buffer_barrier_list;

typedef struct cvm_vk_managed_buffer
{
    atomic_uint_fast32_t acquire_spinlock;///put at top in order to get padding between spinlocks (other being copy_spinlock)
    bool multithreaded;

    VkBuffer buffer;
    VkDeviceMemory memory;

    ///VkBufferUsageFlags usage;/// for use if buffer ends up needing recreation

    uint64_t total_space;///actually DO need to track total buffer size so that buffer can be cleaned (all permanent and temporary allocations removed in one go)
    uint64_t permanent_offset;/// taken from end of buffer
    uint32_t temporary_offset;/// taken from start of buffer, is in terms of base offset


    cvm_vk_temporary_buffer_allocation_data * temporary_allocation_data;
    uint32_t temporary_allocation_data_space;

    cvm_vk_temporary_buffer_allocation_index first_unused_temporary_allocation;///singly linked list of allocations to assign space to
    uint32_t unused_temporary_allocation_count;

    cvm_vk_temporary_buffer_allocation_index last_used_allocation;///used for setting left/right of new allocations

    cvm_vk_available_temporary_allocation_heap available_temporary_allocations[32];
    uint32_t available_temporary_allocation_bitmask;
    uint32_t base_temporary_allocation_size_factor;


    uint8_t * mapping;///can copy data in directly (no need for staging) on UMA systems
    /// also operates as flag as to whether staging is necessary and how to handle release/free operations
    /// can hypothetically make regions available straight away after freeing on non-uma systems (no release queue) but probably not worth it to do so and just use the free sequence

/// relevant data for copying, if UMA none of what follows will be used

    VkBufferCopy * pending_copy_actions;///DELETE
    uint32_t pending_copy_space;///DELETE
    uint32_t pending_copy_count;///DELETE

    cvm_vk_managed_buffer_barrier_list copy_release_barriers;
    cvm_vk_managed_buffer_barrier_list copy_acquire_barriers;
    ///could have an arbitrary number of cvm_vk_managed_buffer_barrier_list, allowing for more than 1 frame of delay
    ///but after the number exceeds the number of swapchain images barriers become unnecessary unless transferring between queue families

    uint32_t copy_src_queue_family;///DELETE
    uint32_t copy_dst_queue_family;///DELETE

    uint16_t copy_update_counter;///DELETE
    uint16_t copy_delay;///DELETE
    /// acquire barrier only relevant when a dedicated transfer queue family is involved
    /// is used to acquire ownership of buffer regions after they were released on transfer queue family
    /// when this is the case, upon submitting copies, need to swap these 2 structs and reset the (new) transfer barrier list

    ///fuuuuck copy actions need to happen in correct command buffer which isn't always just the graphics CB!
    /// schedule preemptive local copy actions




    /**
    creating a buffer allocation adds it to the appropriate list of ALL operations that must happen


    for now EVERYTHING will use low priority queue, high priority queue would/will require the staging buffer to be moved around (QFOT)
        and worse, if high priority mode is to be used on CPT queues they will require their own staging buffer OR the QFOT associated with them will need to be performed per region
            QFOT requirement mean for zero latency to be viable the staging buffer will need to be owned by GFX by default and most staging regions will need to be given to DMA en masse
                ^ probably means its better to have staging buffer (or at least staging regions) per queue OR to only allow high priority staging on graphics

    will only technically support 2 destination queue families (compute and graphics) as copy destinations BUT will want varying entry points (queues) for resource acquisition

    add/get entrypoint (index) which it's the users responsibility to handle, 0th is always graphics, so add should specifically call out compute?
        ^ with such a long delay anyway is there any point allowing entrypoints throughout a single queue? probably not



          CPT (this represents direct handoff (no need to init buffer region)
        /
    GFX   GFX   GFX   GFX
        \     X     /
          DMA   DMA
              \
                CPT

    try to avoid same queue semaphores (GFX specifically) because this imposes a global memory barrier and will hurt performance

    acquire memory:
        if non-mapped and graphics queue: its just available
        if non-mapped and compute: it needs to first be transferred to compute from graphics with ALL possible access and stage masks as source
            ^ (will need to use on compute queue but thats always going to be write, perhaps can avoid? probably not worth it)
        if mapped: need to stage and copy, first transferring to DMA queue with all possible access and stage masks as source



    go to access memory: check if its copy has completed: if so add a barrier, either
        immediately in command buffer -- expensive
        or at start of current command buffer -- requires universal use of secondary command buffers to ensure commands are executed at the correct time
            ^ seeing as first use will be needed by DMA end barrier anyway (and so should be submitted as part of initial acquisition) we KNOW which barrier to insert and on which queue/cb so it should be possible to schedule this!
        or submit barrier in this frame and ACTUALLY make it available in the next -- adds delay


        okay
        add_mapped_buffer_range(uint32_t destination_queue_identifier,VkPipelineStageFlags2 stage_mask,VkAccessFlags2 access_mask)



    entrypoint can be per module queue based number - graphics is always queue 0, and have up to N compute queues exist

    FUCK - need to keep track of stage mask required by command buffer (entrypoint) to use with wait/signal semaphores of submission
        ^ add to signal mask upon release (last use in graphics/compute, copy write in DMA)
        ^ add to wait mask upon acquire (first use in graphics/compute, copy write in DMA)
                ^ copy write is only use in DMA really


    per frame here need:

    end of graphics outgoing barriers (GFX to DMA) and matching semaphore based requirements,
            ^ these barriers should NEVER need source access masks or stage masks as all previous ops should have been flushed via a previous barrier upon deletion
            ^ and dst access will always be VK_ACCESS_MEMORY_WRITE_BIT and stage VK_PIPELINE_STAGE_2_COPY_BIT
    the later incoming barriers (DMA to GFX or DMA to CPT) that will be needed, along with matching semaphores to be signaled (match all together?)
    list of copies to do (DMA only, simple)




    DMA queue's timeline semaphore will increase based on amount of work done across all modules so CANNOT use it to track availability (though it is required as wait for on for all receiving entrypoints)
    need to track availability PER ENTRYPOINT, thus resource must know which entrypoint its applicable to (honestly quite reasonable)


    release memory: release must specify correct access and stage masks related to the resources last use, as well as queuing up operation

    if on graphics is made available next frame as its known that actions are completed (is this strictly true? have all caches been flushed in a guaranteed fashion?)
        ^ perhaps still add a barrier, though a single sided one

    if on compute pass back to compute (QFOT)


    as the next usage isn't known for any of these use VK_ACCESS_MEMORY_WRITE_BIT (only write as the contents are effectively invalid) and VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT
        ^ above commands(barriers) should flush all necessary caches, is it then actually necessary to pass resources to the DMA queue (can try it i guess)

    only once the graphics queue has control again can a resource truly be freed

    how to handle deletion on a frame a resource is created? (or before its actually available) ?? -- question for later




    Signalling a semaphore waits for all stages to complete, and all memory accesses are made available automatically. Similarly, waiting for a semaphore will make all memory accesses available, and prevent further work from being started until it is signalled. Note that in the case of QueueSubmit there is an explicit set of stages to prevent running in VkSubmitInfo::pWaitDstStageMask - for all other semaphore uses execution of all work is prevented.

        ^ barrier still needed for QFOT


             o------------------------- barriers/copies generated during GFX, so as long as all lists of barriers get executed at the start they should be fine to recycle immediately
             |                                         ^ NO! INCORRECT! the return to GFX doesnt happen in the following GFX iteration but the one after that so we need to cycle this shit!
             |              o----- THIS ONE this is the only data that actually needs to get stored long term
             |              |
    GFX->DMA(b) (b)copy(b) (b)DMA->GFX
                         ^ actually DST (w/e the destination is)
    */

    /// following part only is destination queue/cb independent so should/may be stored here (perhaps separate out?)

    cvm_vk_staging_buffer * staging_buffer;///for when mapping is not available :p

    uint16_t cycle_counter;///needs to ONLY ever be incremented once a frame, used for comparing to values stored on buffer ranges

    ///will always be the same regardless of where the contents end up, should be performed in same "frame" the were generated
    VkBufferCopy * pending_copy_actions_;
    uint32_t pending_copy_space_;
    uint32_t pending_copy_count_;

    ///following completely irrelevant as no QFOT is required, see excerpt from spec below
    ///cvm_vk_managed_buffer_barrier_list graphics_to_transfer_release;
    ///cvm_vk_managed_buffer_barrier_list graphics_to_transfer_acquire;


    atomic_uint_fast32_t copy_spinlock;///put at bottom in order to get padding between spinlocks (other being acquire_spinlock)
}
cvm_vk_managed_buffer;

///this can be cleaned up in "the usual fashion" but avoids any need for superfluous semaphores (at the expense of some delay in data availability)
/// being put in the critical section does make me uneasy to be honest...
/// need to ensure any async compute which doesn't make up a requirement for this frames graphics is handled properly by overarching system... (last compute in queue this frame should signal cpu read semaphore)
typedef struct cvm_vk_managed_buffer_deletion_queue
{
    uint32_t * allocation_indices;
    uint32_t count;
    uint32_t space;
}
cvm_vk_managed_buffer_deletion_queue;


/// deletions happen at the end of the frame will save hassle. assuming we're using the per queue semaphore requirement (which i now think is a good idea)
///     ^ store most recent semaphore per relevant queue and add it as a wait for each and every "fisrt per frame" submission of a *different* queue

/// transfer needs to wait on appropriate graphics semaphore with stages VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT from queues where deletions happened (and the same for compute queues with VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT)
///     ^ this is a simple solution to a major problem, i LOVE it, it also reduces number of semaphore waits IF sources of deletions (command buffer/queue and last use) can actually be tracked!
///         ^ because transfer wont be executed every frame, will need to store the latest semaphore and keep masking stages (note compute only has one stage :p) until transfer DOES actually occur

///     ^ technically doesn't solve issue of re use without a trip through DMA queue, essentially means first submit on any queue will have to wait on last submit of every other queue...
///         ^ unless we track all deletions and creations :/
///         ^ ACTUALLY, each queue/block could be responsible for ensuring "frees" only happen execution has definitely finished (CPU side signal wait/check at least once every frame)
///             ^ only signalled if/when deletion ops are needed
///                 ^ gets messy really fast, last-first per queue semaphore dependency is fine IMO, with managed buffer release

typedef struct ///this whole struct is flushed/executed at the start of a queue (usually per module, though not necessarily) then filled up again throughout the frame
{
    VkBufferMemoryBarrier2 * transfer_release_barriers;///QFOT releases, on transfer thread
    VkBufferMemoryBarrier2 * acquire_barriers;///QFOT acquires, emptied/invalidated at start of frame then built up as transfer releases are

    cvm_vk_timeline_semaphore transfer_complete_signal;///transfer complete semaphore from last frame, waited upon by next graphics, always with stages being

    ///FUUUCK deletions REALLY need a CPU side wait, don't they... UNLESS we REALLY do stack up all deletion sources and properly wait on them per managed buffer... (might not actually be so bad....)
    ///     ^ there should be benefit in improved timing if ALWAYS recording last usages of managed buffer per queue (the semaphore and appropriate stages)
    /// REQUIRE that deleted resource is NOT used this frame
}
cvm_vk_managed_buffer_pending_events;

///called with command buffers (payloads) that do work on the resources
void cvm_vk_managed_buffer_add_work_acquire_barriers(cvm_vk_managed_buffer * mb,cvm_vk_module_work_payload * payload);
void cvm_vk_managed_buffer_add_work_release_barriers(cvm_vk_managed_buffer * mb,cvm_vk_module_work_payload * payload);

///called solely on transfer (DMA) payloads/queues
void cvm_vk_managed_buffer_add_copy_acquire_barriers(cvm_vk_managed_buffer * mb,cvm_vk_module_work_payload * payload);
void cvm_vk_managed_buffer_add_copy_release_barriers(cvm_vk_managed_buffer * mb,cvm_vk_module_work_payload * payload);




///though perhaps "invalid" when not transferring ownership of a freed buffer region to the transfer queue to have new writes done (no QFOT on those regions, and doing so would require knowledge of ownership prior to blocks being coalesced) this *should* be fine if we're relying on command buffer having completed (timeline semaphore)
/// ^ no, if all caches were flushed upon semaphore then no barriers would be needed to begin with, need some kind of synchronisation primitive between queues upon deletion, as part of deletion paradigm move resources back to the graphics queue (if necessary) then treat all coalesced resources as belonging to the graphics queue (need some way to acquire on compute should that be first use case though)
///because all barriers must be matched there may end up being some hanging barriers that i'll need to prevent/deal with

///     ^ though... we know via semaphores that the region has finished being read from... that SHOULD be sufficient as well to handle new writes to recycled gpu memory (but doesnt handle QFOT...)
///                                                  ^ lack of QFOT not an issue b/c we're discarding contents anyway?

/**
from the spec:

If memory dependencies are correctly expressed between uses of such a resource between two
queues in different families, but no ownership transfer is defined, the contents of that resource are
undefined for any read accesses performed by the second queue family.

Note
If an application does not need the contents of a resource to remain valid when
transferring from one queue family to another, then the ownership transfer
SHOULD be skipped.


fuck. yes.   confirmation
*/

void cvm_vk_managed_buffer_create(cvm_vk_managed_buffer * buffer,uint32_t buffer_size,uint32_t min_size_factor,VkBufferUsageFlags usage,bool multithreaded,bool host_visible);
void cvm_vk_managed_buffer_destroy(cvm_vk_managed_buffer * mb);

bool cvm_vk_managed_buffer_acquire_temporary_allocation(cvm_vk_managed_buffer * mb,uint64_t size,cvm_vk_temporary_buffer_allocation_index * allocation_index,uint64_t * offset);
void cvm_vk_managed_buffer_release_temporary_allocation(cvm_vk_managed_buffer * mb,cvm_vk_temporary_buffer_allocation_index allocation_index);

uint64_t cvm_vk_managed_buffer_acquire_permanent_allocation(cvm_vk_managed_buffer * mb,uint64_t size,uint64_t alignment);///cannot be released, exists until entire buffer is cleared or deleted

/// from spec regarding QFOT: A release operation is used to release exclusive ownership of a range of a buffer or image subresource range.
/// so availibility being moved for only

/// need high priority and low priority variants of these?
void * cvm_vk_managed_buffer_get_permanent_allocation_mapping(cvm_vk_managed_buffer * mb,uint64_t offset,uint64_t size,VkPipelineStageFlags2 stage_mask,VkAccessFlags2 access_mask,uint16_t * availability_token);
void * cvm_vk_managed_buffer_get_temporary_allocation_mapping(cvm_vk_managed_buffer * mb,uint64_t offset,uint64_t size,VkPipelineStageFlags2 stage_mask,VkAccessFlags2 access_mask,uint16_t * availability_token);

void cvm_vk_managed_buffer_submit_all_acquire_barriers(cvm_vk_managed_buffer * mb,VkCommandBuffer graphics_cb);
void cvm_vk_managed_buffer_submit_all_pending_copy_actions(cvm_vk_managed_buffer * mb,VkCommandBuffer transfer_cb);///includes necessary barriers
/// for the love of god only call this once per buffer per frame, transfer and graphics cb necessary for case of QFOT (process acquisitions for previous frames releases)

void cvm_vk_managed_buffer_bind_as_vertex(VkCommandBuffer cmd_buf,cvm_vk_managed_buffer * mb,uint32_t binding);
void cvm_vk_managed_buffer_bind_as_index(VkCommandBuffer cmd_buf,cvm_vk_managed_buffer * mb,VkIndexType type);







///rename as transient upload buffer as transient workspace (GPU only) buffer would also be a useful paradigm to have access to
typedef struct cvm_vk_transient_buffer
{
    VkBuffer buffer;
    VkDeviceMemory memory;

    VkBufferUsageFlags usage;

    uint32_t total_space;
    uint32_t space_per_frame;
    uint32_t max_offset;///fence, should be updated before multithreading becomes applicable this frame
    uint32_t alignment_size_factor;
    atomic_uint_fast32_t current_offset;
    ///need to test atomic version isn't (significatly) slower than non-atomic

    uint8_t * mapping;
}
cvm_vk_transient_buffer;

void cvm_vk_transient_buffer_create(cvm_vk_transient_buffer * tb,VkBufferUsageFlags usage);
void cvm_vk_transient_buffer_update(cvm_vk_transient_buffer * tb,uint32_t space_per_frame,uint32_t frame_count);
void cvm_vk_transient_buffer_destroy(cvm_vk_transient_buffer * tb);

uint32_t cvm_vk_transient_buffer_get_rounded_allocation_size(cvm_vk_transient_buffer * tb,uint32_t allocation_size,uint32_t alignment);
///having fixed locations wont work if non PO2 alignments are required, though this could be fixed by binding at a set offset

void cvm_vk_transient_buffer_begin(cvm_vk_transient_buffer * tb,uint32_t frame_index);
void cvm_vk_transient_buffer_end(cvm_vk_transient_buffer * tb);

///alignment 0 indicates to use base buffer alignment, likely because it will be bound with this offset rather than at 0 and indexed into
void * cvm_vk_transient_buffer_get_allocation(cvm_vk_transient_buffer * tb,uint32_t allocation_size,uint32_t alignment,VkDeviceSize * acquired_offset);

void cvm_vk_transient_buffer_bind_as_vertex(VkCommandBuffer cmd_buf,cvm_vk_transient_buffer * tb,uint32_t binding,VkDeviceSize offset);
void cvm_vk_transient_buffer_bind_as_index(VkCommandBuffer cmd_buf,cvm_vk_transient_buffer * tb,VkIndexType type,VkDeviceSize offset);




#endif

