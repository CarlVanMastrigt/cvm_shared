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

static void anchor_widget_left_click(overlay_theme * theme,widget * w,int x,int y)
{
    widget * c;

    if(w->anchor.constraint)
    {
        if(check_widget_double_clicked(w))
        {
            toggle_resize_constraint_maximize(w->anchor.constraint);
            set_currently_active_widget(NULL);
        }
        else
        {
            c=w->anchor.constraint->resize_constraint.constrained;
            adjust_coordinates_to_widget_local(w->anchor.constraint,&x,&y);

            if(c)
            {
                w->anchor.x_clicked=x-c->base.r.x;
                w->anchor.y_clicked=y-c->base.r.y;
            }
        }
    }
}

static void anchor_widget_mouse_movement(overlay_theme * theme,widget * w,int x,int y)
{
    widget * c;

    if(w->anchor.constraint)
    {
        c=w->anchor.constraint->resize_constraint.constrained;
        adjust_coordinates_to_widget_local(w->anchor.constraint,&x,&y);

        if(c)
        {
            c->base.r.x=x-w->anchor.x_clicked;
            c->base.r.y=y-w->anchor.y_clicked;

            if(c->base.r.x < 0)c->base.r.x=0;
            if(c->base.r.y < 0)c->base.r.y=0;

            if(c->base.r.x+c->base.r.w > w->anchor.constraint->base.r.w)c->base.r.x= w->anchor.constraint->base.r.w-c->base.r.w;
            if(c->base.r.y+c->base.r.h > w->anchor.constraint->base.r.h)c->base.r.y= w->anchor.constraint->base.r.h-c->base.r.h;
        }
    }
}

static void anchor_widget_delete(widget * w)
{
    if(w->anchor.text)free(w->anchor.text);
}


static widget_behaviour_function_set anchor_behaviour_functions=
(widget_behaviour_function_set)
{
    .l_click        =   anchor_widget_left_click,
    .l_release      =   blank_widget_left_release,
    .r_click        =   blank_widget_right_click,
    .m_move         =   anchor_widget_mouse_movement,
    .scroll         =   blank_widget_scroll,
    .key_down       =   blank_widget_key_down,
    .text_input     =   blank_widget_text_input,
    .text_edit      =   blank_widget_text_edit,
    .click_away     =   blank_widget_click_away,
    .add_child      =   blank_widget_add_child,
    .remove_child   =   blank_widget_remove_child,
    .wid_delete     =   anchor_widget_delete
};





static void text_anchor_widget_render(overlay_data * od,overlay_theme * theme,widget * w,int x_off,int y_off,rectangle bounds)
{
	//theme->h_text_bar_render(rectangle_add_offset(rectangle_new_conversion(w->base.r),x_off,y_off),w->base.status,theme,od,rectangle_new_conversion(bounds),OVERLAY_MAIN_PROMINENT_COLOUR,w->anchor.text,OVERLAY_TEXT_COLOUR_0_);
	theme->h_text_bar_render(rectangle_add_offset(rectangle_new_conversion(w->base.r),x_off,y_off),w->base.status,theme,od,rectangle_new_conversion(bounds),OVERLAY_MAIN_COLOUR_,w->anchor.text,OVERLAY_TEXT_COLOUR_0_);
}

static widget * text_anchor_widget_select(overlay_theme * theme,widget * w,int x_in,int y_in)
{
    if(theme->h_bar_select(rectangle_subtract_offset(rectangle_new_conversion(w->base.r),x_in,y_in),w->base.status,theme))return w;

    return NULL;
}

static void text_anchor_widget_min_w(overlay_theme * theme,widget * w)
{
	w->base.min_w = overlay_size_text_simple(&theme->font_,w->anchor.text)+2*theme->h_bar_text_offset;
}

static void text_anchor_widget_min_h(overlay_theme * theme,widget * w)
{
	w->base.min_h = theme->base_unit_h;
}





static widget_appearence_function_set text_anchor_appearence_functions=
(widget_appearence_function_set)
{
    .render =   text_anchor_widget_render,
    .select =   text_anchor_widget_select,
    .min_w  =   text_anchor_widget_min_w,
    .min_h  =   text_anchor_widget_min_h,
    .set_w  =   blank_widget_set_w,
    .set_h  =   blank_widget_set_h
};

widget * create_anchor(widget * constraint,char * title)
{
    widget * w=create_widget(ANCHOR_WIDGET);

    w->base.behaviour_functions=&anchor_behaviour_functions;
    w->base.appearence_functions=&text_anchor_appearence_functions;

    w->anchor.x_clicked=0;
    w->anchor.y_clicked=0;
    if(title)w->anchor.text=strdup(title);
    else w->anchor.text=NULL;
    w->anchor.constraint=constraint;

    return w;
}


