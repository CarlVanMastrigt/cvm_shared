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


typedef struct widget_adjuster_pair_data
{
    widget * slider_bar;
    widget * enterbox;

    void * data;
    void(*func)(void*);
}
widget_adjuster_pair_data;






void adjuster_pair_slider_bar_function(widget * w)
{
    #warning perhaps instead allow enterbox to read/set its contents in render step when not active? (allows multiple enterboxes to represent same piece of data simultaneously)
    //set_enterbox_text_using_int(w->slider_bar.data,*w->slider_bar.value_ptr);
}

void adjuster_pair_enterbox_function(widget * w)
{
    /// could put contained data in some external structure, e.g. one to handle ints, one to handle floats?
    int r;
    if(sscanf(w->enterbox.text,"%d",&r)==1)
    {
        set_slider_bar_value(w->enterbox.data,r);
    }
}

void adjuster_pair_enterbox_update_contents_function(widget * w)
{
    widget * slider_bar=w->enterbox.data;

    char buffer[16];
    snprintf(buffer,16,"%d",*slider_bar->slider_bar.value_ptr);
    set_enterbox_text(w,buffer);
}

widget * create_adjuster_pair(struct widget_context* context, int * value_ptr,int min_value,int max_value,int text_space,int bar_fraction,int scroll_fraction)
{
    widget * box=create_box(context, WIDGET_HORIZONTAL, WIDGET_FIRST_DISTRIBUTED);


    char text[16];
    sprintf(text,"%d",*value_ptr);
    #warning revise adjuster_pair, does it have any real practical use, will probably always need extended functionality to what is provided, instead package above to be called by propper function
    /// specifically doesnt allow some specialised operation to be called upon update of value, may be useful in most cases though, just reading value externally whenever its needed/used

    widget * slider_bar=add_child_to_parent(box,create_slider_bar_fixed(context, value_ptr,min_value,max_value,bar_fraction,scroll_fraction,adjuster_pair_slider_bar_function,NULL,false));

	//slider_bar->slider_bar.data=add_child_to_parent(box,create_enterbox(text_space,text_space,text_space,text,adjuster_pair_enterbox_function,slider_bar,adjuster_pair_enterbox_update_contents_function,true,false));
	slider_bar->slider_bar.data=add_child_to_parent(box,create_enterbox_simple(context, text_space,text,adjuster_pair_enterbox_function,adjuster_pair_enterbox_update_contents_function,slider_bar,false,true));

    return box;
}









//void general_scroll_change(int * value,int min,int max,int magnitude,int delta)
//{
//    if(!magnitude)magnitude=abs(max - min)/16;
//    if(magnitude<0)magnitude=1;
//
//    *value+=magnitude*delta;
//
//    if(*value < min)*value=min;
//    if(*value > max)*value=max;
//}







void toggle_widget(widget * w)
{
    w->base.status^=WIDGET_ACTIVE;
    organise_toplevel_widget(w);
}

void toggle_widget_button_func(widget * w)
{
    toggle_widget(w->button.data);
}







static void empty_widget_render(overlay_theme * theme,widget * w,int16_t x_off,int16_t y_off,struct cvm_overlay_render_batch * restrict render_batch,rectangle bounds)
{

}


static widget_appearence_function_set empty_appearence_functions=
{
    .render =   empty_widget_render,
    .select =   blank_widget_select,
    .min_w  =   blank_widget_min_w,
    .min_h  =   blank_widget_min_h,
    .set_w  =   blank_widget_set_w,
    .set_h  =   blank_widget_set_h
};

widget * create_empty_widget(struct widget_context* context, int16_t min_w,int16_t min_h)
{
    widget * w = create_widget(context, sizeof(widget_base));///needs no extra data...

    w->base.appearence_functions=&empty_appearence_functions;

    w->base.min_w=min_w;
    w->base.min_h=min_h;

    return w;
}





static void separator_widget_min_w(overlay_theme * theme,widget * w)
{
	w->base.min_w=theme->separator_w;
}

static void separator_widget_min_h(overlay_theme * theme,widget * w)
{
	w->base.min_h=theme->separator_h;
}

static widget_appearence_function_set separator_appearence_functions=
{
    .render =   blank_widget_render,
    .select =   blank_widget_select,
    .min_w  =   separator_widget_min_w,
    .min_h  =   separator_widget_min_h,
    .set_w  =   blank_widget_set_w,
    .set_h  =   blank_widget_set_h
};

widget * create_separator_widget(struct widget_context* context)
{
    widget * w = create_widget(context, sizeof(widget_base));///needs no extra data, all behaviour comes from appearance functions and theme

    w->base.appearence_functions=&separator_appearence_functions;

    return w;
}



