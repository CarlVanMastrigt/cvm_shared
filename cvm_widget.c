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

static uint32_t double_click_time=400;

void set_overlay_double_click_time(uint32_t t)
{
    double_click_time=t;
}

bool check_widget_double_clicked(widget * w)
{
    return (SDL_GetTicks()<(w->base.last_click_time+double_click_time));
}

void render_widget_overlay(overlay_data * od,widget * menu_widget)
{
    render_widget(od,menu_widget,0,0,menu_widget->base.r);
}


static void base_widget_render(overlay_data * od,overlay_theme * theme,widget * w,int x_off,int y_off,rectangle bounds)
{
    puts("error calling base: render");
}

static widget * base_widget_select(overlay_theme * theme,widget * w,int x_in,int y_in)
{
    puts("error calling base: select");
    return NULL;
}

static void base_widget_min_w(overlay_theme * theme,widget * w)
{
    puts("error calling base: min_w");
}

static void base_widget_min_h(overlay_theme * theme,widget * w)
{
    puts("error calling base: min_h");
}

static void base_widget_set_w(overlay_theme * theme,widget * w)
{
    puts("error calling base: size_h");
}

static void base_widget_set_h(overlay_theme * theme,widget * w)
{
    puts("error calling base: set_h");
}


static widget_appearence_function_set base_appearence_functions=
(widget_appearence_function_set)
{
    .render =   base_widget_render,
    .select =   base_widget_select,
    .min_w  =   base_widget_min_w,
    .min_h  =   base_widget_min_h,
    .set_w  =   base_widget_set_w,
    .set_h  =   base_widget_set_h
};



static void base_widget_left_click(overlay_theme * theme,widget * w,int x,int y)
{
    puts("error calling base: l_click");
}

static bool base_widget_left_release(overlay_theme * theme,widget * clicked,widget * released,int x,int y)
{
    puts("error calling base: l_release");
    return false;
}

static void base_widget_right_click(overlay_theme * theme,widget * w,int x,int y)
{
    puts("error calling base: r_click");
}

static void base_widget_mouse_movement(overlay_theme * theme,widget * w,int x,int y)
{
    puts("error calling base: m_move");
}

static bool base_widget_scroll(overlay_theme * theme,widget * w,int delta)
{
    ///this can be called as widgets with no behaviour form part of the search tree

    return false;
}

static bool base_widget_key_down(overlay_theme * theme,widget * w,SDL_Keycode keycode)
{
    puts("error calling base: key_down");
    return false;
}

static void base_widget_click_away(overlay_theme * theme,widget * w)
{
    puts("error calling base: click_away");
}

static void base_widget_add_child(widget * parent,widget * child)
{
    puts("error calling base: add_child");
}

static void base_widget_remove_child(widget * parent,widget * child)
{
    puts("error calling base: remove_child");
}

static void base_widget_delete(widget * w)
{
    puts("error calling base: delete");
    //printf("=> %d\n",w->base.type);
}


static widget_behaviour_function_set base_behaviour_functions =
(widget_behaviour_function_set)
{
    .l_click        =   base_widget_left_click,
    .l_release      =   base_widget_left_release,
    .r_click        =   base_widget_right_click,
    .m_move         =   base_widget_mouse_movement,
    .scroll         =   base_widget_scroll,
    .key_down       =   base_widget_key_down,
    .click_away     =   base_widget_click_away,
    .add_child      =   base_widget_add_child,
    .remove_child   =   base_widget_remove_child,
    .wid_delete     =   base_widget_delete
};

widget * create_widget(widget_type type)
{
    widget * w=malloc(sizeof(widget));

    w->base.status=WIDGET_ACTIVE;
    w->base.type=type;

    w->base.next=NULL;
    w->base.prev=NULL;

    w->base.parent=NULL;

    w->base.r.x=0;
    w->base.r.y=0;

    w->base.r.w=0;
    w->base.r.h=0;

    w->base.min_w=0;
    w->base.min_h=0;

    w->base.last_click_time=0;

    w->base.appearence_functions=&base_appearence_functions;
    w->base.behaviour_functions=&base_behaviour_functions;

    return w;
}


