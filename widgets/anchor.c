/**
Copyright 2020,2021,2022,2024 Carl van Mastrigt

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

static void anchor_widget_left_click(overlay_theme * theme, widget * w, int x, int y, bool double_clicked)
{
    if(w->anchor.constraint)
    {
        if(double_clicked)
        {
            toggle_resize_constraint_maximize(w->anchor.constraint);
            set_currently_active_widget(w->base.context, NULL);
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





static void text_anchor_widget_render(overlay_theme * theme, widget * w, int16_t x_off, int16_t y_off, struct cvm_overlay_render_batch * restrict render_batch, rectangle bounds)
{
	rectangle r=rectangle_add_offset(w->base.r, x_off, y_off);
	theme->h_bar_render(render_batch, theme, bounds, r, w->base.status, OVERLAY_ALTERNATE_MAIN_COLOUR);

	overlay_text_single_line_render_data text_render_data=
	{
	    .flags=0,
	    .x=r.x1+theme->h_bar_text_offset,
	    .y=(r.y1+r.y2-theme->font.glyph_size)>>1,
	    .text=w->anchor.text,
	    .bounds=bounds,
	    .colour=OVERLAY_TEXT_COLOUR_0
	};

    overlay_text_single_line_render(render_batch, theme, &text_render_data);
}

static widget * text_anchor_widget_select(overlay_theme * theme,widget * w,int16_t x_in,int16_t y_in)
{
    if(theme->h_bar_select(theme,rectangle_subtract_offset(w->base.r,x_in,y_in),w->base.status))return w;

    return NULL;
}

static void text_anchor_widget_min_w(overlay_theme * theme,widget * w)
{
	w->base.min_w = overlay_text_single_line_get_pixel_length(&theme->font,w->anchor.text)+2*theme->h_bar_text_offset;
}

static void text_anchor_widget_min_h(overlay_theme * theme,widget * w)
{
	w->base.min_h = theme->base_unit_h;
}





static widget_appearence_function_set text_anchor_appearence_functions=
{
    .render =   text_anchor_widget_render,
    .select =   text_anchor_widget_select,
    .min_w  =   text_anchor_widget_min_w,
    .min_h  =   text_anchor_widget_min_h,
    .set_w  =   blank_widget_set_w,
    .set_h  =   blank_widget_set_h
};

void widget_text_anchor_initialise(widget_anchor* anchor, struct widget_context* context, widget* constraint, char* title)
{
    widget_base_initialise(&anchor->base, context, &text_anchor_appearence_functions, &anchor_behaviour_functions);

    anchor->x_clicked=0;
    anchor->y_clicked=0;
    anchor->constraint=constraint;

    if(title)anchor->text=cvm_strdup(title);
    else anchor->text=NULL;
}

widget * create_anchor(struct widget_context* context, widget* constraint, char* title)
{
    widget * w = malloc(sizeof(widget_anchor));
    
    widget_text_anchor_initialise(&w->anchor, context, constraint, title);

    return w;
}


