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

#ifndef CVM_SHARED_H
#include "cvm_shared.h"
#endif

#ifndef CVM_WIDGET_H
#define CVM_WIDGET_H



#define BLANK_WIDGET_STATUS         0x00000000
#define WIDGET_ACTIVE               0x00000001
#define WIDGET_REQUIRES_RENDERING   0x00000002 /** used to specify completely transparent widgets overlaid over more complex game elements */
#define WIDGET_CLOSE_POPUP_TREE     0x00000004 /** does not collapse parent popup hierarchy upon interaction (e.g. toggle buttons, popup triggering buttons, enterboxes, scrollbars and slider_bars)  */
#define WIDGET_IS_ROOT              0x00000010 /** used for validation in various places*/
#define WIDGET_IS_AUTO_CLOSE_POPUP  0x00000020
#define WIDGET_IS_CONTIGUOUS_BOX    0x00000040
#define WIDGET_DO_NOT_DELETE        0x00000080 /** must only be deleted by specialised method, which will set this to false before executing, REMOVE THIS! */
// could have one that is essentiually "was allocated" that determines if to free upon deletion

#define WIDGET_H_FIRST  0x10000000
#define WIDGET_H_LAST   0x20000000
#define WIDGET_V_FIRST  0x40000000
#define WIDGET_V_LAST   0x80000000

#define WIDGET_POS_FLAGS_CLEAR  0x0FFFFFFF
#define WIDGET_POS_FLAGS_H      0x30000000
#define WIDGET_POS_FLAGS_V      0xC0000000
#define WIDGET_POS_FLAGS        0xF0000000


typedef void(*widget_function)(widget*);

typedef struct widget_appearence_function_set
{
    void    (*const render) (overlay_theme*,widget*,int16_t,int16_t,struct cvm_overlay_render_batch*,rectangle);
    widget* (*const select) (overlay_theme*,widget*,int16_t,int16_t);
    void    (*const min_w)  (overlay_theme*,widget*);
    void    (*const min_h)  (overlay_theme*,widget*);
    void    (*const set_w)  (overlay_theme*,widget*);
    void    (*const set_h)  (overlay_theme*,widget*);
}
widget_appearence_function_set;

// // consider moving this somewhere more generic
// // probably needs review before use
// enum cvm_widget_command
// {
//     /// these apply to text (cut copy) or generally (arrow keys &c.)
//     CVM_TEXT_COMMAND_NONE = 0,
//     CVM_TEXT_COMMAND_COPY,
//     CVM_TEXT_COMMAND_CUT,
//     CVM_TEXT_COMMAND_PASTE,
//     CVM_TEXT_COMMAND_LEFT,
//     CVM_TEXT_COMMAND_RIGHT,
//     CVM_TEXT_COMMAND_UP,
//     CVM_TEXT_COMMAND_DOWN,
//     CVM_TEXT_COMMAND_SELECT_ALL,
// };

// enum cvm_widget_modifier
// {
//     CVM_TEXT_COMMAND_MODIFIER_NONE = 0,
//     // modifiers to text caret move, are mutually exclusive
//     CVM_TEXT_COMMAND_MODIFIER_WORD = 0x01,
//     CVM_TEXT_COMMAND_MODIFIER_LINE = 0x02,
//     // CVM_TEXT_COMMAND_MODIFIER_ALL  = 0x04,
//     // post text caret move action, are mutually exclusive
//     CVM_TEXT_COMMAND_MODIFIER_SELECT = 0x10,
//     CVM_TEXT_COMMAND_MODIFIER_ERASE  = 0x20,
// };

