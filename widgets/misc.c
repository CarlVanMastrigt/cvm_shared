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


typedef struct widget_adjuster_pair_data
{
    widget * sliderbar;
    widget * enterbox;

    void * data;
    void(*func)(void*);
}
widget_adjuster_pair_data;






void adjuster_pair_sliderbar_function(widget * w)
{
    #warning perhaps instead allow enterbox to read/set its contents in render step when not active? (allows multiple enterboxes to represent same piece of data simultaneously)
    //set_enterbox_text_using_int(w->sliderbar.data,*w->sliderbar.value_ptr);
}

void adjuster_pair_enterbox_function(widget * w)
{
    /// could put contained data in some external structure, e.g. one to handle ints, one to handle floats?
    int r;
    if(sscanf(w->enterbox.text,"%d",&r))
    {
        set_sliderbar_value(w->enterbox.data,r);
    }
}

void adjuster_pair_enterbox_update_contents_function(widget * w)
{
    widget * sliderbar=w->enterbox.data;

    char buffer[16];
    snprintf(buffer,16,"%d",*sliderbar->sliderbar.value_ptr);
    set_enterbox_text(w,buffer);
}

widget * create_adjuster_pair(int * value_ptr,int min_value,int max_value,int text_space,int bar_fraction)
{
    widget * box=create_box(WIDGET_HORIZONTAL,WIDGET_FIRST_DISTRIBUTED);


    char text[16];
    sprintf(text,"%d",*value_ptr);
    #warning revise adjuster_pair, does it have any real practical use, will probably always need extended functionality to what is provided, instead package above to be called by propper function
    /// specifically doesnt allow some specialised operation to be called upon update of value, may be useful in most cases though, just reading value externally whenever its needed/used

    widget * sliderbar=add_child_to_parent(box,create_sliderbar(value_ptr,min_value,max_value,adjuster_pair_sliderbar_function,NULL,false,WIDGET_HORIZONTAL,bar_fraction));

	//sliderbar->sliderbar.data=add_child_to_parent(box,create_enterbox(text_space,text_space,text_space,text,adjuster_pair_enterbox_function,sliderbar,adjuster_pair_enterbox_update_contents_function,true,false));
	sliderbar->sliderbar.data=add_child_to_parent(box,create_enterbox_simple(text_space,text,adjuster_pair_enterbox_function,sliderbar,adjuster_pair_enterbox_update_contents_function,true,false));

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







static void empty_widget_render(overlay_data * od,overlay_theme * theme,widget * w,int x_off,int y_off,rectangle_ bounds)
{
//	rectangle_ r=w->base.r;
//
//	r.x+=x_off+2;
//	r.y+=y_off+2;
//	r.w-=4;
//	r.h-=4;
//
//	if((r.h<5)||(r.w<5))return;
}


static widget_appearence_function_set empty_appearence_functions=
(widget_appearence_function_set)
{
    .render =   empty_widget_render,
    .select =   blank_widget_select,
    .min_w  =   blank_widget_min_w,
    .min_h  =   blank_widget_min_h,
    .set_w  =   blank_widget_set_w,
    .set_h  =   blank_widget_set_h
};

widget * create_empty_widget(int min_w,int min_h)
{
    widget * w = create_widget(EMPTY_WIDGET);

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
(widget_appearence_function_set)
{
    .render =   blank_widget_render,
    .select =   blank_widget_select,
    .min_w  =   separator_widget_min_w,
    .min_h  =   separator_widget_min_h,
    .set_w  =   blank_widget_set_w,
    .set_h  =   blank_widget_set_h
};

widget * create_separator_widget(void)
{
    widget * w = create_widget(SEPARATOR_WIDGET);

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
(widget_appearence_function_set)
{
    .render =   blank_widget_render,
    .select =   blank_widget_select,
    .min_w  =   unit_separator_widget_min_w,
    .min_h  =   unit_separator_widget_min_h,
    .set_w  =   blank_widget_set_w,
    .set_h  =   blank_widget_set_h
};

widget * create_unit_separator_widget(void)
{
    widget * w = create_widget(SEPARATOR_WIDGET);

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


widget * create_window_widget(widget ** box,char * title,bool resizable,widget_function custom_exit_function,void * custom_exit_data,bool free_custom_exit_data)
{
    widget *w,*sub_box;

    if(resizable) w=create_resize_constraint(WIDGET_RESIZABLE_FREE_MOVEMENT,true);
    else w=create_resize_constraint(WIDGET_RESIZABLE_LOCKED_HORIZONTAL|WIDGET_RESIZABLE_LOCKED_VERTICAL,false);

    *box=add_child_to_parent(add_child_to_parent(w,create_panel()),create_box(WIDGET_VERTICAL,WIDGET_LAST_DISTRIBUTED));

    sub_box=add_child_to_parent(*box,create_box(WIDGET_HORIZONTAL,WIDGET_FIRST_DISTRIBUTED));
    add_child_to_parent(sub_box,create_anchor(w,title));
    add_child_to_parent(sub_box,create_separator_widget());
    //add_child_to_parent(sub_box,create_unit_separator_widget());
    if(custom_exit_function) add_child_to_parent(sub_box,create_icon_button("🗙",custom_exit_data,free_custom_exit_data,custom_exit_function));
    else add_child_to_parent(sub_box,create_icon_button("🗙",w,false,window_toggle_button_func));

    add_child_to_parent(*box,create_separator_widget());

    return w;
}













void file_search_window_end_function(file_search_instance * fsi)
{
    widget * win=fsi->end_data;
    toggle_widget(win);
}

file_search_error_type file_search_load_action(file_search_instance * fsi)
{
    printf("LOAD: %s\n",fsi->directory_buffer);

    return FILE_SEARCH_NO_ERROR;
}

widget * create_load_window(shared_file_search_data * sfsd,char * title,int * type_filters,file_search_action_function action,void * data,bool free_data,widget * menu_widget)
{
    widget *window,*box_0,*box_1,*box_2,*box_3,*list_box;
    file_search_instance * fsi;

    fsi=create_file_search_instance(sfsd,type_filters,NULL);

    window=create_window_widget(&box_0,title,true,file_search_cancel_button_func,fsi,false);

    if(action==NULL)action=file_search_load_action;

    set_file_search_instance_action_variables(fsi,action,data,free_data);
    set_file_search_instance_end_variables(fsi,file_search_window_end_function,window,false);


    box_1=add_child_to_parent(box_0,create_box(WIDGET_HORIZONTAL,WIDGET_FIRST_DISTRIBUTED));
    add_child_to_parent(box_1,create_file_search_text_bar(fsi));
    add_child_to_parent(box_1,create_file_search_refresh_button(fsi));
    add_child_to_parent(box_1,create_file_search_toggle_hidden_button(fsi));
    add_child_to_parent(box_1,create_file_search_up_button(fsi));

    box_1=add_child_to_parent(box_0,create_box(WIDGET_VERTICAL,WIDGET_FIRST_DISTRIBUTED));

    box_2=add_child_to_parent(box_1,create_box(WIDGET_HORIZONTAL,WIDGET_FIRST_DISTRIBUTED));
    list_box=add_child_to_parent(box_2,create_file_search_file_list(fsi));
    add_child_to_parent(box_2,create_contiguous_box_scrollbar(list_box));

    box_2=add_child_to_parent(box_1,create_box(WIDGET_HORIZONTAL,WIDGET_FIRST_DISTRIBUTED));
    if(type_filters)
    {
        box_3=add_child_to_parent(box_2,create_box(WIDGET_HORIZONTAL,WIDGET_LAST_DISTRIBUTED));
        add_child_to_parent(box_3,create_file_search_filter_type_button(menu_widget,fsi));
        add_child_to_parent(box_3,create_empty_widget(0,0));
    }
    else add_child_to_parent(box_2,create_empty_widget(0,0));
    add_child_to_parent(box_2,create_separator_widget());
    add_child_to_parent(box_2,create_file_search_cancel_button(fsi,"Cancel"));
    add_child_to_parent(box_2,create_file_search_accept_button(fsi,"Load"));

    create_file_search_error_popup(menu_widget,fsi,box_0,"Accept");


    return window;
}

file_search_error_type file_search_save_action(file_search_instance * fsi)
{
    printf("SAVE: %s\n",fsi->directory_buffer);

    return FILE_SEARCH_NO_ERROR;
    //return FILE_SEARCH_ERROR_CAN_NOT_SAVE_FILE;
}

widget * create_save_window(shared_file_search_data * sfsd,char * title,int * type_filters,file_search_action_function action,void * data,bool free_data,widget * menu_widget,char ** export_formats)
{
    widget *window,*box_0,*box_1,*box_2,*box_3,*list_box;
    file_search_instance * fsi;

    fsi=create_file_search_instance(sfsd,type_filters,export_formats);

    window=create_window_widget(&box_0,title,true,file_search_cancel_button_func,fsi,false);

    if(action==NULL)action=file_search_save_action;

    set_file_search_instance_action_variables(fsi,action,data,free_data);
    set_file_search_instance_end_variables(fsi,file_search_window_end_function,window,false);

    box_1=add_child_to_parent(box_0,create_box(WIDGET_HORIZONTAL,WIDGET_FIRST_DISTRIBUTED));
    add_child_to_parent(box_1,create_file_search_text_bar(fsi));
    add_child_to_parent(box_1,create_file_search_refresh_button(fsi));
    add_child_to_parent(box_1,create_file_search_toggle_hidden_button(fsi));
    add_child_to_parent(box_1,create_file_search_up_button(fsi));

    box_1=add_child_to_parent(box_0,create_box(WIDGET_VERTICAL,WIDGET_FIRST_DISTRIBUTED));

    box_2=add_child_to_parent(box_1,create_box(WIDGET_HORIZONTAL,WIDGET_FIRST_DISTRIBUTED));
    list_box=add_child_to_parent(box_2,create_file_search_file_list(fsi));
    add_child_to_parent(box_2,create_contiguous_box_scrollbar(list_box));

    box_2=add_child_to_parent(box_1,create_box(WIDGET_HORIZONTAL,WIDGET_FIRST_DISTRIBUTED));
    add_child_to_parent(box_2,create_file_search_enterbox(fsi));
    if((export_formats)&&(export_formats[0])&&(export_formats[1]))add_child_to_parent(box_2,create_file_search_export_type_button(menu_widget,fsi));

    box_2=add_child_to_parent(box_1,create_box(WIDGET_HORIZONTAL,WIDGET_FIRST_DISTRIBUTED));
    if(type_filters)
    {
        box_3=add_child_to_parent(box_2,create_box(WIDGET_HORIZONTAL,WIDGET_LAST_DISTRIBUTED));
        add_child_to_parent(box_3,create_file_search_filter_type_button(menu_widget,fsi));
        add_child_to_parent(box_3,create_empty_widget(0,0));
    }
    else add_child_to_parent(box_2,create_empty_widget(0,0));
    add_child_to_parent(box_2,create_separator_widget());
    add_child_to_parent(box_2,create_file_search_cancel_button(fsi,"Cancel"));
    add_child_to_parent(box_2,create_file_search_accept_button(fsi,"Save"));




    create_file_search_overwrite_popup(menu_widget,fsi,box_0,"File already exists, overwrite?","Yes","No");
    create_file_search_error_popup(menu_widget,fsi,box_0,"Accept");



    //add_child_to_parent(box_2,create_text_button("test",error_popup,window_toggle_button_func));

    return window;
}












void popup_button_toggle_func(widget * w)
{
    toggle_auto_close_popup(w->button.data);
}

///creates and returns button to toggle popup, also creates panel in popup in popup_container and adds panel_contents in the panel
widget * create_popup_panel_button(widget * popup_container,widget * panel_contents,char * button_text)///input positioning????
{
    widget * popup=add_child_to_parent(popup_container,create_popup(WIDGET_POSITIONING_OFFSET_RIGHT,true));

    //widget * button=create_text_button(button_text,popup,popup_button_toggle_func);
    widget * button=create_text_highlight_toggle_button(button_text,popup,false,popup_button_toggle_func,button_widget_status_func);
    button->base.status&= ~WIDGET_CLOSE_POPUP_TREE;

    add_child_to_parent(add_child_to_parent(popup,create_panel()),panel_contents);

    set_popup_alignment_widgets(popup,panel_contents,button);
    set_popup_trigger_widget(popup,button);

    return button;
}




#warning probably want following as a single button, have special variant of text_icon h_bar button

widget * create_checkbox_button_pair(char* text, void* data, widget_function func, widget_button_toggle_status_func toggle_status,bool free_data)
{
    widget * box=create_box(WIDGET_HORIZONTAL,WIDGET_FIRST_DISTRIBUTED);

    add_child_to_parent(box,create_text_button(text,data,false,func));
    add_child_to_parent(box,create_icon_toggle_button("true","false",data,free_data,func,toggle_status));

    return box;
}

widget * create_bool_checkbox_button_pair(char * text, bool * bool_ptr)
{
    return create_checkbox_button_pair(text,bool_ptr,button_toggle_bool_func,button_bool_status_func,false);
}



widget * create_icon_collapse_button(char * icon_collapse,char * icon_expand,widget * widget_to_control,bool collapse)
{
    widget * button=create_icon_toggle_button(icon_collapse,icon_expand,widget_to_control,false,toggle_widget_button_func,button_widget_status_func);

    if(collapse)toggle_widget(widget_to_control);

    return button;
}
