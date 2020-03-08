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

#ifndef WIDGET_TEXTBOX_H
#define WIDGET_TEXTBOX_H



typedef struct textbox_line
{
    int character_start;
    int character_finish;

    int x_size;
}
textbox_line;

typedef struct widget_textbox
{
    widget_base base;

    char * text;
    overlay_render_data * glyph_render_data;
    int text_length;///this doesn't seem to be used (REMOVE ??)

    textbox_line * render_lines;
    int render_lines_count;
    int render_lines_space;

    int min_visible_lines;

    int selection_begin;
    int selection_end;

    int min_horizontal_glyphs;

    int font_index;

    int text_height;
    int offset;
    int max_offset;
    int visible_size;
    int wheel_delta;
}
widget_textbox;

widget * create_textbox(char * text,int min_horizontal_glyphs,int min_visible_lines);

void change_textbox_contents(widget * w,char * new_contents);

widget * create_textbox_scrollbar(widget * textbox);




#endif



