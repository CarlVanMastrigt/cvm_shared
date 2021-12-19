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

#ifndef CVM_SHARED_H
#include "cvm_shared.h"
#endif

#ifndef CVM_OVERLAY_H
#define CVM_OVERLAY_H

#define CVM_OVERLAY_ELEMENT_FILL        0x0000
#define CVM_OVERLAY_ELEMENT_SHADED      0x1000
#define CVM_OVERLAY_ELEMENT_COLOURED    0x2000
///having both set could signify something??
#define CVM_OVERLAY_ELEMENT_OVERLAP_MIN 0x4000
#define CVM_OVERLAY_ELEMENT_OVERLAP_MUL 0x8000
//typedef enum
//{
//    CVM_OVERLAY_ELEMENT_FILL,
//    CVM_OVERLAY_ELEMENT_SHADED,
//    CVM_OVERLAY_ELEMENT_COLOURED,
//
//}
//cvm_overlay_element_type;

typedef enum
{
    OVERLAY_NO_COLOUR_=0,
    OVERLAY_BACKGROUND_COLOUR_,
    OVERLAY_MAIN_COLOUR_,
    OVERLAY_ALTERNATE_MAIN_COLOUR_,
    OVERLAY_HIGHLIGHTING_COLOUR_,
//    OVERLAY_MAIN_HIGHLIGHTED_COLOUR,
//    OVERLAY_MAIN_ACTIVE_COLOUR,
//    OVERLAY_MAIN_INACTIVE_COLOUR,
//    OVERLAY_MAIN_PROMINENT_COLOUR,
//    OVERLAY_BORDER_COLOUR,


    OVERLAY_TEXT_HIGHLIGHT_COLOUR_,
    OVERLAY_TEXT_COMPOSITION_COLOUR_0_,
    OVERLAY_TEXT_COLOUR_0_,
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

    OVERLAY_NUM_COLOURS_
}
overlay_colour_;


typedef struct cvm_overlay_render_data
{
    uint16_t data0[4];///position
    uint16_t data1[4];///int data: type|colour_id, tex_lookup.xy, fade_offset|fade_speed(for glyphs) can be known based on contents (calculated at sizing step and applied at render based on current x)
    uint16_t data2[4];/// ? ? ? ?
}
cvm_overlay_render_data;

typedef struct cvm_overlay_element_render_buffer
{
    ///could fill buffer in reverse to make render order match input order, but not sure how that would affect write combining of data so will avoid for now;
    cvm_overlay_render_data * buffer;
    uint32_t count,space;//,xm,ym;

    ///better to pass this around (with images that it uses too!) rather than having static allocation or  some split paradigm
    VkCommandBuffer upload_command_buffer;

    cvm_vk_staging_buffer * staging_buffer;

    //cvm_vk_image_atlas * transparent_image_atlas;
    //cvm_vk_image_atlas * colour_image_atlas;
}
cvm_overlay_element_render_buffer;

/*
#define CVM_OVERLAY_SCREEN_COORDINATES(xm,ym,x1,y1,x2,y2){\
((uint16_t)(((uint32_t)(x1)*xm+((uint32_t)(x1)>>1)+0x00008000)>>16)),\
((uint16_t)(((uint32_t)(y1)*ym+((uint32_t)(y1)>>1)+0x00008000)>>16)),\
((uint16_t)(((uint32_t)(x2)*xm+((uint32_t)(x2)>>1)+0x00008000)>>16)),\
((uint16_t)(((uint32_t)(y2)*ym+((uint32_t)(y2)>>1)+0x00008000)>>16))}
#define CVM_OVERLAY_SCREEN_MULTIPLIER(s) (0xFFFF0000/(uint32_t)s);
*/


void initialise_overlay_render_data(void);
void terminate_overlay_render_data(void);

void initialise_overlay_swapchain_dependencies(void);
void terminate_overlay_swapchain_dependencies(void);

cvm_vk_module_work_block * overlay_render_frame(int screen_w,int screen_h,widget * menu_widget);

void overlay_frame_cleanup(uint32_t swapchain_image_index);

