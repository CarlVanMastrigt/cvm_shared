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



void container_widget_add_child(widget * w,widget * child)
{
    child->base.next=NULL;
    child->base.prev=w->container.last;

    if(w->container.first==NULL)
    {
        w->container.first=child;
    }
    else
    {
        w->container.last->base.next=child;
    }

    w->container.last=child;

    child->base.parent=w;
}

void container_widget_remove_child(widget * w,widget * child)
{
    assert(child->base.parent==w);

    if(child->base.parent!=w)
    {
        return;
    }

    if(child==w->container.first)
    {
        w->container.first=child->base.next;
    }
    else
    {
        child->base.prev->base.next=child->base.next;
    }

    if(child==w->container.last)
    {
        w->container.last=child->base.prev;
    }
    else
    {
        child->base.next->base.prev=child->base.prev;
    }

    child->base.parent=NULL;
}

void container_widget_delete(widget * w)
{
    while(w->container.first)
    {
        delete_widget(w->container.first);
    }
}

static widget_behaviour_function_set container_behaviour_functions=
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
    .add_child      =   container_widget_add_child,
    .remove_child   =   container_widget_remove_child,
    .wid_delete     =   container_widget_delete
};









void container_widget_render(overlay_theme * theme,widget * w,int16_t x_off,int16_t y_off,struct cvm_overlay_render_batch * restrict render_batch,rectangle bounds)
{
    x_off+=w->base.r.x1;
    y_off+=w->base.r.y1;

    widget * current=w->container.first;

    while(current)
    {
        render_widget(current, theme, x_off, y_off, render_batch, bounds);

        current=current->base.next;
    }
}

widget * container_widget_select(overlay_theme * theme,widget * w,int16_t x_in,int16_t y_in)
{
    widget * current=w->container.last;
    widget * tmp=NULL;

    x_in-=w->base.r.x1;
    y_in-=w->base.r.y1;

    while(current)
    {
        tmp=select_widget(current, theme, x_in, y_in);

        if(tmp) return tmp;

        current=current->base.prev;
    }

    return NULL;
}

static void container_widget_min_w(overlay_theme * theme,widget * w)
{
    w->base.min_w=0;

    widget * current=w->container.first;

    uint32_t pos_flags=w->base.status&WIDGET_POS_FLAGS_H;

    while(current)
    {
        if(widget_active(current))
        {
            set_widget_minimum_width(current, theme, pos_flags);

            if(w->base.min_w<current->base.min_w)w->base.min_w=current->base.min_w;
        }

        current=current->base.next;
    }
}

static void container_widget_min_h(overlay_theme * theme,widget * w)
{
    w->base.min_h=0;

    widget * current=w->container.first;

    uint32_t pos_flags=w->base.status&WIDGET_POS_FLAGS_V;

    while(current)
    {
        if(widget_active(current))
        {
            set_widget_minimum_height(current, theme, pos_flags);

            if(w->base.min_h<current->base.min_h)w->base.min_h=current->base.min_h;
        }

        current=current->base.next;
    }
}

static void container_widget_set_w(overlay_theme * theme,widget * w)
{
    widget * current=w->container.first;

    while(current)
    {
        if(widget_active(current)) organise_widget_horizontally(current, theme, 0, w->base.r.x2 - w->base.r.x1);

        current=current->base.next;
    }
}

static void container_widget_set_h(overlay_theme * theme,widget * w)
{
    widget * current=w->container.first;

    while(current)
    {
        if(widget_active(current)) organise_widget_vertically(current, theme, 0, w->base.r.y2 - w->base.r.y1);

        current=current->base.next;
    }
}



static widget_appearence_function_set container_appearence_functions=
{
    .render =   container_widget_render,
    .select =   container_widget_select,
    .min_w  =   container_widget_min_w,
    .min_h  =   container_widget_min_h,
    .set_w  =   container_widget_set_w,
    .set_h  =   container_widget_set_h
};



widget * create_container(struct widget_context* context, size_t size)
{
    widget * w=create_widget(context, size);

    w->container.first=NULL;
    w->container.last=NULL;

    w->base.appearence_functions=&container_appearence_functions;
    w->base.behaviour_functions=&container_behaviour_functions;
    return w;
}














