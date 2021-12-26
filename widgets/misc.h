/**
Copyright 2020,2021 Carl van Mastrigt

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

#ifndef CVM_SHARED_H
#include "cvm_shared.h"
#endif

#ifndef WIDGET_MISC_H
#define WIDGET_MISC_H


///miscellaneous widget combinations and variants as well as simple (e.g. blank) widget types



widget * create_adjuster_pair(int * value_ptr,int min_value,int max_value,int text_space,int bar_fraction);

void adjuster_pair_slider_bar_function(widget * w);
void adjuster_pair_enterbox_function(widget * w);

void toggle_widget(widget * w);
void toggle_widget_button_func(widget * w);

widget * create_empty_widget(int min_w,int min_h);
widget * create_separator_widget(void);
widget * create_unit_separator_widget(void);

void window_toggle_button_func(widget * w);
widget * create_window_widget(widget ** box,char * title,bool resizable,widget_function custom_exit_function,void * custom_exit_data,bool free_custom_exit_data);

widget * create_load_window(shared_file_search_data * sfsd,char * title,int * type_filters,file_search_action_function action,void * data,bool free_data,widget * menu_widget);
widget * create_save_window(shared_file_search_data * sfsd,char * title,int * type_filters,file_search_action_function action,void * data,bool free_data,widget * menu_widget,char ** export_formats);

widget * create_popup_panel_button(widget * popup_container,widget * panel_contents,char * button_text);

widget * create_checkbox_button_pair(char * text, void * data, widget_function func, widget_button_toggle_status_func toggle_status,bool free_data);
widget * create_bool_checkbox_button_pair(char * text, bool * bool_ptr);

widget * create_icon_collapse_button(char * icon_collapse,char * icon_expand,widget * widget_to_control,bool collapse);

#endif