cvm_vk_image_atlas_tile * overlay_create_transparent_image_tile_with_staging(cvm_overlay_element_render_buffer * erb,void ** staging,uint32_t w, uint32_t h);
void overlay_destroy_transparent_image_tile(cvm_vk_image_atlas_tile * tile);

cvm_vk_image_atlas_tile * overlay_create_colour_image_tile_with_staging(cvm_overlay_element_render_buffer * erb,void ** staging,uint32_t w, uint32_t h);
void overlay_destroy_colour_image_tile(cvm_vk_image_atlas_tile * tile);

//bool check_click_on_interactable_element(cvm_overlay_interactable_element * element,int x,int y);
#include "cvm_overlay_text.h"



typedef enum
{
    OVERLAY_NO_COLOUR=0,
    OVERLAY_BACKGROUND_COLOUR,
    OVERLAY_MAIN_COLOUR,
    //OVERLAY_ALTERNATE_MAIN_COLOUR,
    OVERLAY_HIGHLIGHTING_COLOUR,
    OVERLAY_MAIN_HIGHLIGHTED_COLOUR,
    OVERLAY_MAIN_ACTIVE_COLOUR,
    OVERLAY_MAIN_INACTIVE_COLOUR,
    OVERLAY_MAIN_PROMINENT_COLOUR,
    OVERLAY_BORDER_COLOUR,


    OVERLAY_TEXT_HIGHLIGHT_COLOUR,
    OVERLAY_TEXT_COLOUR_0,
    OVERLAY_TEXT_COLOUR_1,
    OVERLAY_TEXT_COLOUR_2,
    OVERLAY_TEXT_COLOUR_3,
    OVERLAY_TEXT_COLOUR_4,
    OVERLAY_TEXT_COLOUR_5,
    OVERLAY_TEXT_COLOUR_6,
    OVERLAY_TEXT_COLOUR_7,

    OVERLAY_MISC_COLOUR_0,
    OVERLAY_MISC_COLOUR_1,
    OVERLAY_MISC_COLOUR_2,
    OVERLAY_MISC_COLOUR_3,

    NUM_OVERLAY_COLOURS
}
overlay_colour;


typedef struct overlay_theme overlay_theme;

struct overlay_theme
{
    cvm_overlay_font font_;

    int base_unit_w;
    int base_unit_h;

    int h_bar_text_offset;

    int h_slider_bar_lost_w;///horizontal space tied up in visual elements (not part of range)
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

    int contiguous_all_box_x_offset;
    int contiguous_all_box_y_offset;
    int contiguous_some_box_x_offset;
    int contiguous_some_box_y_offset;
    int contiguous_horizintal_bar_h;

    void * other_data;

    void    (*square_render)            (rectangle r,uint32_t status,overlay_theme * theme,cvm_overlay_element_render_buffer * erb,rectangle bounds,overlay_colour_ colour);
    void    (*h_bar_render)             (rectangle r,uint32_t status,overlay_theme * theme,cvm_overlay_element_render_buffer * erb,rectangle bounds,overlay_colour_ colour);
    void    (*h_bar_slider_render)      (rectangle r,uint32_t status,overlay_theme * theme,cvm_overlay_element_render_buffer * erb,rectangle bounds,overlay_colour_ colour,int range,int value,int bar);
    void    (*h_adjactent_slider_render)(rectangle r,uint32_t status,overlay_theme * theme,cvm_overlay_element_render_buffer * erb,rectangle bounds,overlay_colour_ colour,int range,int value,int bar);///usually/always tacked onto box
    void    (*v_adjactent_slider_render)(rectangle r,uint32_t status,overlay_theme * theme,cvm_overlay_element_render_buffer * erb,rectangle bounds,overlay_colour_ colour,int range,int value,int bar);///usually/always tacked onto box
    void    (*box_render)               (rectangle r,uint32_t status,overlay_theme * theme,cvm_overlay_element_render_buffer * erb,rectangle bounds,overlay_colour_ colour);
    void    (*panel_render)             (rectangle r,uint32_t status,overlay_theme * theme,cvm_overlay_element_render_buffer * erb,rectangle bounds,overlay_colour_ colour);

