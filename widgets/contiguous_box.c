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

#include "cvm_shared.h"








static bool contiguous_box_widget_scroll(overlay_theme * theme,widget * w,int delta)
{
	*w->contiguous_box.offset += delta*w->contiguous_box.wheel_delta;

	if(*w->contiguous_box.offset > w->contiguous_box.max_offset) *w->contiguous_box.offset=w->contiguous_box.max_offset;
	if(*w->contiguous_box.offset < w->contiguous_box.min_offset) *w->contiguous_box.offset=w->contiguous_box.min_offset;

	return true;
}

void contiguous_box_widget_add_child(widget * w,widget * child)
{
    add_child_to_parent(w->contiguous_box.contained_box,child);
}

void contiguous_box_widget_remove_child(widget * w,widget * child)
{
    //puts("FINISH: contiguous_box_widget_remove_child");
    //remove_child_from_parent()
    w->contiguous_box.contained_box=NULL;
    child->base.parent=NULL;
}

void contiguous_box_widget_delete(widget * w)
{
    delete_widget(w->contiguous_box.contained_box);
}

static widget_behaviour_function_set contiguous_box_behaviour_functions=
(widget_behaviour_function_set)
{
    .l_click        =   blank_widget_left_click,
    .l_release      =   blank_widget_left_release,
    .r_click        =   blank_widget_right_click,
    .m_move         =   blank_widget_mouse_movement,
    .scroll         =   contiguous_box_widget_scroll,
    .key_down       =   blank_widget_key_down,
    .click_away     =   blank_widget_click_away,
    .add_child      =   contiguous_box_widget_add_child,
    .remove_child   =   contiguous_box_widget_remove_child,
    .wid_delete     =   contiguous_box_widget_delete
};








void all_visible_contiguous_box_widget_render(overlay_data * od,overlay_theme * theme,widget * w,int x_off,int y_off,rectangle bounds)
{
    theme->box_render(w->base.r,x_off,y_off,w->base.status,theme,od,bounds,OVERLAY_MAIN_COLOUR);

    x_off+=w->base.r.x;
    y_off+=w->base.r.y;

    render_widget(od,w->contiguous_box.contained_box,x_off,y_off,bounds);
}

void some_visible_contiguous_box_widget_render(overlay_data * od,overlay_theme * theme,widget * w,int x_off,int y_off,rectangle bounds)
{
    theme->box_render(w->base.r,x_off,y_off,w->base.status,theme,od,bounds,OVERLAY_MAIN_COLOUR);

    x_off+=w->base.r.x;
    y_off+=w->base.r.y;

    get_rectangle_overlap(&bounds,(rectangle)
    {
        .x=x_off+theme->contiguous_some_box_x_offset,
        .y=y_off+theme->contiguous_some_box_y_offset,
        .w=w->base.r.w-2*theme->contiguous_some_box_x_offset,
        .h=w->base.r.h-2*theme->contiguous_some_box_y_offset
    });

    render_widget(od,w->contiguous_box.contained_box,x_off,y_off,bounds);
}

widget * contiguous_box_widget_select(overlay_theme * theme,widget * w,int x_in,int y_in)
{
    widget * tmp;

    if(theme->box_select(w->base.r,x_in,y_in,w->base.status,theme))
    {
        x_in-=w->base.r.x;
        y_in-=w->base.r.y;

        tmp=select_widget(w->contiguous_box.contained_box,x_in,y_in);
        if(tmp!=NULL)return tmp;

        return w;
    }

    return NULL;
}























static void all_visible_contiguous_box_widget_min_w(overlay_theme * theme,widget * w)
{
    w->base.min_w = set_widget_minimum_width(w->contiguous_box.contained_box,0)+2*theme->contiguous_all_box_x_offset;
}

static void all_visible_contiguous_box_widget_min_h(overlay_theme * theme,widget * w)
{
    w->base.min_h = set_widget_minimum_height(w->contiguous_box.contained_box,0)+2*theme->contiguous_all_box_y_offset;
}

static void all_visible_contiguous_box_widget_set_w(overlay_theme * theme,widget * w)
{
    organise_widget_horizontally(w->contiguous_box.contained_box,theme->contiguous_all_box_x_offset,w->base.r.w-2*theme->contiguous_all_box_x_offset);
}

static void all_visible_contiguous_box_widget_set_h(overlay_theme * theme,widget * w)
{
    organise_widget_vertically(w->contiguous_box.contained_box,theme->contiguous_all_box_y_offset,w->base.r.h-2*theme->contiguous_all_box_y_offset);
}

