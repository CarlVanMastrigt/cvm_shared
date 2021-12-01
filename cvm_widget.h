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

#ifndef CVM_SHARED_H
#include "cvm_shared.h"
#endif

#ifndef CVM_WIDGET_H
#define CVM_WIDGET_H





#define BLANK_WIDGET_STATUS         0x00000000
#define WIDGET_ACTIVE               0x00000001
#define WIDGET_REQUIRES_RENDERING   0x00000002 /** used to specify completely transparent widgets overlaid over more complex game elements */
#define WIDGET_CLOSE_POPUP_TREE     0x00000004 /** does not collapse parent popup hierarchy upon interaction (e.g. toggle buttons, popup triggering buttons, enterboxes, scrollbars and sliderbars)  */
#define WIDGET_IS_MENU              0x00000040 /** used for testing find_toplevel_ancestor worked (is parent-child structure correct) */
#define WIDGET_IS_AUTO_CLOSE_POPUP  0x00000080

#define WIDGET_H_FIRST  0x10000000
#define WIDGET_H_LAST   0x20000000
#define WIDGET_V_FIRST  0x40000000
#define WIDGET_V_LAST   0x80000000

#define WIDGET_POS_FLAGS_CLEAR  0x0FFFFFFF
#define WIDGET_POS_FLAGS_H      0x30000000
#define WIDGET_POS_FLAGS_V      0xC0000000
#define WIDGET_POS_FLAGS        0xF0000000


//typedef union widget widget;

typedef void(*widget_function)(widget*);

typedef struct widget_appearence_function_set
{
    void    (*render)   (overlay_data*,overlay_theme*,widget*,int,int,rectangle);
    widget* (*select)   (overlay_theme*,widget*,int,int);
    void    (*min_w)    (overlay_theme*,widget*);
    void    (*min_h)    (overlay_theme*,widget*);
    void    (*set_w)    (overlay_theme*,widget*);
    void    (*set_h)    (overlay_theme*,widget*);
}
widget_appearence_function_set;

typedef struct widget_behaviour_function_set
{
    void    (*l_click)      (overlay_theme*,widget*,int,int);
    bool    (*l_release)    (overlay_theme*,widget*,widget*,int,int);
    void    (*r_click)      (overlay_theme*,widget*,int,int);
    void    (*m_move)       (overlay_theme*,widget*,int,int);
    bool    (*scroll)       (overlay_theme*,widget*,int);
    bool    (*key_down)     (overlay_theme*,widget*,SDL_Keycode);
    bool    (*text_input)   (overlay_theme*,widget*,char*);
    bool    (*text_edit)    (overlay_theme*,widget*,char*,int,int);
    void    (*click_away)   (overlay_theme*,widget*);

    void    (*add_child)    (widget*,widget*);
    void    (*remove_child) (widget*,widget*);
    void    (*wid_delete)   (widget*);
}
widget_behaviour_function_set;

typedef enum ///remove this enum and type from widget, does nothing
{
    VOID_WIDGET=0,
    BLANK_WIDGET,
    BUTTON_WIDGET,
    ANCHOR_WIDGET,
    EMPTY_WIDGET,
    SEPARATOR_WIDGET,
    //MULTIPURPOSE_WIDGET,
    RESIZE_CONSTRAINT_WIDGET,
    ENTERBOX_WIDGET,
    SLIDERBAR_WIDGET,
    TEXT_BAR_WIDGET,
    SCREEN_WIDGET,
    TEXTBOX_WIDGET,
    CONTAINER_WIDGET,
    //BOX_WIDGET,
    //VARIABLE_BOX_WIDGET,
    CONTIGUOUS_BOX_WIDGET,
    //DIVIDER_WIDGET,
    POPUP_WIDGET,
    WINDOW_WIDGET,
    //TAB_PAGE_WIDGET,
    TAB_FOLDER_WIDGET,
    //LIST_WIDGET,
    PANEL_WIDGET,
    FILE_SEARCH_WIDGET,

    //CUSTOM_WIDGET,

    NUM_WIGET_TYPES
}
widget_type;

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

/*typedef struct widget_status_data
{
    uint32_t active:1;
    uint32_t highlighted:1;
    uint32_t leave_popup_branch_open_upon_interaction:1;
    uint32_t stay_at_bottom:1;
    uint32_t delete_stored_data:1;

    uint32_t h_first:1;
    uint32_t h_last:1;
    uint32_t v_first:1;
    uint32_t v_last:1;
}
widget_status_data;*/

typedef struct widget_base
{
    widget_type type;
    uint32_t status;

    rectangle r;

    int min_w;
    int min_h;

    widget * next;
    widget * prev;

    widget * parent;

    widget_appearence_function_set * appearence_functions;
    widget_behaviour_function_set * behaviour_functions;

    uint32_t last_click_time;
}
widget_base;


