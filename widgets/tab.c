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





static void tab_button_func(widget * button)
{
    widget * page=button->button.data;

    page->base.parent->tab_folder.current_tab_page=page;
}


static void tab_button_widget_render(overlay_data * od,overlay_theme * theme,widget * w,int x_off,int y_off,rectangle bounds)
{
    widget * page=w->button.data;

    if(widget_active(page)) theme->h_text_bar_render(rectangle_add_offset(rectangle_new_conversion(w->base.r),x_off,y_off),w->base.status,theme,od,rectangle_new_conversion(bounds),
        (page->base.parent->tab_folder.current_tab_page==page)?OVERLAY_HIGHLIGHTING_COLOUR:OVERLAY_NO_COLOUR,w->button.text,OVERLAY_TEXT_COLOUR_0_);
}

static widget * tab_button_widget_select(overlay_theme * theme,widget * w,int x_in,int y_in)
{
    if((widget_active(w->button.data))&&(theme->h_bar_select(rectangle_subtract_offset(rectangle_new_conversion(w->base.r),x_in,y_in),w->base.status,theme)))return w;
    return NULL;
}

static void tab_button_widget_min_w(overlay_theme * theme,widget * w)
{
    if(widget_active(w->button.data))w->base.min_w = overlay_size_text_simple(&theme->font_,w->button.text)+2*theme->h_bar_text_offset;//calculate_text_length(theme,w->button.text,0)+2*theme->h_bar_text_offset;
    else w->base.min_w = 0;
}

static void tab_button_widget_min_h(overlay_theme * theme,widget * w)
{
    if(widget_active(w->button.data))w->base.min_h = theme->contiguous_horizintal_bar_h;
    else w->base.min_h = 0;
}

static widget_appearence_function_set tab_button_appearence_functions=
(widget_appearence_function_set)
{
    .render =   tab_button_widget_render,
    .select =   tab_button_widget_select,
    .min_w  =   tab_button_widget_min_w,
    .min_h  =   tab_button_widget_min_h,
    .set_w  =   blank_widget_set_w,
    .set_h  =   blank_widget_set_h
};


static widget * create_tab_button(widget * page_widget,char * page_name)
{
    widget * w=create_text_button(page_name,page_widget,false,tab_button_func);

    w->base.appearence_functions=&tab_button_appearence_functions;

    return w;
}






















static void tab_folder_widget_add_child(widget * w,widget * child)
{
    child->base.next=NULL;
    child->base.prev=w->tab_folder.last;
    child->base.parent=w;

    if(w->tab_folder.last)w->tab_folder.last->base.next=child;
    w->tab_folder.last=child;

    if((w->tab_folder.current_tab_page==NULL)&&(widget_active(child))) w->tab_folder.current_tab_page=child;
}

static void tab_folder_widget_remove_child(widget * w,widget * child)
{
    widget * current;

    if(child->base.parent!=w)
    {
        puts("error: trying to remove child from something not its parent (tab_folder)");
        return;
    }

    if(child==w->tab_folder.current_tab_page)
    {
        current=w->tab_folder.last;

        while(current)
        {
            if((current!=child)&&(widget_active(current))) break;

            current=w->base.prev;
        }

        w->tab_folder.current_tab_page=current;///is possible that this is null if all inactive, is that okay?? disallows removal of inactive, need first in stack???
    }

    if(child==w->tab_folder.last)w->tab_folder.last=child->base.prev;
    if(child->base.prev)child->base.prev->base.next=child->base.next;
    if(child->base.next)child->base.next->base.prev=child->base.prev;

    child->base.parent=NULL;
}

static void tab_folder_widget_delete(widget * w)
{
    widget *current,*prev;

    current=w->tab_folder.last;

    while(current)
    {
        prev=current->base.prev;

        delete_widget(current);

        current=prev;
    }
}

static widget_behaviour_function_set tab_folder_behaviour_functions=
(widget_behaviour_function_set)
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
    .add_child      =   tab_folder_widget_add_child,
    .remove_child   =   tab_folder_widget_remove_child,
    .wid_delete     =   tab_folder_widget_delete
};











