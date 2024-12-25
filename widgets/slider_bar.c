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

#include "cvm_shared.h"


void blank_slider_bar_function(widget * w)
{
	printf("blank slider_bar: %d\n",(*w->slider_bar.value_ptr));
}

static void validate_slider_bar_value(widget * w)
{

    if(w->slider_bar.dynamic_range)
    {
        if(*w->slider_bar.min_value_ptr < *w->slider_bar.max_value_ptr)
        {
            if(*w->slider_bar.value_ptr < *w->slider_bar.min_value_ptr) *w->slider_bar.value_ptr = *w->slider_bar.min_value_ptr;
            else if(*w->slider_bar.value_ptr > *w->slider_bar.max_value_ptr) *w->slider_bar.value_ptr = *w->slider_bar.max_value_ptr;
        }
        else
        {
            if(*w->slider_bar.value_ptr > *w->slider_bar.min_value_ptr) *w->slider_bar.value_ptr = *w->slider_bar.min_value_ptr;
            else if(*w->slider_bar.value_ptr < *w->slider_bar.max_value_ptr) *w->slider_bar.value_ptr = *w->slider_bar.max_value_ptr;
        }
    }
    else
    {
        if(w->slider_bar.min_value < w->slider_bar.max_value)
        {
            if(*w->slider_bar.value_ptr < w->slider_bar.min_value) *w->slider_bar.value_ptr = w->slider_bar.min_value;
            else if(*w->slider_bar.value_ptr > w->slider_bar.max_value) *w->slider_bar.value_ptr = w->slider_bar.max_value;
        }
        else
        {
            if(*w->slider_bar.value_ptr > w->slider_bar.min_value) *w->slider_bar.value_ptr = w->slider_bar.min_value;
            else if(*w->slider_bar.value_ptr < w->slider_bar.max_value) *w->slider_bar.value_ptr = w->slider_bar.max_value;
        }
    }

	if(w->slider_bar.func) w->slider_bar.func(w);
}

int set_slider_bar_value(widget * w,int32_t v)
{
    *w->slider_bar.value_ptr=v;
    validate_slider_bar_value(w);
    return *w->slider_bar.value_ptr;
}

