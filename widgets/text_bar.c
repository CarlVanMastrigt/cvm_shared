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

///must be called any time length or offset is relevant in a function
static inline void text_bar_recalculate_text_size_and_offset(overlay_theme * theme,widget * w)
{
    if(w->text_bar.recalculate_text_size)
    {
        w->text_bar.text_pixel_length=overlay_text_single_line_get_pixel_length(&theme->font,w->text_bar.text);

        if(w->text_bar.text_alignment==WIDGET_TEXT_LEFT_ALIGNED)w->text_bar.visible_offset=0;
        else
        {
            assert(w->text_bar.text_alignment==WIDGET_TEXT_RIGHT_ALIGNED);
            w->text_bar.visible_offset=w->text_bar.text_pixel_length-(w->base.r.x2-w->base.r.x1 - theme->h_bar_text_offset*2 -1);///text length - visible text length; -1 for space for caret
            w->text_bar.visible_offset*=w->text_bar.visible_offset>0;///cannot be negative
        }

        w->text_bar.recalculate_text_size=false;
    }
}

static inline void text_bar_reset_offset(overlay_theme * theme,widget * w)
{
    if(w->text_bar.text_alignment==WIDGET_TEXT_LEFT_ALIGNED)w->text_bar.visible_offset=0;
    else
    {
        assert(w->text_bar.text_alignment==WIDGET_TEXT_RIGHT_ALIGNED);
        w->text_bar.visible_offset=w->text_bar.text_pixel_length-(w->base.r.x2-w->base.r.x1 - theme->h_bar_text_offset*2 -1);///text length - visible text length; -1 for space for caret
        w->text_bar.visible_offset*=w->text_bar.visible_offset>0;///cannot be negative
    }
}

///perhaps call this upon resize of still active widget?
static void text_bar_check_visible_offset(overlay_theme * theme,widget * w)
{
    char tmp;
    int32_t current_offset,text_space,max_offset;

    if(!w->text_bar.text) return;

    text_space=w->base.r.x2-w->base.r.x1-2*theme->h_bar_text_offset-1;

    tmp=*w->text_bar.selection_end;
    *w->text_bar.selection_end='\0';

    current_offset=overlay_text_single_line_get_pixel_length(&theme->font,w->text_bar.text);

    *w->text_bar.selection_end=tmp;

    if(w->text_bar.visible_offset>current_offset-theme->h_text_fade_range) w->text_bar.visible_offset=current_offset-theme->h_text_fade_range;
    if(w->text_bar.visible_offset<current_offset-text_space+theme->h_text_fade_range) w->text_bar.visible_offset=current_offset-text_space+theme->h_text_fade_range;

    ///special condition to put offset at middle???
    if(w->text_bar.visible_offset<0)w->text_bar.visible_offset=0;
    max_offset=w->text_bar.text_pixel_length-text_space;///max offset
    if(max_offset < 0) w->text_bar.visible_offset=0;
    else if(w->text_bar.visible_offset > max_offset) w->text_bar.visible_offset=max_offset;
}


static void text_bar_copy_selection_to_clipboard(widget * w)
{
    char *s_begin,*s_end;
    char tmp;

    if(!w->text_bar.text) return;

    if(w->text_bar.selection_end > w->text_bar.selection_begin) s_begin=w->text_bar.selection_begin, s_end=w->text_bar.selection_end;
    else s_begin=w->text_bar.selection_end, s_end=w->text_bar.selection_begin;

    /// copy all contents if nothing selected? if so uncomment next section
    if(s_begin!=s_end)
    {
        tmp=*s_end;
        *s_end='\0';

        SDL_SetClipboardText(s_begin);

        *s_end=tmp;
    }
//    else if(*w->text_bar.text)///not empty
//    {
//        SDL_SetClipboardText(w->text_bar.text);
//    }
}





static void text_bar_widget_left_click(overlay_theme * theme,widget * w,int x,int y)
{
    text_bar_recalculate_text_size_and_offset(theme,w);

    adjust_coordinates_to_widget_local(w,&x,&y);
    w->text_bar.selection_begin=w->text_bar.selection_end=overlay_text_single_line_find_offset(&theme->font,w->text_bar.text,x-theme->h_bar_text_offset+w->text_bar.visible_offset);

    text_bar_check_visible_offset(theme,w);
}

