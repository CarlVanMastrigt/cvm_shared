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





static bool textbox_widget_scroll(overlay_theme * theme,widget * w,int delta)
{
    w->textbox.y_offset+=delta*w->textbox.wheel_delta;/// 10 = scroll_factor

    if(w->textbox.y_offset > w->textbox.max_offset) w->textbox.y_offset=w->textbox.max_offset;
    if(w->textbox.y_offset < 0) w->textbox.y_offset=0;

    return true;
}

static void textbox_widget_left_click(overlay_theme * theme,widget * w,int x,int y)
{
    adjust_coordinates_to_widget_local(w,&x,&y);
    w->textbox.selection_end=w->textbox.selection_begin=overlay_find_multiline_text_offset(&theme->font_,&w->textbox.text_block,w->textbox.text,
        x-theme->x_box_offset-w->textbox.x_offset,y-theme->y_box_offset-w->textbox.y_offset);
}

#warning use again with return true; if textbox allows selecting/copying again
static bool textbox_widget_left_release(overlay_theme * theme,widget * clicked,widget * released,int x,int y)
{
    return true;
}

#warning possibly have popup copy/paste for here and enterbox
/*static void textbox_widget_right_click(overlay_theme * theme,widget * w,int x,int y)
{
}*/

static void textbox_widget_mouse_movement(overlay_theme * theme,widget * w,int x,int y)
{
    adjust_coordinates_to_widget_local(w,&x,&y);
    w->textbox.selection_end=overlay_find_multiline_text_offset(&theme->font_,&w->textbox.text_block,w->textbox.text,
        x-theme->x_box_offset-w->textbox.x_offset,y-theme->y_box_offset-w->textbox.y_offset);
}

static void textbox_copy_selection_to_clipboard(widget * w)
{
//    int first,last;
//    char tmp;
//
//    if(w->textbox.selection_begin!=w->textbox.selection_end)
//    {
//        if(w->textbox.selection_end > w->textbox.selection_begin)
//        {
//            first   =w->textbox.selection_begin;
//            last    =w->textbox.selection_end;
//        }
//        else
//        {
//            first   =w->textbox.selection_end;
//            last    =w->textbox.selection_begin;
//        }
//
//        tmp=w->textbox.text[last];
//        w->textbox.text[last]='\0';
//
//        SDL_SetClipboardText(w->textbox.text+first);
//
//        w->textbox.text[last]=tmp;
//    }
}

static bool textbox_widget_key_down(overlay_theme * theme,widget * w,SDL_Keycode keycode,SDL_Keymod mod)
{
//    SDL_Keymod mod=SDL_GetModState();
//
//    //bool caps=((mod&KMOD_CAPS)==KMOD_CAPS);
//    //bool shift=(((mod&KMOD_RSHIFT)==KMOD_RSHIFT)||((mod&KMOD_LSHIFT)==KMOD_LSHIFT));
//
//    #warning have last input time here
//
//
//    if(mod&KMOD_CTRL)
//    {
//        switch(keycode)
//        {
//            case 'c':
//            textbox_copy_selection_to_clipboard(w);
//            break;
//
//            case 'x':
//            textbox_copy_selection_to_clipboard(w);
//            break;
//
//            case 'a':
//            w->textbox.selection_begin=0;
//            w->textbox.selection_end=strlen(w->textbox.text);
//            break;
//
//            default: return false;
//        }
//        return true;
//    }
//
    return false;
}

static void textbox_widget_delete(widget * w)
{
    free(w->textbox.text);
    free(w->textbox.text_block.lines);
}

static widget_behaviour_function_set textbox_behaviour_functions=
(widget_behaviour_function_set)
{
    .l_click        =   textbox_widget_left_click,
    .l_release      =   textbox_widget_left_release,
    .r_click        =   blank_widget_right_click,
    .m_move         =   textbox_widget_mouse_movement,
    .scroll         =   textbox_widget_scroll,
    .key_down       =   textbox_widget_key_down,
    .text_input     =   blank_widget_text_input,
    .text_edit      =   blank_widget_text_edit,
    .click_away     =   blank_widget_click_away,
    .add_child      =   blank_widget_add_child,
    .remove_child   =   blank_widget_remove_child,
    .wid_delete     =   textbox_widget_delete
};









static void textbox_widget_render(overlay_theme * theme,widget * w,int x_off,int y_off,cvm_overlay_element_render_buffer * erb,rectangle bounds)
{
    char *sb,*se;

    rectangle r=rectangle_add_offset(w->base.r,x_off,y_off);
	theme->box_render(r,w->base.status,theme,erb,bounds,OVERLAY_MAIN_COLOUR_);

	r.x1+=theme->x_box_offset;
	r.x2-=theme->x_box_offset;
	r.y1+=theme->y_box_offset;
	r.y2-=theme->y_box_offset;

    rectangle b=get_rectangle_overlap(r,bounds);
    if(rectangle_has_positive_area(b))
    {
        if(w->textbox.selection_begin==w->textbox.selection_end)overlay_render_multiline_text(erb,&theme->font_,&w->textbox.text_block,r.x1,r.y1-w->textbox.y_offset,b,OVERLAY_TEXT_COLOUR_0_);
        else
        {
            if(w->textbox.selection_end > w->textbox.selection_begin) sb=w->textbox.text+w->textbox.selection_begin, se=w->textbox.text+w->textbox.selection_end;
            else sb=w->textbox.text+w->textbox.selection_end, se=w->textbox.text+w->textbox.selection_begin;

            overlay_render_multiline_text_selection(erb,&theme->font_,&w->textbox.text_block,r.x1,r.y1-w->textbox.y_offset,b,OVERLAY_TEXT_COLOUR_0_,sb,se);
        }
    }
}

