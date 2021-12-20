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
    if(w->anchor.constraint)
    {
        if(check_widget_double_clicked(w))
        {
            toggle_resize_constraint_maximize(w->anchor.constraint);
            set_currently_active_widget(NULL);
        }
        else
        {
            if(w->anchor.constraint->resize_constraint.constrained)
            {
                adjust_coordinates_to_widget_local(w->anchor.constraint->resize_constraint.constrained,&x,&y);
                w->anchor.x_clicked=x;
                w->anchor.y_clicked=y;
            }
        }
    }
}

static void anchor_widget_mouse_movement(overlay_theme * theme,widget * w,int x,int y)
{
    widget *ci,*co;

    if(w->anchor.constraint)
    {
        co=w->anchor.constraint;
        ci=co->resize_constraint.constrained;

        adjust_coordinates_to_widget_local(co->base.parent,&x,&y);

        if(ci)
        {
            x-=w->anchor.x_clicked+ci->base.r.x1;
            if(ci->base.r.x1+x < co->base.r.x1)x=co->base.r.x1-ci->base.r.x1;
            if(ci->base.r.x2+x > co->base.r.x2)x=co->base.r.x2-ci->base.r.x2;

            y-=w->anchor.y_clicked+ci->base.r.y1;
            if(ci->base.r.y1+y < co->base.r.y1)y=co->base.r.y1-ci->base.r.y1;
            if(ci->base.r.y2+y > co->base.r.y2)y=co->base.r.y2-ci->base.r.y2;

            ci->base.r=rectangle_add_offset(ci->base.r,x,y);
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





static void text_anchor_widget_render(overlay_theme * theme,widget * w,int x_off,int y_off,cvm_overlay_element_render_buffer * erb,rectangle bounds)
{
	rectangle r=rectangle_add_offset(w->base.r,x_off,y_off);
	theme->h_bar_render(erb,theme,bounds,r,w->base.status,OVERLAY_ALTERNATE_MAIN_COLOUR_);

    r=overlay_simple_text_rectangle(r,theme->font_.glyph_size,theme->h_bar_text_offset);
    rectangle b=get_rectangle_overlap(r,bounds);
    if(rectangle_has_positive_area(b))overlay_text_single_line_render(erb,&theme->font_,b,w->anchor.text,r.x1,r.y1,OVERLAY_TEXT_COLOUR_0_);
}

static widget * text_anchor_widget_select(overlay_theme * theme,widget * w,int x_in,int y_in)
{
    if(theme->h_bar_select(theme,rectangle_subtract_offset(w->base.r,x_in,y_in),w->base.status))return w;

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