static widget_appearence_function_set all_visible_contiguous_box_functions=
(widget_appearence_function_set)
{
    .render =   all_visible_contiguous_box_widget_render,
    .select =   contiguous_box_widget_select,
    .min_w  =   all_visible_contiguous_box_widget_min_w,
    .min_h  =   all_visible_contiguous_box_widget_min_h,
    .set_w  =   all_visible_contiguous_box_widget_set_w,
    .set_h  =   all_visible_contiguous_box_widget_set_h
};









static void vertical_some_visible_contiguous_box_widget_min_w(overlay_theme * theme,widget * w)
{
    w->base.min_w = w->contiguous_box.default_min_w(theme);

    set_widget_minimum_width(w->contiguous_box.contained_box,0);

    if(w->base.min_w < w->contiguous_box.contained_box->base.min_w)w->base.min_w = w->contiguous_box.contained_box->base.min_w;

    w->base.min_w+=theme->contiguous_some_box_x_offset*2;
}

static void vertical_some_visible_contiguous_box_widget_min_h(overlay_theme * theme,widget * w)
{
    set_widget_minimum_height(w->contiguous_box.contained_box,0);

    w->base.min_h=w->contiguous_box.min_display_count*w->contiguous_box.default_min_h(theme)+2*theme->contiguous_some_box_y_offset;
}

static void vertical_some_visible_contiguous_box_widget_set_w(overlay_theme * theme,widget * w)
{
    organise_widget_horizontally(w->contiguous_box.contained_box,theme->contiguous_some_box_x_offset,w->base.r.w-2*theme->contiguous_some_box_x_offset);
}

static void vertical_some_visible_contiguous_box_widget_set_h(overlay_theme * theme,widget * w)
{
    organise_widget_vertically(w->contiguous_box.contained_box,*w->contiguous_box.offset,w->contiguous_box.contained_box->base.min_h);

    w->contiguous_box.wheel_delta = w->contiguous_box.default_min_h(theme);
    w->contiguous_box.visible_size=w->base.r.h-2*theme->contiguous_some_box_y_offset;
    w->contiguous_box.min_offset=theme->contiguous_some_box_y_offset- (w->contiguous_box.contained_box->base.r.h - w->contiguous_box.visible_size);
    w->contiguous_box.max_offset=theme->contiguous_some_box_y_offset;

    if(w->contiguous_box.min_offset > w->contiguous_box.max_offset)w->contiguous_box.min_offset=w->contiguous_box.max_offset;

    if(*w->contiguous_box.offset > w->contiguous_box.max_offset) *w->contiguous_box.offset=w->contiguous_box.max_offset;
	if(*w->contiguous_box.offset < w->contiguous_box.min_offset) *w->contiguous_box.offset=w->contiguous_box.min_offset;
}

static widget_appearence_function_set vertical_some_visible_contiguous_box_functions=
(widget_appearence_function_set)
{
    .render =   some_visible_contiguous_box_widget_render,
    .select =   contiguous_box_widget_select,
    .min_w  =   vertical_some_visible_contiguous_box_widget_min_w,
    .min_h  =   vertical_some_visible_contiguous_box_widget_min_h,
    .set_w  =   vertical_some_visible_contiguous_box_widget_set_w,
    .set_h  =   vertical_some_visible_contiguous_box_widget_set_h
};










static void horizontal_some_visible_contiguous_box_widget_min_w(overlay_theme * theme,widget * w)
{
    set_widget_minimum_width(w->contiguous_box.contained_box,0);

    w->base.min_w=w->contiguous_box.min_display_count*w->contiguous_box.default_min_w(theme)+2*theme->contiguous_some_box_x_offset;
}

static void horizontal_some_visible_contiguous_box_widget_min_h(overlay_theme * theme,widget * w)
{
    w->base.min_h = w->contiguous_box.default_min_h(theme);

    set_widget_minimum_height(w->contiguous_box.contained_box,0);

    if(w->base.min_h < w->contiguous_box.contained_box->base.min_h)w->base.min_h = w->contiguous_box.contained_box->base.min_h;

    w->base.min_h+=theme->contiguous_some_box_y_offset*2;
}

static void horizontal_some_visible_contiguous_box_widget_set_w(overlay_theme * theme,widget * w)
{
}

static void horizontal_some_visible_contiguous_box_widget_set_h(overlay_theme * theme,widget * w)
{
}

