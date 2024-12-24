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
#include <SDL2/SDL_timer.h>
#include <assert.h>
#include <stdio.h>


/// one level below root
static widget* get_widgets_toplevel_ancestor(widget * w)
{
    if((w)&&(w->base.parent))
    {
        while(w->base.parent->base.parent)
        {
            w=w->base.parent;
        }
    }

    assert(w && w->base.parent && w->base.parent->base.status&WIDGET_IS_ROOT);///COULD NOT FIND APPROPRIATE WIDGET TOPLEVEL ANCESTOR (ROOT)

    return w;
}

widget* find_root_widget(widget * w)
{
    assert(w);
    while(w->base.parent)
    {
        w = w->base.parent;
    }

    assert(w);///COULD NOT FIND APPROPRIATE WIDGET TOPLEVEL ANCESTOR (ROOT)
    // assert(w->base.status&WIDGET_IS_ROOT);///COULD NOT FIND APPROPRIATE WIDGET TOPLEVEL ANCESTOR (ROOT)
    if(!(w->base.status&WIDGET_IS_ROOT))
    {
        return NULL;
    }

    return w;
}



void render_widget_overlay(struct cvm_overlay_render_batch * restrict render_batch, widget * root_widget)
{
    assert(root_widget->base.status&WIDGET_IS_ROOT);
    render_widget(root_widget, root_widget->root.theme, 0, 0, render_batch, root_widget->base.r);
}


static void base_widget_render(overlay_theme * theme,widget * w,int16_t x_off,int16_t y_off,struct cvm_overlay_render_batch * restrict render_batch,rectangle bounds)
{
    puts("error calling base: render");
}

static widget * base_widget_select(overlay_theme * theme,widget * w,int16_t x_in,int16_t y_in)
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

static bool base_widget_key_down(overlay_theme * theme,widget * w,SDL_Keycode keycode,SDL_Keymod mod)
{
    puts("error calling base: key_down");
    return false;
}

static bool base_widget_text_input(overlay_theme * theme,widget * w,char * text)
{
    puts("error calling base: text_input");
    return false;
}

static bool base_widget_text_edit(overlay_theme * theme,widget * w,char * text,int start,int length)
{
    puts("error calling base: text_edit");
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
}


static widget_behaviour_function_set base_behaviour_functions =
{
    .l_click        =   base_widget_left_click,
    .l_release      =   base_widget_left_release,
    .r_click        =   base_widget_right_click,
    .m_move         =   base_widget_mouse_movement,
    .scroll         =   base_widget_scroll,
    .key_down       =   base_widget_key_down,
    .text_input     =   base_widget_text_input,
    .text_edit      =   base_widget_text_edit,
    .click_away     =   base_widget_click_away,
    .add_child      =   base_widget_add_child,
    .remove_child   =   base_widget_remove_child,
    .wid_delete     =   base_widget_delete
};

widget * create_widget(size_t size)
{
    widget * w=malloc(size);

    w->base.status=WIDGET_ACTIVE;

    w->base.next=NULL;
    w->base.prev=NULL;

    w->base.parent=NULL;

    w->base.r=rectangle_ini(0,0,0,0);

    w->base.min_w=0;
    w->base.min_h=0;

    w->base.appearence_functions=&base_appearence_functions;
    w->base.behaviour_functions=&base_behaviour_functions;

    return w;
}


bool widget_active(widget * w)
{
    if(w->base.status&WIDGET_ACTIVE)return true;
    return false;
}



void adjust_coordinates_to_widget_local(widget * w,int * x,int * y)
{
    while(w->base.parent)
    {
        *x-=w->base.r.x1;
        *y-=w->base.r.y1;

        w=w->base.parent;
    }
}

void get_widgets_global_coordinates(widget * w,int * x,int * y)
{
	*x=0;
	*y=0;

    while(w->base.parent)
    {
        w=w->base.parent;

        *x+=w->base.r.x1;
        *y+=w->base.r.y1;
    }
}



static void organise_widget(widget * w, overlay_theme* theme, int width, int height)
{
    if(widget_active(w))
    {
        set_widget_minimum_width(w, theme, WIDGET_POS_FLAGS_H);
        organise_widget_horizontally(w, theme, 0, width);

        set_widget_minimum_height(w, theme, WIDGET_POS_FLAGS_V);
        organise_widget_vertically(w, theme, 0, height);
    }
}

