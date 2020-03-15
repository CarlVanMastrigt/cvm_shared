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


static void progress_lines(widget * w,cvm_font * font,int * x,int num_characters,int max_width)
{
    textbox_line * line=w->textbox.render_lines+w->textbox.render_lines_count;

    line->character_finish=num_characters;
    line->x_size=((*x)>max_width)?max_width:(*x);

    w->textbox.render_lines_count++;

    if(w->textbox.render_lines_count >= w->textbox.render_lines_space)
    {
        w->textbox.render_lines=realloc(w->textbox.render_lines,sizeof(textbox_line)*(w->textbox.render_lines_space*=2));
    }

    line=w->textbox.render_lines+w->textbox.render_lines_count;

    line->character_start=num_characters;

    *x=0;
}




static void process_textbox_contents(overlay_theme * theme,widget * w,int width,int font_index)///returns required height to fit text
{
    int x=0;
    cvm_font * font= &theme->font;
    char * text=w->textbox.text;


    int i;
    int new_x;
    int word_end_offset;
    //overlay_render_data * glyph_rd;
    cvm_glyph * glyph;
    char prev_tmp,prev=0;

    w->textbox.render_lines_count=0;

    i=0;
    int prev_x;

    w->textbox.render_lines[0].character_start=0;
    bool empty=true;

    GLshort * data;

    while(text[i])
    {
        prev_tmp=prev;
        new_x=x;

        for(word_end_offset=i;process_regular_text_character(font,prev_tmp,text[word_end_offset],&new_x);word_end_offset++)
        {
            prev_tmp=text[word_end_offset];
        }

        if((new_x>width)&&(!empty)) ///empty prevents unnecessarily progressing lines for long words when doing so would leave a line completely empty
        {
            progress_lines(w,font,&x,i,width);
        }
        empty=false;

        while(i<word_end_offset)
        {
            prev_x=x;
            process_regular_text_character(font,prev,text[i],&x);

            if(x>width)/// THIS HANDLES VERY LONG WORDS THAT MUST WRAP
            {
                x=prev_x;
                progress_lines(w,font,&x,i,width);
                process_regular_text_character(font,0,text[i],&x);
            }

            glyph=font->glyphs+(text[i]-'!');


            data=w->textbox.glyph_render_data[i].data1;
            data[0]=x+glyph->bearingX-glyph->advance;
            data[1]=0;///set elsewhere?
            data[2]=glyph->width;
            data[3]=font->font_height;

            data=w->textbox.glyph_render_data[i].data2;
            ///data[0]=2; ///type already set
            ///data[1]=; ///colour set elsewhere
            data[2]=glyph->offset;
            data[3]=font_index;

            data=w->textbox.glyph_render_data[i].data3;
            data[0]=x+glyph->bearingX-glyph->advance;


            prev=text[i++];
        }


        while((text[i])&&((text[i]<'!')||(text[i]>'~')))///process all characters that are not displayable glyphs
        {
            w->textbox.glyph_render_data[i].data1[0]=(x>width)?width:x;///x_pos

            switch(text[i])
            {
                case ' ':
                x+=font->space_width;
                break;

                case '\n':
                x+=font->space_width;
                progress_lines(w,font,&x,i+1,width);
                empty=true;
                break;

                case '\t':
                x=((x/(font->space_width*4))+1)*font->space_width*4;///rounds x to next multiple of 4 spaces, progressing at least 1 space
                break;

                default:break;
            }

            prev=text[i];

            i++;
        }
    }

    w->textbox.render_lines[w->textbox.render_lines_count].character_finish=i;
    w->textbox.render_lines[w->textbox.render_lines_count].x_size=(x>width)?width:x;


    w->textbox.render_lines_count++;
    //return (w->textbox.render_lines_count++)*font->line_spacing+font->font_height;
}






static int test_for_text_tags(char * text,short * current_colour)
{
    bool tag_found;
    int i;

    char tmp_buffer[64];

    if((text[0]=='<')&&(text[1]=='c'))
    {
        tag_found=false;

        for(i=2;(i<5);i++)
        {
            if(text[i]=='>')
            {
                tmp_buffer[i-2]='\0';
                tag_found=true;
                break;
            }

            tmp_buffer[i-2]=text[i];
        }

        if(tag_found)
        {
            if(sscanf(tmp_buffer,"%hd",current_colour))
            {
                return i+1;
            }
        }
    }

    return 0;
}

