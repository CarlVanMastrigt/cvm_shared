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



void blank_button_func(widget * w)
{
    printf("blank button %s\n",w->button.text);
}

void button_toggle_bool_func(widget * w)
{
    if(w->button.data)(*((bool*)w->button.data))^=true;
}




bool button_bool_status_func(widget * w)
{
    if(w->button.data==NULL)return false;
    return *((bool*)w->button.data);
}

bool button_widget_status_func(widget * w)
{
    if(w->button.data==NULL)return false;
    return widget_active(w->button.data);
}




static void button_widget_left_click(overlay_theme * theme,widget * w,int x,int y)
{
    /*if(w->button.double_click_action)
    {
        if(check_widget_double_click(w))
        {
            w->button.double_click_action(w);
            return;
        }
    }*/

    if(w->button.func)
	{
		w->button.func(w);
	}
}

/*static bool button_widget_left_release(overlay_theme * theme,widget * clicked,widget * released,int x,int y)
{
    if(clicked==released)
    {
        if(clicked->button.func)
        {
            clicked->button.func(clicked);
        }
    }

    return false;
}*/

static void button_widget_delete(widget * w)
{
    free(w->button.text);
    if(w->button.free_data)free(w->button.data);
}

static widget_behaviour_function_set button_behaviour_functions=
{
    .l_click        =   button_widget_left_click,
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
    .wid_delete     =   button_widget_delete
};



widget * create_button(void * data,widget_function func,bool free_data)
{
    widget * w=create_widget();

    w->base.behaviour_functions=&button_behaviour_functions;

    w->button.data=data;
    w->button.func=func;

    w->button.text=NULL;

    w->button.toggle_status=NULL;

    w->button.index=0;

    w->button.free_data=free_data;

    w->button.variant_text=false;
    w->button.highlight=false;

    w->base.status|=WIDGET_CLOSE_POPUP_TREE;

    return w;
}









static void text_button_widget_render(overlay_theme * theme,widget * w,int16_t x_off,int16_t y_off,cvm_overlay_element_render_buffer * erb,rectangle bounds)
{
    rectangle r=rectangle_add_offset(w->base.r,x_off,y_off);
    overlay_colour c=OVERLAY_MAIN_COLOUR;

    overlay_text_single_line_render_data text_render_data=
	{
	    .flags=0,
	    .x=r.x1+theme->h_bar_text_offset,
	    .y=(r.y1+r.y2-theme->font.glyph_size)>>1,
	    .text=w->button.text,
	    .bounds=bounds,
	    .colour=OVERLAY_TEXT_COLOUR_0
	};

    if((w->button.toggle_status)&&(w->button.toggle_status(w)))
    {
        if((w->button.variant_text)&&(text_render_data.text))
        {
            while(*text_render_data.text++);
        }

        if(w->button.highlight)
        {
            c=OVERLAY_HIGHLIGHTING_COLOUR;
        }
    }

    theme->h_bar_render(erb,theme,bounds,r,w->base.status,c);

    overlay_text_single_line_render(erb,theme,&text_render_data);
}

static widget * text_button_widget_select(overlay_theme * theme,widget * w,int16_t x_in,int16_t y_in)
{
    if(theme->h_bar_select(theme,rectangle_subtract_offset(w->base.r,x_in,y_in),w->base.status))return w;

    return NULL;
}

static void text_button_widget_min_w(overlay_theme * theme,widget * w)
{
    char * text;

    w->base.min_w = overlay_text_single_line_get_pixel_length(&theme->font,w->button.text);

	if((w->button.variant_text)&&(w->button.toggle_status))
    {
        text=w->button.text;
        while(*text++);

        int16_t min_w=overlay_text_single_line_get_pixel_length(&theme->font,text);
        if(min_w>w->base.min_w)w->base.min_w=min_w;
    }

    w->base.min_w+=2*theme->h_bar_text_offset;
}

static void text_button_widget_min_h(overlay_theme * theme,widget * w)
{
	w->base.min_h = theme->base_unit_h;
}

static widget_appearence_function_set text_button_appearence_functions=
{
    .render =   text_button_widget_render,
    .select =   text_button_widget_select,
    .min_w  =   text_button_widget_min_w,
    .min_h  =   text_button_widget_min_h,
    .set_w  =   blank_widget_set_w,
    .set_h  =   blank_widget_set_h
};

widget * create_text_button(char * text,void * data,bool free_data,widget_function func)
{
    widget * button=create_button(data,func,free_data);

    if(text)button->button.text=cvm_strdup(text);

    button->base.appearence_functions=&text_button_appearence_functions;

    return button;
}

widget * create_text_toggle_button(char * positive_text,char * negative_text,void * data,widget_function func,widget_button_toggle_status_func toggle_status)
{
    widget * w=create_button(data,func,false);

    w->base.appearence_functions=&text_button_appearence_functions;

    w->button.toggle_status=toggle_status;
    w->button.variant_text=true;

    int len=2;
    if(negative_text)len+=strlen(negative_text);
    if(positive_text)len+=strlen(positive_text);

    char * t=w->button.text=malloc(len*sizeof(char));
    if(negative_text)while(*negative_text) *t++ = *negative_text++;
    *t++ = '\0';
    if(positive_text)while(*positive_text) *t++ = *positive_text++;
    *t++ = '\0';

    return w;
}

widget * create_text_highlight_toggle_button(char * text,void * data,bool free_data,widget_function func,widget_button_toggle_status_func toggle_status)
{
    widget * w=create_button(data,func,free_data);

    if(text)w->button.text=cvm_strdup(text);

    w->base.appearence_functions=&text_button_appearence_functions;

    w->button.toggle_status=toggle_status;
    w->button.highlight=true;

    return w;
}