void organise_root_widget(widget * root,int screen_width,int screen_height)
{
    assert(root->base.status&WIDGET_IS_ROOT);
    organise_widget(root, root->root.theme, screen_width, screen_height);

    const rectangle r = root->base.r;
    if(r.x1 != 0 || r.x2 != screen_width || r.y1 != 0 || r.y2 != screen_height)
    {
        fprintf(stderr, "warning: widget could not fit on screen\n>> %d->%d %d->%d vs %d, %d\n",r.x1,r.x2,r.y1,r.y2, screen_width, screen_height);
    }
}



void organise_toplevel_widget(widget * w)
{
    widget* ancestor = get_widgets_toplevel_ancestor(w);
    widget* root = ancestor->base.parent;
    assert(root->base.status&WIDGET_IS_ROOT);

    if(w)
    {
        organise_widget(ancestor, root->root.theme,
            ancestor->base.parent->base.r.x2 - ancestor->base.parent->base.r.x1,
            ancestor->base.parent->base.r.y2 - ancestor->base.parent->base.r.y1);
    }
}

void move_toplevel_widget_to_front(widget * w)
{
    w = get_widgets_toplevel_ancestor(w);

    if((!w)||(w->base.parent->container.last==w))/// ||(w->base.status & WIDGET_STAYS_AT_BOTTOM)
    {
        return;
    }

    if(w->base.prev)
    {
        w->base.prev->base.next=w->base.next;
    }

    if(w->base.next)
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






void blank_widget_render(overlay_theme * theme,widget * w,int16_t x_off,int16_t y_off,struct cvm_overlay_render_batch * restrict render_batch,rectangle bounds)
{
}

widget * blank_widget_select(overlay_theme * theme,widget * w,int16_t x_in,int16_t y_in)
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

bool blank_widget_key_down(overlay_theme * theme,widget * w,SDL_Keycode keycode,SDL_Keymod mod)
{
    return false;
}

bool blank_widget_text_input(overlay_theme * theme,widget * w,char * text)
{
    return false;
}

bool blank_widget_text_edit(overlay_theme * theme,widget * w,char * text,int start,int length)
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


overlay_theme* get_widget_theme(widget* w)
{
    assert(w);
    widget* root = find_root_widget(w);
    return root->root.theme;
}

bool is_currently_active_widget(widget * w)
{
    assert(w);
    widget* root = find_root_widget(w);
    return (w == root->root.currently_active_widget);
}



void set_currently_active_widget(widget * root_widget, widget * w)
{
    assert(!w || w->base.status&WIDGET_ACTIVE);
    assert(root_widget->base.status&WIDGET_IS_ROOT);

    widget* old_active = root_widget->root.currently_active_widget;
    if((old_active) && (old_active != w))
    {
        old_active->base.behaviour_functions->click_away(root_widget->root.theme, old_active);
    }

    root_widget->root.currently_active_widget = w;
}

void set_only_interactable_widget(widget * root_widget, widget * w)
{
    assert(!w || w->base.status&WIDGET_ACTIVE);
    assert(root_widget->base.status&WIDGET_IS_ROOT);

    root_widget->root.only_interactable_widget = w;
}


void render_widget(widget * w, overlay_theme* theme, int x_off, int y_off, struct cvm_overlay_render_batch * restrict render_batch, rectangle bounds)
{
    rectangle r=rectangle_add_offset(w->base.r,x_off,y_off);

    if(w && w->base.status&WIDGET_ACTIVE && rectangles_overlap(r,bounds))
    {
        w->base.appearence_functions->render(theme, w, x_off, y_off, render_batch, bounds);
    }
}

widget * select_widget(widget * w, overlay_theme* theme, int x_in, int y_in)
{
    if(w && w->base.status&WIDGET_ACTIVE && rectangle_surrounds_point(w->base.r,(vec2i){.x=x_in,.y=y_in}))
    {
        return w->base.appearence_functions->select(theme, w, x_in, y_in);
    }
    return NULL;
}

int16_t set_widget_minimum_width(widget * w, overlay_theme* theme, uint32_t pos_flags)
{
    if(w && w->base.status&WIDGET_ACTIVE)
    {
        w->base.status&=WIDGET_POS_FLAGS_CLEAR;
        w->base.status|=(WIDGET_POS_FLAGS_H&pos_flags);

        w->base.appearence_functions->min_w(theme, w);

        return w->base.min_w;
    }
    return 0;
}

int16_t set_widget_minimum_height(widget * w, overlay_theme* theme, uint32_t pos_flags)
{
    if(w && w->base.status&WIDGET_ACTIVE)
    {
        w->base.status|=(WIDGET_POS_FLAGS_V&pos_flags);

        w->base.appearence_functions->min_h(theme, w);

        return w->base.min_h;
    }
    return 0;
}

int16_t organise_widget_horizontally(widget * w, overlay_theme* theme, int16_t x_pos, int16_t width)
{
    if(w && w->base.status&WIDGET_ACTIVE)
    {
        w->base.r.x1=x_pos;
        w->base.r.x2=x_pos+width;
        w->base.appearence_functions->set_w(theme, w);
        return width;
    }
    return 0;
}

int16_t organise_widget_vertically(widget * w, overlay_theme* theme, int16_t y_pos, int16_t height)
{
    if(w && w->base.status&WIDGET_ACTIVE)
    {
        w->base.r.y1=y_pos;
        w->base.r.y2=y_pos+height;
        w->base.appearence_functions->set_h(theme, w);
        return height;
    }
    return 0;
}



static widget * get_widget_under_mouse(widget * root, int x_in, int y_in)
{
    widget *current,*selected;
    overlay_theme* theme;
    assert(root->base.status&WIDGET_IS_ROOT);

    theme = root->root.theme;

    if(root->root.only_interactable_widget)
    {
        adjust_coordinates_to_widget_local(root->root.only_interactable_widget, &x_in, &y_in);
        selected = select_widget(root->root.only_interactable_widget, theme, x_in, y_in);

        if(selected) 
        {
            return selected;
        }
        else
        {
            /// returns base of interactable to ensure no OTHER systems (non-UI) will attempt to capture the input
            #warning consider doing this for other things like key-presses
            return root->root.only_interactable_widget;
        }
    }

    for(current = root->container.last; current; current = current->base.prev)
    {
        selected = select_widget(current, theme, x_in, y_in);

        if(selected)
        {
            //if(move_to_top)move_toplevel_widget_to_front(current);
            return selected;
        }
    }

    return NULL;
}



bool check_widget_double_clicked(widget * w)
{
    assert(w);
    widget* root = find_root_widget(w);

    if(root->root.within_double_click_time)
    {
        return w == root->root.previously_clicked_widget;
    }
}

bool handle_widget_overlay_left_click(widget* root_widget,int x_in,int y_in)
{
    assert(root_widget->base.status&WIDGET_IS_ROOT);
    #warning remove SDL call here!
    uint32_t t = SDL_GetTicks();
    widget * w = get_widget_under_mouse(root_widget, x_in, y_in);

    // consider if this is correct
    set_currently_active_widget(root_widget, w);
    #warning need a step to validate aspects of root widget upon stuff like window change

    if(w)
    {
        /// not the greatest solution
        root_widget->root.within_double_click_time = (t - root_widget->root.previously_clicked_time) < root_widget->root.double_click_time;

        move_toplevel_widget_to_front(w);

        w->base.behaviour_functions->l_click(root_widget->root.theme, w, x_in, y_in);
    }

    close_auto_close_popup_tree(w);

    root_widget->root.previously_clicked_time = t;
    root_widget->root.previously_clicked_widget = w;

    return (w!=NULL);
}

///returns wether this function intercepted (used) the input
bool handle_widget_overlay_left_release(widget* root_widget, int x_in, int y_in)
{
    assert(root_widget->base.status&WIDGET_IS_ROOT);
    widget* active_widget = root_widget->root.currently_active_widget;

    if(active_widget == NULL)
    {
        return false;
    }

    widget * released_upon = get_widget_under_mouse(root_widget, x_in, y_in);

    // release returns whether to ignore this action 
    // (action being releasing on top of the specified widget, usually this is just checking you click and release the same widget)
    if(!active_widget->base.behaviour_functions->l_release(root_widget->root.theme, active_widget, released_upon, x_in, y_in))
    {
        set_currently_active_widget(root_widget, NULL);
    }

    return true;
}

bool handle_widget_overlay_movement(widget * root_widget,int x_in,int y_in)
{
    assert(root_widget->base.status&WIDGET_IS_ROOT);
    widget* active_widget = root_widget->root.currently_active_widget;
    overlay_theme* theme = root_widget->root.theme;

    #warning remove SDL calls from this

    if((active_widget)&&(SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT)))
    {
        active_widget->base.behaviour_functions->m_move(theme, active_widget, x_in, y_in);

        return true;
    }

    return false;
}

