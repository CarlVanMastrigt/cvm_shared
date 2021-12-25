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

void blank_enterbox_function(widget * w)
{
    printf("blank enterbox: %s\n",w->enterbox.text);
}

static inline void enterbox_delete_selection(overlay_theme * theme,widget * w,char * s_begin,char * s_end)
{
    if(s_begin>s_end)
    {
        fprintf(stderr,"ATTEMPTING TO DELETE INVERTED SELECTION FROM ENTERBOX\n");
        exit(-1);
    }

    if(s_begin!=s_end)
    {
        memmove(s_begin,s_end,strlen(s_end) + 1);/// +1 for null terminating character
    }

    w->enterbox.text_pixel_length=overlay_text_single_line_get_pixel_length(&theme->font_,w->enterbox.text);
}

static void enterbox_copy_selection_to_clipboard(widget * w)
{
    char *s_begin,*s_end;
    char tmp;

    if(w->enterbox.selection_end > w->enterbox.selection_begin) s_begin=w->enterbox.selection_begin, s_end=w->enterbox.selection_end;
    else s_begin=w->enterbox.selection_end, s_end=w->enterbox.selection_begin;


    /// copy all contents if nothing selected?
    if(s_begin!=s_end)
    {
        tmp=*s_end;
        *s_end='\0';

        SDL_SetClipboardText(s_begin);

        *s_end=tmp;
    }
    else if(*w->enterbox.text)///not empty
    {
        SDL_SetClipboardText(w->enterbox.text);
    }
}

/// perhaps return pass/fail such that enterbox can flash/fade a colour upon failure
static void enterbox_enter_text(overlay_theme * theme,widget * w,char * text)
{
    char *s_begin,*s_end;
    int new_glyph_count,new_strlen;

    if(w->enterbox.selection_end > w->enterbox.selection_begin) s_begin=w->enterbox.selection_begin, s_end=w->enterbox.selection_end;
    else s_begin=w->enterbox.selection_end, s_end=w->enterbox.selection_begin;

    if( text && (new_strlen=strlen(text)) > 0 && cvm_overlay_utf8_validate_string_and_count_glyphs(text,&new_glyph_count) &&
        ///new text exists, isnt empty and is valid unicode (also get glyph count of input text)
        cvm_overlay_utf8_count_glyphs_outside_range(w->enterbox.text,s_begin,s_end) + new_glyph_count <= w->enterbox.max_glyphs &&
        /// adding the new glyphs while replacing the old ones wont exceed limit
        strlen(w->enterbox.text) + s_begin - s_end + new_strlen <= w->enterbox.max_strlen)
        /// adding the new chars while replacing the old ones wont exceed limit
    {
        memmove(s_begin + new_strlen,s_end,strlen(s_end) + 1);/// +1 for null terminating character
        memcpy(s_begin,text,new_strlen);
        w->enterbox.selection_begin = w->enterbox.selection_end = s_begin + new_strlen;
    }

    w->enterbox.text_pixel_length=overlay_text_single_line_get_pixel_length(&theme->font_,w->enterbox.text);
}

static void enterbox_check_visible_offset(widget * w,overlay_theme * theme)
{
    char tmp;
    int current_offset,text_space;

    text_space=w->base.r.x2-w->base.r.x1-2*theme->h_bar_text_offset-1;

    tmp=*w->enterbox.selection_end;
    *w->enterbox.selection_end='\0';

    current_offset=overlay_text_single_line_get_pixel_length(&theme->font_,w->enterbox.text);

    *w->enterbox.selection_end=tmp;

//    if(w->enterbox.visible_offset>current_offset) w->enterbox.visible_offset=current_offset;
//    if(w->enterbox.visible_offset+text_space<current_offset) w->enterbox.visible_offset=current_offset-text_space;

    if(w->enterbox.visible_offset>current_offset-theme->h_text_fade_range) w->enterbox.visible_offset=current_offset-theme->h_text_fade_range;
    if(w->enterbox.visible_offset<current_offset-text_space+theme->h_text_fade_range) w->enterbox.visible_offset=current_offset-text_space+theme->h_text_fade_range;

    ///special condition to put offset at middle???
    if(w->enterbox.visible_offset<0)w->enterbox.visible_offset=0;
    current_offset=w->enterbox.text_pixel_length-text_space;///max offset
    if(current_offset < 0) w->enterbox.visible_offset=0;
    else if(w->enterbox.visible_offset > current_offset) w->enterbox.visible_offset=current_offset;
}