static bool text_bar_widget_left_release(overlay_theme * theme,widget * clicked,widget * released,int x,int y)
{
    return true;
}

static void text_bar_widget_mouse_movement(overlay_theme * theme,widget * w,int x,int y)
{
    text_bar_recalculate_text_size_and_offset(theme,w);

    adjust_coordinates_to_widget_local(w,&x,&y);
    w->text_bar.selection_end=overlay_text_single_line_find_offset(&theme->font,w->text_bar.text,x-theme->h_bar_text_offset+w->text_bar.visible_offset);

    text_bar_check_visible_offset(theme,w);
}

static bool text_bar_widget_key_down(overlay_theme * theme,widget * w,SDL_Keycode keycode,SDL_Keymod mod)
{
    char *s_begin,*s_end,*s;

    if(!w->text_bar.text) return true;

    text_bar_recalculate_text_size_and_offset(theme,w);

    if(w->text_bar.selection_end > w->text_bar.selection_begin) s_begin=w->text_bar.selection_begin, s_end=w->text_bar.selection_end;
    else s_begin=w->text_bar.selection_end, s_end=w->text_bar.selection_begin;

    switch(keycode)
    {
    case SDLK_c:
        if(mod&KMOD_CTRL)
        {
            text_bar_copy_selection_to_clipboard(w);
        }
        break;

    case SDLK_x:
        if(mod&KMOD_CTRL)
        {
            text_bar_copy_selection_to_clipboard(w);///considering text is immutable just copy
        }
        break;

    case SDLK_a:
        if(mod&KMOD_CTRL)
        {
            w->text_bar.selection_begin=w->text_bar.text;
            w->text_bar.selection_end=w->text_bar.text+strlen(w->text_bar.text);
        }
        break;

    case SDLK_LEFT:
        if(mod&KMOD_CTRL)///can move based on ctrl (jump word),  then set to same based on shift
        {
            if(mod&KMOD_SHIFT) w->text_bar.selection_end = cvm_overlay_utf8_get_previous_word(w->text_bar.text,w->text_bar.selection_end);
            else w->text_bar.selection_begin = w->text_bar.selection_end = cvm_overlay_utf8_get_previous_word(w->text_bar.text,w->text_bar.selection_end);
        }
        else if(mod&KMOD_SHIFT)w->text_bar.selection_end = cvm_overlay_utf8_get_previous_glyph(w->text_bar.text,w->text_bar.selection_end);
        else if(s_begin==s_end) w->text_bar.selection_begin = w->text_bar.selection_end = cvm_overlay_utf8_get_previous_glyph(w->text_bar.text,s_begin);
        else w->text_bar.selection_begin = w->text_bar.selection_end = s_begin;
        break;

    case SDLK_RIGHT:
        if(mod&KMOD_CTRL)
        {
            if(mod&KMOD_SHIFT) w->text_bar.selection_end = cvm_overlay_utf8_get_next_word(w->text_bar.selection_end);
            else w->text_bar.selection_begin = w->text_bar.selection_end = cvm_overlay_utf8_get_next_word(w->text_bar.selection_end);
        }
        else if(mod&KMOD_SHIFT) w->text_bar.selection_end = cvm_overlay_utf8_get_next_glyph(w->text_bar.selection_end);
        else if(s_begin==s_end) w->text_bar.selection_begin = w->text_bar.selection_end = cvm_overlay_utf8_get_next_glyph(s_end);
        else w->text_bar.selection_begin = w->text_bar.selection_end = s_end;
        break;

    case SDLK_KP_7:/// keypad/numpad home
         if(mod&KMOD_NUM)break;
    case SDLK_UP:
    case SDLK_HOME:
        if(mod&KMOD_SHIFT) w->text_bar.selection_end = w->text_bar.text;
        else w->text_bar.selection_begin = w->text_bar.selection_end = w->text_bar.text;
        break;

    case SDLK_KP_1:/// keypad/numpad end
         if(mod&KMOD_NUM)break;
    case SDLK_DOWN:
    case SDLK_END:
        if(mod&KMOD_SHIFT) w->text_bar.selection_end = w->text_bar.text+strlen(w->text_bar.text);
        else w->text_bar.selection_begin = w->text_bar.selection_end = w->text_bar.text+strlen(w->text_bar.text);
        break;

    case SDLK_ESCAPE:
        set_currently_active_widget(NULL);
        break;

        default:;
    }

    text_bar_check_visible_offset(theme,w);

    return true;
}

