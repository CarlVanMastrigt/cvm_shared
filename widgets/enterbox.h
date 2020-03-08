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

#ifndef WIDGET_ENTERBOX_H
#define WIDGET_ENTERBOX_H


typedef struct widget_enterbox
{
    widget_base base;

    char * text;
    int text_max_length;
    int text_min_visible;
    int visible_offset;
    widget_function func;
    void * data;

    widget_function upon_input;

    int selection_begin;
    int selection_end;

    //int font_index;

    //int text_x_offset;
    //int text_y_offset;

    uint32_t delete_all_when_first_selected:1;
    uint32_t activate_upon_deselect:1;
    uint32_t free_data:1;
}
widget_enterbox;

widget * create_enterbox(int text_max_length,int text_min_visible,char * initial_text,widget_function func,void * data,bool activate_upon_deselect,bool free_data);

void blank_enterbox_function(widget * w);

void set_enterbox_text(widget * w,char * text);

void set_enterbox_text_using_int(widget * w,int v);
int get_int_from_enterbox_text(widget * w);

void set_enterbox_action_upon_input(widget * w,widget_function func);

#endif



