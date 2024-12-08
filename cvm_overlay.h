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

#ifndef CVM_SHARED_H
#include "cvm_shared.h"
#endif

#ifndef CVM_OVERLAY_H
#define CVM_OVERLAY_H

#define CVM_OVERLAY_ELEMENT_FILL        0x00000000
#define CVM_OVERLAY_ELEMENT_SHADED      0x10000000
#define CVM_OVERLAY_ELEMENT_COLOURED    0x20000000
///having both set could signify something??
#define CVM_OVERLAY_ELEMENT_OVERLAP_MIN 0x40000000
#define CVM_OVERLAY_ELEMENT_OVERLAP_MUL 0x80000000



typedef struct overlay_theme overlay_theme;

typedef enum
{
    OVERLAY_NO_COLOUR=0,
    OVERLAY_BACKGROUND_COLOUR,
    OVERLAY_MAIN_COLOUR,
    OVERLAY_ALTERNATE_MAIN_COLOUR,
    OVERLAY_HIGHLIGHTING_COLOUR,
//    OVERLAY_MAIN_HIGHLIGHTED_COLOUR,
//    OVERLAY_MAIN_ACTIVE_COLOUR,
//    OVERLAY_MAIN_INACTIVE_COLOUR,
//    OVERLAY_MAIN_PROMINENT_COLOUR,
//    OVERLAY_BORDER_COLOUR,


    OVERLAY_TEXT_HIGHLIGHT_COLOUR,
    OVERLAY_TEXT_COMPOSITION_COLOUR_0,
    OVERLAY_TEXT_COLOUR_0,
//    OVERLAY_TEXT_COLOUR_1,
//    OVERLAY_TEXT_COLOUR_2,
//    OVERLAY_TEXT_COLOUR_3,
//    OVERLAY_TEXT_COLOUR_4,
//    OVERLAY_TEXT_COLOUR_5,
//    OVERLAY_TEXT_COLOUR_6,
//    OVERLAY_TEXT_COLOUR_7,

//    OVERLAY_MISC_COLOUR_0,
//    OVERLAY_MISC_COLOUR_1,
//    OVERLAY_MISC_COLOUR_2,
//    OVERLAY_MISC_COLOUR_3,

    OVERLAY_NUM_COLOURS
}
overlay_colour;




typedef struct cvm_overlay_element_render_data
{
    uint16_t data0[4];///position
    uint32_t data1[2];
    uint16_t data2[4];/// overap_tex_lookup.xy , fade_off_left|fade_off_right , fade_off_top|fade_off_bot
}
cvm_overlay_element_render_data;

CVM_STACK(cvm_overlay_element_render_data, cvm_overlay_element_render_data, 256)
/// make starting size bigger

struct cvm_overlay_render_batch;


#include "cvm_overlay_text.h"

struct cvm_overlay_theme_element_description
{
    /// constraint of rendering
    rectangle bounds;

    /// the region/size applicable to the element
    rectangle r;

    /// status information associated with widget being rendered
    uint32_t status;

    /// colour to use for rendering
    overlay_colour colour;

    /// sizing information associated with scroll bars
    int32_t before;
    int32_t bar;
    int32_t after;

    /// for if widget is constrained by a generic box widget
    rectangle constraining_box_r;
    uint32_t constraining_box_status;

    /// for if fading off
    /// both could probably be made much more clear..
    rectangle fade_bound;
    rectangle fade_range;
    /// ^ may want to consider moving this logic into element rendering
};


struct overlay_theme
{
    cvm_overlay_font font;

    int base_unit_w;
    int base_unit_h;

    int h_bar_text_offset;

    int h_bar_icon_text_offset;///need a variant of this for contiguous elements (h_bar_icon_text_offset = contigouus_h_bar_icon_text_offset + contiguous_box_x_offset)

    int h_text_fade_range;
    int v_text_fade_range;

    //int h_slider_bar_lost_w;///horizontal space tied up in visual elements (not part of range)
    //int v_slider_bar_lost_h;///vertical space tied up in visual elements (not part of range)
    ///int slider_bar_bar_fraction;

    /// relative positions of box contents (from rect of widget)
    int x_box_offset;
    int y_box_offset;

    int icon_bar_extra_w;///if switching from h_text_bar_render to h_icon_text_bar_render how much extra space to use
    int separator_w;
    int separator_h;
    int popup_separation;