void set_textbox_text(widget * w,char * text_in)
{
    char * text_out;
    overlay_render_data * ord;
    int advance;
    short current_colour=0;

    w->textbox.text_length=strlen(text_in)+1;
    w->textbox.text=text_out=malloc(sizeof(char)*w->textbox.text_length);
    w->textbox.glyph_render_data=ord=malloc(sizeof(overlay_render_data)*w->textbox.text_length);



    while(*text_in)
    {
        if((advance=test_for_text_tags(text_in,&current_colour)))
        {
            text_in+=advance;
        }
        else
        {
            *(text_out++)=*(text_in++);
            ord->data2[0]=OVERLAY_ELEMENT_CHARACTER;///set overlay element type
            ord->data2[1]=OVERLAY_TEXT_COLOUR_0+current_colour;///set colour

            ord->data3[1]=0;///glyph y offset
            ord->data3[2]=0;///nothing?
            ord->data3[3]=0;///nothing?

            ord++;
        }
    }
    *text_out='\0';

    w->textbox.text_length=strlen(text_in)+1;
    #warning is above correct? (was added w/o checking)
}











static void render_textbox_text(overlay_data * od,overlay_theme * theme,widget * w,int x_off,int y_off,rectangle bounds)
{
    int i,j;
    overlay_render_data ord;
    char * text=w->textbox.text;

    int highlight_x_start;
    int highlight_x_finish;
    bool highlighting=false;

    int offset= -w->textbox.offset;

    x_off+=w->base.r.x+theme->x_box_offset;
    y_off+=w->base.r.y+theme->y_box_offset;

    get_rectangle_overlap(&bounds,(rectangle){.x=x_off,.y=y_off,.w=w->base.r.w-theme->x_box_offset*2,.h=w->base.r.h-theme->y_box_offset*2});


    int h_begin=w->textbox.selection_begin;
    int h_end=w->textbox.selection_end;

    if(h_begin>h_end)
    {
        h_begin=w->textbox.selection_end;
        h_end=w->textbox.selection_begin;
    }

    bool caret_only=(h_begin==h_end);

    int line_spacing=theme->font.line_spacing;///w->textbox.font_index

    for(i=0;i<w->textbox.render_lines_count;i++)
    {
        if((offset+line_spacing > 0)&&(offset < w->textbox.visible_size))
        {
            highlight_x_start=highlight_x_finish=-1;

            j=w->textbox.render_lines[i].character_start;

            if(!caret_only)
            {
                if(j>h_begin)
                {
                    highlighting=true;
                }

                if(j>=h_end)
                {
                    highlighting=false;
                }
            }

            if(highlighting)
            {
                highlight_x_start=0;
            }

            for(;j<w->textbox.render_lines[i].character_finish;j++)
            {
                if( (text[j]>='!') && (text[j]<='~') )
                {
                    ord=w->textbox.glyph_render_data[j];

                    ord.data1[0]+=x_off;
                    ord.data1[1]+=y_off+offset;

                    ord.data3[0]+=x_off;
                    ord.data3[1]+=y_off+offset;

                    if(ord.data1[1]<bounds.y)
                    {
                        ord.data1[3]-=bounds.y-ord.data1[1];
                        ord.data1[1]=bounds.y;
                    }

                    if(ord.data1[1]+ord.data1[3]>bounds.y+bounds.h)
                    {
                        ord.data1[3]=bounds.y+bounds.h-ord.data1[1];
                    }

                    if(ord.data1[0]<bounds.x)
                    {
                        ord.data1[2]-=bounds.x-ord.data1[0];
                        ord.data1[0]=bounds.x;
                    }

                    if(ord.data1[0]+ord.data1[2]>bounds.x+bounds.w)
                    {
                        ord.data1[2]=bounds.x+bounds.w-ord.data1[0];
                    }

                    if((ord.data1[2]>0)&&(ord.data1[3]>0))upload_overlay_render_datum(od,&ord);
                }

                if(j==h_begin)
                {
                    if(caret_only)
                    {
                        #warning render caret
                    }
                    else
                    {
                        highlight_x_start=w->textbox.glyph_render_data[j].data1[0];
                        highlighting=true;
                    }
                }

                if(j==h_end)
                {
                    highlight_x_finish=w->textbox.glyph_render_data[j].data1[0];
                    highlighting=false;
                }
            }

            if(highlighting)
            {
                highlight_x_finish=w->textbox.render_lines[i].x_size;
            }

            if(highlight_x_start!=-1)
            {
                #warning .h should be set to text_height not line spacing
                render_rectangle(od,(rectangle){.x=highlight_x_start+x_off,.y=y_off+offset,.w=highlight_x_finish-highlight_x_start,.h=theme->font.line_spacing},bounds,OVERLAY_TEXT_HIGHLIGHT_COLOUR);///w->textbox.font_index

            }
        }

        offset+=line_spacing;
    }
}

