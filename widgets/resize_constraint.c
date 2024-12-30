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


static void resize_constraint_widget_add_child(widget * w,widget * child)
{
    /// could set w,h,x,y (of appropriate widgets) to 0 here
    w->resize_constraint.constrained=child;
    child->base.parent=w;
}

static void resize_constraint_widget_remove_child(widget * w,widget * child)
{
    w->resize_constraint.constrained=NULL;
    child->base.parent=NULL;
}

static void resize_constraint_widget_delete(widget * w)
{
    if(w->resize_constraint.constrained)
    {
        delete_widget(w->resize_constraint.constrained);
    }
}

static void resize_constraint_widget_left_click(overlay_theme * theme, widget * w, int x, int y, bool double_clicked)
{
    if((w->resize_constraint.alignment_data&WIDGET_RESIZABLE_LOCKED_HORIZONTAL)&&(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_LOCKED_VERTICAL))return;

    widget * constrained=w->resize_constraint.constrained;

    adjust_coordinates_to_widget_local(w,&x,&y);

    w->resize_constraint.x_clicked=0;
    if(x > (constrained->base.r.x1+constrained->base.r.x2)/2)
    {
        if((x > constrained->base.r.x2-theme->border_resize_selection_range)&&(!(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_TOUCHING_RIGHT)))
        {
            w->resize_constraint.x_clicked=x-constrained->base.r.x2;///negative (far end)
        }
    }
    else
    {
        if((x < constrained->base.r.x1+theme->border_resize_selection_range)&&(!(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_TOUCHING_LEFT)))
        {
            w->resize_constraint.x_clicked=x-constrained->base.r.x1+1;///positive (close end) (pos add 1 to make nonzero at edge)
        }
    }

    w->resize_constraint.y_clicked=0;
    if(y > (constrained->base.r.y1+constrained->base.r.y2)/2)
    {
        if((y > constrained->base.r.y2-theme->border_resize_selection_range)&&(!(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_TOUCHING_BOTTOM)))
        {
            w->resize_constraint.y_clicked=y-constrained->base.r.y2;///negative (far end)
        }
    }
    else
    {
        if((y < constrained->base.r.y1+theme->border_resize_selection_range)&&(!(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_TOUCHING_TOP)))
        {
            w->resize_constraint.y_clicked=y-constrained->base.r.y1+1;///positive (close end) (poss add 1 to make nonzero at edge)
        }
    }
}

static void resize_constraint_widget_mouse_movement(overlay_theme * theme,widget * w,int x,int y)
{
    if((w->resize_constraint.alignment_data&WIDGET_RESIZABLE_LOCKED_HORIZONTAL)&&(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_LOCKED_VERTICAL))return;

    widget * constrained=w->resize_constraint.constrained;
    int n;

    adjust_coordinates_to_widget_local(w,&x,&y);

    if((w->resize_constraint.x_clicked)&&(!(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_LOCKED_HORIZONTAL)))
    {
        n=x-w->resize_constraint.x_clicked;
        if(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_CENTRED_HORIZONTAL)
        {
            if(w->resize_constraint.x_clicked>0) w->resize_constraint.resized_w=w->base.r.x2-w->base.r.x1-2*n;
            else w->resize_constraint.resized_w=2*n-w->base.r.x2+w->base.r.x1;
        }
        else if(w->resize_constraint.x_clicked<0)
        {
            if(n>w->base.r.x2-w->base.r.x1)n=w->base.r.x2-w->base.r.x1;
            w->resize_constraint.resized_w = n-constrained->base.r.x1;
        }
        else
        {
            if(n<0)n=0;
            if(n>constrained->base.r.x2-constrained->base.min_w)n=constrained->base.r.x2-constrained->base.min_w;
            w->resize_constraint.resized_w = constrained->base.r.x2-n;
            constrained->base.r.x1=n;
        }
    }

    if((w->resize_constraint.y_clicked)&&(!(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_LOCKED_HORIZONTAL)))
    {
        n=y-w->resize_constraint.y_clicked;
        if(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_CENTRED_HORIZONTAL)
        {
            if(w->resize_constraint.y_clicked>0) w->resize_constraint.resized_h=w->base.r.y2-w->base.r.y1-2*n;
            else w->resize_constraint.resized_h=2*n-w->base.r.y2+w->base.r.y1;
        }
        else if(w->resize_constraint.y_clicked<0)
        {
            if(n>w->base.r.y2-w->base.r.y1)n=w->base.r.y2-w->base.r.y1;
            w->resize_constraint.resized_h = n-constrained->base.r.y1;
        }
        else
        {
            if(n<0)n=0;
            if(n>constrained->base.r.y2-constrained->base.min_h)n=constrained->base.r.y2-constrained->base.min_h;
            w->resize_constraint.resized_h = constrained->base.r.y2-n;
            constrained->base.r.y1=n;
        }
    }

    organise_toplevel_widget(w);
}