    /// spacing between panel and sides of content
    int x_panel_offset;
    int y_panel_offset;
    /// spaving when against the edge of the screen
    int x_panel_offset_side;
    int y_panel_offset_side;

    /// range of panel deadspace near the edge that allows resizing
    int border_resize_selection_range;


    ///worth assessing whether a lack of separation is even worth it or desirable..., perhaps "contiguous" should just refer to graphical elements
    int base_contiguous_unit_w;
    int base_contiguous_unit_h;

    int contiguous_box_x_offset;
    int contiguous_box_y_offset;


    ///remove these?
    int contiguous_all_box_x_offset;
    int contiguous_all_box_y_offset;
    int contiguous_some_box_x_offset;
    int contiguous_some_box_y_offset;
    int contiguous_horizintal_bar_h;

    void * other_data;

//    void    (*square_render_)           (struct cvm_overlay_render_batch * restrict render_batch, const overlay_theme* theme, const struct cvm_overlay_theme_element_description* description);

    void    (*square_render)            (struct cvm_overlay_render_batch * restrict render_batch, overlay_theme * theme, rectangle bounds, rectangle r, uint32_t status, overlay_colour colour);
    void    (*h_bar_render)             (struct cvm_overlay_render_batch * restrict render_batch, overlay_theme * theme, rectangle bounds, rectangle r, uint32_t status, overlay_colour colour);
    void    (*h_bar_slider_render)      (struct cvm_overlay_render_batch * restrict render_batch, overlay_theme * theme, rectangle bounds, rectangle r, uint32_t status, overlay_colour colour, int32_t before, int32_t bar, int32_t after);
    void    (*h_adjactent_slider_render)(struct cvm_overlay_render_batch * restrict render_batch, overlay_theme * theme, rectangle bounds, rectangle r, uint32_t status, overlay_colour colour, int32_t before, int32_t bar, int32_t after);///usually/always tacked onto box
    void    (*v_adjactent_slider_render)(struct cvm_overlay_render_batch * restrict render_batch, overlay_theme * theme, rectangle bounds, rectangle r, uint32_t status, overlay_colour colour, int32_t before, int32_t bar, int32_t after);///usually/always tacked onto box
    void    (*box_render)               (struct cvm_overlay_render_batch * restrict render_batch, overlay_theme * theme, rectangle bounds, rectangle r, uint32_t status, overlay_colour colour);
    void    (*panel_render)             (struct cvm_overlay_render_batch * restrict render_batch, overlay_theme * theme, rectangle bounds, rectangle r, uint32_t status, overlay_colour colour);
    ///                                 (struct cvm_overlay_render_batch * restrict render_batch, overlay_theme * theme, rectangle bounds, rectangle r, uint32_t status, overlay_colour colour)

    void    (*square_box_constrained_render)(struct cvm_overlay_render_batch * restrict render_batch,overlay_theme * theme,rectangle bounds,rectangle r,uint32_t status,overlay_colour colour,rectangle box_r,uint32_t box_status);
    void    (*h_bar_box_constrained_render) (struct cvm_overlay_render_batch * restrict render_batch,overlay_theme * theme,rectangle bounds,rectangle r,uint32_t status,overlay_colour colour,rectangle box_r,uint32_t box_status);
    void    (*box_box_constrained_render)   (struct cvm_overlay_render_batch * restrict render_batch,overlay_theme * theme,rectangle bounds,rectangle r,uint32_t status,overlay_colour colour,rectangle box_r,uint32_t box_status);
    /// need h_bar_slider_over_box_render as well

    void    (*fill_box_constrained_render)          (struct cvm_overlay_render_batch * restrict render_batch,overlay_theme * theme,rectangle bounds,rectangle r,overlay_colour colour,rectangle box_r,uint32_t box_status);
    void    (*fill_fading_box_constrained_render)   (struct cvm_overlay_render_batch * restrict render_batch,overlay_theme * theme,rectangle bounds,rectangle r,overlay_colour colour,rectangle fade_bound,rectangle fade_range,rectangle box_r,uint32_t box_status);
    void    (*shaded_box_constrained_render)        (struct cvm_overlay_render_batch * restrict render_batch,overlay_theme * theme,rectangle bounds,rectangle r,overlay_colour colour,int16_t x_off,int16_t y_off,rectangle box_r,uint32_t box_status);
    void    (*shaded_fading_box_constrained_render) (struct cvm_overlay_render_batch * restrict render_batch,overlay_theme * theme,rectangle bounds,rectangle r,overlay_colour colour,int16_t x_off,int16_t y_off,rectangle fade_bound,rectangle fade_range,rectangle box_r,uint32_t box_status);

