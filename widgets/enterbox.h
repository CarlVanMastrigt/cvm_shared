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

#ifndef WIDGET_ENTERBOX_H
#define WIDGET_ENTERBOX_H

typedef struct widget_enterbox
{
    widget_base base;

    char * text;
    uint32_t max_strlen;
    uint32_t max_glyphs;
    uint32_t min_glyphs_visible;
    int32_t visible_offset;
    int32_t text_pixel_length;
    widget_function activation_func;///when activated; pressing return, clicking away (if setup to do so) &c.
    widget_function update_contents_func;/// called every frame to maintain parity
    widget_function upon_input_func;///when text changes (only doe to user input?)
    void * data;

    char * selection_begin;
    char * selection_end;

    char composition_text[CVM_OVERLAY_MAX_COMPOSITION_BYTES];
    int composition_visible_offset;

    //uint32_t delete_all_when_first_selected:1;
    uint32_t activate_upon_deselect:1;
    uint32_t free_data:1;
    uint32_t recalculate_text_size:1;
}
widget_enterbox;

widget* create_enterbox(struct widget_context* context, uint32_t max_strlen,uint32_t max_glyphs,uint32_t min_glyphs_visible,char * initial_text,widget_function activation_func,widget_function update_contents_func,widget_function upon_input_func,void * data,bool free_data,bool activate_upon_deselect);

static inline widget* create_enterbox_simple(struct widget_context* context, uint32_t glyph_count, char * initial_text,widget_function activation_func,widget_function update_contents_func,void* data, bool free_data,bool activate_upon_deselect)
{
    return create_enterbox(context, glyph_count * CVM_OVERLAY_MAX_UNICODE_BYTES, glyph_count, glyph_count,initial_text,activation_func,update_contents_func,NULL,data,free_data,activate_upon_deselect);
}


void blank_enterbox_function(widget * w);

void set_enterbox_text(widget * w,const char * text);

#endif



