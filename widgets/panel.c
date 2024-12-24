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



static void panel_widget_add_child(widget * w,widget * child)
{
    w->panel.contents=child;
    child->base.parent=w;
}

static void panel_widget_remove_child(widget * w,widget * child)
{
    w->panel.contents=NULL;
    child->base.parent=NULL;
}

static void panel_widget_delete(widget * w)
{
    delete_widget(w->panel.contents);
}


static widget_behaviour_function_set panel_behaviour_functions=
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






static void panel_widget_render(overlay_theme * theme,widget * w,int16_t x_off,int16_t y_off,struct cvm_overlay_render_batch * restrict render_batch,rectangle bounds)
{
    theme->panel_render(render_batch, theme, bounds, rectangle_add_offset(w->base.r, x_off, y_off), w->base.status, OVERLAY_BACKGROUND_COLOUR);

    render_widget(w->panel.contents, theme, x_off + w->base.r.x1, y_off + w->base.r.y1, render_batch, bounds);
}

static widget * panel_widget_select(overlay_theme * theme,widget * w,int16_t x_in,int16_t y_in)
{
    widget * tmp = select_widget(w->panel.contents, theme, x_in - w->base.r.x1, y_in - w->base.r.y1);
	if(tmp)
    {
        return tmp;
    }

	if(theme->panel_select(theme, rectangle_subtract_offset(w->base.r, x_in, y_in), w->base.status))
    {
        return w;
    }

    return NULL;
}


static void panel_widget_min_w(overlay_theme * theme,widget * w)
{
    w->base.min_w=set_widget_minimum_width(w->panel.contents, theme, w->base.status&WIDGET_POS_FLAGS_H)+
        ((w->base.status&WIDGET_H_FIRST)?theme->x_panel_offset_side:theme->x_panel_offset)+
        ((w->base.status&WIDGET_H_LAST)?theme->x_panel_offset_side:theme->x_panel_offset);
}

static void panel_widget_min_h(overlay_theme * theme,widget * w)
{
    w->base.min_h=set_widget_minimum_height(w->panel.contents, theme, w->base.status&WIDGET_POS_FLAGS_V)+
        ((w->base.status&WIDGET_V_FIRST)?theme->y_panel_offset_side:theme->y_panel_offset)+
        ((w->base.status&WIDGET_V_LAST)?theme->y_panel_offset_side:theme->y_panel_offset);
}

static void panel_widget_set_w(overlay_theme * theme,widget * w)
{
    organise_widget_horizontally(w->panel.contents, theme, ((w->base.status&WIDGET_H_FIRST)?theme->x_panel_offset_side:theme->x_panel_offset),
        w->base.r.x2 - w->base.r.x1 - ((w->base.status&WIDGET_H_FIRST)?theme->x_panel_offset_side:theme->x_panel_offset) - ((w->base.status&WIDGET_H_LAST)?theme->x_panel_offset_side:theme->x_panel_offset));
}

static void panel_widget_set_h(overlay_theme * theme,widget * w)
{
	organise_widget_vertically(w->panel.contents, theme, ((w->base.status&WIDGET_V_FIRST)?theme->y_panel_offset_side:theme->y_panel_offset),
            w->base.r.y2 - w->base.r.y1 -((w->base.status&WIDGET_V_FIRST)?theme->y_panel_offset_side:theme->y_panel_offset) - ((w->base.status&WIDGET_V_LAST)?theme->y_panel_offset_side:theme->y_panel_offset));
}


static widget_appearence_function_set panel_appearence_functions=
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
    widget * w=create_widget(sizeof(widget_panel));

    w->base.appearence_functions=&panel_appearence_functions;
    w->base.behaviour_functions=&panel_behaviour_functions;

    return w;
}


