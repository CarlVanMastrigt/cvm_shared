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

typedef enum
{
    CVM_OVERLAY_ELEMENT_SHADED,
    CVM_OVERLAY_ELEMENT_COLOURED,
    CVM_OVERLAY_ELEMENT_FILL,
}
cvm_overlay_element_type;

typedef enum
{
    OVERLAY_NO_COLOUR_=0,
    OVERLAY_BACKGROUND_COLOUR_,
    OVERLAY_MAIN_COLOUR_,
    OVERLAY_ALTERNATE_MAIN_COLOUR_,
//    OVERLAY_HIGHLIGHTING_COLOUR,
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
    //uint16_t data2[4];/// ? ? ? ?
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


//typedef struct cvm_overlay_theme
//{
//    //
//}
//cvm_overlay_theme;



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




///@todo check if below defines are necessary
#define BASE_OVERLAY_SPRITE_SIZE 16
#define NUM_OVERLAY_SPRITE_LEVELS 11
/// 0=16 10=16k



typedef struct cvm_glyph
{
    int offset;///offset in image

    int width;
    int bearingX;
    int advance;

    int kerning[94];
}
cvm_glyph;

typedef struct cvm_font
{
    GLuint text_image;

    cvm_glyph glyphs[94];
    int font_size;
    int space_width;
    int line_spacing;
    int font_height;
    int max_glyph_width;

    char * text_image_pixels;
    int text_image_width;
    int text_image_height;
    int text_image_offset;
}
cvm_font;

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

typedef struct overlay_render_data
{
    GLshort data1[4];
    GLshort data2[4];
    GLshort data3[4];
}
overlay_render_data;

typedef struct overlay_render_data_
{
    uint16_t data1[4];
    uint16_t data2[4];
    uint16_t data3[4];
}
overlay_render_data_;

typedef struct overlay_data
{
    overlay_render_data * data;

    uint32_t space;
    uint32_t count;
}
overlay_data;

/*typedef struct overlay_sprite_data
{
    uint32_t type:1;///0=shade 1=coloured
    uint32_t x_pos:14;///max 8192 =total 335mb
    uint32_t y_pos:14;
    uint32_t size_factor:3;///size factor: 1<<size_factor == width/height (min value is 4 usually max is 12)

    char * name;///use (binary ?) search (div 2 test) with ordering
}
overlay_sprite_data;*/

typedef enum
{
    OVERLAY_SHADED_SPRITE=0,
    OVERLAY_COLOURED_SPRITE
}
overlay_sprite_type;

typedef struct overlay_sprite_data
{
    overlay_sprite_type type;
    int x_pos;
    int y_pos;
    uint32_t size_factor;///size factor: 16<<size_factor == width/height (min value is 4 usually max is 12)

    char * name;///use (binary ?) search (div 2 test) with ordering
}
overlay_sprite_data;

typedef struct sprite_heirarchy_level
{
    int x_positions[3];
    int y_positions[3];

    int remaining;///0 to 3;
}
sprite_heirarchy_level;


typedef struct overlay_theme overlay_theme;

struct overlay_theme
{
//    char * name;

//    cvm_font font;

//    overlay_sprite_data * sprite_data;
//    uint32_t sprite_count;
//    uint32_t sprite_space;
//
//    sprite_heirarchy_level shaded_sprite_levels[NUM_OVERLAY_SPRITE_LEVELS];
//    sprite_heirarchy_level coloured_sprite_levels[NUM_OVERLAY_SPRITE_LEVELS];

//    bool shaded_texture_updated;
//    GLuint shaded_texture;
//    uint8_t * shaded_texture_data;
//    uint32_t shaded_texture_size;
//    uint32_t shaded_texture_tier;

//    bool coloured_texture_updated;
//    GLuint coloured_texture;
//    uint8_t * coloured_texture_data;
//    uint32_t coloured_texture_size;
//    uint32_t coloured_texture_tier;


    cvm_overlay_font font_;

    int base_unit_w;
    int base_unit_h;

    int h_bar_text_offset;

    int h_slider_bar_lost_w;///horizontal space tied up in visual elements (not part of range)
    int v_slider_bar_lost_h;///vertical space tied up in visual elements (not part of range)
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

    ///remove x_off, y_off, x_in and y_in below by instead just modifying the input rectangles
    ///replace rectangles with new rectangles
    /// replace overlay data with new paradigm (element buffer)

    void    (*square_icon_render)       (rectangle_ r,uint32_t status,overlay_theme * theme,overlay_data * od,rectangle_ bounds,overlay_colour_ colour,char * icon_glyph,overlay_colour_ icon_colour);
    void    (*h_bar_render)             (rectangle_ r,uint32_t status,overlay_theme * theme,overlay_data * od,rectangle_ bounds,overlay_colour_ colour);///just make h_bar?
    void    (*h_bar_slider_render)      (rectangle_ r,uint32_t status,overlay_theme * theme,overlay_data * od,rectangle_ bounds,overlay_colour_ colour,int range,int value,int bar);
    void    (*h_adjactent_slider_render)(rectangle_ r,uint32_t status,overlay_theme * theme,overlay_data * od,rectangle_ bounds,overlay_colour_ colour,int range,int value,int bar);///usually/always tacked onto box
    void    (*v_adjactent_slider_render)(rectangle_ r,uint32_t status,overlay_theme * theme,overlay_data * od,rectangle_ bounds,overlay_colour_ colour,int range,int value,int bar);///usually/always tacked onto box
    void    (*box_render)               (rectangle_ r,uint32_t status,overlay_theme * theme,overlay_data * od,rectangle_ bounds,overlay_colour_ colour);
    void    (*panel_render)             (rectangle_ r,uint32_t status,overlay_theme * theme,overlay_data * od,rectangle_ bounds,overlay_colour_ colour);

    bool    (*square_select)            (rectangle_ r,uint32_t status,overlay_theme * theme);
    bool    (*h_bar_select)             (rectangle_ r,uint32_t status,overlay_theme * theme);
    bool    (*box_select)               (rectangle_ r,uint32_t status,overlay_theme * theme);
    bool    (*panel_select)             (rectangle_ r,uint32_t status,overlay_theme * theme);
};

//bool process_regular_text_character(cvm_font * cf,char prev,char current,int * offset);
//int get_new_text_offset(cvm_font * cf,char prev,char current,int current_offset);
//int calculate_text_length(overlay_theme * theme,char * text,int font_index);
//char * shorten_text_to_fit_width_start_ellipses(overlay_theme * theme,int width,char * text,int font_index,char * buffer,int buffer_size,int * x_offset);
//char * shorten_text_to_fit_width_end_ellipses(overlay_theme * theme,int width,char * text,int font_index,char * buffer,int buffer_size);
//


//overlay_theme create_overlay_theme(gl_functions * glf,uint32_t shaded_texture_size,uint32_t coloured_texture_size);///make malloced pointer
//void delete_overlay_theme(overlay_theme * theme);
//void load_font_to_overlay(gl_functions * glf,overlay_theme * theme,char * ttf_file,int size);

//void update_theme_textures_on_videocard(gl_functions * glf,overlay_theme * theme);





//typedef enum
//{
//    OVERLAY_ELEMENT_RECTANGLE=0,
//    OVERLAY_ELEMENT_BORDER,
//    OVERLAY_ELEMENT_CHARACTER,
//    OVERLAY_ELEMENT_SHADED,
//    OVERLAY_ELEMENT_COLOURED,
//
//    NUM_OVERLAY_ELEMENT_TYPES
//}
//overlay_element_type;
//
//void initialise_overlay(gl_functions * glf);
//void render_overlay(gl_functions * glf,overlay_data * od,overlay_theme * theme,int screen_w,int screen_h,vec4f * overlay_colours);
//
//
//
//
//
//
//void initialise_overlay_data(overlay_data * od);///make malloced pointer instead
//void delete_overlay_data(overlay_data * od);
//
//
//void ensure_overlay_data_space(overlay_data * od);
//
//
//void render_rectangle(overlay_data * od,rectangle r,rectangle bound,overlay_colour colour_index);
//void render_border(overlay_data * od,rectangle r,rectangle bound,rectangle discard,overlay_colour colour_index);
//void render_character(overlay_data * od,rectangle r,rectangle bound,int char_offset,int font_index,overlay_colour colour_index);
////void render_overlay_element(overlay_data * od,rectangle r,rectangle bound,int source_x,int source_y,overlay_colour main_colour);
//void render_overlay_shade(overlay_data * od,rectangle r,rectangle bound,int source_x,int source_y,overlay_colour colour);///REMOVE
//void render_shaded_sprite(overlay_data * od,rectangle r,rectangle bound,int source_x,int source_y,overlay_colour colour);
//void render_coloured_sprite(overlay_data * od,rectangle r,rectangle bound,int source_x,int source_y);///has own colour and alpha data
//
//void upload_overlay_render_datum(overlay_data * od,overlay_render_data * ord);
//
////bool check_click_on_overlay_sprite(overlay_theme * theme,rectangle r,overlay_sprite_data osd);
//bool check_click_on_shaded_sprite(overlay_theme * theme,rectangle r,int sprite_x,int sprite_y);
//bool check_click_on_coloured_sprite(overlay_theme * theme,rectangle r,int sprite_x,int sprite_y);
//
//void render_overlay_text(overlay_data * od,overlay_theme * theme,char * text,int x_off,int y_off,rectangle bounds,int font_index,int colour);
//













//overlay_sprite_data create_overlay_sprite(overlay_theme * theme,char * name,int max_w,int max_h,overlay_sprite_type type);
//overlay_sprite_data get_overlay_sprite_data(overlay_theme * theme,char * name);
//
//void set_overlay_sprite_image_data(overlay_theme * theme,overlay_sprite_data osd,uint8_t * source_data,int source_w,int w,int h,int x,int y);
//void set_overlay_sprite_image_data_from_sdl_surface(overlay_theme * theme,overlay_sprite_data osd,SDL_Surface * surface,int w,int h,int x,int y);
//
//overlay_sprite_data create_overlay_sprite_from_sdl_surface(overlay_theme * theme,char * name,SDL_Surface * surface,int w,int h,int x,int y,overlay_sprite_type type);



#include "themes/cubic.h"



#endif




