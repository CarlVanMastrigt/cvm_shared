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
    rectangle_ pos;
    int advance;
}
cvm_overlay_glyph;///hmmm, glyph vs string based rendering... glyph has more potential to be loaded at once and is faster in worst case, but slower in average

typedef struct cvm_overlay_font
{
    FT_Face face;

    int glyph_size;

    int space_advance;
    int space_character_index;

    int max_advance;

    ///not a great solution but bubble sort new glyphs from end?, is rare enough op and gives low enough cache use to justify?
    cvm_overlay_glyph * glyphs;
    uint32_t glyph_count;
    uint32_t glyph_space;
}
cvm_overlay_font;


void cvm_overlay_open_freetype(void);
void cvm_overlay_close_freetype(void);

void cvm_overlay_create_font(cvm_overlay_font * font,char * filename,int pixel_size);
void cvm_overlay_destroy_font(cvm_overlay_font * font);


///some generic
bool cvm_overlay_utf8_validate_string(char * text);
int cvm_overlay_utf8_count_glyphs(char * text);
int cvm_overlay_utf8_count_glyphs_outside_range(char * text,int begin,int end);///end is uninclusive offset, start is inclusive offset
bool cvm_overlay_utf8_validate_string_and_count_glyphs(char * text,int * c);
///following also check for variation sequences, assumes string is valid utf8 already
int cvm_overlay_utf8_get_previous(char * text,int offset);
int cvm_overlay_utf8_get_next(char * text,int offset);



///returns the required width of a string
int overlay_size_text_simple(cvm_overlay_font * font,char * text);
void overlay_render_text_simple(cvm_overlay_element_render_buffer * element_render_buffer,cvm_overlay_font * font,char * text,int x,int y,rectangle_ * bounds,overlay_colour_ colour);

///returns the height (in pixels) to accomodate the given string with the given wrapping width
int overlay_get_text_box_height(cvm_overlay_font * font,char * text,int wrapping_width);
///render complex is somewhat detached from the required height above, as complex render can handle more situations than just text boxes
///perhaps want to revisit this design though
void overlay_render_text_complex(cvm_overlay_element_render_buffer * element_render_buffer,cvm_overlay_font * font,char * text,int x,int y,rectangle_ * bounds,overlay_colour_ colour,int wrapping_width);

cvm_overlay_glyph * overlay_get_glyph(cvm_overlay_font * font,char * text);///assumes a single glyph in text

int overlay_text_find_offset_simple(cvm_overlay_font * font,char * text,int relative_x);
//int overlay_text_find_offset_simple(cvm_overlay_font * font,char * text,int relative_x,int relative_y,int wrapping_width);

void overlay_render_text_selection_simple(cvm_overlay_element_render_buffer * element_render_buffer,cvm_overlay_font * font,char * text,int x,int y,rectangle_ * bounds,int selection_start,int selection_end);
//void overlay_render_text_complex_selection_box(cvm_overlay_element_render_buffer * element_render_buffer,cvm_overlay_font * font,char * text,int x,int y,rectangle_ * bounds,int wrapping_width,int selection_start,int selection_end);




#endif





