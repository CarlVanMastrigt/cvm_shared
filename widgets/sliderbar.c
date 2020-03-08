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


void blank_sliderbar_function(widget * w)
{
	printf("blank sliderbar: %d\n",(*w->sliderbar.value_ptr));
}

static void validate_sliderbar_range(widget * w)
{
    if(w->sliderbar.min_value_ptr) w->sliderbar.min_value = *w->sliderbar.min_value_ptr;
    if(w->sliderbar.max_value_ptr) w->sliderbar.max_value = *w->sliderbar.max_value_ptr;
}

static void validate_sliderbar_value(widget * w)
{
    validate_sliderbar_range(w);

    if(w->sliderbar.min_value < w->sliderbar.max_value)
	{
	    if((*w->sliderbar.value_ptr) < w->sliderbar.min_value) *w->sliderbar.value_ptr = w->sliderbar.min_value;
        if((*w->sliderbar.value_ptr) > w->sliderbar.max_value) *w->sliderbar.value_ptr = w->sliderbar.max_value;
	}
	else
    {
        if((*w->sliderbar.value_ptr) > w->sliderbar.min_value) *w->sliderbar.value_ptr = w->sliderbar.min_value;
        if((*w->sliderbar.value_ptr) < w->sliderbar.max_value) *w->sliderbar.value_ptr = w->sliderbar.max_value;
    }

	if(w->sliderbar.func) w->sliderbar.func(w);
}

int set_sliderbar_value(widget * w,int v)
{
    *w->sliderbar.value_ptr=v;
    validate_sliderbar_value(w);
    return *w->sliderbar.value_ptr;
}

static bool sliderbar_widget_scroll(overlay_theme * theme,widget * w,int delta)
{
	int magnitude;

	validate_sliderbar_range(w);

	if(w->sliderbar.wheel_delta_ptr) magnitude = *w->sliderbar.wheel_delta_ptr;
    else magnitude=abs(w->sliderbar.max_value - w->sliderbar.min_value)/16;

	if(magnitude==0)magnitude=1;

	*w->sliderbar.value_ptr += delta*magnitude;
	validate_sliderbar_value(w);

	return true;
}

static void sliderbar_widget_delete(widget * w)
{
    if(w->sliderbar.free_data)free(w->sliderbar.data);
}


static void set_sliderbar_value_using_mouse_x(overlay_theme * theme,widget * w,int x)
{
    int dummy_y;
    adjust_coordinates_to_widget_local(w,&x,&dummy_y);

    validate_sliderbar_range(w);


	int width=w->base.r.w-theme->h_sliderbar_lost_w;
	int range=w->sliderbar.max_value-w->sliderbar.min_value;
	int bar_width;

	if((w->sliderbar.bar_size_ptr)&&((*w->sliderbar.bar_size_ptr)+abs(range))) bar_width = ((*w->sliderbar.bar_size_ptr)*width)/((*w->sliderbar.bar_size_ptr)+abs(range));
    else if(w->sliderbar.bar_fraction) bar_width=width/w->sliderbar.bar_fraction;
    else bar_width=0;

	width-=bar_width;
	if(width<1)width=1;

    *w->sliderbar.value_ptr = w->sliderbar.min_value + ((x-(theme->h_sliderbar_lost_w+bar_width)/2)*range + width/(2-4*(range<0)) )/width;

    validate_sliderbar_value(w);
}

static void horizontal_sliderbar_widget_left_click(overlay_theme * theme,widget * w,int x,int y)
{
	set_sliderbar_value_using_mouse_x(theme,w,x);
}

static void horizontal_sliderbar_widget_mouse_movement(overlay_theme * theme,widget * w,int x,int y)
{
	set_sliderbar_value_using_mouse_x(theme,w,x);
}

