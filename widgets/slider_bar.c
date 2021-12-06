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


void blank_slider_bar_function(widget * w)
{
	printf("blank slider_bar: %d\n",(*w->slider_bar.value_ptr));
}

static void validate_slider_bar_range(widget * w)
{
    if(w->slider_bar.min_value_ptr) w->slider_bar.min_value = *w->slider_bar.min_value_ptr;
    if(w->slider_bar.max_value_ptr) w->slider_bar.max_value = *w->slider_bar.max_value_ptr;
}

static void validate_slider_bar_value(widget * w)
{
    validate_slider_bar_range(w);

    if(w->slider_bar.min_value < w->slider_bar.max_value)
	{
	    if((*w->slider_bar.value_ptr) < w->slider_bar.min_value) *w->slider_bar.value_ptr = w->slider_bar.min_value;
        if((*w->slider_bar.value_ptr) > w->slider_bar.max_value) *w->slider_bar.value_ptr = w->slider_bar.max_value;
	}
	else
    {
        if((*w->slider_bar.value_ptr) > w->slider_bar.min_value) *w->slider_bar.value_ptr = w->slider_bar.min_value;
        if((*w->slider_bar.value_ptr) < w->slider_bar.max_value) *w->slider_bar.value_ptr = w->slider_bar.max_value;
    }

	if(w->slider_bar.func) w->slider_bar.func(w);
}

int set_slider_bar_value(widget * w,int v)
{
    *w->slider_bar.value_ptr=v;
    validate_slider_bar_value(w);
    return *w->slider_bar.value_ptr;
}

static bool slider_bar_widget_scroll(overlay_theme * theme,widget * w,int delta)
{
	int magnitude;

	validate_slider_bar_range(w);

	if(w->slider_bar.wheel_delta_ptr) magnitude = *w->slider_bar.wheel_delta_ptr;
    else magnitude=abs(w->slider_bar.max_value - w->slider_bar.min_value)/16;

	if(magnitude==0)magnitude=1;

	*w->slider_bar.value_ptr += delta*magnitude;
	validate_slider_bar_value(w);

	return true;
}

static void slider_bar_widget_delete(widget * w)
{
    if(w->slider_bar.free_data)free(w->slider_bar.data);
}


static void set_slider_bar_value_using_mouse_x(overlay_theme * theme,widget * w,int x)
{
    int dummy_y;
    adjust_coordinates_to_widget_local(w,&x,&dummy_y);

    validate_slider_bar_range(w);


	int width=w->base.r.x2-w->base.r.x1-theme->h_slider_bar_lost_w;
	int range=w->slider_bar.max_value-w->slider_bar.min_value;
	int bar_width;

	if((w->slider_bar.bar_size_ptr)&&((*w->slider_bar.bar_size_ptr)+abs(range))) bar_width = ((*w->slider_bar.bar_size_ptr)*width)/((*w->slider_bar.bar_size_ptr)+abs(range));
    else if(w->slider_bar.bar_fraction) bar_width=width/w->slider_bar.bar_fraction;
    else bar_width=0;

	width-=bar_width;
	if(width<1)width=1;

    *w->slider_bar.value_ptr = w->slider_bar.min_value + ((x-(theme->h_slider_bar_lost_w+bar_width)/2)*range + width/(2-4*(range<0)) )/width;

    validate_slider_bar_value(w);
}

static void horizontal_slider_bar_widget_left_click(overlay_theme * theme,widget * w,int x,int y)
{
	set_slider_bar_value_using_mouse_x(theme,w,x);
}

static void horizontal_slider_bar_widget_mouse_movement(overlay_theme * theme,widget * w,int x,int y)
{
	set_slider_bar_value_using_mouse_x(theme,w,x);
}

static widget_behaviour_function_set horizontal_slider_bar_behaviour_functions=
(widget_behaviour_function_set)
{
    .l_click        =   horizontal_slider_bar_widget_left_click,
    .l_release      =   blank_widget_left_release,
    .r_click        =   blank_widget_right_click,
    .m_move         =   horizontal_slider_bar_widget_mouse_movement,
    .scroll         =   slider_bar_widget_scroll,
    .key_down       =   blank_widget_key_down,
    .text_input     =   blank_widget_text_input,
    .text_edit      =   blank_widget_text_edit,
    .click_away     =   blank_widget_click_away,
    .add_child      =   blank_widget_add_child,
    .remove_child   =   blank_widget_remove_child,
    .wid_delete     =   slider_bar_widget_delete
};



static void set_slider_bar_value_using_mouse_y(overlay_theme * theme,widget * w,int y)
{
    int dummy_x;
    adjust_coordinates_to_widget_local(w,&dummy_x,&y);

    validate_slider_bar_range(w);

	int height=w->base.r.y2-w->base.r.y1-theme->v_slider_bar_lost_h;
	int range=w->slider_bar.max_value-w->slider_bar.min_value;
	int bar_height;

	if((w->slider_bar.bar_size_ptr)&&((*w->slider_bar.bar_size_ptr)+abs(range))) bar_height = ((*w->slider_bar.bar_size_ptr)*height)/((*w->slider_bar.bar_size_ptr)+abs(range));
    else if(w->slider_bar.bar_fraction) bar_height=height/w->slider_bar.bar_fraction;
    else bar_height=0;

	height-=bar_height;
	if(height<1)height=1;

    *w->slider_bar.value_ptr = w->slider_bar.min_value + ((y-(theme->v_slider_bar_lost_h+bar_height)/2)*range+ height/(2-4*(range<0)) )/height;

    validate_slider_bar_value(w);
}

