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




static void popup_widget_render(overlay_theme * theme,widget * w,int16_t x_off,int16_t y_off,cvm_overlay_element_render_buffer * erb,rectangle bounds)
{
	render_widget(w->popup.contents,x_off+w->base.r.x1,y_off+w->base.r.y1,erb,bounds);
}

static widget * popup_widget_select(overlay_theme * theme,widget * w,int16_t x_in,int16_t y_in)
{
	return select_widget(w->popup.contents,x_in-w->base.r.x1,y_in-w->base.r.y1);
}

static void popup_widget_min_w(overlay_theme * theme,widget * w)
{
    w->base.min_w=set_widget_minimum_width(w->popup.contents,0);

    ///if WIDGET_POSITIONING_DOWN   if external_aligned.min_w is bigger than internal_aligned.min_w set increase contents.min_w by difference ???? (also w.min_w)
}

static void popup_widget_min_h(overlay_theme * theme,widget * w)
{
    w->base.min_h=set_widget_minimum_height(w->popup.contents,0);
}

static void popup_widget_set_w(overlay_theme * theme,widget * w)
{
    organise_widget_horizontally(w->popup.contents,0,w->base.min_w);///fix x_pos in set_h because it is always called second
}

static void popup_widget_set_h(overlay_theme * theme,widget * w)
{
//    organise_widget_vertically(w->popup.contents,0,w->base.min_h);
//
//    widget * c=w->popup.contents;
//
//    if((c)&&(w->popup.internal_alignment_widget)&&(w->popup.external_alignment_widget))
//    {
//        int xi=0;
//        int yi=0;
//        int xe=0;
//        int ye=0;
//
//        get_widgets_global_coordinates(w->popup.external_alignment_widget,&xe,&ye);
//        get_widgets_global_coordinates(w->popup.internal_alignment_widget,&xi,&yi);
//
//        c->base.r.x+=xe-xi;
//        c->base.r.y+=ye-yi;
//
//        if((w->popup.positioning==WIDGET_POSITIONING_CENTRED)||(w->popup.positioning==WIDGET_POSITIONING_CENTRED_DOWN))
//            c->base.r.x+=(w->popup.external_alignment_widget->base.r.w - w->popup.internal_alignment_widget->base.r.w)/2;
//
//        if(w->popup.positioning==WIDGET_POSITIONING_CENTRED)
//            c->base.r.y+=(w->popup.external_alignment_widget->base.r.h - w->popup.internal_alignment_widget->base.r.h)/2;
//
//        if(w->popup.positioning==WIDGET_POSITIONING_OFFSET_RIGHT)
//            c->base.r.x+=w->popup.external_alignment_widget->base.r.w + theme->popup_separation;
//
//        if(w->popup.positioning==WIDGET_POSITIONING_OFFSET_DOWN)
//            c->base.r.y+=w->popup.external_alignment_widget->base.r.h + theme->popup_separation;
//
//        if((c->base.r.x+c->base.r.w)>w->base.r.w)c->base.r.x=w->base.r.w-c->base.r.w;
//        if((c->base.r.y+c->base.r.h)>w->base.r.h)c->base.r.y=w->base.r.h-c->base.r.h;
//
//        if(c->base.r.x<0)c->base.r.x=0;
//        if(c->base.r.y<0)c->base.r.y=0;
//    }
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
    widget * w=create_widget();

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
    popup->base.status^=WIDGET_ACTIVE;
    if(widget_active(popup))
    {
        organise_toplevel_widget(popup);
        move_toplevel_widget_to_front(popup);
        set_only_interactable_widget(popup);
    }
    else set_only_interactable_widget(NULL);
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


