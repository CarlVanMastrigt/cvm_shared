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

#ifndef WIDGET_SLIDERBAR_H
#define WIDGET_SLIDERBAR_H


typedef struct widget_sliderbar
{
    widget_base base;

    int * value_ptr;
    int min_value;
    int max_value;
    //int minumum_width;

    int * min_value_ptr;
    int * max_value_ptr;
    int * bar_size_ptr;
    int * wheel_delta_ptr;


    //int wasted_space;
    int bar_fraction;

    //widget_layout layout;

    widget_function func;
    void * data;

    uint32_t free_data:1;
}
widget_sliderbar;

widget * create_sliderbar(int * value_ptr,int min_value,int max_value,widget_function func,void * data,bool free_data,widget_layout layout,int bar_fraction);

void set_sliderbar_other_values(widget * w,int * min_value_ptr,int * max_value_ptr,int * bar_size_ptr,int * wheel_delta_ptr);

//void validate_sliderbar_value(widget * w);
int set_sliderbar_value(widget * w,int v);

void blank_sliderbar_function(widget * w);

#endif




