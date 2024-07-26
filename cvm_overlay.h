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

CVM_STACK(cvm_overlay_element_render_data,cvm_overlay_element_render_data,16)
/// make starting size bigger


#include "cvm_overlay_text.h"


struct overlay_theme
{
    cvm_vk_image_atlas * backing_image_atlas;
    /// maybe wants to have the transparent one as well and have this be init specifically via an overlay renderer??
    /// perhaps split the actual rendering part of the renderer away from the backing part (images and the font library) (`FT_Library overlay_freetype_library`)

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

    int x_box_offset;///both text and other???
    int y_box_offset;

    int icon_bar_extra_w;///if switching from h_text_bar_render to h_icon_text_bar_render how much extra space to use
    int separator_w;
    int separator_h;
    int popup_separation;

    int x_panel_offset;
    int x_panel_offset_side;
    int y_panel_offset;
    int y_panel_offset_side;

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

    void    (*square_render)            (cvm_overlay_element_render_data_stack * restrict render_stack,overlay_theme * theme,rectangle bounds,rectangle r,uint32_t status,overlay_colour colour);
    void    (*h_bar_render)             (cvm_overlay_element_render_data_stack * restrict render_stack,overlay_theme * theme,rectangle bounds,rectangle r,uint32_t status,overlay_colour colour);
    void    (*h_bar_slider_render)      (cvm_overlay_element_render_data_stack * restrict render_stack,overlay_theme * theme,rectangle bounds,rectangle r,uint32_t status,overlay_colour colour,int32_t before,int32_t bar,int32_t after);
    void    (*h_adjactent_slider_render)(cvm_overlay_element_render_data_stack * restrict render_stack,overlay_theme * theme,rectangle bounds,rectangle r,uint32_t status,overlay_colour colour,int32_t before,int32_t bar,int32_t after);///usually/always tacked onto box
    void    (*v_adjactent_slider_render)(cvm_overlay_element_render_data_stack * restrict render_stack,overlay_theme * theme,rectangle bounds,rectangle r,uint32_t status,overlay_colour colour,int32_t before,int32_t bar,int32_t after);///usually/always tacked onto box
    void    (*box_render)               (cvm_overlay_element_render_data_stack * restrict render_stack,overlay_theme * theme,rectangle bounds,rectangle r,uint32_t status,overlay_colour colour);
    void    (*panel_render)             (cvm_overlay_element_render_data_stack * restrict render_stack,overlay_theme * theme,rectangle bounds,rectangle r,uint32_t status,overlay_colour colour);
    ///                                 (cvm_overlay_element_render_data_stack * restrict render_stack,overlay_theme * theme,rectangle bounds,rectangle r,uint32_t status,overlay_colour colour)

    void    (*square_box_constrained_render)(cvm_overlay_element_render_data_stack * restrict render_stack,overlay_theme * theme,rectangle bounds,rectangle r,uint32_t status,overlay_colour colour,rectangle box_r,uint32_t box_status);
    void    (*h_bar_box_constrained_render) (cvm_overlay_element_render_data_stack * restrict render_stack,overlay_theme * theme,rectangle bounds,rectangle r,uint32_t status,overlay_colour colour,rectangle box_r,uint32_t box_status);
    void    (*box_box_constrained_render)   (cvm_overlay_element_render_data_stack * restrict render_stack,overlay_theme * theme,rectangle bounds,rectangle r,uint32_t status,overlay_colour colour,rectangle box_r,uint32_t box_status);
    /// need h_bar_slider_over_box_render as well

    void    (*fill_box_constrained_render)          (cvm_overlay_element_render_data_stack * restrict render_stack,overlay_theme * theme,rectangle bounds,rectangle r,overlay_colour colour,rectangle box_r,uint32_t box_status);
    void    (*fill_fading_box_constrained_render)   (cvm_overlay_element_render_data_stack * restrict render_stack,overlay_theme * theme,rectangle bounds,rectangle r,overlay_colour colour,rectangle fade_bound,rectangle fade_range,rectangle box_r,uint32_t box_status);
    void    (*shaded_box_constrained_render)        (cvm_overlay_element_render_data_stack * restrict render_stack,overlay_theme * theme,rectangle bounds,rectangle r,overlay_colour colour,int16_t x_off,int16_t y_off,rectangle box_r,uint32_t box_status);
    void    (*shaded_fading_box_constrained_render) (cvm_overlay_element_render_data_stack * restrict render_stack,overlay_theme * theme,rectangle bounds,rectangle r,overlay_colour colour,int16_t x_off,int16_t y_off,rectangle fade_bound,rectangle fade_range,rectangle box_r,uint32_t box_status);

