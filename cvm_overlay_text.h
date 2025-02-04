/**
Copyright 2021,2022 Carl van Mastrigt

This file is part of solipsix.

solipsix is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

solipsix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with solipsix.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef solipsix_H
#include "solipsix.h"
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
    int16_t advance;
}
cvm_overlay_glyph;///hmmm, glyph vs string based rendering... glyph has more potential to be loaded at once and is faster in worst case, but slower in average

typedef struct cvm_overlay_font
{
    FT_Face face;

    int16_t glyph_size;

    int16_t space_advance;
    int16_t space_character_index;

    int16_t line_spacing;

    int16_t max_advance;

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

void cvm_overlay_create_font(cvm_overlay_font * font, const FT_Library * freetype_library, char * filename, int pixel_size);
void cvm_overlay_destroy_font(cvm_overlay_font * font, cvm_vk_image_atlas * backing_image_atlas);/// atlas temporary var until atlas has better internl management


///some generic
bool cvm_overlay_utf8_validate_string(const char * text);
uint32_t cvm_overlay_utf8_count_glyphs(const char * text);
uint32_t cvm_overlay_utf8_count_glyphs_outside_range(const char * text,const char * begin,const char * end);///end is uninclusive offset, start is inclusive offset
bool cvm_overlay_utf8_validate_string_and_count_glyphs(const char * text,uint32_t * c);
///following also check for variation sequences, assumes string is valid utf8 already
char * cvm_overlay_utf8_get_previous_glyph(char * base,char * t);
char * cvm_overlay_utf8_get_next_glyph(char * t);
char * cvm_overlay_utf8_get_previous_word(char * base,char * t);
char * cvm_overlay_utf8_get_next_word(char * t);

#define OVERLAY_TEXT_RENDER_SELECTION       0x0001
#define OVERLAY_TEXT_RENDER_FADING          0x0002
#define OVERLAY_TEXT_RENDER_BOX_CONSTRAINED 0x0004

typedef struct overlay_text_single_line_render_data
{
    const char * restrict text;

    rectangle bounds;

    overlay_colour colour;///can make u8

    uint16_t flags;///can make u8

    int16_t x;
    int16_t y;

    /// fading inputs
    int16_t text_length;
    rectangle text_area;/// area in which text can exist

    /// selection inputs
    /// these needn't be restrict as their contents won't be accessed, likewise as these would alias text variable above actually dereferencing either would be a violation of restrict
    const char * selection_begin;
    const char * selection_end;

    /// box constrained inputs
    rectangle box_r;
    uint32_t box_status;

    ///4 bytes left for packing more data
}
overlay_text_single_line_render_data;

///theme cannot be const as glyphs will only be loaded to it as necessary
void overlay_text_single_line_render(struct cvm_overlay_render_batch * restrict render_batch,overlay_theme * restrict theme,const overlay_text_single_line_render_data * restrict data);

int16_t overlay_text_single_line_get_pixel_length(cvm_overlay_font * font,const char * text);
char * overlay_text_single_line_find_offset(cvm_overlay_font * font,char * text,int relative_x);

void overlay_text_multiline_processing(cvm_overlay_font * font,cvm_overlay_text_block * block,char * text,int wrapping_width);

void overlay_text_multiline_render(struct cvm_overlay_render_batch * restrict render_batch,cvm_overlay_font * font,rectangle bounds,cvm_overlay_text_block * block,int x,int y,overlay_colour colour);
void overlay_text_multiline_selection_render(struct cvm_overlay_render_batch * restrict render_batch,cvm_overlay_font * font,rectangle bounds,cvm_overlay_text_block * block,int x,int y,overlay_colour colour,char * selection_start,char * selection_end);

char * overlay_text_multiline_find_offset(cvm_overlay_font * font,cvm_overlay_text_block * block,int relative_x,int relative_y);


static inline rectangle overlay_text_single_line_get_text_area(rectangle r,int16_t glyph_size,int16_t x_border)
{
    r.y1=(r.y1+r.y2-glyph_size)>>1;
    r.y2=r.y1+glyph_size;
    r.x1+=x_border;
    r.x2-=x_border;
    return r;
}

cvm_overlay_glyph * overlay_get_glyph(cvm_overlay_font * font,const char * text, struct cvm_overlay_render_batch * restrict render_batch);///assumes a single glyph in text

void overlay_text_centred_glyph_render(struct cvm_overlay_render_batch * restrict render_batch,cvm_overlay_font * font,rectangle bounds,rectangle r,const char * icon_glyph,overlay_colour colour);
void overlay_text_centred_glyph_box_constrained_render(struct cvm_overlay_render_batch * restrict render_batch,overlay_theme * theme,rectangle bounds,rectangle r,const char * icon_glyph,overlay_colour colour,rectangle box_r,uint32_t box_status);
///need way to blend out text towards end of textbox

#endif





