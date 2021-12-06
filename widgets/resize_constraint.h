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

#ifndef WIDGET_RESIZE_CONSTRAINT_H
#define WIDGET_RESIZE_CONSTRAINT_H

#define WIDGET_RESIZABLE_FREE_MOVEMENT      0x0000
#define WIDGET_RESIZABLE_TOUCHING_LEFT      0x0001
#define WIDGET_RESIZABLE_TOUCHING_RIGHT     0x0002
#define WIDGET_RESIZABLE_TOUCHING_TOP       0x0004
#define WIDGET_RESIZABLE_TOUCHING_BOTTOM    0x0008
#define WIDGET_RESIZABLE_CENTRED_HORIZONTAL 0x0010
#define WIDGET_RESIZABLE_CENTRED_VERTICAL   0x0020
#define WIDGET_RESIZABLE_LOCKED_HORIZONTAL  0x0040
#define WIDGET_RESIZABLE_LOCKED_VERTICAL    0x0080
#define WIDGET_RESIZABLE_FILL_AREA          0x000F

/// WIDGET_RESIZABLE_CENTRED_VERTICAL and WIDGET_RESIZABLE_CENTRED_HORIZONTAL are intended for items touching edges that automatically centre (e.g. dropdown game panels)

typedef struct widget_resize_constraint
{
    widget_base base;

    widget * constrained;

    uint16_t alignment_data;///for maximize left shift and replace lower bits with appropriate values
    uint16_t maximized:1;
    uint16_t maximizable:1;
    uint16_t initially_centred:1;

    int alt_x_pos;///store positional data rather than reading from constrained
    int alt_y_pos;

    int x_clicked;
    int y_clicked;

    int resized_w;
    int resized_h;
}
widget_resize_constraint;


widget * create_resize_constraint(uint16_t alignment_data,bool maximizable);

void toggle_resize_constraint_maximize(widget * w);

#endif