    bool    (*square_select)            (overlay_theme * theme,rectangle r,uint32_t status);
    bool    (*h_bar_select)             (overlay_theme * theme,rectangle r,uint32_t status);
    bool    (*box_select)               (overlay_theme * theme,rectangle r,uint32_t status);
    bool    (*panel_select)             (overlay_theme * theme,rectangle r,uint32_t status);

    rectangle   (*get_sliderbar_offsets)(overlay_theme * theme,uint32_t status);///returns offsets from each respective side
    #warning re-assess above!
};





typedef struct cvm_overlay_images
{
    VkDeviceMemory memory;

    /// alpha - colour
    VkImage images[2];
    VkImageView views[2];

    cvm_vk_image_atlas alpha_atlas;
    cvm_vk_image_atlas colour_atlas;
}
cvm_overlay_images;


/// target information and synchronization requirements
struct cvm_overlay_target
{
    /// framebuffer depends on these
    VkImageView image_view;
    cvm_vk_resource_identifier image_view_unique_identifier;
    /// render pass depends on these
    VkExtent2D extent;
    VkFormat format;
    // start respecting colour space, to do so must support different input colour space (i believe) and must have information on which space blending is done in and how
    VkColorSpaceKHR color_space;/// respecting the colour space is NYI

    VkImageLayout initial_layout;
    VkImageLayout final_layout;
    bool clear_image;

    /// in / out synchronization setup data
    uint32_t wait_semaphore_count;
    uint32_t acquire_barrier_count;
    VkSemaphoreSubmitInfo wait_semaphores[4];
    VkImageMemoryBarrier2 acquire_barriers[4];

    uint32_t signal_semaphore_count;
    uint32_t release_barrier_count;
    VkSemaphoreSubmitInfo signal_semaphores[4];
    VkImageMemoryBarrier2 release_barriers[4];
};



struct cvm_overlay_frame_resources
{
    #warning does this require a last use moment!?
    /// copy of target image view, used as key to the framebuffer cache
    VkImageView image_view;
    cvm_vk_resource_identifier image_view_unique_identifier;

    /// data to cache
    VkFramebuffer framebuffer;
};

#define CVM_CACHE_CMP( entry , key ) (entry->image_view_unique_identifier == key->image_view_unique_identifier) && (entry->image_view == key->image_view)
CVM_CACHE(struct cvm_overlay_frame_resources, struct cvm_overlay_target*, cvm_overlay_frame_resources)
#undef CVM_CACHE_CMP

/// needs a better name
struct cvm_overlay_target_resources
{
    /// used for finding extant resources in cache
    VkExtent2D extent;
    VkFormat format;
    VkColorSpaceKHR color_space;
    VkImageLayout initial_layout;
    VkImageLayout final_layout;
    bool clear_image;

    /// data to cache
    VkRenderPass render_pass;
    VkPipeline pipeline;

    /// rely on all frame resources being deleted to ensure not in use
    cvm_overlay_frame_resources_cache frame_resources;

    /// moment when this cache entry is no longer in use and can thus be evicted
    cvm_vk_timeline_semaphore_moment last_use_moment;
};

CVM_QUEUE(struct cvm_overlay_target_resources, cvm_overlay_target_resources, 8)
/// queue as is done above might not be best if it's desirable to maintain multiple targets as renderable

/// resources used in a per cycle/frame fashion
struct cvm_overlay_transient_resources
{
    cvm_vk_command_pool command_pool;

    VkDescriptorSet frame_descriptor_set;

    cvm_vk_timeline_semaphore_moment last_use_moment;
};

CVM_QUEUE(struct cvm_overlay_transient_resources*,cvm_overlay_transient_resources,8)

/// fixed sized queue of these?
/// queue init at runtime? (custom size)
/// make cache init at runtime too? (not great but w/e)

struct cvm_overlay_rendering_static_resources
{
    cvm_overlay_images images;

    VkDescriptorPool descriptor_pool;

    ///these descriptors don't change (only need 1 set, always has the same bindings)
    VkDescriptorSetLayout image_descriptor_set_layout;
    VkDescriptorSet image_descriptor_set;