static void text_bar_widget_click_away(overlay_theme * theme,widget * w)
{
    text_bar_reset_offset(theme,w);
}

static void text_bar_widget_delete(widget * w)
{
    if(w->text_bar.free_text) free(w->text_bar.text);
}



static widget_behaviour_function_set text_bar_behaviour_functions=
{
    .l_click        =   text_bar_widget_left_click,
    .l_release      =   text_bar_widget_left_release,
    .r_click        =   blank_widget_right_click,
    .m_move         =   text_bar_widget_mouse_movement,
    .scroll         =   blank_widget_scroll,
    .key_down       =   text_bar_widget_key_down,
    .text_input     =   blank_widget_text_input,
    .text_edit      =   blank_widget_text_edit,
    .click_away     =   text_bar_widget_click_away,
    .add_child      =   blank_widget_add_child,
    .remove_child   =   blank_widget_remove_child,
    .wid_delete     =   text_bar_widget_delete
};





static void text_bar_widget_render(overlay_theme * theme,widget * w,int16_t x_off,int16_t y_off,cvm_overlay_element_render_buffer * erb,rectangle bounds)
{
    rectangle r=rectangle_add_offset(w->base.r,x_off,y_off);

    text_bar_recalculate_text_size_and_offset(theme,w);

	theme->h_bar_render(erb,theme,bounds,r,w->base.status,OVERLAY_MAIN_COLOUR);

    overlay_text_single_line_render_data text_render_data=
	{
        .flags=0,
	    .x=r.x1+theme->h_bar_text_offset,
	    .y=(r.y1+r.y2-theme->font.glyph_size)>>1,
	    .text=w->text_bar.text,
	    .bounds=bounds,
	    .colour=OVERLAY_TEXT_COLOUR_0
	};

	{
	    text_render_data.text_length=w->text_bar.text_pixel_length;
        text_render_data.text_area=overlay_text_single_line_get_text_area(r,theme->font.glyph_size,theme->h_bar_text_offset);
        text_render_data.x-=w->text_bar.visible_offset;
        if(w->text_bar.selection_end > w->text_bar.selection_begin) text_render_data.selection_begin=w->text_bar.selection_begin, text_render_data.selection_end=w->text_bar.selection_end;
        else text_render_data.selection_begin=w->text_bar.selection_end, text_render_data.selection_end=w->text_bar.selection_begin;
	}

    text_render_data.flags|=!!w->text_bar.min_glyph_render_count*OVERLAY_TEXT_RENDER_FADING;
    text_render_data.flags|=is_currently_active_widget(w)*OVERLAY_TEXT_RENDER_SELECTION;

    overlay_text_single_line_render(erb,theme,&text_render_data);
}

static widget * text_bar_widget_select(overlay_theme * theme,widget * w,int16_t x_in,int16_t y_in)
{
    if(theme->h_bar_select(theme,rectangle_subtract_offset(w->base.r,x_in,y_in),w->base.status))return w;

    return NULL;
}

static void text_bar_widget_min_w(overlay_theme * theme,widget * w)
{
    if(w->text_bar.min_glyph_render_count)
    {
        w->base.min_w = 2*theme->h_bar_text_offset + w->text_bar.min_glyph_render_count * theme->font.max_advance;
        ///also need to set flag to recalculate text size here b/c that functionality won't be provoked upon theme change but this function will be called and the product of the change will be necessary
        w->text_bar.recalculate_text_size=true;
    }
    else w->base.min_w = 2*theme->h_bar_text_offset + overlay_text_single_line_get_pixel_length(theme,w->text_bar.text);

}

static void text_bar_widget_min_h(overlay_theme * theme,widget * w)
{
    w->base.min_h = theme->base_unit_h;
}