typedef struct widget_behaviour_function_set
{
    void    (*const l_click)    (overlay_theme*,widget*,int,int,bool);
    bool    (*const l_release)  (overlay_theme*,widget*,widget*,int,int);
    void    (*const r_click)    (overlay_theme*,widget*,int,int);
    void    (*const m_move)     (overlay_theme*,widget*,int,int);
    bool    (*const scroll)     (overlay_theme*,widget*,int);
    bool    (*const key_down)   (overlay_theme*,widget*,SDL_Keycode,SDL_Keymod);
    bool    (*const text_input) (overlay_theme*,widget*,char*);
    bool    (*const text_edit)  (overlay_theme*,widget*,char*,int,int);
    /// rename click_away to lose_focus? or something?
    void    (*const click_away) (overlay_theme*,widget*);

    void    (*const add_child)      (widget*,widget*);
    void    (*const remove_child)   (widget*,widget*);
    void    (*const wid_delete)     (widget*);
}
widget_behaviour_function_set;

typedef enum
{
    WIDGET_HORIZONTAL=0,
    WIDGET_VERTICAL
}
widget_layout;

typedef enum
{
    WIDGET_FIRST_DISTRIBUTED=0,
    WIDGET_LAST_DISTRIBUTED,
    WIDGET_EVENLY_DISTRIBUTED,
    WIDGET_NORMALLY_DISTRIBUTED,
    WIDGET_ALL_SAME_DISTRIBUTED
}
widget_distribution;

typedef enum
{
    WIDGET_TEXT_LEFT_ALIGNED=0,
    WIDGET_TEXT_RIGHT_ALIGNED
}
widget_text_alignment;

typedef enum
{
    WIDGET_POSITIONING_CENTRED=0,
    WIDGET_POSITIONING_OFFSET_RIGHT,
    WIDGET_POSITIONING_OFFSET_DOWN,
    WIDGET_POSITIONING_CENTRED_DOWN,
    WIDGET_POSITIONING_NONE
}
widget_relative_positioning;


struct widget_context
{
    overlay_theme* theme;
    /// put settings here?? make it a pointer!

    uint32_t registered_widget_count;//for debug


    uint32_t double_click_time;//move to settings


    widget* currently_active_widget;// needs better name selected_widget? even just active_widget?
    widget* only_interactable_widget;

    /// used for double click detection
    widget* previously_clicked_widget;
    uint32_t previously_clicked_time;

    widget* root_widget;///
};

void widget_context_initialise(struct widget_context* context, overlay_theme* theme);
void widget_context_terminate(struct widget_context* context);



typedef struct widget_base
{
    struct widget_context* context;

    uint32_t status;

    int16_t min_w;
    int16_t min_h;

    rectangle r;

    widget* parent;

    widget* next;
    widget* prev;

    const widget_appearence_function_set* appearence_functions;
    const widget_behaviour_function_set* behaviour_functions;
}
widget_base;


#include "widgets/container.h"
#include "widgets/box.h"
#include "widgets/button.h"
#include "widgets/enterbox.h"
#include "widgets/slider_bar.h"
#include "widgets/text_bar.h"
#include "widgets/anchor.h"
#include "widgets/textbox.h"
#include "widgets/panel.h"
#include "widgets/resize_constraint.h"
#include "widgets/popup.h"
#include "widgets/tab.h"
#include "widgets/file_list.h"
#include "widgets/contiguous_box.h"


#include "widgets/misc.h"

union widget
{
    widget_base                 base;


    widget_button               button;
    widget_anchor               anchor;
    widget_textbox              textbox;
    widget_enterbox             enterbox;
    widget_slider_bar           slider_bar;
    widget_text_bar             text_bar;

    widget_container            container;
    widget_contiguous_box       contiguous_box;
    widget_panel                panel;
    widget_resize_constraint    resize_constraint;
    widget_popup                popup;
    widget_file_list            file_list;

    widget_tab_folder           tab_folder;
};


widget * create_widget(struct widget_context* context, size_t size);

void widget_base_initialise(widget_base* base, struct widget_context* context, const widget_appearence_function_set * appearence_functions, const widget_behaviour_function_set * behaviour_functions);



void organise_root_widget(widget* root_widget,int screen_width,int screen_height);



void organise_toplevel_widget(widget * w);
void move_toplevel_widget_to_front(widget * target);

void adjust_coordinates_to_widget_local(widget * w,int * x,int * y);
void get_widgets_global_coordinates(widget * w,int * x,int * y);