static void tab_folder_widget_render(overlay_data * od,overlay_theme * theme,widget * w,int x_off,int y_off,rectangle bounds)
{
    if(w->tab_folder.current_tab_page)render_widget(od,w->tab_folder.current_tab_page,x_off+w->base.r.x,y_off+w->base.r.y,bounds);
}

static widget * tab_folder_widget_select(overlay_theme * theme,widget * w,int x_in,int y_in)
{
    if(w->tab_folder.current_tab_page)return select_widget(w->tab_folder.current_tab_page,x_in-w->base.r.x,y_in-w->base.r.y);
    return NULL;
}

static void tab_folder_widget_min_w(overlay_theme * theme,widget * w)
{
    w->base.min_w=0;

    widget * current=w->tab_folder.last;

    while(current)
    {
        set_widget_minimum_width(current,w->base.status&WIDGET_POS_FLAGS_H);
        if(current->base.min_w > w->base.min_w) w->base.min_w = current->base.min_w;
        current=current->base.prev;
    }

    ///set currently active widget if null or not active
    if((w->tab_folder.current_tab_page==NULL)||(!widget_active(w->tab_folder.current_tab_page)))
    {
        current=w->tab_folder.last;

        while(current)
        {
            if(widget_active(current)) break;

            current=w->base.prev;
        }

        w->tab_folder.current_tab_page=current;
    }
}

static void tab_folder_widget_min_h(overlay_theme * theme,widget * w)
{
    w->base.min_h=0;

    widget * current=w->tab_folder.last;

    while(current)
    {
        set_widget_minimum_height(current,w->base.status&WIDGET_POS_FLAGS_V);
        if(current->base.min_h > w->base.min_h) w->base.min_h = current->base.min_h;
        current=current->base.prev;
    }
}

static void tab_folder_widget_set_w(overlay_theme * theme,widget * w)
{
    widget * current=w->tab_folder.last;

    while(current)
    {
        organise_widget_horizontally(current,0,w->base.r.w);
        current=current->base.prev;
    }
}

static void tab_folder_widget_set_h(overlay_theme * theme,widget * w)
{
    widget * current=w->tab_folder.last;

    while(current)
    {
        organise_widget_vertically(current,0,w->base.r.h);

        current=current->base.prev;
    }
}

static widget_appearence_function_set tab_folder_appearence_functions=
(widget_appearence_function_set)
{
    .render =   tab_folder_widget_render,
    .select =   tab_folder_widget_select,
    .min_w  =   tab_folder_widget_min_w,
    .min_h  =   tab_folder_widget_min_h,
    .set_w  =   tab_folder_widget_set_w,
    .set_h  =   tab_folder_widget_set_h
};



widget * create_tab_folder(widget ** button_box,widget_layout button_box_layout)
{
    widget * w=create_widget(TAB_FOLDER_WIDGET);

    *button_box=w->tab_folder.tab_button_container=create_contiguous_box(button_box_layout,0);

    w->tab_folder.current_tab_page=NULL;
    w->tab_folder.last=NULL;

    w->base.appearence_functions=&tab_folder_appearence_functions;
    w->base.behaviour_functions=&tab_folder_behaviour_functions;

    return w;
}


widget * create_tab_page(widget * folder,char * title,widget * page_widget)
{
    add_child_to_parent(folder->tab_folder.tab_button_container,create_tab_button(page_widget,title));

    add_child_to_parent(folder,page_widget);

    return page_widget;
}




#warning have more generic add_tab page, that provides button (of any sub-type) rather than creating one in a specified location

#warning possibly unnecessary
widget * create_vertical_tab_pair_box(widget ** tab_folder)///if kept, also create horizontal version
{
    widget *box_0,*box_1,*buttons;

    box_0=create_box(WIDGET_VERTICAL,WIDGET_LAST_DISTRIBUTED);
    box_1=add_child_to_parent(box_0,create_box(WIDGET_HORIZONTAL,WIDGET_LAST_DISTRIBUTED));
    add_child_to_parent(box_0,create_separator_widget());
    *tab_folder=add_child_to_parent(box_0,create_tab_folder(&buttons,WIDGET_HORIZONTAL));
    add_child_to_parent(box_1,buttons);
    add_child_to_parent(box_1,create_empty_widget(0,0));

    return box_0;
}