static widget_behaviour_function_set horizontal_sliderbar_behaviour_functions=
(widget_behaviour_function_set)
{
    .l_click        =   horizontal_sliderbar_widget_left_click,
    .l_release      =   blank_widget_left_release,
    .r_click        =   blank_widget_right_click,
    .m_move         =   horizontal_sliderbar_widget_mouse_movement,
    .scroll         =   sliderbar_widget_scroll,
    .key_down       =   blank_widget_key_down,
    .click_away     =   blank_widget_click_away,
    .add_child      =   blank_widget_add_child,
    .remove_child   =   blank_widget_remove_child,
    .wid_delete     =   sliderbar_widget_delete
};



static void set_sliderbar_value_using_mouse_y(overlay_theme * theme,widget * w,int y)
{
    int dummy_x;
    adjust_coordinates_to_widget_local(w,&dummy_x,&y);

    validate_sliderbar_range(w);

	int height=w->base.r.h-theme->v_sliderbar_lost_h;
	int range=w->sliderbar.max_value-w->sliderbar.min_value;
	int bar_height;

	if((w->sliderbar.bar_size_ptr)&&((*w->sliderbar.bar_size_ptr)+abs(range))) bar_height = ((*w->sliderbar.bar_size_ptr)*height)/((*w->sliderbar.bar_size_ptr)+abs(range));
    else if(w->sliderbar.bar_fraction) bar_height=height/w->sliderbar.bar_fraction;
    else bar_height=0;

	height-=bar_height;
	if(height<1)height=1;

    *w->sliderbar.value_ptr = w->sliderbar.min_value + ((y-(theme->v_sliderbar_lost_h+bar_height)/2)*range+ height/(2-4*(range<0)) )/height;

    validate_sliderbar_value(w);
}

static void vertical_sliderbar_widget_left_click(overlay_theme * theme,widget * w,int x,int y)
{
	set_sliderbar_value_using_mouse_y(theme,w,y);
}

static void vertical_sliderbar_widget_mouse_movement(overlay_theme * theme,widget * w,int x,int y)
{
	set_sliderbar_value_using_mouse_y(theme,w,y);
}

static widget_behaviour_function_set vertical_sliderbar_behaviour_functions=
(widget_behaviour_function_set)
{
    .l_click        =   vertical_sliderbar_widget_left_click,
    .l_release      =   blank_widget_left_release,
    .r_click        =   blank_widget_right_click,
    .m_move         =   vertical_sliderbar_widget_mouse_movement,
    .scroll         =   sliderbar_widget_scroll,
    .key_down       =   blank_widget_key_down,
    .click_away     =   blank_widget_click_away,
    .add_child      =   blank_widget_add_child,
    .remove_child   =   blank_widget_remove_child,
    .wid_delete     =   sliderbar_widget_delete
};









static void horizontal_sliderbar_widget_render(overlay_data * od,overlay_theme * theme,widget * w,int x_off,int y_off,rectangle bounds)
{
    validate_sliderbar_range(w);

    int bar=(w->sliderbar.bar_size_ptr) ? *w->sliderbar.bar_size_ptr : -w->sliderbar.bar_fraction;
    if(bar==0)bar=1;

	theme->h_slider_bar_render(w->base.r,x_off,y_off,w->base.status,theme,od,bounds,OVERLAY_MAIN_COLOUR,abs(w->sliderbar.max_value-w->sliderbar.min_value),abs(*w->sliderbar.value_ptr-w->sliderbar.min_value),bar);
}

static widget * horizontal_sliderbar_widget_select(overlay_theme * theme,widget * w,int x_in,int y_in)
{
    if(theme->h_bar_select(w->base.r,x_in,y_in,w->base.status,theme))return w;

    return NULL;
}

static void horizontal_sliderbar_widget_min_w(overlay_theme * theme,widget * w)
{
    w->base.min_w=theme->base_unit_w*2;
}

static void horizontal_sliderbar_widget_min_h(overlay_theme * theme,widget * w)
{
    w->base.min_h=theme->base_unit_h;
}