static widget_behaviour_function_set resize_constraint_behaviour_functions=
{
    .l_click        =   resize_constraint_widget_left_click,
    .l_release      =   blank_widget_left_release,
    .r_click        =   blank_widget_right_click,
    .m_move         =   resize_constraint_widget_mouse_movement,
    .scroll         =   blank_widget_scroll,
    .key_down       =   blank_widget_key_down,
    .text_input     =   blank_widget_text_input,
    .text_edit      =   blank_widget_text_edit,
    .click_away     =   blank_widget_click_away,
    .add_child      =   resize_constraint_widget_add_child,
    .remove_child   =   resize_constraint_widget_remove_child,
    .wid_delete     =   resize_constraint_widget_delete
};


static void resize_constraint_widget_render(overlay_theme * theme,widget * w,int16_t x_off,int16_t y_off,struct cvm_overlay_render_batch * restrict render_batch,rectangle bounds)
{
    if(w->resize_constraint.constrained) render_widget(w->resize_constraint.constrained,  theme, x_off+w->base.r.x1, y_off+w->base.r.y1, render_batch, bounds);
}

static widget * resize_constraint_widget_select(overlay_theme * theme,widget * w,int16_t x_in,int16_t y_in)
{
    widget * tmp=NULL;

    if(w->resize_constraint.constrained)
    {
        x_in-=w->base.r.x1;
        y_in-=w->base.r.y1;

        tmp = select_widget(w->resize_constraint.constrained, theme, x_in, y_in);

        if(tmp == w->resize_constraint.constrained)
        {
            if((x_in < tmp->base.r.x1+theme->border_resize_selection_range)&&(!(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_TOUCHING_LEFT)))return w;
            if((x_in > tmp->base.r.x2-theme->border_resize_selection_range)&&(!(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_TOUCHING_RIGHT)))return w;

            if((y_in < tmp->base.r.y1+theme->border_resize_selection_range)&&(!(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_TOUCHING_TOP)))return w;
            if((y_in > tmp->base.r.y2-theme->border_resize_selection_range)&&(!(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_TOUCHING_BOTTOM)))return w;
        }
    }

    return tmp;
}

static void resize_constraint_widget_min_w(overlay_theme * theme,widget * w)
{
    w->base.min_w=0;

    if(w->resize_constraint.constrained)
    {
        uint32_t alignment=0;

        if((w->base.status&WIDGET_H_FIRST)&&(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_TOUCHING_LEFT))
        {
            alignment|=WIDGET_H_FIRST;
        }
        if((w->base.status&WIDGET_H_LAST)&&(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_TOUCHING_RIGHT))
        {
            alignment|=WIDGET_H_LAST;
        }

        w->base.min_w=set_widget_minimum_width(w->resize_constraint.constrained, theme, alignment);
    }
}

static void resize_constraint_widget_min_h(overlay_theme * theme,widget * w)
{
    w->base.min_h=0;

    if(w->resize_constraint.constrained)
    {
        uint32_t alignment=0;

        if((w->base.status&WIDGET_V_FIRST)&&(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_TOUCHING_TOP))alignment|=WIDGET_V_FIRST;
        if((w->base.status&WIDGET_V_LAST)&&(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_TOUCHING_BOTTOM))alignment|=WIDGET_V_LAST;

        w->base.min_h=set_widget_minimum_height(w->resize_constraint.constrained, theme, alignment);
    }
}

static void resize_constraint_widget_set_w(overlay_theme * theme,widget * w)
{
    int width,x_pos;

    if(w->resize_constraint.constrained)
	{
	    width=w->resize_constraint.resized_w;
        if(width < w->resize_constraint.constrained->base.min_w) width=w->resize_constraint.constrained->base.min_w;
        if(width > w->base.r.x2-w->base.r.x1 || ((w->resize_constraint.alignment_data & WIDGET_RESIZABLE_TOUCHING_LEFT)&&(w->resize_constraint.alignment_data & WIDGET_RESIZABLE_TOUCHING_RIGHT)))
            width=w->base.r.x2-w->base.r.x1;

        if(w->resize_constraint.alignment_data & WIDGET_RESIZABLE_TOUCHING_LEFT)x_pos=0;
        else if(w->resize_constraint.alignment_data & WIDGET_RESIZABLE_TOUCHING_RIGHT)x_pos=w->base.r.x2-w->base.r.x1-width;
        else if((w->resize_constraint.alignment_data & WIDGET_RESIZABLE_CENTRED_HORIZONTAL)||(w->resize_constraint.initially_centred))x_pos=(w->base.r.x2-w->base.r.x1-width)/2;
        else x_pos=w->resize_constraint.constrained->base.r.x1;

        if(x_pos+width > w->base.r.x2-w->base.r.x1)x_pos=w->base.r.x2-w->base.r.x1-width;
	    if(x_pos<0)x_pos=0;

	    organise_widget_horizontally(w->resize_constraint.constrained, theme, x_pos, width);
	}
}