static bool enterbox_widget_key_down(overlay_theme * theme,widget * w,SDL_Keycode keycode,SDL_Keymod mod)
{
    char *s_begin,*s_end,*s;

    if(w->enterbox.selection_end > w->enterbox.selection_begin) s_begin=w->enterbox.selection_begin, s_end=w->enterbox.selection_end;
    else s_begin=w->enterbox.selection_end, s_end=w->enterbox.selection_begin;

    switch(keycode)
    {
    case SDLK_c:
        if(mod&KMOD_CTRL)
        {
            enterbox_copy_selection_to_clipboard(w);
        }
        break;

    case SDLK_x:
        if(mod&KMOD_CTRL)
        {
            enterbox_copy_selection_to_clipboard(w);
            enterbox_delete_selection(theme,w,s_begin,s_end);
            w->enterbox.selection_begin = w->enterbox.selection_end = s_begin;
        }
        break;

    case SDLK_v:
        if(mod&KMOD_CTRL)
        {
            if(SDL_HasClipboardText())
            {
                enterbox_enter_text(theme,w,SDL_GetClipboardText());
            }
        }
        break;

    case SDLK_a:
        if(mod&KMOD_CTRL)
        {
            w->enterbox.selection_begin=w->enterbox.text;
            w->enterbox.selection_end=w->enterbox.text+strlen(w->enterbox.text);
        }
        break;

    case SDLK_BACKSPACE:
        if(mod&KMOD_CTRL)
        {
            if(mod&KMOD_SHIFT)s=w->textbox.text;
            else s=cvm_overlay_utf8_get_previous_word(w->enterbox.text,w->enterbox.selection_end);

            if(w->enterbox.selection_begin > w->enterbox.selection_end) w->enterbox.selection_begin-=w->enterbox.selection_end-s;
            else if(w->enterbox.selection_begin > s) w->enterbox.selection_begin=s;

            enterbox_delete_selection(theme,w,s,w->enterbox.selection_end);

            w->enterbox.selection_end=s;
        }
        else
        {
            if(s_begin==s_end) s_begin=cvm_overlay_utf8_get_previous_glyph(w->enterbox.text,s_begin);
            enterbox_delete_selection(theme,w,s_begin,s_end);
            w->enterbox.selection_begin = w->enterbox.selection_end = s_begin;
        }
        break;

    case SDLK_DELETE:
        if(mod&KMOD_CTRL)
        {
            if(mod&KMOD_SHIFT)s=w->enterbox.text+strlen(w->enterbox.text);
            else s=cvm_overlay_utf8_get_next_word(w->enterbox.selection_end);

            if(w->enterbox.selection_begin > w->enterbox.selection_end)
            {
                if(w->enterbox.selection_begin > s) w->enterbox.selection_begin -= s-w->enterbox.selection_end;
                else w->enterbox.selection_begin = w->enterbox.selection_end;
            }

            enterbox_delete_selection(theme,w,w->enterbox.selection_end,s);
        }
        else
        {
            if(s_begin==s_end) s_end=cvm_overlay_utf8_get_next_glyph(s_end);
            enterbox_delete_selection(theme,w,s_begin,s_end);
            w->enterbox.selection_begin = w->enterbox.selection_end = s_begin;
        }
        break;

    case SDLK_LEFT:
        if(mod&KMOD_CTRL)///can move based on ctrl (jump word),  then set to same based on shift
        {
            if(mod&KMOD_SHIFT) w->enterbox.selection_end = cvm_overlay_utf8_get_previous_word(w->enterbox.text,w->enterbox.selection_end);
            else w->enterbox.selection_begin = w->enterbox.selection_end = cvm_overlay_utf8_get_previous_word(w->enterbox.text,w->enterbox.selection_end);
        }
        else if(mod&KMOD_SHIFT)w->enterbox.selection_end = cvm_overlay_utf8_get_previous_glyph(w->enterbox.text,w->enterbox.selection_end);
        else if(s_begin==s_end) w->enterbox.selection_begin = w->enterbox.selection_end = cvm_overlay_utf8_get_previous_glyph(w->enterbox.text,s_begin);
        else w->enterbox.selection_begin = w->enterbox.selection_end = s_begin;
        break;

    case SDLK_RIGHT:
        if(mod&KMOD_CTRL)
        {
            if(mod&KMOD_SHIFT) w->enterbox.selection_end = cvm_overlay_utf8_get_next_word(w->enterbox.selection_end);
            else w->enterbox.selection_begin = w->enterbox.selection_end = cvm_overlay_utf8_get_next_word(w->enterbox.selection_end);
        }
        else if(mod&KMOD_SHIFT) w->enterbox.selection_end = cvm_overlay_utf8_get_next_glyph(w->enterbox.selection_end);
        else if(s_begin==s_end) w->enterbox.selection_begin = w->enterbox.selection_end = cvm_overlay_utf8_get_next_glyph(s_end);
        else w->enterbox.selection_begin = w->enterbox.selection_end = s_end;
        break;

    case SDLK_UP:
    case SDLK_HOME:
        if(mod&KMOD_SHIFT) w->enterbox.selection_end = w->enterbox.text;
        else w->enterbox.selection_begin = w->enterbox.selection_end = w->enterbox.text;
        break;

    case SDLK_DOWN:
    case SDLK_END:
        if(mod&KMOD_SHIFT) w->enterbox.selection_end = w->enterbox.text+strlen(w->enterbox.text);
        else w->enterbox.selection_begin = w->enterbox.selection_end = w->enterbox.text+strlen(w->enterbox.text);
        break;

    case SDLK_RETURN:
        if((w->enterbox.activation_func)&&(!w->enterbox.activate_upon_deselect))
        {
            w->enterbox.activation_func(w);
        }
        set_currently_active_widget(NULL);
        break;

    case SDLK_ESCAPE:
        #warning probably dont want escape to ever activate, or at least not most of the time, ay wat to accomplish this, should it be the same as clicking away?
        set_currently_active_widget(NULL);
        break;

        default:;
    }

    enterbox_check_visible_offset(w,theme);

    return true;
}

