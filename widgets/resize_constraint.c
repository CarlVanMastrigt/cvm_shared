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


void resize_constraint_widget_add_child(widget * w,widget * child)
{
    /// could set w,h,x,y (of appropriate widgets) to 0 here
    w->resize_constraint.constrained=child;
    child->base.parent=w;
}

void resize_constraint_widget_remove_child(widget * w,widget * child)
{
    w->resize_constraint.constrained=NULL;
    child->base.parent=NULL;
}

void resize_constraint_widget_delete(widget * w)
{
    if(w->resize_constraint.constrained)
    {
        delete_widget(w->resize_constraint.constrained);
    }
}



static void resize_constraint_widget_left_click(overlay_theme * theme,widget * w,int x,int y)
{
    if((w->resize_constraint.alignment_data&WIDGET_RESIZABLE_LOCKED_HORIZONTAL)&&(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_LOCKED_VERTICAL))return;

    widget * constrained=w->resize_constraint.constrained;

    adjust_coordinates_to_widget_local(constrained,&x,&y);

    w->resize_constraint.x_clicked=0;
    w->resize_constraint.x_far_corner=0;
    if(x > constrained->base.r.w/2)
    {
        if((x > (constrained->base.r.w-theme->border_resize_selection_range))&&(!(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_TOUCHING_RIGHT)))
        {
            w->resize_constraint.x_clicked=x-constrained->base.r.w;///negative (far end)
        }
    }
    else if((x < theme->border_resize_selection_range)&&(!(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_TOUCHING_LEFT)))
    {
        w->resize_constraint.x_clicked=x;///positive (close end) (poss add 1 to make nonzero at edge)
        w->resize_constraint.x_far_corner=constrained->base.r.x+constrained->base.r.w;
    }

    w->resize_constraint.y_clicked=0;
    w->resize_constraint.y_far_corner=0;
    if(y > constrained->base.r.h/2)
    {
        if((y > (constrained->base.r.h-theme->border_resize_selection_range))&&(!(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_TOUCHING_BOTTOM)))
        {
            w->resize_constraint.y_clicked=y-constrained->base.r.h;///negative (far end)
        }
    }
    else if((y < theme->border_resize_selection_range)&&(!(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_TOUCHING_TOP)))
    {
        w->resize_constraint.y_clicked=y;///positive (close end) (poss add 1 to make nonzero at edge)
        w->resize_constraint.y_far_corner=constrained->base.r.y+constrained->base.r.h;
    }
}

static void resize_constraint_widget_mouse_movement(overlay_theme * theme,widget * w,int x,int y)
{
    if((w->resize_constraint.alignment_data&WIDGET_RESIZABLE_LOCKED_HORIZONTAL)&&(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_LOCKED_VERTICAL))return;

    widget * constrained=w->resize_constraint.constrained;

    adjust_coordinates_to_widget_local(w,&x,&y);

    if((w->resize_constraint.x_clicked)&&(!(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_LOCKED_HORIZONTAL)))
    {
        if(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_CENTRED_HORIZONTAL)
        {
            if(w->resize_constraint.x_clicked>0)
            {
                w->resize_constraint.resized_w=w->base.r.w-2*(x-w->resize_constraint.x_clicked);
            }
            else
            {
                w->resize_constraint.resized_w=2*(x-w->resize_constraint.x_clicked)-w->base.r.w;
            }
        }
        else if(w->resize_constraint.x_clicked<0)
        {
            w->resize_constraint.resized_w=x - constrained->base.r.x - w->resize_constraint.x_clicked;
            if(w->resize_constraint.resized_w > w->base.r.w-constrained->base.r.x)w->resize_constraint.resized_w = w->base.r.w-constrained->base.r.x;
        }
        else
        {
            constrained->base.r.x=x-w->resize_constraint.x_clicked;
            if(constrained->base.r.x < 0)constrained->base.r.x=0;
            if(constrained->base.r.x > w->resize_constraint.x_far_corner-constrained->base.min_w)constrained->base.r.x=w->resize_constraint.x_far_corner-constrained->base.min_w;
            w->resize_constraint.resized_w=w->resize_constraint.x_far_corner-constrained->base.r.x;
        }
    }

    if((w->resize_constraint.y_clicked)&&(!(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_LOCKED_VERTICAL)))
    {
        if(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_CENTRED_VERTICAL)
        {
            if(w->resize_constraint.y_clicked>0)
            {
                w->resize_constraint.resized_h=w->base.r.h-2*(y-w->resize_constraint.y_clicked);
            }
            else
            {
                w->resize_constraint.resized_h=2*(y-w->resize_constraint.y_clicked)-w->base.r.h;
            }
        }
        else if(w->resize_constraint.y_clicked<0)
        {
            w->resize_constraint.resized_h=y - constrained->base.r.y - w->resize_constraint.y_clicked;
            if(w->resize_constraint.resized_h > w->base.r.h-constrained->base.r.y)w->resize_constraint.resized_h = w->base.r.h-constrained->base.r.y;
        }
        else
        {
            constrained->base.r.y=y-w->resize_constraint.y_clicked;
            if(constrained->base.r.y < 0)constrained->base.r.y=0;
            if(constrained->base.r.y > w->resize_constraint.y_far_corner-constrained->base.min_h)constrained->base.r.y=w->resize_constraint.y_far_corner-constrained->base.min_h;
            w->resize_constraint.resized_h=w->resize_constraint.y_far_corner-constrained->base.r.y;
        }
    }


    organise_toplevel_widget(w);
}


