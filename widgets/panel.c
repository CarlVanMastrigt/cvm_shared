/**
Copyright 2020,2021 Carl van Mastrigt

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



void panel_widget_add_child(widget * w,widget * child)
{
    w->panel.contents=child;
    child->base.parent=w;
}

void panel_widget_remove_child(widget * w,widget * child)
{
    w->panel.contents=NULL;
    child->base.parent=NULL;
}

void panel_widget_delete(widget * w)
{
    if(w->panel.contents)
    {
        delete_widget(w->panel.contents);
    }
}


static widget_behaviour_function_set panel_behaviour_functions=
(widget_behaviour_function_set)
{
    .l_click        =   blank_widget_left_click,
    .l_release      =   blank_widget_left_release,
    .r_click        =   blank_widget_right_click,
    .m_move         =   blank_widget_mouse_movement,
    .scroll         =   blank_widget_scroll,
    .key_down       =   blank_widget_key_down,
    .text_input     =   blank_widget_text_input,
    .text_edit      =   blank_widget_text_edit,
    .click_away     =   blank_widget_click_away,
    .add_child      =   panel_widget_add_child,
    .remove_child   =   panel_widget_remove_child,
    .wid_delete     =   panel_widget_delete
};






void panel_widget_render(overlay_theme * theme,widget * w,int x_off,int y_off,cvm_overlay_element_render_buffer * erb,rectangle bounds)
{
    theme->panel_render(erb,theme,bounds,rectangle_add_offset(w->base.r,x_off,y_off),w->base.status,OVERLAY_BACKGROUND_COLOUR_);

    render_widget(w->panel.contents,x_off+w->base.r.x1,y_off+w->base.r.y1,erb,bounds);
}

widget * panel_widget_select(overlay_theme * theme,widget * w,int x_in,int y_in)
{
    widget * tmp=select_widget(w->panel.contents,x_in-w->base.r.x1,y_in-w->base.r.y1);
	if(tmp)return tmp;

	if(theme->panel_select(theme,rectangle_subtract_offset(w->base.r,x_in,y_in),w->base.status))return w;
    return NULL;
}






void panel_widget_min_w(overlay_theme * theme,widget * w)
{
    w->base.min_w=0;

    if(w->panel.contents)
    {
        set_widget_minimum_width(w->panel.contents,w->base.status&WIDGET_POS_FLAGS_H);
        w->base.min_w=w->panel.contents->base.min_w+((w->base.status&WIDGET_H_FIRST)?theme->x_panel_offset_side:theme->x_panel_offset)+((w->base.status&WIDGET_H_LAST)?theme->x_panel_offset_side:theme->x_panel_offset);
    }
}

void panel_widget_min_h(overlay_theme * theme,widget * w)
{
	w->base.min_h=0;

    if(w->panel.contents)
    {
        set_widget_minimum_height(w->panel.contents,w->base.status&WIDGET_POS_FLAGS_V);
        w->base.min_h=w->panel.contents->base.min_h+((w->base.status&WIDGET_V_FIRST)?theme->y_panel_offset_side:theme->y_panel_offset)+((w->base.status&WIDGET_V_LAST)?theme->y_panel_offset_side:theme->y_panel_offset);
    }
}

void panel_widget_set_w(overlay_theme * theme,widget * w)
{
    if(w->panel.contents)
	{
	    organise_widget_horizontally(w->panel.contents,((w->base.status&WIDGET_H_FIRST)?theme->x_panel_offset_side:theme->x_panel_offset),
            w->base.r.x2-w->base.r.x1-((w->base.status&WIDGET_H_FIRST)?theme->x_panel_offset_side:theme->x_panel_offset)-((w->base.status&WIDGET_H_LAST)?theme->x_panel_offset_side:theme->x_panel_offset));
	}
}

void panel_widget_set_h(overlay_theme * theme,widget * w)
{
	if(w->panel.contents)
	{
	    organise_widget_vertically(w->panel.contents,((w->base.status&WIDGET_V_FIRST)?theme->y_panel_offset_side:theme->y_panel_offset),
            w->base.r.y2-w->base.r.y1-((w->base.status&WIDGET_V_FIRST)?theme->y_panel_offset_side:theme->y_panel_offset)-((w->base.status&WIDGET_V_LAST)?theme->y_panel_offset_side:theme->y_panel_offset));
	}
}


static widget_appearence_function_set panel_appearence_functions=
(widget_appearence_function_set)
{
    .render =   panel_widget_render,
    .select =   panel_widget_select,
    .min_w  =   panel_widget_min_w,
    .min_h  =   panel_widget_min_h,
    .set_w  =   panel_widget_set_w,
    .set_h  =   panel_widget_set_h
};


widget * create_panel(void)
{
    widget * w=create_widget(PANEL_WIDGET);

    w->base.appearence_functions=&panel_appearence_functions;
    w->base.behaviour_functions=&panel_behaviour_functions;

    return w;
}





//void panel_widget_render(overlay_data * od,overlay_theme * theme,widget * w,int x_off,int y_off,rectangle bounds)
//{
//    theme->panel_render(w->base.r,x_off,y_off,0,theme,od,bounds,OVERLAY_BACKGROUND_COLOUR);
//
//    render_widget(od,w->panel.contents,x_off+w->base.r.x,y_off+w->base.r.y,bounds);
//}
//
//widget * panel_widget_select(overlay_theme * theme,widget * w,int x_in,int y_in)
//{
//    widget * tmp=select_widget(w->panel.contents,x_in-w->base.r.x,y_in-w->base.r.y);
//	if(tmp)return tmp;
//
//	if(theme->panel_select(w->base.r,x_in,y_in,0,theme))return w;
//    return NULL;
//}

//static void panel_widget_min_w(overlay_theme * theme,widget * w)
//{
//    w->base.min_w=0;
//
//    if(w->panel.contents)
//    {
//        set_widget_minimum_width(w->panel.contents,w->base.status&WIDGET_POS_FLAGS_H);
//        w->base.min_w=w->panel.contents->base.min_w+2*theme->x_panel_offset;
//    }
//}
//static void panel_widget_min_h(overlay_theme * theme,widget * w)
//{
//	w->base.min_h=0;
//
//    if(w->panel.contents)
//    {
//        set_widget_minimum_height(w->panel.contents,w->base.status&WIDGET_POS_FLAGS_V);
//        w->base.min_h=w->panel.contents->base.min_h+2*theme->y_panel_offset;
//    }
//}
//
//static void panel_widget_set_w(overlay_theme * theme,widget * w)
//{
//    if(w->panel.contents)
//	{
//	    organise_widget_horizontally(w->panel.contents,theme->x_panel_offset,w->base.r.w-2*theme->x_panel_offset);
//	}
//}
//
//static void panel_widget_set_h(overlay_theme * theme,widget * w)
//{
//	if(w->panel.contents)
//	{
//	    organise_widget_vertically(w->panel.contents,theme->x_panel_offset,w->base.r.h-2*theme->x_panel_offset);
//	}
//}
//
//
//static widget_appearence_function_set panel_appearence_functions=
//(widget_appearence_function_set)
//{
//    .render =   panel_widget_render,
//    .select =   panel_widget_select,
//    .min_w  =   panel_widget_min_w,
//    .min_h  =   panel_widget_min_h,
//    .set_w  =   panel_widget_set_w,
//    .set_h  =   panel_widget_set_h
//};
//
//
//widget * create_panel(void)
//{
//    widget * w=create_widget(PANEL_WIDGET);
//
//    w->base.appearence_functions=&panel_appearence_functions;
//    w->base.behaviour_functions=&panel_behaviour_functions;
//
//    return w;
//}





















/*

static void small_panel_widget_render(overlay_data * od,overlay_theme * theme,widget * w,int x_off,int y_off,rectangle bounds)
{
    render_small_overlay_background_sidebox(w->base.r,x_off,y_off,w->base.status,bounds,od);

    x_off+=w->base.r.x;
    y_off+=w->base.r.y;

    render_widget(od,w->panel.contents,x_off,y_off,bounds);
}

static widget * small_panel_widget_select(overlay_theme * theme,widget * w,int x_in,int y_in)
{
    widget * tmp=select_widget(w->panel.contents,x_in-w->base.r.x,y_in-w->base.r.y);
	if(tmp)return tmp;

	if(select_small_overlay_background_sidebox(w->base.r,x_in,y_in,w->base.status,theme))return w;

    return NULL;
}

static void small_panel_widget_min_w(overlay_theme * theme,widget * w)
{
    w->base.min_w=0;

    if(w->panel.contents)
    {
        set_widget_minimum_width(w->panel.contents,w->base.status&WIDGET_POS_FLAGS_H);
        w->base.min_w=w->panel.contents->base.min_w+((w->base.status&WIDGET_H_FIRST)?2:4)+((w->base.status&WIDGET_H_LAST)?2:4);
    }
}

static void small_panel_widget_min_h(overlay_theme * theme,widget * w)
{
	w->base.min_h=0;

    if(w->panel.contents)
    {
        set_widget_minimum_height(w->panel.contents,w->base.status&WIDGET_POS_FLAGS_V);
        w->base.min_h=w->panel.contents->base.min_h+((w->base.status&WIDGET_V_FIRST)?3:5)+((w->base.status&WIDGET_V_LAST)?3:5);
    }
}

static void small_panel_widget_set_w(overlay_theme * theme,widget * w)
{
    if(w->panel.contents)
	{
	    organise_widget_horizontally(w->panel.contents,((w->base.status&WIDGET_H_FIRST)?2:4),w->base.r.w-((w->base.status&WIDGET_H_FIRST)?2:4)-((w->base.status&WIDGET_H_LAST)?2:4));
	}
}

static void small_panel_widget_set_h(overlay_theme * theme,widget * w)
{
	if(w->panel.contents)
	{
	    organise_widget_vertically(w->panel.contents,((w->base.status&WIDGET_V_FIRST)?3:5),w->base.r.h-((w->base.status&WIDGET_V_FIRST)?3:5)-((w->base.status&WIDGET_V_LAST)?3:5));
	}
}


static widget_appearence_function_set small_panel_appearence_functions=
(widget_appearence_function_set)
{
    .render =   small_panel_widget_render,
    .select =   small_panel_widget_select,
    .min_w  =   small_panel_widget_min_w,
    .min_h  =   small_panel_widget_min_h,
    .set_w  =   small_panel_widget_set_w,
    .set_h  =   small_panel_widget_set_h
};


widget * create_small_panel(void)
{
    widget * w=create_widget(panel_WIDGET);

    w->base.appearence_functions=&small_panel_appearence_functions;
    w->base.behaviour_functions=&panel_behaviour_functions;

    return w;
}
*/