    bool    (*square_select)            (overlay_theme * theme,rectangle r,uint32_t status);
    bool    (*h_bar_select)             (overlay_theme * theme,rectangle r,uint32_t status);
    bool    (*box_select)               (overlay_theme * theme,rectangle r,uint32_t status);
    bool    (*panel_select)             (overlay_theme * theme,rectangle r,uint32_t status);

    rectangle   (*get_sliderbar_offsets)(overlay_theme * theme,uint32_t status);///returns offsets from each respective side
    #warning re-assess above!
};

/// x/y_off are the texture space coordinates to read data from at position r, i.e. at r the texture coordinates looked up would be x_off,y_off
static inline void cvm_render_shaded_overlay_element(cvm_overlay_element_render_data_stack * restrict render_stack,rectangle b,rectangle r,overlay_colour colour,int16_t x_off,int16_t y_off)
{
    b=get_rectangle_overlap(r,b);

    if(rectangle_has_positive_area(b))
    {
        *cvm_overlay_element_render_data_stack_new(render_stack)=(cvm_overlay_element_render_data)
        {
            {b.x1,b.y1,b.x2,b.y2},
            {CVM_OVERLAY_ELEMENT_SHADED,colour<<24},
            {b.x1-r.x1+x_off,b.y1-r.y1+y_off,0,0}
        };
    }
}

static inline void cvm_render_shaded_fading_overlay_element(cvm_overlay_element_render_data_stack * restrict render_stack,rectangle b,rectangle r,overlay_colour colour,int x_off,int y_off,rectangle fade_bound,rectangle fade_range)
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
        *cvm_overlay_element_render_data_stack_new(render_stack)=(cvm_overlay_element_render_data)
        {
            {b.x1,b.y1,b.x2,b.y2},
            {CVM_OVERLAY_ELEMENT_SHADED | fade_bound.x1<<18 | fade_bound.y1<<12 | fade_bound.x2<<6 | fade_bound.y2 , colour<<24 | fade_range.x1<<18 | fade_range.y1<<12 | fade_range.x2<<6 | fade_range.y2},
            {b.x1-r.x1+x_off,b.y1-r.y1+y_off,0,0}
        };
    }
}

/// x/y_over_b equates to combination of, screen space coordinates of base of "overlap" element (negative) with texture coordinates of the tile the "overlap" element uses
static inline void cvm_render_shaded_overlap_min_overlay_element(cvm_overlay_element_render_data_stack * restrict render_stack,rectangle b,rectangle r,overlay_colour colour,int x_off,int y_off,int x_over_b,int y_over_b)
{
    b=get_rectangle_overlap(r,b);

    if(rectangle_has_positive_area(b))
    {
        *cvm_overlay_element_render_data_stack_new(render_stack)=(cvm_overlay_element_render_data)
        {
            {b.x1,b.y1,b.x2,b.y2},
            {CVM_OVERLAY_ELEMENT_SHADED|CVM_OVERLAY_ELEMENT_OVERLAP_MIN,colour<<24},
            {b.x1-r.x1+x_off,b.y1-r.y1+y_off , b.x1+x_over_b,b.y1+y_over_b}
        };
    }
}

static inline void cvm_render_shaded_fading_overlap_min_overlay_element(cvm_overlay_element_render_data_stack * restrict render_stack,rectangle b,rectangle r,overlay_colour colour,int x_off,int y_off,
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
        *cvm_overlay_element_render_data_stack_new(render_stack)=(cvm_overlay_element_render_data)
        {
            {b.x1,b.y1,b.x2,b.y2},
            {CVM_OVERLAY_ELEMENT_SHADED|CVM_OVERLAY_ELEMENT_OVERLAP_MIN | fade_bound.x1<<18 | fade_bound.y1<<12 | fade_bound.x2<<6 | fade_bound.y2 , colour<<24 | fade_range.x1<<18 | fade_range.y1<<12 | fade_range.x2<<6 | fade_range.y2},
            {b.x1-r.x1+x_off,b.y1-r.y1+y_off , b.x1+x_over_b,b.y1+y_over_b}
        };
    }
}

