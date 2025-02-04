/**
Copyright 2020,2021,2022 Carl van Mastrigt

This file is part of solipsix.

solipsix is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

solipsix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with solipsix.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef solipsix_H
#include "solipsix.h"
#endif

#ifndef WIDGET_MISC_H
#define WIDGET_MISC_H


///miscellaneous widget combinations and variants as well as simple (e.g. blank) widget types



widget * create_adjuster_pair(struct widget_context* context, int * value_ptr,int min_value,int max_value,int text_space,int bar_fraction,int scroll_fraction);

void adjuster_pair_slider_bar_function(widget * w);
void adjuster_pair_enterbox_function(widget * w);

void toggle_widget(widget * w);
void toggle_widget_button_func(widget * w);

widget * create_empty_widget(struct widget_context* context, int16_t min_w,int16_t min_h);
widget * create_separator_widget(struct widget_context* context);
widget * create_unit_separator_widget(struct widget_context* context);

void window_toggle_button_func(widget * w);
widget * create_window_widget(struct widget_context* context, widget ** box,char * title,bool resizable,widget_function custom_exit_function,void * custom_exit_data,bool free_custom_exit_data);

widget * create_popup_panel_button(struct widget_context* context, widget * popup_container,widget * panel_contents,char * button_text);

widget * create_checkbox_button_pair(struct widget_context* context, char * text, void * data, widget_function func, widget_button_toggle_status_func toggle_status,bool free_data);
widget * create_bool_checkbox_button_pair(struct widget_context* context, char * text, bool * bool_ptr);

widget * create_icon_collapse_button(struct widget_context* context, char * icon_collapse,char * icon_expand,widget * widget_to_control,bool collapse);

void create_self_deleting_dialogue(struct widget_context* context, widget* root_widget, const char* message_str, const char* accept_str, const char* cancel_str, void* data, void (*accept_function)(void*), void (*cancel_function)(void*));







// // this *basically* needs to be an ad-hoc widget, otherwise would need to somehow pass through behaviour for internal widget(s) -- maybe thats possible though?
// typedef struct widget_file_list_2
// {
//     widget_base base;

//     struct cvm_directory* directory;


// }
// widget_file_list_2;


struct multibox_functions // appearance?
{
    // internal/content/element functions first

    /// widget(multibox), x_off, y_off, render_batch, bounds, element/entry index
    void    (*const entry_render) (widget*,int16_t,int16_t,struct cvm_overlay_render_batch*,rectangle,uint32_t);
    int16_t (*const entry_min_w ) (widget*);
    int16_t (*const entry_min_h ) (widget*);


    ///uses shared_data to get count for display purposes
    uint32_t (*const get_count)(void*);

    ///uses shared_data and index to get a particular entry for rendering &c.
    void*    (*const get_entry)(void*, int32_t);
};

// way to support grid, h-list, v-list?
typedef struct widget_multibox
{
    widget_base base;

    void* shared_data;

    const struct multibox_functions* functions;

    uint32_t selected_widget;

    /**
     * layout - horizontal, vertical, grid
     * need a way to represent more complex behaviour (more than just click/activate - drag &c.)
     * ^ this almost warrants interaction rewrite in itself, but how will this shit even work?
     * ^ drag can be accomplished with down-up distinction, with currently active widget tracking data for this
     *   ^ mouse button down/up, gamepad up/down
     *   ^ drag can be handled as special property for rendering this widget (this very much extended functionality)
     *     ^ does not allow complex inter-widget drag functionality, but probably will work where necessary
    */

    /** consider following for content behaviour, may need something more specialised as not all will be necessary*/
    // const widget_behaviour_function_set* content_behaviour_functions;

    #warning other widgets that keep pointers (e.g. sliderbar) should perhaps instead have accessor functions?? (as above) to allow more complex or threadsafe behaviour


    // what should these take as input to get data? widget* or void* ???
    // render same as `widget_appearence_function_set` with the entry number included


    // behaviour functions WAY more complex!

    // for rendering purposes, need x and y to account for grid capabilities
    // affect widget w/h which are int16
    int16_t min_displayed_count_x;
    int16_t min_displayed_count_y;
}
widget_multibox;




#endif