static widget_appearence_function_set horizontal_some_visible_contiguous_box_functions=
(widget_appearence_function_set)
{
    .render =   some_visible_contiguous_box_widget_render,
    .select =   contiguous_box_widget_select,
    .min_w  =   horizontal_some_visible_contiguous_box_widget_min_w,
    .min_h  =   horizontal_some_visible_contiguous_box_widget_min_h,
    .set_w  =   horizontal_some_visible_contiguous_box_widget_set_w,
    .set_h  =   horizontal_some_visible_contiguous_box_widget_set_h
};







static int contiguous_element_default_w(overlay_theme * theme)
{
    return theme->base_unit_w;
}

static int contiguous_element_default_h(overlay_theme * theme)
{
    return theme->contiguous_horizintal_bar_h;
}


widget * create_contiguous_box(widget_layout layout,int min_display_count)
{
    widget * w=create_container();

    if(min_display_count)
    {
        if(layout==WIDGET_VERTICAL) w->base.appearence_functions=&vertical_some_visible_contiguous_box_functions;///NOT CORRECTED/FINISHED
        else if(layout==WIDGET_HORIZONTAL) w->base.appearence_functions=&horizontal_some_visible_contiguous_box_functions;
        else puts("ERROR contiguous box layout unhandled");
    }
    else w->base.appearence_functions=&all_visible_contiguous_box_functions;
    w->base.behaviour_functions=&contiguous_box_behaviour_functions;

    w->contiguous_box.contained_box=create_box(layout,WIDGET_NORMALLY_DISTRIBUTED);
    w->contiguous_box.contained_box->base.parent=w;

    w->base.type=CONTIGUOUS_BOX_WIDGET;

    if(layout==WIDGET_VERTICAL) w->contiguous_box.offset = &w->contiguous_box.contained_box->base.r.y;
    else w->contiguous_box.offset = &w->contiguous_box.contained_box->base.r.x;

    w->contiguous_box.min_display_count=min_display_count;
    w->contiguous_box.visible_size=0;
    w->contiguous_box.max_offset=0;
    w->contiguous_box.min_offset=0;
    w->contiguous_box.wheel_delta=0;

    w->contiguous_box.default_min_w=contiguous_element_default_w;
    w->contiguous_box.default_min_h=contiguous_element_default_h;

    w->contiguous_box.layout=layout;

    return w;
}

void set_contiguous_box_default_contained_dimensions(widget * contiguous_box,widget_dimension_function default_min_w,widget_dimension_function default_min_h)
{
    if(default_min_w)contiguous_box->contiguous_box.default_min_w=default_min_w;
    if(default_min_h)contiguous_box->contiguous_box.default_min_h=default_min_h;
}



void ensure_widget_in_contiguous_box_is_visible(widget * w,widget * cb)
{
    //widget * cb=w->base.parent->base.parent;

    #warning rather than using w->base.r.x use absolute / relative coordinates of w within cb to allow sub-boxes within cb

    if((widget_active(w))&&(cb->contiguous_box.min_display_count))
    {
        if(cb->contiguous_box.layout==WIDGET_HORIZONTAL)
        {
            if(*cb->contiguous_box.offset < (cb->contiguous_box.max_offset - w->base.r.x))
                *cb->contiguous_box.offset = cb->contiguous_box.max_offset - w->base.r.x;

            if(*cb->contiguous_box.offset > (cb->contiguous_box.max_offset - w->base.r.x - w->base.r.w + cb->contiguous_box.visible_size))
                *cb->contiguous_box.offset = cb->contiguous_box.max_offset - w->base.r.x - w->base.r.w + cb->contiguous_box.visible_size;
        }
        if(cb->contiguous_box.layout==WIDGET_VERTICAL)
        {
            if(*cb->contiguous_box.offset < (cb->contiguous_box.max_offset - w->base.r.y))
                *cb->contiguous_box.offset = cb->contiguous_box.max_offset - w->base.r.y;

            if(*cb->contiguous_box.offset > (cb->contiguous_box.max_offset - w->base.r.y - w->base.r.h + cb->contiguous_box.visible_size))
                *cb->contiguous_box.offset = cb->contiguous_box.max_offset - w->base.r.y - w->base.r.h + cb->contiguous_box.visible_size;
        }
    }
}

widget * create_contiguous_box_scrollbar(widget * box)
{
    widget * w=create_sliderbar(box->contiguous_box.offset,0,0,NULL,NULL,false,box->contiguous_box.layout,0);

    set_sliderbar_other_values(w,&box->contiguous_box.max_offset,&box->contiguous_box.min_offset,&box->contiguous_box.visible_size,&box->contiguous_box.wheel_delta);

    return w;
}