static widget_behaviour_function_set resize_constraint_behaviour_functions=
(widget_behaviour_function_set)
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




void resize_constraint_widget_render(overlay_data * od,overlay_theme * theme,widget * w,int x_off,int y_off,rectangle bounds)
{
    if(w->resize_constraint.constrained) render_widget(od,w->resize_constraint.constrained,x_off+w->base.r.x,y_off+w->base.r.y,bounds);
}

widget * resize_constraint_widget_select(overlay_theme * theme,widget * w,int x_in,int y_in)
{
    widget * tmp=NULL;

    if(w->resize_constraint.constrained)
    {
        tmp = select_widget(w->resize_constraint.constrained,x_in-w->base.r.x,y_in-w->base.r.y);

        if(tmp == w->resize_constraint.constrained)
        {
            x_in-=tmp->base.r.x+w->base.r.x;
            y_in-=tmp->base.r.y+w->base.r.y;

            if((x_in < theme->border_resize_selection_range)&&(!(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_TOUCHING_LEFT)))return w;
            if((x_in > tmp->base.r.w-theme->border_resize_selection_range)&&(!(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_TOUCHING_RIGHT)))return w;

            if((y_in < theme->border_resize_selection_range)&&(!(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_TOUCHING_TOP)))return w;
            if((y_in > tmp->base.r.h-theme->border_resize_selection_range)&&(!(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_TOUCHING_BOTTOM)))return w;
        }
    }

    return tmp;
}






void resize_constraint_widget_min_w(overlay_theme * theme,widget * w)
{
    w->base.min_w=0;

    if(w->resize_constraint.constrained)
    {
        uint32_t alignment=0;

        if((w->base.status&WIDGET_H_FIRST)&&(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_TOUCHING_LEFT))alignment|=WIDGET_H_FIRST;
        if((w->base.status&WIDGET_H_LAST)&&(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_TOUCHING_RIGHT))alignment|=WIDGET_H_LAST;

        w->base.min_w=set_widget_minimum_width(w->resize_constraint.constrained,alignment);
    }
}

void resize_constraint_widget_min_h(overlay_theme * theme,widget * w)
{
    w->base.min_h=0;

    if(w->resize_constraint.constrained)
    {
        uint32_t alignment=0;

        if((w->base.status&WIDGET_V_FIRST)&&(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_TOUCHING_TOP))alignment|=WIDGET_V_FIRST;
        if((w->base.status&WIDGET_V_LAST)&&(w->resize_constraint.alignment_data&WIDGET_RESIZABLE_TOUCHING_BOTTOM))alignment|=WIDGET_V_LAST;

        w->base.min_h=set_widget_minimum_height(w->resize_constraint.constrained,alignment);
    }
}