static int find_textbox_character_index_from_position(overlay_theme * theme,widget * w,int mouse_x,int mouse_y)
{
    adjust_coordinates_to_widget_local(w,&mouse_x,&mouse_y);

    mouse_x-=theme->x_box_offset;
    mouse_y-=theme->y_box_offset;

    int x,i,line_id;
    bool beyond_end=false;

    if(mouse_y<0) return w->textbox.render_lines[w->textbox.offset/theme->font.line_spacing].character_start;///w->textbox.font_index
    if(mouse_y >= w->textbox.visible_size)
    {
        mouse_y=w->textbox.visible_size-1;
        beyond_end=true;
    }

    mouse_y+=w->textbox.offset;

    line_id=mouse_y/theme->font.line_spacing;///w->textbox.font_index

    if(line_id >= w->textbox.render_lines_count)return w->textbox.render_lines[w->textbox.render_lines_count-1].character_finish;
    if(beyond_end)return w->textbox.render_lines[line_id].character_finish;

    cvm_font * font= &theme->font;///w->textbox.font_index;
    char * text=w->textbox.text;
    char prev=0;

    x=0;

    for(i=w->textbox.render_lines[line_id].character_start;i < w->textbox.render_lines[line_id].character_finish;i++)
    {
        x=get_new_text_offset(font,prev,text[i],x);
        prev=text[i];

        if(x>=mouse_x)
        {
            return i;
        }
    }
    return i;

}






static bool textbox_widget_scroll(overlay_theme * theme,widget * w,int delta)
{
    w->textbox.offset+=delta*w->textbox.wheel_delta;/// 10 = scroll_factor

    if(w->textbox.offset > w->textbox.max_offset) w->textbox.offset=w->textbox.max_offset;
    if(w->textbox.offset < 0) w->textbox.offset=0;

    return true;
}

static void textbox_widget_left_click(overlay_theme * theme,widget * w,int x,int y)
{
    w->textbox.selection_end=w->textbox.selection_begin=find_textbox_character_index_from_position(theme,w,x,y);
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
    w->textbox.selection_end=find_textbox_character_index_from_position(theme,w,x,y);
}

static void textbox_copy_selection_to_clipboard(widget * w)
{
    int first,last;
    char tmp;

    if(w->textbox.selection_begin!=w->textbox.selection_end)
    {
        if(w->textbox.selection_end > w->textbox.selection_begin)
        {
            first   =w->textbox.selection_begin;
            last    =w->textbox.selection_end;
        }
        else
        {
            first   =w->textbox.selection_end;
            last    =w->textbox.selection_begin;
        }

        tmp=w->textbox.text[last];
        w->textbox.text[last]='\0';

        SDL_SetClipboardText(w->textbox.text+first);

        w->textbox.text[last]=tmp;
    }
}

static bool textbox_widget_key_down(overlay_theme * theme,widget * w,SDL_Keycode keycode)
{
    SDL_Keymod mod=SDL_GetModState();

    //bool caps=((mod&KMOD_CAPS)==KMOD_CAPS);
    //bool shift=(((mod&KMOD_RSHIFT)==KMOD_RSHIFT)||((mod&KMOD_LSHIFT)==KMOD_LSHIFT));

    #warning have last input time here


    if(mod&KMOD_CTRL)
    {
        switch(keycode)
        {
            case 'c':
            textbox_copy_selection_to_clipboard(w);
            break;

            case 'x':
            textbox_copy_selection_to_clipboard(w);
            break;

            case 'a':
            w->textbox.selection_begin=0;
            w->textbox.selection_end=strlen(w->textbox.text);
            break;

            default: return false;
        }
        return true;
    }

    return false;
}

static void textbox_widget_delete(widget * w)
{
    free(w->textbox.text);
    free(w->textbox.glyph_render_data);
    free(w->textbox.render_lines);
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
    .click_away     =   blank_widget_click_away,
    .add_child      =   blank_widget_add_child,
    .remove_child   =   blank_widget_remove_child,
    .wid_delete     =   textbox_widget_delete
};









