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

static widget * old_auto_close_popup=NULL;
static widget * new_auto_close_popup=NULL;




static void popup_widget_render(overlay_theme * theme,widget * w,int16_t x_off,int16_t y_off,struct cvm_overlay_render_batch * restrict render_batch,rectangle bounds)
{
	render_widget(w->popup.contents, theme, x_off + w->base.r.x1, y_off + w->base.r.y1, render_batch, bounds);
}

static widget * popup_widget_select(overlay_theme * theme,widget * w,int16_t x_in,int16_t y_in)
{
	return select_widget(w->popup.contents, theme, x_in - w->base.r.x1 , y_in - w->base.r.y1);
}

static void popup_widget_min_w(overlay_theme * theme,widget * w)
{
    w->base.min_w=set_widget_minimum_width(w->popup.contents, theme, 0);
}

static void popup_widget_min_h(overlay_theme * theme,widget * w)
{
    w->base.min_h=set_widget_minimum_height(w->popup.contents, theme, 0);
}

static void popup_widget_set_w(overlay_theme * theme,widget * w)
{
    organise_widget_horizontally(w->popup.contents, theme, 0, w->base.min_w);
}

static void popup_widget_set_h(overlay_theme * theme,widget * w)
{
    /// all this is done in set_w because that should happen after the aligned widget has been assessed
    #warning better to make alignment a post processing step affecting only toplevel widgets!
    #warning make int16_t
    int delta_x,delta_y,width,height,max;
    organise_widget_vertically(w->popup.contents, theme, 0, w->base.min_h);

    widget *contained,*external,*internal;
    contained=w->popup.contents;
    internal=w->popup.internal_alignment_widget;
    external=w->popup.external_alignment_widget;

    if(contained)
    {
        if(!internal)internal=contained;
        if(!external)external=w;
        ///allow internal not to exist (use contained) and external not to exist (use w)

        get_widgets_global_coordinates(external,&delta_x,&delta_y);
        adjust_coordinates_to_widget_local(internal,&delta_x,&delta_y);

        if(w->popup.positioning==WIDGET_POSITIONING_CENTRED)
        {
            delta_x+=((external->base.r.x2-external->base.r.x1) - (internal->base.r.x2-internal->base.r.x1))/2;
            delta_y+=((external->base.r.y2-external->base.r.y1) - (internal->base.r.y2-internal->base.r.y1))/2;
        }

        ///implement the others, can/should probably specify relative alignment separately for horizontal and vertical

        max=w->base.r.x2-w->base.r.x1 - contained->base.r.x2;///required to move contained.x2 to overlap w.x2
        if(delta_x > max)delta_x=max;
        if(delta_x < 0)delta_x=0;

        max=w->base.r.y2-w->base.r.y1 - contained->base.r.y2;///required to move contained.x2 to overlap w.x2
        if(delta_y > max)delta_y=max;
        if(delta_y < 0)delta_y=0;

        contained->base.r=rectangle_add_offset(contained->base.r, delta_x, delta_y);
    }
}





static widget_appearence_function_set popup_appearence_functions=
{
    .render =   popup_widget_render,
    .select =   popup_widget_select,
    .min_w  =   popup_widget_min_w,
    .min_h  =   popup_widget_min_h,
    .set_w  =   popup_widget_set_w,
    .set_h  =   popup_widget_set_h
};








static void popup_widget_add_child(widget * w,widget * child)
{
	w->popup.contents=child;
    child->base.parent=w;
}

static void popup_widget_remove_child(widget * w,widget * child)
{
	w->popup.contents=NULL;
    child->base.parent=NULL;
}

static void popup_widget_delete(widget * w)
{
    if(w==old_auto_close_popup)old_auto_close_popup=NULL;

	if(w->popup.contents) delete_widget(w->popup.contents);
}

static widget_behaviour_function_set popup_behaviour_functions=
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
    .add_child      =   popup_widget_add_child,
    .remove_child   =   popup_widget_remove_child,
    .wid_delete     =   popup_widget_delete
};



widget * create_popup(widget_relative_positioning positioning,bool auto_close)
{
    widget * w=create_widget(sizeof(widget_popup));

    w->base.appearence_functions=&popup_appearence_functions;
    w->base.behaviour_functions=&popup_behaviour_functions;

    w->base.status&= ~WIDGET_ACTIVE;

    w->popup.contents=NULL;
    w->popup.external_alignment_widget=NULL;
    w->popup.internal_alignment_widget=NULL;
    w->popup.trigger_widget=NULL;

    w->popup.positioning=positioning;

    if(auto_close)w->base.status|=WIDGET_IS_AUTO_CLOSE_POPUP;

    return w;
}

void set_popup_alignment_widgets(widget * popup,widget * internal_alignment_widget,widget * external_alignment_widget)
{
    popup->popup.external_alignment_widget=external_alignment_widget;
    popup->popup.internal_alignment_widget=internal_alignment_widget;
}

void set_popup_trigger_widget(widget * popup,widget * trigger_widget)
{
    popup->popup.trigger_widget=trigger_widget;
}

void toggle_exclusive_popup(widget * popup)
{
    widget* root_widget = find_root_widget(popup);
    assert(root_widget);

    popup->base.status^=WIDGET_ACTIVE;
    if(widget_active(popup))
    {
        organise_toplevel_widget(popup);
        move_toplevel_widget_to_front(popup);
        set_only_interactable_widget(root_widget, popup);
    }
    else
    {
        set_only_interactable_widget(root_widget, NULL);
    }
}




void toggle_auto_close_popup(widget * popup)
{
    if(!widget_active(popup))
    {
        new_auto_close_popup=popup;
        move_toplevel_widget_to_front(popup);
    }
}


bool close_auto_close_popup_tree(widget * interacted)
{
    widget * w;
    bool any_closed=false;

    w=old_auto_close_popup;

    while(w)
    {
        if(w->base.status & WIDGET_IS_AUTO_CLOSE_POPUP)
        {
            if(!widget_active(w))puts("ERROR INACTIVE POPUP IN CLOSE CHAIN");

            w->base.status&= ~WIDGET_ACTIVE;

            any_closed=true;

            w=w->popup.trigger_widget;
            continue;
        }

        w=w->base.parent;
    }


    if(new_auto_close_popup) w=new_auto_close_popup;
    else if((interacted)&&(!(interacted->base.status&WIDGET_CLOSE_POPUP_TREE))) w=interacted;
    else w=NULL;

    while(w)
    {
        if(w->base.status & WIDGET_IS_AUTO_CLOSE_POPUP)
        {
            if(new_auto_close_popup==NULL)new_auto_close_popup=w;

            if(widget_active(w))puts("ERROR ACTIVE POPUP IN OPEN CHAIN");

            w->base.status|=WIDGET_ACTIVE;

            organise_toplevel_widget(w);

            w=w->popup.trigger_widget;
            continue;
        }

        w=w->base.parent;
    }

    old_auto_close_popup=new_auto_close_popup;
    new_auto_close_popup=NULL;

    return any_closed;
}