bool widget_active(widget * w)
{
    if(w->base.status&WIDGET_ACTIVE)return true;
    return false;
}

//bool click_within_bounds(rectangle r,int x,int y)
//{
//    return ((x>=r.x)&&(y>=r.y)&&(x<(r.x+r.w))&&(y<(r.y+r.h)));
//}
//
//bool check_toplevel_container(widget * w)
//{
//    if(w->base.parent==NULL)
//    {
//        return true;
//    }
//
//    return false;
//}



//widget * get_widgets_menu(widget * w)
//{
//    while(w->base.parent!=NULL)
//    {
//        w=w->base.parent;
//    }
//
//    return w;
//}

void adjust_coordinates_to_widget_local(widget * w,int * x,int * y)
{
    while(w->base.parent!=NULL)
    {
        *x-=w->base.r.x;
        *y-=w->base.r.y;

        w=w->base.parent;
    }
}

void get_widgets_global_coordinates(widget * w,int * x,int * y)
{
	*x=0;
	*y=0;

    while(w->base.parent!=NULL)
    {
        *x+=w->base.r.x;
        *y+=w->base.r.y;

        w=w->base.parent;
    }
}



static void organise_widget(widget * w,int width,int height)
{
    if(widget_active(w))
    {
        set_widget_minimum_width(w,WIDGET_POS_FLAGS_H);
        organise_widget_horizontally(w,0,width);

        set_widget_minimum_height(w,WIDGET_POS_FLAGS_V);
        organise_widget_vertically(w,0,height);
    }
}

void organise_menu_widget(widget * menu_widget,int screen_width,int screen_height)///for extenal access, like create_menu_widget
{
    organise_widget(menu_widget,screen_width,screen_height);
}

widget * create_widget_menu(void)
{
    widget * w= create_container();
    w->base.status|=WIDGET_IS_MENU;
    return w;
}



static widget * get_widgets_toplevel_ancestor(widget * w)
{
    if((w)&&(w->base.parent))
    {
        while(w->base.parent->base.parent)
        {
            w=w->base.parent;
        }

        if(!(w->base.parent->base.status&WIDGET_IS_MENU))
        {
            puts("COULD NOT FIND APPROPRIATE WIDGET TOPLEVEL ANCESTOR");
            return NULL;
        }

        return w;
    }

    puts("COULD NOT FIND WIDGET TOPLEVEL ANCESTOR");
    return NULL;
}

void organise_toplevel_widget(widget * w)
{
    w=get_widgets_toplevel_ancestor(w);

    if(w) organise_widget(w,w->base.parent->base.r.w,w->base.parent->base.r.h);
}

void move_toplevel_widget_to_front(widget * w)
{
    w=get_widgets_toplevel_ancestor(w);

    if((w==NULL)||(w->base.parent->container.last==w))/// ||(w->base.status & WIDGET_STAYS_AT_BOTTOM)
    {
        return;
    }

    if(w->base.prev!=NULL)
    {
        w->base.prev->base.next=w->base.next;
    }

    if(w->base.next!=NULL)
    {
        w->base.next->base.prev=w->base.prev;
    }

    if(w->base.parent->container.first==w)
    {
        w->base.parent->container.first=w->base.next;
    }

    w->base.parent->container.last->base.next=w;
    w->base.prev=w->base.parent->container.last;
    w->base.parent->container.last=w;
    w->base.next=NULL;
}














void blank_widget_render(overlay_data * od,overlay_theme * theme,widget * w,int x_off,int y_off,rectangle bounds)
{
}

widget * blank_widget_select(overlay_theme * theme,widget * w,int x_in,int y_in)
{
    return NULL;
}

void blank_widget_min_w(overlay_theme * theme,widget * w)
{
}

void blank_widget_min_h(overlay_theme * theme,widget * w)
{
}

void blank_widget_set_w(overlay_theme * theme,widget * w)
{
}