#warning SDL_SystemCursor

static void enterbox_widget_left_click(overlay_theme * theme,widget * w,int x,int y)
{
    if(!SDL_IsTextInputActive()) SDL_StartTextInput();///need to only call if not already active...

    if(*w->enterbox.composition_text)return;

    adjust_coordinates_to_widget_local(w,&x,&y);
    w->enterbox.selection_begin=w->enterbox.selection_end=overlay_text_single_line_find_offset(&theme->font_,w->enterbox.text,x-theme->h_bar_text_offset+w->enterbox.visible_offset);

    if(w->enterbox.upon_input!=NULL)
    {
        w->enterbox.upon_input(w);
    }
}

static bool enterbox_widget_left_release(overlay_theme * theme,widget * clicked,widget * released,int x,int y)
{
    /// should also find test position to set start/end of selection with... (actually move does this, which makes more sense)
    return true;
}

static void enterbox_widget_mouse_movement(overlay_theme * theme,widget * w,int x,int y)
{
    if(*w->enterbox.composition_text)return;

    adjust_coordinates_to_widget_local(w,&x,&y);
    w->enterbox.selection_end=overlay_text_single_line_find_offset(&theme->font_,w->enterbox.text,x-theme->h_bar_text_offset+w->enterbox.visible_offset);

    enterbox_check_visible_offset(w,theme);
}

static bool enterbox_widget_text_input(overlay_theme * theme,widget * w,char * text)
{
    enterbox_enter_text(theme,w,text);
    enterbox_check_visible_offset(w,theme);
    return true;
}

static bool enterbox_widget_text_edit(overlay_theme * theme,widget * w,char * text,int start,int length)
{
    strncpy(w->enterbox.composition_text,text,CVM_OVERLAY_MAX_COMPOSITION_BYTES);
    w->enterbox.composition_text[CVM_OVERLAY_MAX_COMPOSITION_BYTES-1]='\0';

    ///same visiblity checking as enterbox_check_visible_offset, but only needed here, and applying to specific vars so just in place code it
    int current_offset,text_space;

    text_space=w->base.r.x2-w->base.r.x1-2*theme->h_bar_text_offset-1;

    current_offset=overlay_text_single_line_get_pixel_length(&theme->font_,w->enterbox.composition_text);

    if(w->enterbox.composition_visible_offset>current_offset) w->enterbox.composition_visible_offset=current_offset;
    if(w->enterbox.composition_visible_offset+text_space<current_offset) w->enterbox.composition_visible_offset=current_offset-text_space;
    current_offset-=text_space;///max offset
    if(current_offset < 0) w->enterbox.composition_visible_offset=0;
    else if(w->enterbox.composition_visible_offset > current_offset) w->enterbox.composition_visible_offset=current_offset;

    return true;
}