typedef struct widget_custom
{
    widget_base base;
    void * data;
    int i0;
}
widget_custom;


#include "widgets/container.h"
#include "widgets/box.h"
#include "widgets/button.h"
#include "widgets/enterbox.h"
#include "widgets/sliderbar.h"
#include "widgets/text_bar.h"
#include "widgets/anchor.h"
#include "widgets/textbox.h"
#include "widgets/panel.h"
#include "widgets/resize_constraint.h"
#include "widgets/popup.h"
#include "widgets/tab.h"
#include "widgets/file_search.h"
#include "widgets/contiguous_box.h"


#include "widgets/misc.h"

union widget
{
    widget_base                 base;


    widget_button               button;
    widget_anchor               anchor;
    widget_textbox              textbox;
    widget_enterbox             enterbox;
    widget_sliderbar            sliderbar;
    widget_text_bar             text_bar;

    widget_container            container;
    widget_contiguous_box       contiguous_box;
    widget_panel                panel;
    widget_resize_constraint    resize_constraint;
    widget_popup                popup;
    //widget_file_search          file_search;

    widget_tab_folder           tab_folder;


    widget_custom               custom;
};


widget * create_widget(widget_type type);




bool widget_active(widget * w);
//bool click_within_bounds(rectangle r,int x,int y);///remove????
//bool check_toplevel_container(widget * w);
widget * create_widget_menu(void);

void organise_menu_widget(widget * menu_widget,int screen_width,int screen_height);



//widget * get_widgets_toplevel_ancestor(widget * w);
void organise_toplevel_widget(widget * w);
void move_toplevel_widget_to_front(widget * target);
//widget * get_widgets_menu(widget * w);
void adjust_coordinates_to_widget_local(widget * w,int * x,int * y);
void get_widgets_global_coordinates(widget * w,int * x,int * y);




void render_widget(overlay_data * od,widget * w,int x_off,int y_off,rectangle bounds);
widget * select_widget(widget * w,int x_in,int y_in);
int set_widget_minimum_width(widget * w,uint32_t pos_flags);
int set_widget_minimum_height(widget * w,uint32_t pos_flags);
int organise_widget_horizontally(widget * w,int x_pos,int width);
int organise_widget_vertically(widget * w,int y_pos,int height);



void        blank_widget_render         (overlay_data * od,overlay_theme * theme,widget * w,int x_off,int y_off,rectangle bounds);
widget *    blank_widget_select         (overlay_theme * theme,widget * w,int x_in,int y_in);
void        blank_widget_min_w          (overlay_theme * theme,widget * w);
void        blank_widget_min_h          (overlay_theme * theme,widget * w);
void        blank_widget_set_w          (overlay_theme * theme,widget * w);
void        blank_widget_set_h          (overlay_theme * theme,widget * w);

void        blank_widget_left_click     (overlay_theme * theme,widget * w,int x,int y);
bool        blank_widget_left_release   (overlay_theme * theme,widget * clicked,widget * released,int x,int y);
void        blank_widget_right_click    (overlay_theme * theme,widget * w,int x,int y);
void        blank_widget_mouse_movement (overlay_theme * theme,widget * w,int x,int y);
bool        blank_widget_scroll         (overlay_theme * theme,widget * w,int delta);
bool        blank_widget_key_down       (overlay_theme * theme,widget * w,SDL_Keycode keycode);
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

void render_widget_overlay(overlay_data * od,widget * menu_widget);

bool handle_widget_overlay_left_click(widget * menu_widget,int x_in,int y_in);
bool handle_widget_overlay_left_release(widget * menu_widget,int x_in,int y_in);
bool handle_widget_overlay_movement(widget * menu_widget,int x_in,int y_in);
bool handle_widget_overlay_wheel(widget * menu_widget,int x_in,int y_in,int delta);
bool handle_widget_overlay_keyboard(widget * menu_widget,SDL_Keycode keycode);
bool handle_widget_overlay_text_input(widget * menu_widget,char * text);
bool handle_widget_overlay_text_edit(widget * menu_widget,char * text,int start,int length);

widget * add_child_to_parent(widget * parent,widget * child);
void remove_child_from_parent(widget * child);
void delete_widget(widget * w);






void set_current_overlay_theme(overlay_theme * theme);
//overlay_theme * get_current_overlay_theme(void);

void set_only_interactable_widget(widget * w);
void set_currently_active_widget(widget * w);
//void set_potential_interaction_widget(widget * w);

bool is_currently_active_widget(widget * w);
bool is_potential_interaction_widget(widget * w);
//bool test_currently_active_widget_key_input(void);

void find_potential_interaction_widget(widget * menu_widget,int mouse_x,int mouse_y);

void set_overlay_double_click_time(uint32_t t);
bool check_widget_double_clicked(widget * w);

#endif




