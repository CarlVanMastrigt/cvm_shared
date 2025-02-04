/**
Copyright 2020,2021,2022 Carl van Mastrigt

This file is part of solipsix.

solipsix is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

solipsix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with solipsix.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "solipsix.h"



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




static void button_widget_left_click(overlay_theme * theme, widget * w, int x, int y, bool double_clicked)
{
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


void widget_button_initialise(widget_button* button, struct widget_context* context, const widget_appearence_function_set * appearence_functions, widget_function func, void * data, bool free_data)
{
    widget_base_initialise(&button->base, context, appearence_functions, &button_behaviour_functions);

    button->data = data;
    button->func = func;
    button->text = NULL;
    button->toggle_status = NULL;
    button->free_data = free_data;
    button->variant_text = false;
    button->highlight = false;

    button->base.status |= WIDGET_CLOSE_POPUP_TREE;
}


/// combine 2 strings
static inline char* cvm_strdup_button_toggle_packed(const char* negative_text, const char* positive_text)
{
    char *text,*t;
    int len;

    len = 2;// space for null terminating characters
    if(negative_text) len+= strlen(negative_text);
    if(positive_text) len+= strlen(positive_text);

    text = t = malloc(len * sizeof(char));
    if(negative_text) while((*t++ = *negative_text++));
    if(positive_text) while((*t++ = *positive_text++));

    return text;
}






static void text_button_widget_render(overlay_theme * theme,widget * w,int16_t x_off,int16_t y_off,struct cvm_overlay_render_batch * restrict render_batch,rectangle bounds)
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

    theme->h_bar_render(render_batch, theme, bounds, r, w->base.status, c);

    overlay_text_single_line_render(render_batch, theme, &text_render_data);
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

widget * create_text_button(struct widget_context* context, const char * text,void * data,bool free_data,widget_function func)
{
    widget * w = malloc(sizeof(widget_button));

    widget_button_initialise(&w->button, context, &text_button_appearence_functions, func, data, free_data);
    w->button.text = cvm_strdup(text);

    return w;
}

widget * create_text_toggle_button(struct widget_context* context, char * positive_text,char * negative_text,void * data, bool free_data,widget_function func,widget_button_toggle_status_func toggle_status)
{
    widget * w = malloc(sizeof(widget_button));

    widget_button_initialise(&w->button, context, &text_button_appearence_functions, func, data, free_data);
    w->button.toggle_status = toggle_status;
    w->button.variant_text = true;
    w->button.text = cvm_strdup_button_toggle_packed(negative_text, positive_text);

    return w;
}

widget * create_text_highlight_toggle_button(struct widget_context* context, char * text,void * data,bool free_data,widget_function func,widget_button_toggle_status_func toggle_status)
{
    widget * w = malloc(sizeof(widget_button));

    widget_button_initialise(&w->button, context, &text_button_appearence_functions, func, data, free_data);
    w->button.toggle_status = toggle_status;
    w->button.highlight = true;
    w->button.text = cvm_strdup(text);

    return w;
}







static void contiguous_text_button_widget_render(overlay_theme * theme,widget * w,int16_t x_off,int16_t y_off,struct cvm_overlay_render_batch * restrict render_batch,rectangle bounds)
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
            theme->h_bar_box_constrained_render(render_batch,theme,bounds,r,w->base.status,OVERLAY_HIGHLIGHTING_COLOUR,text_render_data.box_r,text_render_data.box_status);
        }
    }

    overlay_text_single_line_render(render_batch,theme,&text_render_data);
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

widget * create_contiguous_text_button(struct widget_context* context, const char * text,void * data,bool free_data,widget_function func)
{
    widget * w = malloc(sizeof(widget_button));

    widget_button_initialise(&w->button, context, &contiguous_text_button_appearence_functions, func, data, free_data);
    w->button.text = cvm_strdup(text);

    return w;
}

widget * create_contiguous_text_highlight_toggle_button(struct widget_context* context, const char * text,void * data,bool free_data,widget_function func,widget_button_toggle_status_func toggle_status)
{
    widget * w = malloc(sizeof(widget_button));

    widget_button_initialise(&w->button, context, &contiguous_text_button_appearence_functions, func, data, free_data);
    w->button.toggle_status = toggle_status;
    w->button.highlight = true;
    w->button.text = cvm_strdup(text);

    return w;
}











static void icon_button_widget_render(overlay_theme * theme,widget * w,int16_t x_off,int16_t y_off,struct cvm_overlay_render_batch * restrict render_batch,rectangle bounds)
{
    char * t=w->button.text;

    if((w->button.variant_text)&&(t)&&(w->button.toggle_status)&&(w->button.toggle_status(w)))
    {
        while(*t++);
    }

    rectangle r=rectangle_add_offset(w->base.r,x_off,y_off);

    theme->square_render(render_batch,theme,bounds,r,w->base.status,OVERLAY_MAIN_COLOUR);
    overlay_text_centred_glyph_render(render_batch,&theme->font,bounds,r,t,OVERLAY_TEXT_COLOUR_0);
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

widget * create_icon_button(struct widget_context* context, const char * icon_name,void * data,bool free_data,widget_function func)
{
    widget * w = malloc(sizeof(widget_button));

    widget_button_initialise(&w->button, context, &icon_button_appearence_functions, func, data, free_data);
    w->button.text = cvm_strdup(icon_name);

	return w;
}

widget * create_icon_toggle_button(struct widget_context* context, const char * positive_icon, const char * negative_icon,void * data,bool free_data,widget_function func,widget_button_toggle_status_func toggle_status)
{
    widget * w = malloc(sizeof(widget_button));

    widget_button_initialise(&w->button, context, &icon_button_appearence_functions, func, data, free_data);
    w->button.toggle_status = toggle_status;
    w->button.variant_text = true;
    w->button.text = cvm_strdup_button_toggle_packed(negative_icon, positive_icon);

    return w;
}