static void vertical_slider_bar_widget_left_click(overlay_theme * theme,widget * w,int x,int y)
{
	set_slider_bar_value_using_mouse_y(theme,w,y);
}

static void vertical_slider_bar_widget_mouse_movement(overlay_theme * theme,widget * w,int x,int y)
{
	set_slider_bar_value_using_mouse_y(theme,w,y);
}

static widget_behaviour_function_set vertical_slider_bar_behaviour_functions=
(widget_behaviour_function_set)
{
    .l_click        =   vertical_slider_bar_widget_left_click,
    .l_release      =   blank_widget_left_release,
    .r_click        =   blank_widget_right_click,
    .m_move         =   vertical_slider_bar_widget_mouse_movement,
    .scroll         =   slider_bar_widget_scroll,
    .key_down       =   blank_widget_key_down,
    .text_input     =   blank_widget_text_input,
    .text_edit      =   blank_widget_text_edit,
    .click_away     =   blank_widget_click_away,
    .add_child      =   blank_widget_add_child,
    .remove_child   =   blank_widget_remove_child,
    .wid_delete     =   slider_bar_widget_delete
};









static void horizontal_slider_bar_widget_render(overlay_data * od,overlay_theme * theme,widget * w,int x_off,int y_off,rectangle_ bounds)
{
    validate_slider_bar_range(w);

    int bar=(w->slider_bar.bar_size_ptr) ? *w->slider_bar.bar_size_ptr : -w->slider_bar.bar_fraction;
    if(bar==0)bar=1;

    rectangle_ r=rectangle_add_offset(w->base.r,x_off,y_off);

    theme->h_bar_render(r,w->base.status,theme,od,bounds,OVERLAY_MAIN_COLOUR_);
	theme->h_bar_slider_render(r,w->base.status,theme,od,bounds,OVERLAY_TEXT_COLOUR_0_,abs(w->slider_bar.max_value-w->slider_bar.min_value),abs(*w->slider_bar.value_ptr-w->slider_bar.min_value),bar);
}

static widget * horizontal_slider_bar_widget_select(overlay_theme * theme,widget * w,int x_in,int y_in)
{
    if(theme->h_bar_select(rectangle_subtract_offset(w->base.r,x_in,y_in),w->base.status,theme))return w;

    return NULL;
}

static void horizontal_slider_bar_widget_min_w(overlay_theme * theme,widget * w)
{
    w->base.min_w=theme->base_unit_w*2;
}

static void horizontal_slider_bar_widget_min_h(overlay_theme * theme,widget * w)
{
    w->base.min_h=theme->base_unit_h;
}

static widget_appearence_function_set horizontal_slider_bar_appearence_functions=
(widget_appearence_function_set)
{
    .render =   horizontal_slider_bar_widget_render,
    .select =   horizontal_slider_bar_widget_select,
    .min_w  =   horizontal_slider_bar_widget_min_w,
    .min_h  =   horizontal_slider_bar_widget_min_h,
    .set_w  =   blank_widget_set_w,
    .set_h  =   blank_widget_set_h
};




widget * create_slider_bar(int * value_ptr,int min_value,int max_value,widget_function func,void * data,bool free_data,int bar_fraction)
{
	widget * w=create_widget(slider_bar_WIDGET);

	w->slider_bar.data=data;
	w->slider_bar.func=func;

	w->slider_bar.value_ptr=value_ptr;

	w->slider_bar.max_value=max_value;
	w->slider_bar.min_value=min_value;
	if(bar_fraction)w->slider_bar.bar_fraction=bar_fraction;
	else w->slider_bar.bar_fraction=1;

	w->slider_bar.min_value_ptr=NULL;
	w->slider_bar.max_value_ptr=NULL;
	w->slider_bar.bar_size_ptr=NULL;
	w->slider_bar.wheel_delta_ptr=NULL;

	w->slider_bar.free_data=free_data;

    w->base.appearence_functions=&horizontal_slider_bar_appearence_functions;
    w->base.behaviour_functions=&horizontal_slider_bar_behaviour_functions;

	return w;
}


void set_slider_bar_other_values(widget * w,int * min_value_ptr,int * max_value_ptr,int * bar_size_ptr,int * wheel_delta_ptr)
{
    w->slider_bar.min_value_ptr=min_value_ptr;
	w->slider_bar.max_value_ptr=max_value_ptr;
	w->slider_bar.bar_size_ptr=bar_size_ptr;
	w->slider_bar.wheel_delta_ptr=wheel_delta_ptr;
}