static void resize_constraint_widget_set_h(overlay_theme * theme,widget * w)
{
    int height,y_pos;

    if(w->resize_constraint.constrained)
	{
        height=w->resize_constraint.resized_h;
        if(height < w->resize_constraint.constrained->base.min_h) height=w->resize_constraint.constrained->base.min_h;
        if(height > w->base.r.y2-w->base.r.y1 || ((w->resize_constraint.alignment_data & WIDGET_RESIZABLE_TOUCHING_TOP)&&(w->resize_constraint.alignment_data & WIDGET_RESIZABLE_TOUCHING_BOTTOM)))
            height=w->base.r.y2-w->base.r.y1;

        if(w->resize_constraint.alignment_data & WIDGET_RESIZABLE_TOUCHING_TOP)y_pos=0;
        else if(w->resize_constraint.alignment_data & WIDGET_RESIZABLE_TOUCHING_BOTTOM)y_pos=w->base.r.y2-w->base.r.y1-height;
        else if((w->resize_constraint.alignment_data & WIDGET_RESIZABLE_CENTRED_VERTICAL)||(w->resize_constraint.initially_centred))y_pos=(w->base.r.y2-w->base.r.y1-height)/2;
        else y_pos=w->resize_constraint.constrained->base.r.y1;

        if(y_pos+height > w->base.r.y2-w->base.r.y1)y_pos=w->base.r.y2-w->base.r.y1-height;
	    if(y_pos<0)y_pos=0;

	    organise_widget_vertically(w->resize_constraint.constrained, theme, y_pos, height);
	}

	if(!w->resize_constraint.maximized)w->resize_constraint.initially_centred=false;
}


static widget_appearence_function_set resize_constraint_appearence_functions=
{
    .render =   resize_constraint_widget_render,
    .select =   resize_constraint_widget_select,
    .min_w  =   resize_constraint_widget_min_w,
    .min_h  =   resize_constraint_widget_min_h,
    .set_w  =   resize_constraint_widget_set_w,
    .set_h  =   resize_constraint_widget_set_h
};


widget * create_resize_constraint(struct widget_context* context, uint16_t alignment_data,bool maximizable)
{
    widget * w=create_widget(context, sizeof(widget_resize_constraint));

    w->base.appearence_functions=&resize_constraint_appearence_functions;
    w->base.behaviour_functions=&resize_constraint_behaviour_functions;

    if((alignment_data&WIDGET_RESIZABLE_TOUCHING_LEFT)||(alignment_data&WIDGET_RESIZABLE_TOUCHING_RIGHT))alignment_data&= ~WIDGET_RESIZABLE_CENTRED_HORIZONTAL;
    if((alignment_data&WIDGET_RESIZABLE_TOUCHING_TOP)||(alignment_data&WIDGET_RESIZABLE_TOUCHING_BOTTOM))alignment_data&= ~WIDGET_RESIZABLE_CENTRED_VERTICAL;

    w->resize_constraint.alignment_data=alignment_data;

    w->resize_constraint.constrained=NULL;

    w->resize_constraint.alt_x_pos=0;
    w->resize_constraint.alt_y_pos=0;
    w->resize_constraint.resized_w=0;
    w->resize_constraint.resized_h=0;
    w->resize_constraint.x_clicked=0;
    w->resize_constraint.y_clicked=0;

    w->resize_constraint.maximizable=maximizable;
    w->resize_constraint.maximized=false;
    w->resize_constraint.initially_centred=true;

    return w;
}


void toggle_resize_constraint_maximize(widget * w)
{
    if(w->resize_constraint.maximizable)
    {
        w->resize_constraint.maximized^=true;

        if(w->resize_constraint.maximized)
        {
            w->resize_constraint.alignment_data<<=8;
            w->resize_constraint.alignment_data|=WIDGET_RESIZABLE_FILL_AREA;

            w->resize_constraint.alt_x_pos=w->resize_constraint.constrained->base.r.x1;
            w->resize_constraint.alt_y_pos=w->resize_constraint.constrained->base.r.y1;
        }
        else
        {
            w->resize_constraint.alignment_data>>=8;

            w->resize_constraint.constrained->base.r.x1=w->resize_constraint.alt_x_pos;
            w->resize_constraint.constrained->base.r.y1=w->resize_constraint.alt_y_pos;
        }

        organise_toplevel_widget(w);
    }
}


