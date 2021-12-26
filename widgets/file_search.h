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

#ifndef FILE_SEARCH_H
#define FILE_SEARCH_H



typedef enum
{
    FILE_SEARCH_NO_ERROR=0,
    FILE_SEARCH_ERROR_INVALID_FILE,
    FILE_SEARCH_ERROR_INVALID_DIRECTORY,
    FILE_SEARCH_ERROR_CAN_NOT_CHANGE_DIRECTORY,
    FILE_SEARCH_ERROR_CAN_NOT_SAVE_FILE,
    FILE_SEARCH_ERROR_CAN_NOT_LOAD_FILE,
    FILE_SEARCH_ERROR_COUNT,
}
file_search_error_type;


typedef struct shared_file_search_data shared_file_search_data;

typedef struct file_search_instance file_search_instance;

typedef file_search_error_type(*file_search_action_function)(file_search_instance*);

typedef void(*file_search_end_function)(file_search_instance*);

typedef struct file_search_file_type
{
    char * name;
    char * icon_name;
    char ** type_extensions;
}
file_search_file_type;

///typedef struct file_search_data file_search_data;

typedef struct widget_file_search_entry/// instead use this to load directory data, set buttons as outlined next to variables (need options to avoid deletion of button strings)
{
    file_search_instance * fsi;
    uint32_t name_offset:24;///could be set using button.text to get text location in buffer (assuming buffer is set already and won't be resized)???
    uint32_t type:7;///could be set using button.icon_name and button.func, 0=directory, 1=misc_file, other=(offset+2) in type list
    uint32_t hidden:1;///not really needed ??? alter functionality when loading directory
}
widget_file_search_entry;



struct file_search_instance
{
    file_search_instance * next;

    shared_file_search_data * sfsd;
    uint32_t shared_data_update_count;
    //widget_base base;

    widget * file_box;

    widget * enterbox;///can be null, used to set text upon button click

    widget * selected_entry;

    widget * overwrite_popup;

    widget * error_popup;
    file_search_error_type error_type;

    widget * type_filter_popup;

    widget * type_export_popup;

    char * directory_buffer;///change to location???
    size_t directory_buffer_size;

    //file_search_filter * filters;
    int * type_filters;///from shared_file_search_data.types
    int active_type_filter;

    char ** export_formats;
    int active_export_format;

    widget_file_search_entry * entries;///keeping them seperate allows for ordering (when loading directory)
    uint32_t entry_count;
    uint32_t entry_space;

    char * entry_name_data;
    uint32_t entry_name_data_count;
    uint32_t entry_name_data_space;

    file_search_action_function action_function;
    void * action_data;
    //bool free_action_data;

    file_search_end_function end_function;
    void * end_data;
    //bool free_end_data;

    uint32_t free_action_data:1;
    uint32_t free_end_data:1;
    uint32_t show_directories:1;///change to directory fixed???
    uint32_t show_misc_files:1;
    uint32_t show_control_entries:1;

    uint32_t show_hidden:1;///this is only "bool" that can change ??? how are    show all files/show_misc_files   different/differentiated ???
};

file_search_instance * create_file_search_instance(shared_file_search_data * sfsd,int * type_filters,char ** export_formats);

void set_file_search_instance_settings(file_search_instance * fsi,bool show_hidden,bool show_directories,bool show_misc_files,bool show_control_entries);
//widget * get_file_search_data_enterbox(file_search_data * fsd);
//widget * get_file_search_data_file_list(file_search_data * fsd);

void set_file_search_instance_action_variables(file_search_instance * fsi,file_search_action_function action_function,void * action_data,bool free_action_data);
void set_file_search_instance_end_variables(file_search_instance * fsi,file_search_end_function end_function,void * end_data,bool free_end_data);

//widget * create_default_file_search_cancel_button(file_search_data * fsd,widget * parent_widget);
//widget * create_default_file_search_exit_button(file_search_data * fsd,widget * parent_widget);
//widget * create_default_file_search_up_directory_button(file_search_data * fsd);
//widget * create_default_file_search_refresh_button(file_search_data * fsd);
//widget * create_default_file_search_toggle_hidden_button(file_search_data * fsd);
//widget * create_default_file_search_up_directory_button(file_search_data * fsd);
//widget * create_default_file_search_save_button(file_search_data * fsd);
//widget * create_default_file_search_load_button(file_search_data * fsd);

void file_search_up_button_func(widget * w);
void file_search_toggle_hidden_button_func(widget * w);
void file_search_refresh_button_func(widget * w);
void file_search_accept_button_func(widget * w);
void file_search_cancel_button_func(widget * w);
//void file_search_directory_panel_func(widget * w);
//void file_search_load_button_func(widget * w);
//void file_search_save_button_func(widget * w);
//void file_search_cancel_button_func(widget * w);
//void file_search_save_enterbox_function(widget * w);
//void file_search_save_enterbox_upon_input_function(widget * w);


//void default_file_search_widget_load_button_action(widget * w);
//void default_file_search_widget_save_button_action(widget * w);
//void default_file_search_widget_save_enterbox_action(widget * w);



widget * create_file_search_file_list(file_search_instance * fsi);
//widget * create_file_search_scrollbar(file_search_instance * fsi);
widget * create_file_search_text_bar(file_search_instance * fsi);
widget * create_file_search_enterbox(file_search_instance * fsi);

widget * create_file_search_up_button(file_search_instance * fsi);
widget * create_file_search_toggle_hidden_button(file_search_instance * fsi);
widget * create_file_search_refresh_button(file_search_instance * fsi);

widget * create_file_search_cancel_button(file_search_instance * fsi,char * text);
widget * create_file_search_accept_button(file_search_instance * fsi,char * text);

void create_file_search_overwrite_popup(widget * menu_widget,file_search_instance * fsi,widget * external_alignment_widget,char * overwrite_message,char * affirmative_text,char * negative_text);
void create_file_search_error_popup(widget * menu_widget,file_search_instance * fsi,widget * external_alignment_widget,char * affirmative_text);

widget * create_file_search_filter_type_button(widget * menu_widget,file_search_instance * fsi);
widget * create_file_search_export_type_button(widget * menu_widget,file_search_instance * fsi);



struct shared_file_search_data
{
    file_search_file_type * types;

    uint32_t update_count;

    char * directory_buffer;
    uint32_t directory_buffer_size;

    ///char * filename_buffer;///move this to fsi (prev_file_buffer)
    ///uint32_t filename_buffer_size;

    char ** error_messages;
    int error_count;

    file_search_instance * first;
};

shared_file_search_data * create_shared_file_search_data(char * initial_directory,file_search_file_type * types);
void delete_shared_file_search_data(shared_file_search_data * sfsd);
void update_shared_file_search_data_directory_use(shared_file_search_data * sfsd);
//file_search_instance * allocate_file_search_instance(shared_file_search_data * sfsd);




#endif