    void    (*square_over_box_render)   (rectangle r,uint32_t status,overlay_theme * theme,cvm_overlay_element_render_buffer * erb,rectangle bounds,overlay_colour_ colour,rectangle box_r,uint32_t box_status);
    void    (*h_bar_over_box_render)    (rectangle r,uint32_t status,overlay_theme * theme,cvm_overlay_element_render_buffer * erb,rectangle bounds,overlay_colour_ colour,rectangle box_r,uint32_t box_status);
    void    (*box_over_box_render)      (rectangle r,uint32_t status,overlay_theme * theme,cvm_overlay_element_render_buffer * erb,rectangle bounds,overlay_colour_ colour,rectangle box_r,uint32_t box_status);

    void    (*shaded_over_box_render)   (rectangle r,overlay_theme * theme,cvm_overlay_element_render_buffer * erb,rectangle bounds,overlay_colour_ colour,rectangle box_r,uint32_t box_status,int x_off,int y_off);
    void    (*fill_over_box_render)     (rectangle r,overlay_theme * theme,cvm_overlay_element_render_buffer * erb,rectangle bounds,overlay_colour_ colour,rectangle box_r,uint32_t box_status);

    bool    (*square_select)            (rectangle r,uint32_t status,overlay_theme * theme);
    bool    (*h_bar_select)             (rectangle r,uint32_t status,overlay_theme * theme);
    bool    (*box_select)               (rectangle r,uint32_t status,overlay_theme * theme);
    bool    (*panel_select)             (rectangle r,uint32_t status,overlay_theme * theme);
};

/// x/y_off are the texture space coordinates to read data from at position r, i.e. at r the texture coordinates looked up would be x_off,y_off
static inline void cvm_render_shaded_overlay_element(cvm_overlay_element_render_buffer * erb,rectangle r,rectangle b,int x_off,int y_off,overlay_colour_ colour)
{
    b=get_rectangle_overlap(r,b);

    if(erb->count != erb->space && rectangle_has_positive_area(b))
        erb->buffer[erb->count++]=(cvm_overlay_render_data)
        {
            {b.x1,b.y1,b.x2,b.y2},
            {CVM_OVERLAY_ELEMENT_SHADED|(colour&0x0FFF),b.x1-r.x1+x_off,b.y1-r.y1+y_off,0/*unused*/},
            {0,0,0,0}
        };
}

/// x/y_over_b equates to combination of, screen space coordinates of base of "overlap" element (negative) with texture coordinates of the tile the "overlap" element uses
static inline void cvm_render_shaded_overlap_min_overlay_element(cvm_overlay_element_render_buffer * erb,rectangle r,rectangle b,int x_off,int y_off,int x_over_b,int y_over_b,overlay_colour_ colour)
{
    b=get_rectangle_overlap(r,b);

    if(erb->count != erb->space && rectangle_has_positive_area(b))
        erb->buffer[erb->count++]=(cvm_overlay_render_data)
        {
            {b.x1,b.y1,b.x2,b.y2},
            {CVM_OVERLAY_ELEMENT_SHADED|CVM_OVERLAY_ELEMENT_OVERLAP_MIN|(colour&0x0FFF),b.x1-r.x1+x_off,b.y1-r.y1+y_off,0/*unused*/},
            {b.x1+x_over_b,b.y1+y_over_b,0,0}
        };
}

static inline void cvm_render_fill_overlay_element(cvm_overlay_element_render_buffer * erb,rectangle r,rectangle b,overlay_colour_ colour)
{
    b=get_rectangle_overlap(r,b);

    if(erb->count != erb->space && rectangle_has_positive_area(b))
        erb->buffer[erb->count++]=(cvm_overlay_render_data)
        {
            {b.x1,b.y1,b.x2,b.y2},
            {CVM_OVERLAY_ELEMENT_FILL|(colour&0x0FFF),0,0,0/*unused*/},
            {0,0,0,0}
        };
}

#include "themes/cubic.h"



#endif