    ///these descriptors will have a different binding per frame
    VkDescriptorSetLayout frame_descriptor_set_layout;
    /// the actual descriptor sets will exist on the work entry (per frame)

    /// for creating pipelines
    VkPipelineLayout pipeline_layout;
    VkPipelineShaderStageCreateInfo pipeline_stages[2];//vertex,fragment
};

#warning this should NOT require a shunt buffer
void cvm_overlay_rendering_static_resources_initialise(struct cvm_overlay_rendering_static_resources * static_resources, const cvm_vk_device * device, uint32_t renderer_transient_count);
void cvm_overlay_rendering_static_resources_terminate(struct cvm_overlay_rendering_static_resources * static_resources, const cvm_vk_device * device);

typedef struct cvm_overlay_renderer
{
    /// for uploading to images, is NOT locally owned
    cvm_vk_staging_buffer_ * staging_buffer;


    uint32_t transient_count;
    uint32_t transient_count_initialised;
    struct cvm_overlay_transient_resources* transient_resources_backing;
    cvm_overlay_transient_resources_queue transient_resources_queue;

    /// are separate shunt buffers even the best way to do this??
//    cvm_overlay_element_render_data_stack element_render_stack;
    struct cvm_overlay_render_batch* render_batch;/// <- temporary, move elsewhere
//    cvm_vk_shunt_buffer shunt_buffer;

    struct cvm_overlay_rendering_static_resources static_resources;

    cvm_overlay_target_resources_queue target_resources;
}
cvm_overlay_renderer;

typedef struct cvm_overlay_setup
{
    cvm_vk_staging_buffer_ * staging_buffer;

    VkImageLayout initial_target_layout;
    VkImageLayout final_target_layout;

    uint32_t renderer_resource_queue_count;///effectively max frames in flight, size of work queue
    uint32_t target_resource_cache_size;/// size of cache for target dependent resources, effectively max swapchain size, should be lightweight so 8 or 16 is reasonable
}
cvm_overlay_setup;


void cvm_overlay_renderer_initialise(cvm_overlay_renderer * renderer, cvm_vk_device * device, cvm_vk_staging_buffer_ * staging_buffer, uint32_t renderer_cycle_count);
void cvm_overlay_renderer_terminate(cvm_overlay_renderer * renderer, cvm_vk_device * device);

cvm_vk_timeline_semaphore_moment cvm_overlay_render_to_target(const cvm_vk_device * device, cvm_overlay_renderer * renderer, widget * menu_widget, const struct cvm_overlay_target* target);

cvm_vk_timeline_semaphore_moment cvm_overlay_render_to_presentable_image(const cvm_vk_device * device, cvm_overlay_renderer * renderer, widget * menu_widget, cvm_vk_swapchain_presentable_image * presentable_image, bool last_use);









struct cvm_overlay_image_atlases
{
    /// shared by colour atlas and
    VkDeviceMemory image_memory;

    VkImage alpha_image;
    VkImageView alpha_image_view;
    cvm_vk_image_atlas alpha_image_atlas;

    VkImage colour_image;
    VkImageView colour_image_view;
    cvm_vk_image_atlas colour_image_atlas;
};

void cvm_overlay_image_atlases_initialise(struct cvm_overlay_image_atlases* image_atlases, const struct cvm_vk_device* device, uint32_t alpha_w, uint32_t alpha_h, uint32_t colour_w, uint32_t colour_h);
void cvm_overlay_image_atlases_terminate (struct cvm_overlay_image_atlases* image_atlases, const struct cvm_vk_device* device);

struct cvm_overlay_rendering_resources
{
    /// held reference
//    struct cvm_overlay_image_atlases* image_atlases;
    /// possibility for external management would be nice, so instead pass in images in a (semi?) generalised fashion -- just view to bind?

    /// above might be worth separating out, is required for knowing where to upload image elements in prepartory rendering stage

    uint32_t descriptor_sets_available;// debug, used to ensure all descriptor sets get used

    // dont manage descripror pool? would require another stage to allocate it... probably not worthwhile
    VkDescriptorPool descriptor_pool;

    /// combine images (to allow them to be dynamic) and
    VkDescriptorSetLayout descriptor_set_layout;

    ///these descriptors will have a different binding per frame
    VkDescriptorSetLayout transient_descriptor_set_layout;
    /// the actual descriptor sets will exist on the work entry (per frame)