void resize_constraint_widget_set_w(overlay_theme * theme,widget * w)
{
    int width,x_pos;

    if(w->resize_constraint.constrained)
	{
	    if(w->resize_constraint.resized_w > w->base.r.w)w->resize_constraint.resized_w = w->base.r.w;

	    if((w->resize_constraint.alignment_data & WIDGET_RESIZABLE_TOUCHING_LEFT)&&(w->resize_constraint.alignment_data & WIDGET_RESIZABLE_TOUCHING_RIGHT))width=w->base.r.w;
	    else width=w->resize_constraint.resized_w;

	    if(width < w->base.min_w)width=w->base.min_w;///min_w same as constrained

        if(w->resize_constraint.alignment_data & WIDGET_RESIZABLE_TOUCHING_LEFT)x_pos=0;
        else if(w->resize_constraint.alignment_data & WIDGET_RESIZABLE_TOUCHING_RIGHT)x_pos=w->base.r.w-width;
        else if((w->resize_constraint.alignment_data & WIDGET_RESIZABLE_CENTRED_HORIZONTAL)||(w->resize_constraint.initially_centred))x_pos=(w->base.r.w-width)/2;
	    else x_pos=w->resize_constraint.constrained->base.r.x;

	    if((x_pos+width) > w->base.r.w)x_pos=w->base.r.w-width;
	    if(x_pos<0)x_pos=0;

	    organise_widget_horizontally(w->resize_constraint.constrained,x_pos,width);
	}
}

void resize_constraint_widget_set_h(overlay_theme * theme,widget * w)
{
    int height,y_pos;

    if(w->resize_constraint.constrained)
	{
	    if(w->resize_constraint.resized_h > w->base.r.h)w->resize_constraint.resized_h=w->base.r.h;

	    if((w->resize_constraint.alignment_data & WIDGET_RESIZABLE_TOUCHING_TOP)&&(w->resize_constraint.alignment_data & WIDGET_RESIZABLE_TOUCHING_BOTTOM))height=w->base.r.h;
	    else height=w->resize_constraint.resized_h;

	    if(height < w->base.min_h)height=w->base.min_h;///min_h same as constrained

	    if(w->resize_constraint.alignment_data & WIDGET_RESIZABLE_TOUCHING_TOP)y_pos=0;
        else if(w->resize_constraint.alignment_data & WIDGET_RESIZABLE_TOUCHING_BOTTOM)y_pos=w->base.r.h-height;
        else if((w->resize_constraint.alignment_data & WIDGET_RESIZABLE_CENTRED_VERTICAL)||(w->resize_constraint.initially_centred))y_pos=(w->base.r.h-height)/2;
	    else y_pos=w->resize_constraint.constrained->base.r.y;

	    if((y_pos+height) > w->base.r.h)y_pos=w->base.r.h-height;
	    if(y_pos<0)y_pos=0;

	    organise_widget_vertically(w->resize_constraint.constrained,y_pos,height);
	}

	if(!w->resize_constraint.maximized)w->resize_constraint.initially_centred=false;
}


static widget_appearence_function_set resize_constraint_appearence_functions=
(widget_appearence_function_set)
{
    .render =   resize_constraint_widget_render,
    .select =   resize_constraint_widget_select,
    .min_w  =   resize_constraint_widget_min_w,
    .min_h  =   resize_constraint_widget_min_h,
    .set_w  =   resize_constraint_widget_set_w,
    .set_h  =   resize_constraint_widget_set_h
};


widget * create_resize_constraint(uint16_t alignment_data,bool maximizable)
{
    widget * w=create_widget(RESIZE_CONSTRAINT_WIDGET);

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
    w->resize_constraint.x_far_corner=0;
    w->resize_constraint.y_far_corner=0;

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

            w->resize_constraint.alt_x_pos=w->resize_constraint.constrained->base.r.x;
            w->resize_constraint.alt_y_pos=w->resize_constraint.constrained->base.r.y;
        }
        else
        {
            w->resize_constraint.alignment_data>>=8;

            w->resize_constraint.constrained->base.r.x=w->resize_constraint.alt_x_pos;
            w->resize_constraint.constrained->base.r.y=w->resize_constraint.alt_y_pos;
        }

        organise_toplevel_widget(w);
    }
}


