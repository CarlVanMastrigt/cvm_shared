/**
Copyright 2021 Carl van Mastrigt

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

#ifndef CVM_OVERLAY_TEXT_H
#define CVM_OVERLAY_TEXT_H

//#define CVM_OVERLAY_MAX_UNICODE_BYTES 7 /* use this if including variation sequences as part of a "single glyph" */
#define CVM_OVERLAY_MAX_UNICODE_BYTES 4
#define CVM_OVERLAY_MAX_COMPOSITION_BYTES 32
/// above is to include variation sequences as part of a "single character"

typedef struct cvm_overlay_glyph
{
    cvm_vk_image_atlas_tile * tile;///if null create upon render

    FT_UInt glyph_index;///because of variant selectors, cannot search by code point, search by glyph index
    uint32_t usage_counter;///when hitting 0 it gets deleted? (or scheduled for deletion) is this even necessary?
    ///perhaps need better way to handle unused overlay elements and glyphs (could just agressively delete...)
    rectangle pos;
    int advance;
}
cvm_overlay_glyph;///hmmm, glyph vs string based rendering... glyph has more potential to be loaded at once and is faster in worst case, but slower in average

typedef struct cvm_overlay_font
{
    FT_Face face;

    int glyph_size;

    int space_advance;
    int space_character_index;

    int line_spacing;

    int max_advance;

    ///not a great solution but bubble sort new glyphs from end?, is rare enough op and gives low enough cache use to justify?
    cvm_overlay_glyph * glyphs;
    uint32_t glyph_count;
    uint32_t glyph_space;
}
cvm_overlay_font;

///obviously this will need to be updated when changing the enterbox text
typedef struct cvm_overlay_text_line
{
    char * start;
    char * finish;
}
cvm_overlay_text_line;

typedef struct cvm_overlay_text_block
{
    cvm_overlay_text_line * lines;
    uint32_t line_count;
    uint32_t line_space;
}
cvm_overlay_text_block;

void cvm_overlay_open_freetype(void);
void cvm_overlay_close_freetype(void);

void cvm_overlay_create_font(cvm_overlay_font * font,char * filename,int pixel_size);
void cvm_overlay_destroy_font(cvm_overlay_font * font);


///some generic
bool cvm_overlay_utf8_validate_string(char * text);
int cvm_overlay_utf8_count_glyphs(char * text);
int cvm_overlay_utf8_count_glyphs_outside_range(char * text,char * begin,char * end);///end is uninclusive offset, start is inclusive offset
bool cvm_overlay_utf8_validate_string_and_count_glyphs(char * text,int * c);
///following also check for variation sequences, assumes string is valid utf8 already
char * cvm_overlay_utf8_get_previous_glyph(char * base,char * t);
char * cvm_overlay_utf8_get_next_glyph(char * t);
char * cvm_overlay_utf8_get_previous_word(char * base,char * t);
char * cvm_overlay_utf8_get_next_word(char * t);


#define OVERLAY_TEXT_NORMAL_RENDER      0x00000000
#define OVERLAY_TEXT_CONSTRAINED_RENDER 0x00000001
#define OVERLAY_TEXT_FADING_RENDER      0x00000002
#define OVERLAY_TEXT_SELECTED_RENDER    0x00000004



typedef struct overlay_text_single_line_render_data
{
    cvm_overlay_element_render_buffer * erb;
    overlay_theme * theme;
    rectangle bounds;
    char * text;
    int x;
    int y;
    overlay_colour_ colour;

    uint32_t box_status;
    rectangle box_r;

    rectangle text_area;
    int text_length;

    char * selection_begin;
    char * selection_end;

    uint32_t flags;///selection,fade,constrained
}
overlay_text_single_line_render_data;

void overlay_text_single_line_render(overlay_text_single_line_render_data* restrict data);

int overlay_text_single_line_get_pixel_length(cvm_overlay_font * font,char * text);
char * overlay_text_single_line_find_offset(cvm_overlay_font * font,char * text,int relative_x);




void overlay_text_multiline_processing(cvm_overlay_font * font,cvm_overlay_text_block * block,char * text,int wrapping_width);

void overlay_text_multiline_render(cvm_overlay_element_render_buffer * erb,cvm_overlay_font * font,rectangle bounds,cvm_overlay_text_block * block,int x,int y,overlay_colour_ colour);
void overlay_text_multiline_selection_render(cvm_overlay_element_render_buffer * erb,cvm_overlay_font * font,rectangle bounds,cvm_overlay_text_block * block,int x,int y,overlay_colour_ colour,char * selection_start,char * selection_end);

char * overlay_text_multiline_find_offset(cvm_overlay_font * font,cvm_overlay_text_block * block,int relative_x,int relative_y);


static inline rectangle overlay_text_single_line_get_text_area(rectangle r,int glyph_size,int x_border)
{
    r.y1=(r.y1+r.y2-glyph_size)>>1;
    r.y2=r.y1+glyph_size;
    r.x1+=x_border;
    r.x2-=x_border;
    return r;
}

cvm_overlay_glyph * overlay_get_glyph(cvm_overlay_element_render_buffer * erb,cvm_overlay_font * font,char * text);///assumes a single glyph in text

void overlay_text_centred_glyph_render(cvm_overlay_element_render_buffer * erb,cvm_overlay_font * font,rectangle bounds,rectangle r,char * icon_glyph,overlay_colour_ colour);
void overlay_text_centred_glyph_box_constrained_render(cvm_overlay_element_render_buffer * erb,overlay_theme * theme,rectangle bounds,rectangle r,char * icon_glyph,overlay_colour_ colour,rectangle box_r,uint32_t box_status);
///need way to blend out text towards end of textbox

#endif