static widget_appearence_function_set horizontal_sliderbar_appearence_functions=
(widget_appearence_function_set)
{
    .render =   horizontal_sliderbar_widget_render,
    .select =   horizontal_sliderbar_widget_select,
    .min_w  =   horizontal_sliderbar_widget_min_w,
    .min_h  =   horizontal_sliderbar_widget_min_h,
    .set_w  =   blank_widget_set_w,
    .set_h  =   blank_widget_set_h
};




static void vertical_sliderbar_widget_render(overlay_data * od,overlay_theme * theme,widget * w,int x_off,int y_off,rectangle bounds)
{
    validate_sliderbar_range(w);

    int bar=(w->sliderbar.bar_size_ptr) ? *w->sliderbar.bar_size_ptr : -w->sliderbar.bar_fraction;
    if(bar==0)bar=1;

	theme->v_slider_bar_render(w->base.r,x_off,y_off,w->base.status,theme,od,bounds,OVERLAY_MAIN_COLOUR,abs(w->sliderbar.max_value-w->sliderbar.min_value),abs(*w->sliderbar.value_ptr-w->sliderbar.min_value),bar);
}

static widget * vertical_sliderbar_widget_select(overlay_theme * theme,widget * w,int x_in,int y_in)
{
    if(theme->v_bar_select(w->base.r,x_in,y_in,w->base.status,theme))return w;

    return NULL;
}

static void vertical_sliderbar_widget_min_w(overlay_theme * theme,widget * w)
{
    w->base.min_w=theme->base_unit_w;
}

static void vertical_sliderbar_widget_min_h(overlay_theme * theme,widget * w)
{
    w->base.min_h=theme->base_unit_h*2;
}


static widget_appearence_function_set vertical_sliderbar_appearence_functions=
(widget_appearence_function_set)
{
    .render =   vertical_sliderbar_widget_render,
    .select =   vertical_sliderbar_widget_select,
    .min_w  =   vertical_sliderbar_widget_min_w,
    .min_h  =   vertical_sliderbar_widget_min_h,
    .set_w  =   blank_widget_set_w,
    .set_h  =   blank_widget_set_h
};






widget * create_sliderbar(int * value_ptr,int min_value,int max_value,widget_function func,void * data,bool free_data,widget_layout layout,int bar_fraction)
{
	widget * w=create_widget(SLIDERBAR_WIDGET);

	w->sliderbar.data=data;
	w->sliderbar.func=func;

	w->sliderbar.value_ptr=value_ptr;

	w->sliderbar.max_value=max_value;
	w->sliderbar.min_value=min_value;
	if(bar_fraction)w->sliderbar.bar_fraction=bar_fraction;
	else w->sliderbar.bar_fraction=1;

	w->sliderbar.min_value_ptr=NULL;
	w->sliderbar.max_value_ptr=NULL;
	w->sliderbar.bar_size_ptr=NULL;
	w->sliderbar.wheel_delta_ptr=NULL;

	w->sliderbar.free_data=free_data;

	if(layout==WIDGET_HORIZONTAL)
    {
        w->base.appearence_functions=&horizontal_sliderbar_appearence_functions;
        w->base.behaviour_functions=&horizontal_sliderbar_behaviour_functions;
    }
    if(layout==WIDGET_VERTICAL)
    {
        w->base.appearence_functions=&vertical_sliderbar_appearence_functions;
        w->base.behaviour_functions=&vertical_sliderbar_behaviour_functions;
    }

	return w;
}


void set_sliderbar_other_values(widget * w,int * min_value_ptr,int * max_value_ptr,int * bar_size_ptr,int * wheel_delta_ptr)
{
    w->sliderbar.min_value_ptr=min_value_ptr;
	w->sliderbar.max_value_ptr=max_value_ptr;
	w->sliderbar.bar_size_ptr=bar_size_ptr;
	w->sliderbar.wheel_delta_ptr=wheel_delta_ptr;
}