void blank_widget_set_h(overlay_theme * theme,widget * w)
{
}




void blank_widget_left_click(overlay_theme * theme,widget * w,int x,int y)
{
}

bool blank_widget_left_release(overlay_theme * theme,widget * clicked,widget * released,int x,int y)
{
    return false;
}

void blank_widget_right_click(overlay_theme * theme,widget * w,int x,int y)
{
}

void blank_widget_mouse_movement(overlay_theme * theme,widget * w,int x,int y)
{
}

bool blank_widget_scroll(overlay_theme * theme,widget * w,int delta)
{
    return false;
}

bool blank_widget_key_down(overlay_theme * theme,widget * w,SDL_Keycode keycode)
{
    return false;
}

void blank_widget_click_away(overlay_theme * theme,widget * w)
{
}

void blank_widget_add_child(widget * parent,widget * child)
{
    puts("error calling blank: add_child");
}

void blank_widget_remove_child(widget * parent,widget * child)
{
    puts("error calling blank: remove_child");
}

void blank_widget_delete(widget * w)
{
    puts("error calling blank: delete");
}







static overlay_theme * current_theme;
static widget * currently_active_widget=NULL;
static widget * only_interactable_widget=NULL;
static widget * potential_interaction_widget=NULL;


void set_current_overlay_theme(overlay_theme * theme)
{
    current_theme=theme;
}

void set_only_interactable_widget(widget * w)
{
    set_currently_active_widget(NULL);///if triggerd when something is selected
    only_interactable_widget=w;
}

bool is_currently_active_widget(widget * w)
{
    return (w==currently_active_widget);
}

bool is_potential_interaction_widget(widget * w)
{
    return (w==potential_interaction_widget);
}

void set_currently_active_widget(widget * w)
{
    if((currently_active_widget)&&(currently_active_widget!=w))currently_active_widget->base.behaviour_functions->click_away(current_theme,currently_active_widget);
    currently_active_widget=w;
}


void render_widget(overlay_data * od,widget * w,int x_off,int y_off,rectangle bounds)
{
    rectangle r=w->base.r;
    r.x+=x_off;
    r.y+=y_off;

    if((w)&&(w->base.status&WIDGET_ACTIVE)&&(rectangles_overlap(r,bounds)))
    {
        w->base.appearence_functions->render(od,current_theme,w,x_off,y_off,bounds);
    }
}

widget * select_widget(widget * w,int x_in,int y_in)
{
    if((w)&&(w->base.status&WIDGET_ACTIVE)&&(rectangle_surrounds_point(w->base.r,(vec2i){.x=x_in,.y=y_in})))
    {
        return w->base.appearence_functions->select(current_theme,w,x_in,y_in);
    }
    return NULL;
}

int set_widget_minimum_width(widget * w,uint32_t pos_flags)
{
    if((w)&&(w->base.status&WIDGET_ACTIVE))
    {
        w->base.status&=WIDGET_POS_FLAGS_CLEAR;
        w->base.status|=(WIDGET_POS_FLAGS_H&pos_flags);

        w->base.appearence_functions->min_w(current_theme,w);

        return w->base.min_w;
    }
    return 0;
}

int set_widget_minimum_height(widget * w,uint32_t pos_flags)
{
    if((w)&&(w->base.status&WIDGET_ACTIVE))
    {
        w->base.status|=(WIDGET_POS_FLAGS_V&pos_flags);

        w->base.appearence_functions->min_h(current_theme,w);

        return w->base.min_h;
    }
    return 0;
}

int organise_widget_horizontally(widget * w,int x_pos,int width)
{
    if((w)&&(w->base.status&WIDGET_ACTIVE))
    {
        w->base.r.x=x_pos;
        w->base.r.w=width;
        w->base.appearence_functions->set_w(current_theme,w);
        return width;
    }
    return 0;
}

