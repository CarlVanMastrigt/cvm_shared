/**
Copyright 2020,2021,2022 Carl van Mastrigt

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

#ifndef WIDGET_TEXTBOX_H
#define WIDGET_TEXTBOX_H



typedef struct widget_textbox
{
    widget_base base;

    char * text;

    char * selection_begin;
    char * selection_end;

    int min_visible_lines;

    int min_horizontal_glyphs;

    cvm_overlay_text_block text_block;

    int text_height;
    int x_offset;
    int y_offset;
    int max_offset;
    int visible_size;
    int wheel_delta;///instead scroll by line(s)?

    ///if forcing all text to be visible (not recommended) use straight rendering, otherwise probably best to have per line rendering technique implemented for performance
}
widget_textbox;

widget * create_textbox(struct widget_context* context, char * text,bool owns_text,int min_horizontal_glyphs,int min_visible_lines);

void change_textbox_text(widget * w,char * new_text,bool owns_new_text);

widget * create_textbox_scrollbar(struct widget_context* context, widget * textbox);




#endif