static void unit_separator_widget_min_w(overlay_theme * theme,widget * w)
{
	w->base.min_w=theme->base_unit_w;
}

static void unit_separator_widget_min_h(overlay_theme * theme,widget * w)
{
	w->base.min_h=theme->base_unit_h;
}

static widget_appearence_function_set unit_separator_appearence_functions=
{
    .render =   blank_widget_render,
    .select =   blank_widget_select,
    .min_w  =   unit_separator_widget_min_w,
    .min_h  =   unit_separator_widget_min_h,
    .set_w  =   blank_widget_set_w,
    .set_h  =   blank_widget_set_h
};

widget * create_unit_separator_widget(struct widget_context* context)
{
    widget * w = create_widget(context, sizeof(widget_base));///needs no extra data, all behaviour comes from appearance functions and theme

    w->base.appearence_functions=&unit_separator_appearence_functions;

    return w;
}






void window_toggle_button_func(widget * w)
{
    widget * rc;

    rc=w->button.data;
    rc->base.status^=WIDGET_ACTIVE;

    if(widget_active(rc))
    {
        organise_toplevel_widget(rc);
        move_toplevel_widget_to_front(rc);
    }
}


widget * create_window_widget(struct widget_context* context, widget ** box,char * title,bool resizable,widget_function custom_exit_function,void * custom_exit_data,bool free_custom_exit_data)
{
    widget *w,*sub_box;

    if(resizable) w=create_resize_constraint(context, WIDGET_RESIZABLE_FREE_MOVEMENT,true);
    else w=create_resize_constraint(context, WIDGET_RESIZABLE_LOCKED_HORIZONTAL|WIDGET_RESIZABLE_LOCKED_VERTICAL,false);

    *box=add_child_to_parent(add_child_to_parent(w,create_panel(context)),create_box(context, WIDGET_VERTICAL, WIDGET_LAST_DISTRIBUTED));

    sub_box=add_child_to_parent(*box,create_box(context, WIDGET_HORIZONTAL,WIDGET_FIRST_DISTRIBUTED));
    add_child_to_parent(sub_box,create_anchor(context, w, title));
    add_child_to_parent(sub_box,create_separator_widget(context));
    //add_child_to_parent(sub_box,create_unit_separator_widget());
    if(custom_exit_function)
    {
        add_child_to_parent(sub_box,create_icon_button(context, "🗙",custom_exit_data,free_custom_exit_data,custom_exit_function));
    }
    else
    {
        add_child_to_parent(sub_box,create_icon_button(context, "🗙",w,false,window_toggle_button_func));
    }

    add_child_to_parent(*box,create_separator_widget(context));

    return w;
}








void popup_button_toggle_func(widget * w)
{
    toggle_auto_close_popup(w->button.data);
}

///creates and returns button to toggle popup, also creates panel in popup in popup_container and adds panel_contents in the panel
widget * create_popup_panel_button(struct widget_context* context, widget* popup_container, widget* panel_contents, char* button_text)///input positioning????
{
    widget * popup=add_child_to_parent(popup_container,create_popup(context, WIDGET_POSITIONING_OFFSET_RIGHT,true));

    //widget * button=create_text_button(button_text,popup,popup_button_toggle_func);
    widget * button=create_text_highlight_toggle_button(context, button_text,popup,false,popup_button_toggle_func,button_widget_status_func);
    button->base.status&= ~WIDGET_CLOSE_POPUP_TREE;

    add_child_to_parent(add_child_to_parent(popup,create_panel(context)),panel_contents);

    set_popup_alignment_widgets(popup,panel_contents,button);
    set_popup_trigger_widget(popup,button);

    return button;
}




#warning probably want following as a single button, have special variant of text_icon h_bar button

widget * create_checkbox_button_pair(struct widget_context* context, char* text, void* data, widget_function func, widget_button_toggle_status_func toggle_status,bool free_data)
{
    widget * box=create_box(context, WIDGET_HORIZONTAL, WIDGET_FIRST_DISTRIBUTED);

    add_child_to_parent(box,create_text_button(context, text,data,false,func));
    add_child_to_parent(box,create_icon_toggle_button(context, "+", "-", data, free_data, func, toggle_status));

    return box;
}

widget * create_bool_checkbox_button_pair(struct widget_context* context, char* text, bool* bool_ptr)
{
    return create_checkbox_button_pair(context, text, bool_ptr, button_toggle_bool_func, button_bool_status_func, false);
}



widget * create_icon_collapse_button(struct widget_context* context, char* icon_collapse, char* icon_expand, widget* widget_to_control, bool collapse)
{
    widget * button=create_icon_toggle_button(context, icon_collapse, icon_expand, widget_to_control, false, toggle_widget_button_func, button_widget_status_func);

    if(collapse)toggle_widget(widget_to_control);

    return button;
}