void render_widget(widget * w,overlay_theme* theme,int x_off,int y_off,struct cvm_overlay_render_batch * restrict render_batch,rectangle bounds);
widget * select_widget(widget * w, overlay_theme* theme, int x_in, int y_in);
int16_t set_widget_minimum_width(widget * w, overlay_theme* theme, uint32_t pos_flags);
int16_t set_widget_minimum_height(widget * w, overlay_theme* theme, uint32_t pos_flags);
int16_t organise_widget_horizontally(widget * w, overlay_theme* theme, int16_t x_pos, int16_t width);
int16_t organise_widget_vertically(widget * w, overlay_theme* theme, int16_t y_pos, int16_t height);



void        blank_widget_render         (overlay_theme * theme,widget * w,int16_t x_off,int16_t y_off,struct cvm_overlay_render_batch * restrict render_batch,rectangle bounds);
widget *    blank_widget_select         (overlay_theme * theme,widget * w,int16_t x_in,int16_t y_in);
void        blank_widget_min_w          (overlay_theme * theme,widget * w);
void        blank_widget_min_h          (overlay_theme * theme,widget * w);
void        blank_widget_set_w          (overlay_theme * theme,widget * w);
void        blank_widget_set_h          (overlay_theme * theme,widget * w);

void        blank_widget_left_click     (overlay_theme * theme, widget * w, int x, int y, bool double_clicked);
bool        blank_widget_left_release   (overlay_theme * theme,widget * clicked,widget * released,int x,int y);
void        blank_widget_right_click    (overlay_theme * theme,widget * w,int x,int y);
void        blank_widget_mouse_movement (overlay_theme * theme,widget * w,int x,int y);
bool        blank_widget_scroll         (overlay_theme * theme,widget * w,int delta);
bool        blank_widget_key_down       (overlay_theme * theme,widget * w,SDL_Keycode keycode,SDL_Keymod mod);
bool        blank_widget_text_input     (overlay_theme * theme,widget * w,char * text);
bool        blank_widget_text_edit      (overlay_theme * theme,widget * w,char * text,int start,int length);
void        blank_widget_click_away     (overlay_theme * theme,widget * w);
void        blank_widget_add_child      (widget * parent,widget * child);
void        blank_widget_remove_child   (widget * parent,widget * child);
void        blank_widget_delete         (widget * w);














//void initialise_menu_header(menu_header * mh,int num_menus,int * screen_w,int * screen_h);
//void delete_all_widgets(menu_header * mh);
//
//widget * get_widget_menu(menu_header * mh,int index);
//
//void switch_currently_active_menu(menu_header * mh,int new_current_menu);

void render_widget_overlay(struct cvm_overlay_render_batch * restrict render_batch, widget * root_widget);

bool handle_widget_overlay_left_click(widget* root_widget,int x_in,int y_in);
bool handle_widget_overlay_left_release(widget* root_widget,int x_in,int y_in);
bool handle_widget_overlay_movement(widget* root_widget,int x_in,int y_in);
bool handle_widget_overlay_wheel(widget* root_widget,int x_in,int y_in,int delta);
bool handle_widget_overlay_keyboard(widget* root_widget,SDL_Keycode keycode,SDL_Keymod mod);
bool handle_widget_overlay_text_input(widget* root_widget,char * text);
bool handle_widget_overlay_text_edit(widget* root_widget,char * text,int start,int length);

widget* add_child_to_parent(widget * parent,widget * child);
void remove_child_from_parent(widget * child);
void delete_widget(widget * w);




// the separation of these can be avoided if ROOT is required to be provided
void set_currently_active_widget(struct widget_context* context, widget * w);
void set_only_interactable_widget(struct widget_context* context, widget * w);

bool is_currently_active_widget(widget * w);
//bool test_currently_active_widget_key_input(void);


bool widget_active(widget * w);

widget* find_root_widget(widget* w);


widget* create_root_widget(struct widget_context* context, overlay_theme* theme);

#endif // CVM_WIDGET_H




