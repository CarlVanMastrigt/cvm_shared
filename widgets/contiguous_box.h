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

#ifndef CONTIGUOUS_BOX_H
#define CONTIGUOUS_BOX_H


typedef int(*widget_dimension_function)(overlay_theme*);


///is a contiguous box actually desirable... are ad-hoc solutions better?

typedef struct widget_contiguous_box
{
    widget_base base;

    int min_display_count;///if 0 display all
//    int visible_size;
    //int * offset;///when min_display_count only
//    int max_offset;
//    int min_offset;
//    int wheel_delta;///used for scrolling

    ///widget_layout layout;///this appers to have never been used...

//    widget_dimension_function default_min_w;
//    widget_dimension_function default_min_h;

    ///set wheel delta using min nonzero button/content width/height

    widget * contained_box;
}
widget_contiguous_box;

widget * create_contiguous_box(widget_layout layout,int min_display_count);

void set_contiguous_box_default_contained_dimensions(widget * contiguous_box,widget_dimension_function default_min_w,widget_dimension_function default_min_h);

void ensure_widget_in_contiguous_box_is_visible(widget * w,widget * cb);
widget * create_contiguous_box_scrollbar(widget * box);

bool get_ancestor_contiguous_box_data(widget * w,rectangle * r,uint32_t * status);

#endif