static void textbox_widget_render(overlay_data * od,overlay_theme * theme,widget * w,int x_off,int y_off,rectangle bounds)
{
	theme->box_render(w->base.r,x_off,y_off,w->base.status,theme,od,bounds,OVERLAY_MAIN_COLOUR);
    render_textbox_text(od,theme,w,x_off,y_off,bounds);
}

static widget * textbox_widget_select(overlay_theme * theme,widget * w,int x_in,int y_in)
{
    if(theme->box_select(w->base.r,x_in,y_in,w->base.status,theme))return w;
    return NULL;
}

static void textbox_widget_min_w(overlay_theme * theme,widget * w)
{
    w->textbox.font_index=0;
    w->base.min_w=w->textbox.min_horizontal_glyphs * theme->font.max_glyph_width;///w->textbox.font_index
}

static void textbox_widget_min_h(overlay_theme * theme,widget * w)
{
    cvm_font * font= &theme->font;///w->textbox.font_index

    process_textbox_contents(theme,w,w->base.r.w-theme->x_box_offset*2,0);

    w->textbox.text_height=(w->textbox.render_lines_count-1)*font->line_spacing+font->font_height;

    if(w->textbox.min_visible_lines)
    {
        w->base.min_h=(w->textbox.min_visible_lines-1)*font->line_spacing+font->font_height+theme->y_box_offset*2;
    }
    else
    {
        w->base.min_h=w->textbox.text_height+theme->y_box_offset*2;
    }
}



static void textbox_widget_set_h(overlay_theme * theme,widget * w)
{
    w->textbox.wheel_delta= -theme->font.line_spacing;///w->textbox.font_index
    w->textbox.visible_size= w->base.r.h-2*theme->y_box_offset;

    if(w->textbox.min_visible_lines)
    {
        w->textbox.max_offset=w->textbox.text_height-w->textbox.visible_size;
        if(w->textbox.max_offset < 0)w->textbox.max_offset=0;
        if(w->textbox.offset > w->textbox.max_offset)w->textbox.offset = w->textbox.max_offset;
    }
    else
    {
        w->textbox.offset=0;
        w->textbox.max_offset=0;
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








void change_textbox_contents(widget * w,char * new_contents)///use responsibly, should call organise widgets function after calling this
{
    free(w->textbox.text);
    free(w->textbox.glyph_render_data);

    set_textbox_text(w,new_contents);


    w->textbox.offset=0;

    if(w->textbox.min_visible_lines)
    {
        set_widget_minimum_height(w,w->base.status&WIDGET_POS_FLAGS_V);
        organise_widget_vertically(w,w->base.r.y,w->base.min_h);
    }
    else
    {
        organise_toplevel_widget(w);
    }

}

widget * create_textbox(char * text,int min_horizontal_glyphs,int min_visible_lines)
{
    widget * w = create_widget(TEXTBOX_WIDGET);

    w->base.appearence_functions = &textbox_appearence_functions;
    w->base.behaviour_functions = &textbox_behaviour_functions;

    //w->textbox.text_alignment = alignment;

    w->textbox.glyph_render_data=NULL;
    w->textbox.text=NULL;

    set_textbox_text(w,text);

    w->textbox.render_lines = malloc(sizeof(textbox_line));
    w->textbox.render_lines_space = 1;
    w->textbox.render_lines_count = 0;

    w->textbox.min_visible_lines = min_visible_lines;///0=show all

    w->textbox.selection_begin = w->textbox.selection_end = 0;

    //w->textbox.previous_width=0;
    w->textbox.min_horizontal_glyphs=min_horizontal_glyphs;



    //w->textbox.content_changed=false;

    //w->textbox.wheel_delta=0;

    w->textbox.offset=0;
    w->textbox.max_offset=0;
    w->textbox.visible_size=0;
    w->textbox.wheel_delta=0;

    return w;
}


widget * create_textbox_scrollbar(widget * textbox)
{
    widget * w=create_sliderbar(&textbox->textbox.offset,0,0,NULL,NULL,false,WIDGET_VERTICAL,0);
    set_sliderbar_other_values(w,NULL,&textbox->textbox.max_offset,&textbox->textbox.visible_size,&textbox->textbox.wheel_delta);

    return w;
}