static void enterbox_widget_click_away(overlay_theme * theme,widget * w)
{
    if(SDL_IsTextInputActive())SDL_StopTextInput();

    if((w->enterbox.activation_func)&&(w->enterbox.activate_upon_deselect))
    {
        w->enterbox.activation_func(w);
    }
}

static void enterbox_widget_delete(widget * w)
{
    free(w->enterbox.text);
    if(w->enterbox.free_data)free(w->enterbox.data);
}

static widget_behaviour_function_set enterbox_behaviour_functions=
(widget_behaviour_function_set)
{
    .l_click        =   enterbox_widget_left_click,
    .l_release      =   enterbox_widget_left_release,
    .r_click        =   blank_widget_right_click,
    .m_move         =   enterbox_widget_mouse_movement,
    .scroll         =   blank_widget_scroll,
    .key_down       =   enterbox_widget_key_down,
    .text_input     =   enterbox_widget_text_input,
    .text_edit      =   enterbox_widget_text_edit,
    .click_away     =   enterbox_widget_click_away,
    .add_child      =   blank_widget_add_child,
    .remove_child   =   blank_widget_remove_child,
    .wid_delete     =   enterbox_widget_delete
};

static void enterbox_widget_render(overlay_theme * theme,widget * w,int x_off,int y_off,cvm_overlay_element_render_buffer * erb,rectangle bounds)
{
    if(w->enterbox.update_contents_func && !is_currently_active_widget(w))w->enterbox.update_contents_func(w);

    if(w->enterbox.recalculate_text_size)
    {
        w->enterbox.text_pixel_length=overlay_text_single_line_get_pixel_length(&theme->font_,w->enterbox.text);
        w->enterbox.recalculate_text_size=false;
    }

    overlay_text_single_line_render_data otslrd;
    otslrd.flags=OVERLAY_TEXT_NORMAL_RENDER;
    otslrd.erb=erb;
    otslrd.theme=theme;
    otslrd.bounds=bounds;


	if(*w->enterbox.composition_text)
    {
        otslrd.colour=OVERLAY_TEXT_COMPOSITION_COLOUR_0_;
        otslrd.text=w->enterbox.composition_text;
        otslrd.selection_begin=otslrd.selection_end=otslrd.text+strlen(otslrd.text);
        otslrd.x=-w->enterbox.composition_visible_offset;
    }
    else
    {
        otslrd.colour=OVERLAY_TEXT_COLOUR_0_;
        otslrd.text=w->enterbox.text;
        if(w->enterbox.selection_end > w->enterbox.selection_begin) otslrd.selection_begin=w->enterbox.selection_begin, otslrd.selection_end=w->enterbox.selection_end;
        else otslrd.selection_begin=w->enterbox.selection_end, otslrd.selection_end=w->enterbox.selection_begin;
        otslrd.x=-w->enterbox.visible_offset;
    }

	rectangle r=rectangle_add_offset(w->base.r,x_off,y_off);
	theme->h_bar_render(erb,theme,bounds,r,w->base.status,OVERLAY_MAIN_COLOUR_);

    r=overlay_text_single_line_get_text_area(r,theme->font_.glyph_size,theme->h_bar_text_offset);
    otslrd.x+=r.x1;
    otslrd.y=r.y1;

    #warning make is_currently_active_widget effectively part of w->base.status, would require setting flags in slightly painful manner but w/e
    if(is_currently_active_widget(w))
    {
        otslrd.flags|=OVERLAY_TEXT_SELECTED_RENDER;
    }

    if(w->enterbox.min_glyphs_visible<w->enterbox.max_glyphs)
    {
        otslrd.flags|=OVERLAY_TEXT_FADING_RENDER;
        otslrd.text_area=r;
        otslrd.text_length=w->enterbox.text_pixel_length;
    }

    overlay_text_single_line_render(&otslrd);

//    if(w->enterbox.min_glyphs_visible<w->enterbox.max_glyphs)
//    {
//        if(is_currently_active_widget(w))overlay_text_single_line_fading_selection_render(erb,&theme->font_,bounds,text,r.x1-visible_offset,r.y1,colour,r,theme->h_text_fade_range,w->enterbox.text_pixel_length,sb,se);
//        else overlay_text_single_line_fading_render(erb,&theme->font_,bounds,text,r.x1-visible_offset,r.y1,colour,r,theme->h_text_fade_range,w->enterbox.text_pixel_length);
//    }
//    else
//    {
//        if(is_currently_active_widget(w))overlay_text_single_line_selection_render(erb,&theme->font_,bounds,text,r.x1-visible_offset,r.y1,colour,sb,se);
//        else overlay_text_single_line_render(erb,&theme->font_,bounds,text,r.x1-visible_offset,r.y1,colour);
//    }
}

