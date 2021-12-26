/**
Copyright 2020,2021 Carl van Mastrigt

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



static void text_bar_widget_delete(widget * w)
{
    if(w->text_bar.free_text) free(w->text_bar.text);
    if(w->text_bar.free_data) free(w->text_bar.data);
}

static widget_behaviour_function_set text_bar_behaviour_functions=
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
    .add_child      =   blank_widget_add_child,
    .remove_child   =   blank_widget_remove_child,
    .wid_delete     =   text_bar_widget_delete
};





static void text_bar_widget_render(overlay_theme * theme,widget * w,int x_off,int y_off,cvm_overlay_element_render_buffer * erb,rectangle bounds)
{
//    int sub_x_off=0;;
//
//    char text_buffer[256];
//    char * text_ptr=w->text_bar.text;
//    //size_t length;
//
//    if(w->text_bar.min_glyph_render_count)
//    {
//        if(w->text_bar.set_text)w->text_bar.set_text(w);
//
//        if(w->text_bar.text_alignment==WIDGET_TEXT_RIGHT_ALIGNED)
//        {
//            theme->h_text_bar_render(w->base.r,x_off,y_off,w->base.status,theme,od,bounds,OVERLAY_MAIN_COLOUR,NULL);
//
//            text_ptr=shorten_text_to_fit_width_start_ellipses(theme,w->base.r.w - 2*theme->h_bar_text_offset,w->text_bar.text,0,text_buffer,256,&sub_x_off);
//
//            render_overlay_text(od,theme,text_ptr,x_off+w->base.r.x+theme->h_bar_text_offset+sub_x_off,y_off+w->base.r.y+(w->base.r.h-theme->font.font_height)/2,bounds,0,0);
//
//            return;
//        }
//        else text_ptr=shorten_text_to_fit_width_end_ellipses(theme,w->base.r.w - 2*theme->h_bar_text_offset,w->text_bar.text,0,text_buffer,256);
//    }
//
//    theme->h_text_bar_render(w->base.r,x_off,y_off,w->base.status,theme,od,bounds,OVERLAY_MAIN_COLOUR,text_ptr);
}

static void text_bar_widget_min_w(overlay_theme * theme,widget * w)
{
//    if(w->text_bar.min_glyph_render_count) w->base.min_w = 2*theme->h_bar_text_offset + w->text_bar.min_glyph_render_count * theme->font.max_glyph_width;
//    else
//    {
//        if(w->text_bar.set_text)w->text_bar.set_text(w);
//        w->base.min_w = 2*theme->h_bar_text_offset + calculate_text_length(theme,w->text_bar.text,0);
//    }
}

static void text_bar_widget_min_h(overlay_theme * theme,widget * w)
{
    w->base.min_h = theme->base_unit_h;
}






static widget_appearence_function_set text_bar_appearence_functions=
(widget_appearence_function_set)
{
    .render =   text_bar_widget_render,
    .select =   blank_widget_select,
    .min_w  =   text_bar_widget_min_w,
    .min_h  =   text_bar_widget_min_h,
    .set_w  =   blank_widget_set_w,
    .set_h  =   blank_widget_set_h
};






widget * create_static_text_bar(int min_glyph_render_count,char * text,widget_text_alignment text_alignment)
{
	widget * w=create_widget(TEXT_BAR_WIDGET);

	w->base.appearence_functions=&text_bar_appearence_functions;
	w->base.behaviour_functions=&text_bar_behaviour_functions;

	w->text_bar.set_text=NULL;
	w->text_bar.data=NULL;

	w->text_bar.min_glyph_render_count=min_glyph_render_count;

	w->text_bar.free_data=false;
	w->text_bar.free_text=true;

	w->text_bar.text_alignment=text_alignment;

	if(text) w->text_bar.text=strdup(text);
    else w->text_bar.text=NULL;

	return w;
}

widget * create_dynamic_text_bar(int min_glyph_render_count,widget_function set_text,bool free_text,void * data,bool free_data,widget_text_alignment text_alignment)
{
	widget * w=create_widget(TEXT_BAR_WIDGET);

	w->base.appearence_functions=&text_bar_appearence_functions;
	w->base.behaviour_functions=&text_bar_behaviour_functions;

	w->text_bar.set_text=set_text;
	w->text_bar.data=data;

	w->text_bar.min_glyph_render_count=min_glyph_render_count;

	w->text_bar.free_data=free_data;
	w->text_bar.free_text=free_text;

	w->text_bar.text_alignment=text_alignment;

	w->text_bar.text=NULL;

	return w;
}

//void set_text_bar_text_pointer(widget * w,char * text)
//{
//    if(w->text_bar.set_text)
//    {
//        puts("ERROR: SHOULD NOT SET TEXT FOR PROGRAMMABLE TEXT_BAR");
//        return;
//    }
//
//    if(w->text_bar.owns_text)free(w->text_bar.text);
//
//    w->text_bar.text=text;
//    w->text_bar.owns_text=false;
//}
//
//void set_text_bar_text(widget * w,char * text_to_copy)
//{
//    if(w->text_bar.owns_text)free(w->text_bar.text);
//
//    if(text_to_copy)
//    {
//        w->text_bar.text=strdup(text_to_copy);
//        w->text_bar.owns_text=true;
//    }
//    else
//    {
//        w->text_bar.text=NULL;
//        w->text_bar.owns_text=false;
//    }
//}
