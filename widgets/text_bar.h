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

#ifndef WIDGET_TEXT_BAR_H
#define WIDGET_TEXT_BAR_H


typedef struct widget_text_bar
{
    widget_base base;

    widget_function set_text;
    void * data;

	char * text;

	uint32_t free_data:1;
	uint32_t free_text:1;

	int min_glyph_render_count;
	widget_text_alignment text_alignment;
}
widget_text_bar;

//widget * create_text_bar(int min_glyph_space,char * initial_text,widget_function set_text,void * data,bool free_data,widget_text_alignment text_alignment);
widget * create_static_text_bar(int min_glyph_render_count,char * text,widget_text_alignment text_alignment);
widget * create_dynamic_text_bar(int min_glyph_render_count,widget_function set_text,bool free_text,void * data,bool free_data,widget_text_alignment text_alignment);

//void set_text_bar_text_pointer(widget * w,char * text);
//void set_text_bar_text(widget * w,char * text_to_copy);

#endif