int organise_widget_vertically(widget * w,int y_pos,int height)
{
    if((w)&&(w->base.status&WIDGET_ACTIVE))
    {
        w->base.r.y=y_pos;
        w->base.r.h=height;
        w->base.appearence_functions->set_h(current_theme,w);
        return height;
    }
    return 0;
}



static widget * get_widget_under_mouse(widget * menu_widget,int x_in,int y_in)
{
    widget *current,*tmp;

    if(only_interactable_widget)
    {
        adjust_coordinates_to_widget_local(only_interactable_widget,&x_in,&y_in);
        tmp=select_widget(only_interactable_widget,x_in,y_in);

        if(tmp) return tmp;
        else return only_interactable_widget;
    }

    current=menu_widget->container.last;

    while(current)
    {
        tmp=select_widget(current,x_in,y_in);

        if(tmp)
        {
            //if(move_to_top)move_toplevel_widget_to_front(current);
            return tmp;
        }

        current=current->base.prev;
    }

    return NULL;
}

void find_potential_interaction_widget(widget * menu_widget,int mouse_x,int mouse_y)
{
    potential_interaction_widget=get_widget_under_mouse(menu_widget,mouse_x,mouse_y);
}





bool handle_widget_overlay_left_click(widget * menu_widget,int x_in,int y_in)
{
    widget * w=get_widget_under_mouse(menu_widget,x_in,y_in);

    set_currently_active_widget(w);

    if(w)
    {
        move_toplevel_widget_to_front(w);

        w->base.behaviour_functions->l_click(current_theme,w,x_in,y_in);

        w->base.last_click_time=SDL_GetTicks();
    }

    close_auto_close_popup_tree(w);

    return (w!=NULL);
}

///returns wether this function intercepted (used) the input
bool handle_widget_overlay_left_release(widget * menu_widget,int x_in,int y_in)
{
    if(currently_active_widget==NULL)return false;

    widget * released_upon=get_widget_under_mouse(menu_widget,x_in,y_in);

    if(!currently_active_widget->base.behaviour_functions->l_release(current_theme,currently_active_widget,released_upon,x_in,y_in))
    {
        set_currently_active_widget(NULL);
    }

    return true;
}

bool handle_widget_overlay_movement(widget * menu_widget,int x_in,int y_in)
{
    if((currently_active_widget)&&(SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT)))
    {
        currently_active_widget->base.behaviour_functions->m_move(current_theme,currently_active_widget,x_in,y_in);

        return true;
    }

    return false;
}

bool handle_widget_overlay_wheel(widget * menu_widget,int x_in,int y_in,int delta)
{
    widget * w=get_widget_under_mouse(menu_widget,x_in,y_in);

    if(w==NULL)return false;

    bool finished=false;

    while((w)&&(!finished))
    {
        finished=w->base.behaviour_functions->scroll(current_theme,w,delta);

        w=w->base.parent;
    }

    handle_widget_overlay_movement(menu_widget,x_in,y_in);///to avoid scrollbar/sliderbar etc. changing when currently clicked

    return true;
}

bool handle_widget_overlay_keyboard(widget * menu_widget,SDL_Keycode keycode)
{
    if(currently_active_widget)
    {
        return currently_active_widget->base.behaviour_functions->key_down(current_theme,currently_active_widget,keycode);
    }

    return false;
}





widget * add_child_to_parent(widget * parent,widget * child)
{
    parent->base.behaviour_functions->add_child(parent,child);
    return child;
}

void remove_child_from_parent(widget * child)
{
    widget * parent=child->base.parent;
    if(parent!=NULL)parent->base.behaviour_functions->remove_child(parent,child);
}

void delete_widget(widget * w)
{
    if(w==currently_active_widget)currently_active_widget=NULL;///avoid calling click away when deleting by not using set_currently_active_widget
    if(w==only_interactable_widget)only_interactable_widget=NULL;

    /// link to other widgets in popup.trigger_widget and any button that toggles another widget prevents deletion from being clean (deletion of any widget in this king of structure may cause a segfault)

    remove_child_from_parent(w);
    w->base.behaviour_functions->wid_delete(w);
    free(w);
}