static inline void cvm_render_fill_overlay_element(cvm_overlay_element_render_data_stack * restrict render_stack,rectangle b,rectangle r,overlay_colour colour)
{
    b=get_rectangle_overlap(r,b);

    if(rectangle_has_positive_area(b))
    {
        *cvm_overlay_element_render_data_stack_new(render_stack)=(cvm_overlay_element_render_data)
        {
            {b.x1,b.y1,b.x2,b.y2},
            {CVM_OVERLAY_ELEMENT_FILL,colour<<24},
            {0,0,0,0}
        };
    }
}

static inline void cvm_render_fill_fading_overlay_element(cvm_overlay_element_render_data_stack * restrict render_stack,rectangle b,rectangle r,overlay_colour colour,rectangle fade_bound,rectangle fade_range)
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
        *cvm_overlay_element_render_data_stack_new(render_stack)=(cvm_overlay_element_render_data)
        {
            {b.x1,b.y1,b.x2,b.y2},
            {CVM_OVERLAY_ELEMENT_FILL | fade_bound.x1<<18 | fade_bound.y1<<12 | fade_bound.x2<<6 | fade_bound.y2 , colour<<24 | fade_range.x1<<18 | fade_range.y1<<12 | fade_range.x2<<6 | fade_range.y2},
            {0,0,0,0}
        };
    }
}

#include "themes/cubic.h"




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
typedef struct cvm_overlay_target
{
    /// framebuffer depends on these
    VkImageView image_view;
    /// render pass depends on these
    VkExtent2D extent;
    VkFormat format;
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
}
cvm_overlay_target;



struct cvm_overlay_frame_resources
{
    /// used for finding extant resources in cache
    VkImageView image_view;

    /// data to cache
    VkFramebuffer framebuffer;

    /// used to ensure isn't in use before deletion when being evicted
    cvm_vk_timeline_semaphore_moment last_use_moment;/// when this cache entry is completed and can thus be evicted
};

#define CVM_CACHE_CMP( lhs , rhs ) lhs->image_view == rhs->image_view
CVM_CACHE(struct cvm_overlay_frame_resources, cvm_overlay_target*, cvm_overlay_frame_resources, 8)
#undef CVM_CACHE_CMP

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
};

#define CVM_CACHE_CMP( lhs, rhs ) lhs->extent.width == rhs->extent.width && lhs->extent.height == rhs->extent.height && lhs->format == rhs->format && \
lhs->color_space == rhs->color_space && lhs->initial_layout == rhs->initial_layout && lhs->final_layout == rhs->final_layout && lhs->clear_image == rhs->clear_image
CVM_CACHE(struct cvm_overlay_target_resources, cvm_overlay_target*, cvm_overlay_target_resources, 8)
#undef CVM_CACHE_CMP



typedef struct cvm_overlay_renderer
{
    cvm_vk_device * device;///is there a better place to put this? probably...
    /// for uploading to images, is NOT locally owned
    cvm_vk_staging_buffer_ * staging_buffer;

    uint32_t cycle_count;/// needs better name
    cvm_vk_work_queue work_queue;

    /// are separate shunt buffers even the best way to do this??
    cvm_overlay_element_render_data_stack element_render_stack;
    cvm_vk_staging_shunt_buffer shunt_buffer;

    /// image info
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

    cvm_overlay_target_resources_cache target_resources;
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

cvm_vk_timeline_semaphore_moment cvm_overlay_render_to_target(const cvm_vk_device * device, cvm_overlay_renderer * renderer, widget * menu_widget, const cvm_overlay_target* target);

cvm_vk_timeline_semaphore_moment cvm_overlay_render_to_presentable_image(const cvm_vk_device * device, cvm_overlay_renderer * renderer, widget * menu_widget, cvm_vk_swapchain_presentable_image * presentable_image, bool last_use);

#endif




