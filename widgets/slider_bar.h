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

#ifndef WIDGET_slider_bar_H
#define WIDGET_slider_bar_H


typedef struct widget_slider_bar
{
    widget_base base;

    int32_t * value_ptr;

    union
    {
        int32_t min_value;
        const int32_t * min_value_ptr;
    };
    union
    {
        int32_t max_value;
        const int32_t * max_value_ptr;
    };
    union
    {
        int32_t bar_fraction;
        const int32_t * bar_size_ptr;
    };
    union
    {
        int32_t scroll_fraction;
        const int32_t * scroll_delta_ptr;
    };

    //widget_layout layout;

    widget_function func;
    void * data;

    uint32_t free_data:1;
    ///use bitfields to distinguish value vs value ptr's above (min/max, then put values in union)
    uint32_t dynamic_range:1;
    uint32_t dynamic_bar:1;
    uint32_t dynamic_scroll:1;
}
widget_slider_bar;

widget * create_slider_bar(int * value,widget_function func,void * data,bool free_data);///set control mechanisms manually later

widget * create_slider_bar_fixed(int32_t * value,int32_t min_value,int32_t max_value,int32_t bar_fraction,int32_t scroll_fraction,widget_function func,void * data,bool free_data);
widget * create_slider_bar_dynamic(int32_t * value,const int32_t * min_value,const int32_t * max_value,const int32_t * bar_size,const int32_t * scroll_delta,widget_function func,void * data,bool free_data);
//widget * create_adjacent_slider(int * value_ptr,int min_value,int max_value,widget_function func,void * data,bool free_data,widget_layout layout,int bar_fraction);

void set_slider_bar_other_values(widget * w,int * min_value_ptr,int * max_value_ptr,int * bar_size_ptr,int * wheel_delta_ptr);

//void validate_slider_bar_value(widget * w);
int set_slider_bar_value(widget * w,int32_t v);

void blank_slider_bar_function(widget * w);

#endif