static widget * enterbox_widget_select(overlay_theme * theme,widget * w,int x_in,int y_in)
{
    if(theme->h_bar_select(theme,rectangle_subtract_offset(w->base.r,x_in,y_in),w->base.status))return w;

    return NULL;
}

static void enterbox_widget_min_w(overlay_theme * theme,widget * w)
{
    w->enterbox.text_pixel_length=overlay_text_single_line_get_pixel_length(&theme->font_,w->enterbox.text);
    w->base.min_w = 2*theme->h_bar_text_offset+theme->font_.max_advance*w->enterbox.min_glyphs_visible+1;///+1 for caret, only necessary when bearingX is 0
}

static void enterbox_widget_min_h(overlay_theme * theme,widget * w)
{
    w->base.min_h = theme->base_unit_h;
}

static void enterbox_widget_set_w(overlay_theme * theme,widget * w)
{
    enterbox_check_visible_offset(w,theme);
}

static widget_appearence_function_set enterbox_appearence_functions=
(widget_appearence_function_set)
{
    .render =   enterbox_widget_render,
    .select =   enterbox_widget_select,
    .min_w  =   enterbox_widget_min_w,
    .min_h  =   enterbox_widget_min_h,
    .set_w  =   enterbox_widget_set_w,
    .set_h  =   blank_widget_set_h
};

widget * create_enterbox(int max_strlen,int max_glyphs,int min_glyphs_visible,char * initial_text,widget_function activation_func,void * data,widget_function update_contents_func,bool activate_upon_deselect,bool free_data)
{
	widget * w=create_widget(ENTERBOX_WIDGET);

	w->enterbox.data=data;
	w->enterbox.activation_func=activation_func;
	w->enterbox.update_contents_func=update_contents_func;

	w->enterbox.max_strlen=max_strlen;
	w->enterbox.max_glyphs=max_glyphs;
	w->enterbox.min_glyphs_visible=min_glyphs_visible;

	w->enterbox.text=malloc(max_strlen+1);///multiply by max unicode character size
	w->enterbox.upon_input=NULL;

    w->enterbox.activate_upon_deselect=activate_upon_deselect;
	w->enterbox.free_data=free_data;

	set_enterbox_text(w,initial_text);
	w->enterbox.visible_offset=0;

	w->enterbox.composition_text[0]='\0';///use first character as key as to whether text is present/valid
	w->enterbox.composition_visible_offset=0;

	w->base.appearence_functions=&enterbox_appearence_functions;
	w->base.behaviour_functions=&enterbox_behaviour_functions;

	return w;
}


void set_enterbox_text(widget * w,char * text)
{
    ///error checking, don't put in release?
    if(strlen(text)>w->enterbox.max_strlen)
    {
        fprintf(stderr,"ATTEMPTING TO SET A STRING WITHOUT PROVIDING ENOUGH CAPACITY\n");
        exit(-1);
    }

    int gc;
    if(text && cvm_overlay_utf8_validate_string_and_count_glyphs(text,&gc) && gc<=w->enterbox.max_glyphs && strlen(text)<=w->enterbox.max_strlen)
    {
        strcpy(w->enterbox.text,text);
    }
    else w->enterbox.text[0]='\0';

    w->enterbox.visible_offset=0;
    w->enterbox.selection_begin=w->enterbox.text;
    w->enterbox.selection_end=w->enterbox.text;
    w->enterbox.recalculate_text_size=true;
}

void set_enterbox_action_upon_input(widget * w,widget_function func)
{
    w->enterbox.upon_input=func;
}