static void contiguous_text_button_widget_render(overlay_theme * theme,widget * w,int16_t x_off,int16_t y_off,cvm_overlay_element_render_buffer * erb,rectangle bounds)
{
    rectangle r=rectangle_add_offset(w->base.r,x_off,y_off);

    overlay_text_single_line_render_data text_render_data=
	{
	    .flags=OVERLAY_TEXT_RENDER_BOX_CONSTRAINED,
	    .x=r.x1+theme->h_bar_text_offset,
	    .y=(r.y1+r.y2-theme->font.glyph_size)>>1,
	    .text=w->button.text,
	    .bounds=bounds,
	    .colour=OVERLAY_TEXT_COLOUR_0
	};

	bool valid=get_ancestor_contiguous_box_data(w,&text_render_data.box_r,&text_render_data.box_status);

    assert(valid);///CONTIGUOUS BUTTON SHOULD BE IN CONTIGUOUS BOX

    if((w->button.toggle_status)&&(w->button.toggle_status(w)))
    {
        if((w->button.variant_text)&&(text_render_data.text))
        {
            while(*text_render_data.text++);
        }

        if(w->button.highlight)
        {
            theme->h_bar_box_constrained_render(erb,theme,bounds,r,w->base.status,OVERLAY_HIGHLIGHTING_COLOUR,text_render_data.box_r,text_render_data.box_status);
        }
    }

    overlay_text_single_line_render(erb,theme,&text_render_data);
}

static void contiguous_text_button_widget_min_h(overlay_theme * theme,widget * w)
{
	w->base.min_h = theme->contiguous_horizintal_bar_h;
}

static widget_appearence_function_set contiguous_text_button_appearence_functions=
{
    .render =   contiguous_text_button_widget_render,
    .select =   text_button_widget_select,
    .min_w  =   text_button_widget_min_w,
    .min_h  =   contiguous_text_button_widget_min_h,
    .set_w  =   blank_widget_set_w,
    .set_h  =   blank_widget_set_h
};

widget * create_contiguous_text_button(char * text,void * data,bool free_data,widget_function func)
{
    widget * w=create_button(data,func,free_data);

    if(text)w->button.text=cvm_strdup(text);

    w->base.appearence_functions=&contiguous_text_button_appearence_functions;

    return w;
}

widget * create_contiguous_text_highlight_toggle_button(char * text,void * data,bool free_data,widget_function func,widget_button_toggle_status_func toggle_status)
{
    widget * w=create_button(data,func,free_data);

    if(text)w->button.text=cvm_strdup(text);

    w->base.appearence_functions=&contiguous_text_button_appearence_functions;

    w->button.toggle_status=toggle_status;
    w->button.highlight=true;

    return w;
}











static void icon_button_widget_render(overlay_theme * theme,widget * w,int16_t x_off,int16_t y_off,cvm_overlay_element_render_buffer * erb,rectangle bounds)
{
    char * t=w->button.text;

    if((w->button.variant_text)&&(t)&&(w->button.toggle_status)&&(w->button.toggle_status(w)))
    {
        while(*t++);
    }

    rectangle r=rectangle_add_offset(w->base.r,x_off,y_off);

    theme->square_render(erb,theme,bounds,r,w->base.status,OVERLAY_MAIN_COLOUR);
    overlay_text_centred_glyph_render(erb,&theme->font,bounds,r,t,OVERLAY_TEXT_COLOUR_0);
}

static widget * icon_button_widget_select(overlay_theme * theme,widget * w,int16_t x_in,int16_t y_in)
{
	if(theme->square_select(theme,rectangle_subtract_offset(w->base.r,x_in,y_in),w->base.status)) return w;

    return NULL;
}

static void icon_button_widget_min_w(overlay_theme * theme,widget * w)
{
	w->base.min_w = theme->base_unit_w;
}

static void icon_button_widget_min_h(overlay_theme * theme,widget * w)
{
	w->base.min_h = theme->base_unit_h;
}

static widget_appearence_function_set icon_button_appearence_functions=
{
    .render =   icon_button_widget_render,
    .select =   icon_button_widget_select,
    .min_w  =   icon_button_widget_min_w,
    .min_h  =   icon_button_widget_min_h,
    .set_w  =   blank_widget_set_w,
    .set_h  =   blank_widget_set_h
};

widget * create_icon_button(char * icon_name,void * data,bool free_data,widget_function func)
{
    widget * button=create_button(data,func,free_data);

    if(icon_name)button->button.text=cvm_strdup(icon_name);

    button->base.appearence_functions=&icon_button_appearence_functions;

	return button;
}

widget * create_icon_toggle_button(char * positive_icon,char * negative_icon,void * data,bool free_data,widget_function func,widget_button_toggle_status_func toggle_status)
{
    widget * w=create_button(data,func,free_data);

    w->base.appearence_functions=&icon_button_appearence_functions;

    w->button.toggle_status=toggle_status;
    w->button.variant_text=true;

    int len=2;
    if(negative_icon)len+=strlen(negative_icon);
    if(positive_icon)len+=strlen(positive_icon);

    char * t=w->button.text=malloc(len*sizeof(char));
    if(negative_icon)while((*t++ = *negative_icon++));
    else *t++ = '\0';
    if(positive_icon)while((*t++ = *positive_icon++));
    else *t++ = '\0';

    return w;
}









