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

#ifndef WIDGET_BUTTON_H
#define WIDGET_BUTTON_H


typedef bool (*widget_button_toggle_status_func) (widget*);

typedef struct widget_button
{
    widget_base base;

    widget_function func;
    void * data;

    uint32_t free_data:1;/// worth making part of base status?

    ///forms of toggle
    uint32_t variant_text:1;
    uint32_t highlight:1;

    char * text;

    /// worth moving to another type?
    widget_button_toggle_status_func toggle_status;///for toggle button, use this to do toggle upon potential_interaction ???
}
widget_button;

void blank_button_func(widget * w);
void button_toggle_bool_func(widget * w);

bool button_bool_status_func(widget * w);
bool button_widget_status_func(widget * w);

widget * create_text_button(struct widget_context* context, const char * text,void * data,bool free_data,widget_function func);

widget * create_text_toggle_button(struct widget_context* context, char * positive_text,char * negative_text,void * data, bool free_data,widget_function func,widget_button_toggle_status_func toggle_status);
widget * create_text_highlight_toggle_button(struct widget_context* context, char * text,void * data,bool free_data,widget_function func,widget_button_toggle_status_func toggle_status);

widget * create_contiguous_text_button(struct widget_context* context, const char * text,void * data,bool free_data,widget_function func);
widget * create_contiguous_text_highlight_toggle_button(struct widget_context* context, const char * text,void * data,bool free_data,widget_function func,widget_button_toggle_status_func toggle_status);

widget * create_icon_button(struct widget_context* context, const char * icon_name,void * data,bool free_data,widget_function func);
widget * create_icon_toggle_button(struct widget_context* context, const char * positive_icon, const char * negative_icon,void * data,bool free_data,widget_function func,widget_button_toggle_status_func toggle_status);


#endif