bool handle_widget_overlay_wheel(widget * root_widget,int x_in,int y_in,int delta)
{
    assert(root_widget->base.status&WIDGET_IS_ROOT);
    overlay_theme* theme = root_widget->root.theme;

    widget * w = get_widget_under_mouse(root_widget, x_in, y_in);

    if(w==NULL)
    {
        return false;
    }

    bool finished=false;

    while((w)&&(!finished))
    {
        finished=w->base.behaviour_functions->scroll(theme, w, delta);

        w = w->base.parent;
    }

    #warning following is a hack, should be handled by scriollbar internally by checking id currently active widget 
    handle_widget_overlay_movement(root_widget, x_in, y_in);///to avoid scrollbar/slider_bar etc. changing when currently clicked

    return true;
}

bool handle_widget_overlay_keyboard(widget * root_widget, SDL_Keycode keycode, SDL_Keymod mod)
{
    assert(root_widget->base.status&WIDGET_IS_ROOT);
    widget* active_widget = root_widget->root.currently_active_widget;
    overlay_theme* theme = root_widget->root.theme;

    #warning this function should be genericized the most, should handle ACTIONS/COMMANDS instead of specific keys, that way it can handle controller input as well

    if(active_widget)
    {
        return active_widget->base.behaviour_functions->key_down(theme, active_widget, keycode, mod);
    }

    return false;
}