    /// for creating pipelines
    VkPipelineLayout pipeline_layout;
    VkPipelineShaderStageCreateInfo vertex_pipeline_stage;
    VkPipelineShaderStageCreateInfo fragment_pipeline_stage;
};

/// data to be passed form setup stage to rendering batch/packet
/// used both in setup/prep and actual_render functions
///     ^ actually... aside from the push constants (which COULD be moved to uniform) this is all only used by the prep stage!

struct cvm_overlay_render_batch
{
    /// preparation resources
    struct cvm_overlay_element_render_data_stack render_elements;// essentailly overlay element instances

    struct cvm_vk_shunt_buffer upload_shunt_buffer;// used for putting data in atlases

    /// the atlases can be used for other purposes, but the shunt buffer and copy list must be kept in sync (ergo them going together here)

    /// need a good way to track use of tiles in atlas, can have usage mask (u8/u16) that can be reset en-masse?
    ///     ^ or perhaps a linked list per usage frame (max N) that gets shuffled off all together to the LL of removable items
    /// atlases are unowned and merely passed by reference
    cvm_vk_image_atlas* colour_atlas;
    cvm_vk_image_atlas* alpha_atlas;
    /// separation of uploads and the image atlas itself in this way allows frames to be set up in advance (multiple render batches in flight)
    /// i.e. not having the copy stack be part of the image atlas is a net positive

    /// copies to perform from shunt buffer to the atlases
    cvm_vk_buffer_image_copy_stack alpha_atlas_copy_actions;
    cvm_vk_buffer_image_copy_stack colour_atlas_copy_actions;
    /// end of preparation resources




    /// all elemnts are tentative / subject to change/review

    /// shunt buffer of deltas required by overlay changes (e.g. image data to blit into atlas)
    ///     ^ list of copy ops to blit into textures and similar
    /// array/list/stack of render elements
    /// image views to both render and upload to (write to descriptor set for example)
    /// active colour array (or pointer to it)
    /// screen res used

    /// descripror set to write changes to
    //      ^ provided extrenally

    /// if above is input to prep stage the foollowing is generated from it and *actually* used in rendering

    /// decriptor set (again, now has actually been written)
    /// (staging) buffer with offset of array of render elements/insances

    cvm_vk_staging_buffer_allocation staging_buffer_allocation;

    /// also record staging buffer itself here?
    VkDeviceSize render_element_offset;
};

/// other data to be passed into rendering



/// command buffer
/// descriptor set
/// staging buffer to load all this shit into



// frame_cycle_count informs how many descriptor sets can be created
void cvm_overlay_rendering_resources_initialise(struct cvm_overlay_rendering_resources* rendering_resources, const struct cvm_vk_device* device, uint32_t active_render_count);
void cvm_overlay_rendering_resources_terminate (struct cvm_overlay_rendering_resources* rendering_resources, const struct cvm_vk_device* device);

/// theser can be destroyed as normal
/// images and colour data
VkDescriptorSet cvm_overlay_descriptor_set_create(const struct cvm_vk_device* device, const struct cvm_overlay_rendering_resources* rendering_resources);
VkPipeline cvm_overlay_render_pipeline_create(const struct cvm_vk_device* device, const struct cvm_overlay_rendering_resources* rendering_resources, VkRenderPass render_pass, VkExtent2D extent, uint32_t subpass);


/// need a setup/prep stage (outside/before render pass) to upload all relevant data (and potentially update/write vulkan objects like descriptor sets)
/// ^ this will need to be "manually" synchonized with rendering (b/c it could be on another queue entirely or on the same command bffer as rendering)

/// pass in colour set?
/// move colour set and image atlas views to a struct?
/// need a command buffer, and some other config/fixed data
void cvm_overlay_render(const struct cvm_vk_device* device, const struct cvm_overlay_rendering_resources* rendering_resources);


// by requiring input of a particular type set we can communicate what resources need to be initialised







/// overlay render helper functions follow



/// x/y_off are the texture space coordinates to read data from at position r, i.e. at r the texture coordinates looked up would be x_off,y_off
static inline void cvm_render_shaded_overlay_element(struct cvm_overlay_render_batch * restrict render_batch,rectangle b,rectangle r,overlay_colour colour,int16_t x_off,int16_t y_off)
{
    b=get_rectangle_overlap(r,b);

    if(rectangle_has_positive_area(b))
    {
        *cvm_overlay_element_render_data_stack_new(&render_batch->render_elements)=(cvm_overlay_element_render_data)
        {
            {b.x1,b.y1,b.x2,b.y2},
            {CVM_OVERLAY_ELEMENT_SHADED,colour<<24},
            {b.x1-r.x1+x_off,b.y1-r.y1+y_off,0,0}
        };
    }
}

