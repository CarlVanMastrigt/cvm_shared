/**
Copyright 2022 Carl van Mastrigt

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



static void centring_container_widget_add_child(widget * w,widget * child)
{
    w->centring_container.contents=child;
    child->base.parent=w;
}

static void centring_container_widget_remove_child(widget * w,widget * child)
{
    w->centring_container.contents=NULL;
    child->base.parent=NULL;
}

static void centring_container_widget_delete(widget * w)
{
    delete_widget(w->centring_container.contents);
}


static widget_behaviour_function_set centring_container_behaviour_functions=
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
    .add_child      =   centring_container_widget_add_child,
    .remove_child   =   centring_container_widget_remove_child,
    .wid_delete     =   centring_container_widget_delete
};






static void centring_container_widget_render(overlay_theme * theme,widget * w,int16_t x_off,int16_t y_off,cvm_overlay_element_render_buffer * erb,rectangle bounds)
{
    render_widget(w->centring_container.contents,x_off+w->base.r.x1,y_off+w->base.r.y1,erb,bounds);
}

static widget * centring_container_widget_select(overlay_theme * theme,widget * w,int16_t x_in,int16_t y_in)
{
    widget * tmp=select_widget(w->centring_container.contents,x_in-w->base.r.x1,y_in-w->base.r.y1);
	if(tmp)return tmp;
}


static void centring_container_widget_min_w(overlay_theme * theme,widget * w)
{
    w->base.min_w=set_widget_minimum_width(w->centring_container.contents,0);
}

static void centring_container_widget_min_h(overlay_theme * theme,widget * w)
{
    w->base.min_h=set_widget_minimum_height(w->centring_container.contents,0);
}

static void centring_container_widget_set_w(overlay_theme * theme,widget * w)
{
    widget * ref=w->centring_container.reference_widget;
    widget * con=w->centring_container.contents;
    if(ref)
    {
        organise_widget_horizontally(con,(ref->base.r.x2-ref->base.r.x1-con->base.min_w)>>1,con->base.min_w);
    }
    else
	{
	    organise_widget_horizontally(con,(w->base.r.x2-w->base.r.x1-con->base.min_w)>>1,con->base.min_w);
	}
}

static void centring_container_widget_set_h(overlay_theme * theme,widget * w)
{
    widget * ref=w->centring_container.reference_widget;
    widget * con=w->centring_container.contents;
	if(ref)
    {
        organise_widget_vertically(con,(ref->base.r.y2-ref->base.r.y1-con->base.min_h)>>1,con->base.min_h);
    }
    else
	{
	    organise_widget_vertically(con,(w->base.r.y2-w->base.r.y1-con->base.min_h)>>1,con->base.min_h);
	}
}


static widget_appearence_function_set centring_container_appearence_functions=
{
    .render =   centring_container_widget_render,
    .select =   centring_container_widget_select,
    .min_w  =   centring_container_widget_min_w,
    .min_h  =   centring_container_widget_min_h,
    .set_w  =   centring_container_widget_set_w,
    .set_h  =   centring_container_widget_set_h
};


widget * create_centring_container(void)
{
    widget * w=create_widget();

    w->centring_container.reference_widget=NULL;

    w->base.appearence_functions=&centring_container_appearence_functions;
    w->base.behaviour_functions=&centring_container_behaviour_functions;

    return w;
}