static widget * textbox_widget_select(overlay_theme * theme,widget * w,int x_in,int y_in)
{
    if(theme->box_select(rectangle_subtract_offset(w->base.r,x_in,y_in),w->base.status,theme))return w;
    return NULL;
}

static void textbox_widget_min_w(overlay_theme * theme,widget * w)
{
    w->base.min_w=w->textbox.min_horizontal_glyphs * theme->font_.max_advance + 2*theme->x_box_offset;
}

static void textbox_widget_min_h(overlay_theme * theme,widget * w)
{
    overlay_process_multiline_text(&theme->font_,&w->textbox.text_block,w->textbox.text,w->base.r.x2-w->base.r.x1-2*theme->x_box_offset);

    if(w->textbox.min_visible_lines==0)
    {
        w->base.min_h=(w->textbox.text_block.line_count-1)*theme->font_.line_spacing+theme->font_.glyph_size+theme->y_box_offset*2;
    }
    else
    {
        w->base.min_h=(w->textbox.min_visible_lines-1)*theme->font_.line_spacing+theme->font_.glyph_size+theme->y_box_offset*2;
    }
}



static void textbox_widget_set_h(overlay_theme * theme,widget * w)
{
    w->textbox.wheel_delta= -theme->font_.line_spacing;///w->textbox.font_index
    w->textbox.visible_size= w->base.r.y2-w->base.r.y1-2*theme->y_box_offset;

    if(w->textbox.min_visible_lines)
    {
        w->textbox.max_offset=(w->textbox.text_block.line_count-1)*theme->font_.line_spacing+theme->font_.glyph_size - w->textbox.visible_size;
        if(w->textbox.max_offset < 0)w->textbox.max_offset=0;
        if(w->textbox.y_offset > w->textbox.max_offset)w->textbox.y_offset = w->textbox.max_offset;
    }
}


static widget_appearence_function_set textbox_appearence_functions=
(widget_appearence_function_set)
{
    .render =   textbox_widget_render,
    .select =   textbox_widget_select,
    .min_w  =   textbox_widget_min_w,
    .min_h  =   textbox_widget_min_h,
    .set_w  =   blank_widget_set_w,
    .set_h  =   textbox_widget_set_h
};








void change_textbox_text(widget * w,char * new_text,bool owns_new_text)///use responsibly, potentially need to reorganise widgets function as part of (or after) calling this
{
    free(w->textbox.text);

    if(owns_new_text) w->textbox.text=new_text;
    else w->textbox.text=strdup(new_text);

    w->textbox.y_offset=0;
    w->textbox.text_block.line_count=0;
    w->textbox.selection_begin=0;
    w->textbox.selection_end=0;

    if(w->textbox.min_visible_lines)
    {
        overlay_theme * ot=get_current_overlay_theme();
        textbox_widget_min_h(ot,w);
        textbox_widget_set_h(ot,w);
    }
    else
    {
        organise_toplevel_widget(w);
    }
}

widget * create_textbox(char * text,bool owns_text,int min_horizontal_glyphs,int min_visible_lines)///enable wrapping as input param? forces sizing and allows horizontal scrolling otherwise
{
    widget * w = create_widget(TEXTBOX_WIDGET);

    w->base.appearence_functions = &textbox_appearence_functions;
    w->base.behaviour_functions = &textbox_behaviour_functions;

    if(owns_text) w->textbox.text=text;
    else w->textbox.text=strdup(text);

    w->textbox.text_block.lines=malloc(sizeof(cvm_overlay_text_line)*16);
    w->textbox.text_block.line_space=16;
    w->textbox.text_block.line_count=0;

    w->textbox.min_visible_lines = min_visible_lines;///0=show all

    w->textbox.selection_begin = w->textbox.selection_end = 0;

    w->textbox.min_horizontal_glyphs=min_horizontal_glyphs;

    w->textbox.x_offset=0;
    w->textbox.y_offset=0;
    w->textbox.max_offset=0;
    w->textbox.visible_size=0;
    w->textbox.wheel_delta=0;

    return w;
}


widget * create_textbox_scrollbar(widget * textbox)
{
    widget * w=create_slider_bar(&textbox->textbox.y_offset,0,0,NULL,NULL,false,0);
    #warning replace above with adjacent slider
    set_slider_bar_other_values(w,NULL,&textbox->textbox.max_offset,&textbox->textbox.visible_size,&textbox->textbox.wheel_delta);

    return w;
}