static inline void cvm_render_shaded_fading_overlay_element(struct cvm_overlay_render_batch * restrict render_batch,rectangle b,rectangle r,overlay_colour colour,int x_off,int y_off,rectangle fade_bound,rectangle fade_range)
{
    if(r.x1<fade_bound.x1)///beyond this opacity is 0 (completely transparent)
    {
        x_off+=fade_bound.x1-r.x1;
        r.x1=fade_bound.x1;
    }
    fade_bound.x1=r.x1-fade_bound.x1;///convert bound to distance from side
    if(fade_bound.x1>fade_range.x1) fade_bound.x1=fade_range.x1=0;

    if(r.x2>fade_bound.x2) r.x2=fade_bound.x2;
    fade_bound.x2=fade_bound.x2-r.x2;///convert bound to distance from side
    if(fade_bound.x2>fade_range.x2) fade_bound.x2=fade_range.x2=0;


    if(r.y1<fade_bound.y1)
    {
        y_off+=fade_bound.y1-r.y1;
        r.y1=fade_bound.y1;
    }
    fade_bound.y1=r.y1-fade_bound.y1;///convert bound to distance from side
    if(fade_bound.y1>fade_range.y1) fade_bound.y1=fade_range.y1=0;

    if(r.y2>fade_bound.y2)r.y2=fade_bound.y2;///beyond this opacity is 0 (completely transparent)
    fade_bound.y2=fade_bound.y2-r.y2;///convert bound to distance from side
    if(fade_bound.y2>fade_range.y2) fade_bound.y2=fade_range.y2=0;


    b=get_rectangle_overlap(r,b);

    if(rectangle_has_positive_area(b))
    {
        *cvm_overlay_element_render_data_stack_new(&render_batch->render_elements)=(cvm_overlay_element_render_data)
        {
            {b.x1,b.y1,b.x2,b.y2},
            {CVM_OVERLAY_ELEMENT_SHADED | fade_bound.x1<<18 | fade_bound.y1<<12 | fade_bound.x2<<6 | fade_bound.y2 , colour<<24 | fade_range.x1<<18 | fade_range.y1<<12 | fade_range.x2<<6 | fade_range.y2},
            {b.x1-r.x1+x_off,b.y1-r.y1+y_off,0,0}
        };
    }
}

/// x/y_over_b equates to combination of, screen space coordinates of base of "overlap" element (negative) with texture coordinates of the tile the "overlap" element uses
static inline void cvm_render_shaded_overlap_min_overlay_element(struct cvm_overlay_render_batch * restrict render_batch,rectangle b,rectangle r,overlay_colour colour,int x_off,int y_off,int x_over_b,int y_over_b)
{
    b=get_rectangle_overlap(r,b);

    if(rectangle_has_positive_area(b))
    {
        *cvm_overlay_element_render_data_stack_new(&render_batch->render_elements)=(cvm_overlay_element_render_data)
        {
            {b.x1,b.y1,b.x2,b.y2},
            {CVM_OVERLAY_ELEMENT_SHADED|CVM_OVERLAY_ELEMENT_OVERLAP_MIN,colour<<24},
            {b.x1-r.x1+x_off,b.y1-r.y1+y_off , b.x1+x_over_b,b.y1+y_over_b}
        };
    }
}

