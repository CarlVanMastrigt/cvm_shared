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

#ifndef CVM_SHARED_H
#include "cvm_shared.h"
#endif

#ifndef FILE_LIST_H
#define FILE_LIST_H

typedef struct file_list_entry/// instead use this to load directory data, set buttons as outlined next to variables (need options to avoid deletion of button strings)
{
    union
    {
        const char * filename;///widget_file_list remove this, it only exists to allow qsort to be used, custom sort would work better...
        uint32_t filename_offset_;
    };
    //uint32_t filename_offset;///could be set using button.text to get text location in buffer (assuming buffer is set already and won't be resized)???
    uint16_t type_id:10;///could be set using button.icon_name and button.func, 0=misc file, 1=directory, , other=(offset+2) in type list
    uint16_t is_hidden_file:1;
    uint16_t is_control_entry:1;

    uint16_t text_length_calculated:1;///could be set using button.icon_name and button.func, 0=misc file, 1=directory, , other=(offset+2) in type list
    int16_t text_length;
    //uint16_t hidden:1;///not really needed ??? alter functionality when loading directory
}
file_list_entry;

typedef struct file_list_type
{
    const char * name;
    const char * icon;
    const char * type_extensions;///appended character list, first is always preferred type when exporting
}
file_list_type;

///returns whether popup is required, should always have completed file/folder result in file_list.composite_buffer will require modifying error popup in function but that's probably okay
/// second parameter is whether to force the operation
typedef bool(*file_list_widget_action)(widget*,bool);
#warning above should instead not consider the widget at all (if possible) and should just operate on the widget_context and filename

//void load_file_search_directory_entries(file_search_data * fsd);

//void initialise_file_search_data(file_search_data * fsd,const char * directory,const file_search_file_type * types,uint32_t type_count);


/// essentially a contiguous box, consider replacing contiguous box with many ad-hoc variants of this, or adapting contiguous box as base for those variants, forms the core of both a save and load variant
typedef struct widget_file_list
{
    widget_base base;

    char * filename_buffer;///shared buffer for all entry names
    uint32_t filename_buffer_space;

    ///current directory
    uint32_t directory_buffer_size;///placed here for better packing
    char * directory_buffer;///current directory, should expand dynamically to fit selected/enetered file for the different purposes that that serves, with removal

    ///combined directory and selected entry
    uint32_t composite_buffer_size;
    char * composite_buffer;

    file_list_entry * entries;
    uint32_t entry_space;
    uint32_t valid_entry_count;///number of entries that are valid for this configuration of search criterion
    uint32_t entry_count;
    ///store "valid" entry count, display only these, allows resorting w/o reloading entries

    ///can be the same or null, could dynamically switch these out if shared system is actually desirable... (or programmer could just set directory using value from the other themselves)
    const file_list_type * file_types;
    uint32_t file_type_count;
    file_list_widget_action action;
    void * action_data;

    uint16_t free_action_data:1;///REMOVE
    uint16_t fixed_directory:1;///allow changing directory, show directories in file list &c.
    uint16_t hide_misc_files:1;///show files not relevant to any applicable filter
    uint16_t hide_control_entries:1;/// show  ./  ../   &c.
    uint16_t hide_hidden_entries:1;///this is only "bool" that can change ??? how are    show all files/show_misc_files   different/differentiated ???
    uint16_t render_type_icons:1;///should this always be true?
    uint16_t save_mode_active:1;///if true; save, if false; load REMOVE
    uint16_t select_folder:1;/// only show directories(?), open folder upon open button (action) press, but not double click (name button select folder)

    ///link to widgets that operate in tandem with this widget, is basically saying cannot have file interaction without the file list widget (unsure this is desirable but w/e)
    widget * enterbox;///enterbox automplete w/ tab autofill?
    widget * directory_text_bar;///displays full directory ending in current widget, fade on left hand side
    widget * type_select_popup;

    widget * parent_widget; ///needs way to close widget should that happen, can be null to avoid

    uint32_t selected_out_type;

    int32_t selected_entry_index;
    int32_t offset;
    int32_t max_offset;
    int32_t visible_height;///bar size
    int32_t entry_height;

    int16_t min_visible_rows;///minimum number of visible rows, input variable, sets min_h
    int16_t min_visible_glyphs;///minimum number of visible glyphs per item name

}
widget_file_list;

///initial directory can be null, must handle this case
widget * create_file_list(struct widget_context* context, int16_t min_visible_rows,int16_t min_visible_glyphs,const char * initial_directory,const char *const * error_messages,uint16_t error_count);

///hmm, allowing tabs to perform action may be a good idea if wanting unified save/load widget spread across tabs
void file_list_widget_set_action_information(const file_list_type * supported_types,uint32_t type_count,void * data,file_list_widget_action action,bool hide_misc_files,bool allow_selecting_folder);


widget * create_file_list_widget_directory_text_bar(struct widget_context* context, widget * file_list,uint32_t min_glyphs_visible);
widget * create_file_list_widget_enterbox(struct widget_context* context, widget * file_list,uint32_t min_glyphs_visible);

widget * create_file_list_widget_refresh_button(struct widget_context* context, widget * file_list);
widget * create_file_list_widget_up_button(struct widget_context* context, widget * file_list);
widget * create_file_list_widget_home_button(struct widget_context* context, widget * file_list);


const char * file_list_widget_get_selected_filepath(widget * w);///e.g. for delete function

void file_list_widget_load_directory_entries(widget * w);/// reloads directory, use when contents are expected to have changed



#endif