bool handle_widget_overlay_text_input(widget * root_widget, char * text)
{
    assert(root_widget->base.status&WIDGET_IS_ROOT);
    widget* active_widget = root_widget->root.currently_active_widget;
    overlay_theme* theme = root_widget->root.theme;

    if(active_widget)
    {
        return active_widget->base.behaviour_functions->text_input(theme, active_widget, text);
    }

    return false;
}

bool handle_widget_overlay_text_edit(widget * root_widget, char * text, int start, int length)
{
    assert(root_widget->base.status&WIDGET_IS_ROOT);
    widget* active_widget = root_widget->root.currently_active_widget;
    overlay_theme* theme = root_widget->root.theme;

    if(active_widget)
    {
        return active_widget->base.behaviour_functions->text_edit(theme, active_widget, text, start, length);
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

    #warning calling this inside every delete function is fucking terrible! -- there must be a better way
    /// needs to support widgets being moved in and out of existence (either here or delete... dont want root to hold onto invalid reference!)
    /// perhaps require that remove_child_from_parent is ONLY called as part of widget (child) deletion... (not great API choice TBH)
    widget* root_widget = find_root_widget(child);

    if(root_widget)
    {
        #warning could instead track root widget in every widget, at that point having a separate widget context may be better! <-- this
        #warning this is stupid, messy and proably broken as is; require that removing child from parent recursively ensures the contents aren't the currently active widget
        if(child == root_widget->root.currently_active_widget)
        {
            // don't invalidate as that may call click away
            root_widget->root.currently_active_widget = NULL;
        }
        if(child == root_widget->root.only_interactable_widget)
        {
            // don't invalidate as that may call click away
            root_widget->root.only_interactable_widget = NULL;
        }
    }

    if(parent)
    {
        assert(root_widget);

        parent->base.behaviour_functions->remove_child(parent,child);
    }
    child->base.parent=NULL;///there is a lot of duplication of this, remove from all add child instances?
}

void delete_widget(widget * w)
{
    assert(w);

    if(!w)
    {
        fprintf(stderr, "ERROR: deleting NULL widget");
        return;
    }

    assert(!(w->base.status&WIDGET_DO_NOT_DELETE));///should not be trying to delete a widget that has been marked as undeleteable, but handle it anyway
    if(w->base.status&WIDGET_DO_NOT_DELETE)
    {
        return;
    }

    #warning links to other widgets in popup.trigger_widget and any button that toggles another widget prevents deletion from being a clean process! (deletion of any widget in this king of structure may cause a segfault)

    w->base.behaviour_functions->wid_delete(w);

    #warning this will potentially call click_away WHICH IS AN ISSUE IF WE INVALIDATE CURRENTLY CATIVE INSIDE THIS
    remove_child_from_parent(w);

    free(w);
}


