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

#ifndef WIDGET_CONTAINER_H
#define WIDGET_CONTAINER_H


typedef struct widget_container
{
    widget_base base;

    widget * first;
    widget * last;
}
widget_container;


widget * create_container(void);


void container_widget_render(overlay_theme * theme,widget * w,int16_t x_off,int16_t y_off,cvm_overlay_element_render_buffer * erb,rectangle bounds);
widget * container_widget_select(overlay_theme * theme,widget * w,int16_t x_in,int16_t y_in);


void container_widget_add_child(widget * w,widget * child);
void container_widget_remove_child(widget * w,widget * child);
void container_widget_delete(widget * w);


#endif