static bool slider_bar_widget_scroll(overlay_theme * theme,widget * w,int delta)
{
	int32_t magnitude;

	if(w->slider_bar.dynamic_scroll)
    {
        magnitude = *w->slider_bar.scroll_delta_ptr;
    }
	else
    {
        magnitude=(w->slider_bar.max_value - w->slider_bar.min_value)/w->slider_bar.scroll_fraction;
        if(magnitude==0) magnitude = 2*(w->slider_bar.max_value >= w->slider_bar.min_value)-1;
    }

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
    int32_t dummy_y,lost_width,width,range,min_v,max_v;
    adjust_coordinates_to_widget_local(w,&x,&dummy_y);

    ///crop lost range from both sides (theme and bar) then map range onto relative position in remaining range
    if(w->slider_bar.dynamic_range)
    {
        min_v=*w->slider_bar.min_value_ptr;
        max_v=*w->slider_bar.max_value_ptr;
    }
    else
    {
        min_v=w->slider_bar.min_value;
        max_v=w->slider_bar.max_value;
    }

    rectangle lost_space=theme->get_sliderbar_offsets(theme,w->base.status);
	width=w->base.r.x2-w->base.r.x1 - (lost_space.x1-lost_space.x2);

	range=max_v-min_v;

	lost_width = w->slider_bar.dynamic_bar ? ((*w->slider_bar.bar_size_ptr*width)/(*w->slider_bar.bar_size_ptr+range)) : (width/w->slider_bar.bar_fraction);///fraction of remaining width given to the bar
	width-=lost_width;

    x-=lost_width>>1;
    x-=lost_space.x1;

    if(x<0) *w->slider_bar.value_ptr = min_v;
    else if(x>width) *w->slider_bar.value_ptr = max_v;
    else *w->slider_bar.value_ptr = min_v + (x*range+(width>>1))/width;

    if(w->slider_bar.func) w->slider_bar.func(w);
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



//static void set_slider_bar_value_using_mouse_y(overlay_theme * theme,widget * w,int y)
//{
//    int dummy_x;
//    adjust_coordinates_to_widget_local(w,&dummy_x,&y);
//
//    validate_slider_bar_range(w);
//
//	int height=w->base.r.y2-w->base.r.y1-theme->v_slider_bar_lost_h;
//	int range=w->slider_bar.max_value-w->slider_bar.min_value;
//	int bar_height;
//
//	if((w->slider_bar.bar_size_ptr)&&((*w->slider_bar.bar_size_ptr)+abs(range))) bar_height = ((*w->slider_bar.bar_size_ptr)*height)/((*w->slider_bar.bar_size_ptr)+abs(range));
//    else if(w->slider_bar.bar_fraction) bar_height=height/w->slider_bar.bar_fraction;
//    else bar_height=0;
//
//	height-=bar_height;
//	if(height<1)height=1;
//
//    *w->slider_bar.value_ptr = w->slider_bar.min_value + ((y-(theme->v_slider_bar_lost_h+bar_height)/2)*range+ height/(2-4*(range<0)) )/height;
//
//    validate_slider_bar_value(w);
//}

//static void vertical_slider_bar_widget_left_click(overlay_theme * theme,widget * w,int x,int y)
//{
//	set_slider_bar_value_using_mouse_y(theme,w,y);
//}
//
//static void vertical_slider_bar_widget_mouse_movement(overlay_theme * theme,widget * w,int x,int y)
//{
//	set_slider_bar_value_using_mouse_y(theme,w,y);
//}
//
//static widget_behaviour_function_set vertical_slider_bar_behaviour_functions=
//(widget_behaviour_function_set)
//{
//    .l_click        =   vertical_slider_bar_widget_left_click,
//    .l_release      =   blank_widget_left_release,
//    .r_click        =   blank_widget_right_click,
//    .m_move         =   vertical_slider_bar_widget_mouse_movement,
//    .scroll         =   slider_bar_widget_scroll,
//    .key_down       =   blank_widget_key_down,
//    .text_input     =   blank_widget_text_input,
//    .text_edit      =   blank_widget_text_edit,
//    .click_away     =   blank_widget_click_away,
//    .add_child      =   blank_widget_add_child,
//    .remove_child   =   blank_widget_remove_child,
//    .wid_delete     =   slider_bar_widget_delete
//};









static void horizontal_slider_bar_widget_render(overlay_theme * theme,widget * w,int16_t x_off,int16_t y_off,struct cvm_overlay_render_batch * restrict render_batch,rectangle bounds)
{
    int32_t before,bar,after,range,m;

    if(w->slider_bar.dynamic_range)
    {
        before=*w->slider_bar.value_ptr-*w->slider_bar.min_value_ptr;
        after=*w->slider_bar.max_value_ptr-*w->slider_bar.value_ptr;
    }
    else
    {
        before=*w->slider_bar.value_ptr-w->slider_bar.min_value;
        after=w->slider_bar.max_value-*w->slider_bar.value_ptr;
    }

    rectangle r=rectangle_add_offset(w->base.r,x_off,y_off);

    if(w->slider_bar.dynamic_bar)bar=*w->slider_bar.bar_size_ptr;
    else
    {
        range=before+after;

        m=r.x2-r.x1;
        assert(range < 2 || m < 0x7FFFFFFF/range-1);///SLIDERBAR RANGE TOO LARGE FOR SIZE ON SCREEN, YOU SHOULD IMPOSE SOME LIMITS ON RANGE AND/OR SIZE
        range*=m;
        before*=m;
        after*=m;

        bar=range/(w->slider_bar.bar_fraction);
        before=(before*(w->slider_bar.bar_fraction-1))/w->slider_bar.bar_fraction;
        after=(after*(w->slider_bar.bar_fraction-1))/w->slider_bar.bar_fraction;
    }

    theme->h_bar_render(render_batch, theme, bounds, r, w->base.status, OVERLAY_MAIN_COLOUR);
	theme->h_bar_slider_render(render_batch, theme, bounds, r, w->base.status, OVERLAY_TEXT_COLOUR_0, before, bar, after);
}

static widget * horizontal_slider_bar_widget_select(overlay_theme * theme,widget * w,int16_t x_in,int16_t y_in)
{
    if(theme->h_bar_select(theme,rectangle_subtract_offset(w->base.r,x_in,y_in),w->base.status))return w;

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
{
    .render =   horizontal_slider_bar_widget_render,
    .select =   horizontal_slider_bar_widget_select,
    .min_w  =   horizontal_slider_bar_widget_min_w,
    .min_h  =   horizontal_slider_bar_widget_min_h,
    .set_w  =   blank_widget_set_w,
    .set_h  =   blank_widget_set_h
};




widget * create_slider_bar(struct widget_context* context, int * value,widget_function func,void * data,bool free_data)
{
	widget * w = create_widget(context, sizeof(widget_slider_bar));

	w->slider_bar.data=data;
	w->slider_bar.func=func;

	w->slider_bar.value_ptr=value;

	w->slider_bar.min_value=0;
	w->slider_bar.max_value=1;
	w->slider_bar.bar_fraction=2;
	w->slider_bar.scroll_fraction=1;

	w->slider_bar.dynamic_range=false;
	w->slider_bar.dynamic_bar=false;
	w->slider_bar.dynamic_scroll=false;

	w->slider_bar.free_data=free_data;

    w->base.appearence_functions=&horizontal_slider_bar_appearence_functions;
    w->base.behaviour_functions=&horizontal_slider_bar_behaviour_functions;

	return w;
}

widget * create_slider_bar_fixed(struct widget_context* context, int32_t * value,int32_t min_value,int32_t max_value,int32_t bar_fraction,int32_t scroll_fraction,widget_function func,void * data,bool free_data)
{
	widget * w=create_widget(context, sizeof(widget_slider_bar));

	assert(bar_fraction>=2);///CANNOT TAKE UP WHOLE BAR OR DIVIDE BY 0

	w->slider_bar.data=data;
	w->slider_bar.func=func;

	w->slider_bar.value_ptr=value;

	w->slider_bar.min_value=min_value;
	w->slider_bar.max_value=max_value;
	w->slider_bar.bar_fraction=bar_fraction;
	w->slider_bar.scroll_fraction=scroll_fraction;

	w->slider_bar.dynamic_range=false;
	w->slider_bar.dynamic_bar=false;
	w->slider_bar.dynamic_scroll=false;

	w->slider_bar.free_data=free_data;

    w->base.appearence_functions=&horizontal_slider_bar_appearence_functions;
    w->base.behaviour_functions=&horizontal_slider_bar_behaviour_functions;

	return w;
}

widget * create_slider_bar_dynamic(struct widget_context* context, int32_t * value,const int32_t * min_value,const int32_t * max_value,const int32_t * bar_size,const int32_t * scroll_delta,widget_function func,void * data,bool free_data)
{
	widget * w=create_widget(context, sizeof(widget_slider_bar));

	w->slider_bar.data=data;
	w->slider_bar.func=func;

	w->slider_bar.value_ptr=value;

	w->slider_bar.min_value_ptr=min_value;
	w->slider_bar.max_value_ptr=max_value;
	w->slider_bar.bar_size_ptr=bar_size;
	w->slider_bar.scroll_delta_ptr=scroll_delta;

	w->slider_bar.dynamic_range=true;
	w->slider_bar.dynamic_bar=true;
	w->slider_bar.dynamic_scroll=true;

	w->slider_bar.free_data=free_data;

    w->base.appearence_functions=&horizontal_slider_bar_appearence_functions;
    w->base.behaviour_functions=&horizontal_slider_bar_behaviour_functions;

	return w;
}