static void text_bar_widget_set_w(overlay_theme * theme,widget * w)
{
    text_bar_check_visible_offset(theme,w);
}



static widget_appearence_function_set text_bar_appearence_functions=
{
    .render =   text_bar_widget_render,
    .select =   text_bar_widget_select,
    .min_w  =   text_bar_widget_min_w,
    .min_h  =   text_bar_widget_min_h,
    .set_w  =   text_bar_widget_set_w,
    .set_h  =   blank_widget_set_h
};





///selection being valid requires the ability to index into a buffer (selection pointers), ergo NEED to have some stable buffer, likely a local buffer (cannot be changed externally)




///static provably isnt best name, or want to add another variant for when text references string that is modified externally
widget * create_static_text_bar(char * text)
{
	widget * w=create_widget();

	w->base.appearence_functions=&text_bar_appearence_functions;
	w->base.behaviour_functions=&text_bar_behaviour_functions;

	///w->text_bar.set_text=NULL;
	///w->text_bar.data=NULL;

	w->text_bar.min_glyph_render_count=0;

	w->text_bar.free_text=true;
	w->text_bar.allow_selection=false;
	w->text_bar.recalculate_text_size=false;

	w->text_bar.text_alignment=WIDGET_TEXT_LEFT_ALIGNED;

	if(text) w->text_bar.text=cvm_strdup(text);
    else w->text_bar.text=NULL;

    w->text_bar.visible_offset=0;
    w->text_bar.text_pixel_length=0;

    w->text_bar.selection_begin=NULL;
    w->text_bar.selection_end=NULL;

	return w;
}

widget * create_dynamic_text_bar(int min_glyph_render_count,widget_text_alignment text_alignment,bool allow_selection)
{
	widget * w=create_widget();

	w->base.appearence_functions=&text_bar_appearence_functions;
	w->base.behaviour_functions=&text_bar_behaviour_functions;

	///w->text_bar.set_text=set_text;
	///w->text_bar.data=data;

	assert(min_glyph_render_count);///ZERO GLYPH RENDER COUNT INVALID FOR DYNAMIC TEXT BARS
	w->text_bar.min_glyph_render_count=min_glyph_render_count;

	w->text_bar.free_text=false;
	w->text_bar.allow_selection=allow_selection;
	w->text_bar.recalculate_text_size=true;

	w->text_bar.text_alignment=text_alignment;

	w->text_bar.text=NULL;

	w->text_bar.visible_offset=0;
    w->text_bar.text_pixel_length=0;

    w->text_bar.selection_begin=NULL;
    w->text_bar.selection_end=NULL;

	return w;
}


void set_text_bar_text_pointer(widget * w,char * text_pointer)
{
    if(w->text_bar.free_text)free(w->text_bar.text);

    w->text_bar.text=text_pointer;

    w->text_bar.free_text=false;
    w->text_bar.visible_offset=0;
    ///perhaps null isn't best, instead use just .text
    w->text_bar.selection_begin=w->text_bar.text;
    w->text_bar.selection_end=w->text_bar.text;
    w->text_bar.recalculate_text_size=true;
}

void set_text_bar_text(widget * w,char * text_to_copy)
{
    if(w->text_bar.free_text)free(w->text_bar.text);

    if(text_to_copy) w->text_bar.text=cvm_strdup(text_to_copy);
    else w->text_bar.text=NULL;

    w->text_bar.free_text=true;
    w->text_bar.visible_offset=0;
    ///perhaps null isn't best, instead use just .text
    w->text_bar.selection_begin=w->text_bar.text;
    w->text_bar.selection_end=w->text_bar.text;
    w->text_bar.recalculate_text_size=true;
}

/**

programmable text bar is almost certainly undesirable, requires constant update, which i would prefer to avoid, instead changes should be externally managed and updates called

for patching file search elements together; the file search manager should be doing the work (in a separate buffer presumably)



both here and in enterbox allow automatic scrolling by holding cursor beyond edges of screen, ideally NOT JUST UPON MOVEMENT EITHER (maybe not possible though)

implement textbar/enterbox scrolling?

*/