static inline void cvm_render_shaded_fading_overlap_min_overlay_element(struct cvm_overlay_render_batch * restrict render_batch,rectangle b,rectangle r,overlay_colour colour,int x_off,int y_off,
                                                                        rectangle fade_bound,rectangle fade_range,int x_over_b,int y_over_b)
{
    if(r.x1<fade_bound.x1)///beyond this opacity is 0 (completely transparent)
    {
        x_off+=fade_bound.x1-r.x1;
        r.x1=fade_bound.x1;
    }
    fade_bound.x1=r.x1-fade_bound.x1;///convert bound to distance from side
    if(fade_bound.x1>fade_range.x1) fade_bound.x1=fade_range.x1=0;

    if(r.x2>fade_bound.x2) r.x2=fade_bound.x2;
    fade_bound.x2=fade_bound.x2-r.x2;///convert bound to distance from side
    if(fade_bound.x2>fade_range.x2) fade_bound.x2=fade_range.x2=0;


    if(r.y1<fade_bound.y1)
    {
        y_off+=fade_bound.y1-r.y1;
        r.y1=fade_bound.y1;
    }
    fade_bound.y1=r.y1-fade_bound.y1;///convert bound to distance from side
    if(fade_bound.y1>fade_range.y1) fade_bound.y1=fade_range.y1=0;

    if(r.y2>fade_bound.y2)r.y2=fade_bound.y2;///beyond this opacity is 0 (completely transparent)
    fade_bound.y2=fade_bound.y2-r.y2;///convert bound to distance from side
    if(fade_bound.y2>fade_range.y2) fade_bound.y2=fade_range.y2=0;


    b=get_rectangle_overlap(r,b);

    if(rectangle_has_positive_area(b))
    {
        *cvm_overlay_element_render_data_stack_new(&render_batch->render_elements)=(cvm_overlay_element_render_data)
        {
            {b.x1,b.y1,b.x2,b.y2},
            {CVM_OVERLAY_ELEMENT_SHADED|CVM_OVERLAY_ELEMENT_OVERLAP_MIN | fade_bound.x1<<18 | fade_bound.y1<<12 | fade_bound.x2<<6 | fade_bound.y2 , colour<<24 | fade_range.x1<<18 | fade_range.y1<<12 | fade_range.x2<<6 | fade_range.y2},
            {b.x1-r.x1+x_off,b.y1-r.y1+y_off , b.x1+x_over_b,b.y1+y_over_b}
        };
    }
}

static inline void cvm_render_fill_overlay_element(struct cvm_overlay_render_batch * restrict render_batch,rectangle b,rectangle r,overlay_colour colour)
{
    b=get_rectangle_overlap(r,b);

    if(rectangle_has_positive_area(b))
    {
        *cvm_overlay_element_render_data_stack_new(&render_batch->render_elements)=(cvm_overlay_element_render_data)
        {
            {b.x1,b.y1,b.x2,b.y2},
            {CVM_OVERLAY_ELEMENT_FILL,colour<<24},
            {0,0,0,0}
        };
    }
}

static inline void cvm_render_fill_fading_overlay_element(struct cvm_overlay_render_batch * restrict render_batch,rectangle b,rectangle r,overlay_colour colour,rectangle fade_bound,rectangle fade_range)
{
    if(r.x1<fade_bound.x1)r.x1=fade_bound.x1;///beyond this opacity is 0 (completely transparent)
    fade_bound.x1=r.x1-fade_bound.x1;///convert bound to distance from side
    if(fade_bound.x1>fade_range.x1) fade_bound.x1=fade_range.x1=0;

    if(r.x2>fade_bound.x2) r.x2=fade_bound.x2;
    fade_bound.x2=fade_bound.x2-r.x2;///convert bound to distance from side
    if(fade_bound.x2>fade_range.x2) fade_bound.x2=fade_range.x2=0;


    if(r.y1<fade_bound.y1) r.y1=fade_bound.y1;
    fade_bound.y1=r.y1-fade_bound.y1;///convert bound to distance from side
    if(fade_bound.y1>fade_range.y1) fade_bound.y1=fade_range.y1=0;

    if(r.y2>fade_bound.y2)r.y2=fade_bound.y2;///beyond this opacity is 0 (completely transparent)
    fade_bound.y2=fade_bound.y2-r.y2;///convert bound to distance from side
    if(fade_bound.y2>fade_range.y2) fade_bound.y2=fade_range.y2=0;


    b=get_rectangle_overlap(r,b);

    if(rectangle_has_positive_area(b))
    {
        *cvm_overlay_element_render_data_stack_new(&render_batch->render_elements)=(cvm_overlay_element_render_data)
        {
            {b.x1,b.y1,b.x2,b.y2},
            {CVM_OVERLAY_ELEMENT_FILL | fade_bound.x1<<18 | fade_bound.y1<<12 | fade_bound.x2<<6 | fade_bound.y2 , colour<<24 | fade_range.x1<<18 | fade_range.y1<<12 | fade_range.x2<<6 | fade_range.y2},
            {0,0,0,0}
        };
    }
}

#include "themes/cubic.h"


#endif


