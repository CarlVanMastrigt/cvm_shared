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



static void delete_selected_enterbox_contents(widget * w)
{
    int first,last,length;
    char * text;

    length=strlen(w->enterbox.text);

    if(w->enterbox.selection_begin<0)w->enterbox.selection_begin=0;
    if(w->enterbox.selection_end<0)w->enterbox.selection_end=0;

    if(w->enterbox.selection_begin>length)w->enterbox.selection_begin=length;
    if(w->enterbox.selection_end>length)w->enterbox.selection_end=length;

    if(w->enterbox.selection_begin!=w->enterbox.selection_end)
    {
        text=w->enterbox.text;

        if(w->enterbox.selection_begin>w->enterbox.selection_end)
        {
            first=w->enterbox.selection_end;
            last=w->enterbox.selection_begin;
        }
        else
        {
            first=w->enterbox.selection_begin;
            last=w->enterbox.selection_end;
        }

        memmove(text+first,text+last,length-last+1);

        w->enterbox.selection_end=w->enterbox.selection_begin=first;
    }
}

static void enterbox_copy_selection_to_clipboard(widget * w)
{
    int first,last;
    char tmp;

    if(w->enterbox.selection_begin!=w->enterbox.selection_end)
    {
        if(w->enterbox.selection_end > w->enterbox.selection_begin)
        {
            first   =w->enterbox.selection_begin;
            last    =w->enterbox.selection_end;
        }
        else
        {
            first   =w->enterbox.selection_end;
            last    =w->enterbox.selection_begin;
        }

        tmp=w->enterbox.text[last];
        w->enterbox.text[last]='\0';

        SDL_SetClipboardText(w->enterbox.text+first);

        w->enterbox.text[last]=tmp;
    }
}

static void enterbox_input_character(widget * w,char c)
{
    delete_selected_enterbox_contents(w);

    char * text=w->enterbox.text;

    int length=strlen(text);
    int start=w->enterbox.selection_begin;

    if(length < w->enterbox.text_max_length)
    {
        memmove(text+start+1,text+start,length-start+1);
        text[start]=c;

        w->enterbox.selection_begin= ++w->enterbox.selection_end;
    }
}

static void enterbox_input_text(widget * w,char * in)
{
    delete_selected_enterbox_contents(w);

    char * text=w->enterbox.text;

    int start=w->enterbox.selection_begin;
    int length=strlen(text);
    int in_length=strlen(in);

    if(length < w->enterbox.text_max_length)
    {
        if(length+in_length > w->enterbox.text_max_length)
        {
            in_length=w->enterbox.text_max_length-length;
        }

        memmove(text+start+in_length,text+start,length-start+1);
        memcpy(text+start,in,in_length);

        w->enterbox.selection_begin= (w->enterbox.selection_end+=in_length);
    }
}

static void enterbox_check_visible_offset(widget * w,overlay_theme * theme)
{
    int i,text_length,caret_offset=0,e=w->enterbox.selection_end;
    char prev=0;

    cvm_font * font=theme->fonts+0;
    char * text=w->enterbox.text;

    int side_space=2*theme->h_bar_text_offset;

    text_length=calculate_text_length(theme,text,0);
    if((text_length+1)<(w->base.r.w-side_space))///+1 for caret
    {
        w->enterbox.visible_offset=0;
        return;
    }

    if((text_length-w->enterbox.visible_offset)<(w->base.r.w-side_space-1))///1 extra for caret
    {
        w->enterbox.visible_offset=text_length-w->base.r.w+side_space+1;///1 extra for caret
    }

    for(i=0;text[i];i++)
    {
        if(i==e)
        {
            break;
        }
        caret_offset=get_new_text_offset(font,prev,text[i],caret_offset);
        prev=text[i];
        ///combine calculate text  length with this
    }

    if(caret_offset<w->enterbox.visible_offset)
    {
        w->enterbox.visible_offset=caret_offset;
    }

    if(caret_offset>=(w->enterbox.visible_offset+w->base.r.w-side_space))
    {
        w->enterbox.visible_offset=caret_offset-w->base.r.w+side_space+1;///caret is 1 wide ergo 1 extra space
    }

    if(w->enterbox.visible_offset<0)
    {
        w->enterbox.visible_offset=0;
    }
}

static int find_enterbox_character_index_from_position(overlay_theme * theme,widget * w,int mouse_x,int mouse_y)
{
    adjust_coordinates_to_widget_local(w,&mouse_x,&mouse_y);

    int i,x=theme->h_bar_text_offset-w->enterbox.visible_offset;
    //mouse_x-= //w->enterbox.text_x_offset;
    char prev=0;

    cvm_font * font=theme->fonts+0;
    char * text=w->enterbox.text;

    for(i=0;text[i];i++)
    {
        x=get_new_text_offset(font,prev,text[i],x);
        prev=text[i];

        if(x>=mouse_x)
        {
            break;
        }
    }

    return i;
}

static bool enterbox_key_actions(overlay_theme * theme,widget * w,SDL_Keycode keycode)
{
    #warning change keycodes to scancodes

    //puts(SDL_GetKeyName(keycode));
    SDL_Keymod mod=SDL_GetModState();

    bool caps=((mod&KMOD_CAPS)==KMOD_CAPS);
    bool shift=(((mod&KMOD_RSHIFT)==KMOD_RSHIFT)||((mod&KMOD_LSHIFT)==KMOD_LSHIFT));

    #warning have last input time here


    if(mod&KMOD_CTRL)
    {
        switch(keycode)
        {
            case 'c':
            enterbox_copy_selection_to_clipboard(w);
            break;

            case 'x':
            enterbox_copy_selection_to_clipboard(w);
            delete_selected_enterbox_contents(w);
            break;

            case 'v':
            if(SDL_HasClipboardText())
            {
                enterbox_input_text(w,SDL_GetClipboardText());
            }
            break;

            case 'a':
            w->enterbox.selection_begin=0;
            w->enterbox.selection_end=strlen(w->enterbox.text);
            break;

            default: return false;
        }
        return true;
    }
    if(mod&KMOD_ALT)
    {
        return false;
    }

    switch(keycode)
    {
        case SDLK_BACKSPACE:
        if((w->enterbox.selection_begin==w->enterbox.selection_end)&&(w->enterbox.selection_begin>0))w->enterbox.selection_begin--;///generates appropriate selection to remove if "selection" == caret
        delete_selected_enterbox_contents(w);
        break;

        case SDLK_DELETE:
        if((w->enterbox.selection_begin==w->enterbox.selection_end)&&(w->enterbox.selection_end<w->enterbox.text_max_length))w->enterbox.selection_end++;///generates appropriate selection to remove if "selection" == caret
        delete_selected_enterbox_contents(w);
        break;

        case SDLK_LEFT:
        if(w->enterbox.selection_begin==w->enterbox.selection_end)
        {
            if(w->enterbox.selection_begin>0)
            {
                w->enterbox.selection_begin--;
                w->enterbox.selection_end--;
            }
        }
        else
        {
            if(w->enterbox.selection_begin<w->enterbox.selection_end)w->enterbox.selection_end=w->enterbox.selection_begin;
            else w->enterbox.selection_begin=w->enterbox.selection_end;
        }
        break;

        case SDLK_RIGHT:
        if(w->enterbox.selection_begin==w->enterbox.selection_end)
        {
            if(w->enterbox.selection_begin<strlen(w->enterbox.text))
            {
                w->enterbox.selection_begin++;
                w->enterbox.selection_end++;
            }
        }
        else
        {
            if(w->enterbox.selection_begin>w->enterbox.selection_end)w->enterbox.selection_end=w->enterbox.selection_begin;
            else w->enterbox.selection_begin=w->enterbox.selection_end;
        }
        break;

        case SDLK_UP:
        w->enterbox.selection_begin=w->enterbox.selection_end=0;
        break;

        case SDLK_DOWN:
        w->enterbox.selection_begin=w->enterbox.selection_end=strlen(w->enterbox.text);
        break;

        case SDLK_HOME:
        w->enterbox.selection_begin=w->enterbox.selection_end=0;
        break;

        case SDLK_END:
        w->enterbox.selection_begin=w->enterbox.selection_end=strlen(w->enterbox.text);
        break;

        ///SPECIAL_ENTERBOX_BEHAVIOUR
        case SDLK_RETURN:
        if((w->enterbox.func)&&(!w->enterbox.activate_upon_deselect))/// !activate_upon_deselect because the set_currently_active_widget(NULL) will call click_away which executes func if activate_upon_deselect
        {
            w->enterbox.func(w);
        }
        set_currently_active_widget(NULL);
        break;
    }


    /// actual characters

    if((keycode<' ')||(keycode>'~'))
    {
        return false;
    }

    if((keycode>='a')&&(keycode<='z'))
    {
        enterbox_input_character(w,keycode+(caps^shift)*('A'-'a'));
        return true;
    }

    if(!shift)
    {
        enterbox_input_character(w,keycode);
        return true;
    }

    switch(keycode)
    {
        case '`':keycode='~';break;
        case '1':keycode='!';break;
        case '2':keycode='@';break;
        case '3':keycode='#';break;
        case '4':keycode='$';break;
        case '5':keycode='%';break;
        case '6':keycode='^';break;
        case '7':keycode='&';break;
        case '8':keycode='*';break;
        case '9':keycode='(';break;
        case '0':keycode=')';break;
        case '-':keycode='_';break;
        case '=':keycode='+';break;
        case '[':keycode='{';break;
        case ']':keycode='}';break;
        case '\\':keycode='|';break;
        case ';':keycode=':';break;
        case '\'':keycode='"';break;
        case ',':keycode='<';break;
        case '.':keycode='>';break;
        case '/':keycode='?';break;
        default:return false;
    }

    enterbox_input_character(w,keycode);

    return true;
}

static bool handle_enterbox_key(overlay_theme * theme,widget * w,SDL_Keycode keycode)
{
    bool rtrn=enterbox_key_actions(theme,w,keycode);

    if(w->enterbox.upon_input!=NULL)
    {
        w->enterbox.upon_input(w);
    }

    return rtrn;
}






static void enterbox_widget_left_click(overlay_theme * theme,widget * w,int x,int y)
{
    w->enterbox.selection_begin=w->enterbox.selection_end=find_enterbox_character_index_from_position(theme,w,x,y);

    if(w->enterbox.upon_input!=NULL)
    {
        w->enterbox.upon_input(w);
    }
}

static bool enterbox_widget_left_release(overlay_theme * theme,widget * clicked,widget * released,int x,int y)
{
    return true;
}

static void enterbox_widget_mouse_movement(overlay_theme * theme,widget * w,int x,int y)
{
    w->enterbox.selection_end=find_enterbox_character_index_from_position(theme,w,x,y);
    enterbox_check_visible_offset(w,theme);
}

static bool enterbox_widget_key_down(overlay_theme * theme,widget * w,SDL_Keycode keycode)
{
    bool rtrn=handle_enterbox_key(theme,w,keycode);

    enterbox_check_visible_offset(w,theme);

    return rtrn;
}

static void enterbox_widget_click_away(overlay_theme * theme,widget * w)
{
    if((w->enterbox.func)&&(w->enterbox.activate_upon_deselect))
    {
        w->enterbox.func(w);
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
    .click_away     =   enterbox_widget_click_away,
    .add_child      =   blank_widget_add_child,
    .remove_child   =   blank_widget_remove_child,
    .wid_delete     =   enterbox_widget_delete
};






static void render_enterbox_text_highlighting(overlay_data * od,overlay_theme * theme,widget * w,int x_off,int y_off,rectangle bounds,int font_index)
{
    if(!is_currently_active_widget(w))return;

    int i;

    cvm_font * font=theme->fonts+font_index;
    char * text=w->enterbox.text;

    int x1,x2;
    bool highlighting=false;

    int sb=w->enterbox.selection_begin;
    int se=w->enterbox.selection_end;

    int x=0;
    char prev=0;

    x1=x2=0;


    if(sb==se)
    {
        for(i=0;text[i];i++)
        {
            if((i==sb))
            {
                break;
            }

            x=get_new_text_offset(font,prev,text[i],x);
            prev=text[i];
        }

        render_rectangle(od,(rectangle){.x=x+x_off,.y=y_off,.w=1,.h=theme->fonts[font_index].font_height},bounds,OVERLAY_TEXT_COLOUR_0);
    }
    else
    {
        for(i=0;text[i];i++)
        {
            if((i==sb)||(i==se))
            {
                if(highlighting)
                {
                    x2=x;
                    highlighting=false;
                }
                else
                {
                    x1=x;
                    highlighting=true;
                }
            }

            x=get_new_text_offset(font,prev,text[i],x);
            prev=text[i];
        }

        if(highlighting)x2=x;

        render_rectangle(od,(rectangle){.x=x1+x_off,.y=y_off,.w=x2-x1,.h=theme->fonts[font_index].font_height},bounds,OVERLAY_TEXT_HIGHLIGHT_COLOUR);
    }
}












static void enterbox_widget_render(overlay_data * od,overlay_theme * theme,widget * w,int x_off,int y_off,rectangle bounds)
{
	theme->h_text_bar_render(w->base.r,x_off,y_off,w->base.status,theme,od,bounds,OVERLAY_MAIN_COLOUR,NULL);

    y_off+=w->base.r.y+(w->base.r.h-theme->fonts[0].font_height)/2;
    x_off+=w->base.r.x+theme->h_bar_text_offset;
    get_rectangle_overlap(&bounds,(rectangle){.x=x_off,.y=y_off,.w=w->base.r.w-2*theme->h_bar_text_offset,.h=theme->fonts[0].font_height});

    x_off-=w->enterbox.visible_offset;

    render_overlay_text(od,theme,w->enterbox.text,x_off,y_off,bounds,0,0);
	render_enterbox_text_highlighting(od,theme,w,x_off,y_off,bounds,0);
}

static widget * enterbox_widget_select(overlay_theme * theme,widget * w,int x_in,int y_in)
{
    if(theme->h_bar_select(w->base.r,x_in,y_in,w->base.status,theme))return w;

    return NULL;
}

static void enterbox_widget_min_w(overlay_theme * theme,widget * w)
{
    w->base.min_w = 2*theme->h_bar_text_offset+theme->fonts[0].max_glyph_width*w->enterbox.text_min_visible+1;///+1 for caret, only necessary when bearingX is 0
}

static void enterbox_widget_min_h(overlay_theme * theme,widget * w)
{
    w->base.min_h = theme->base_unit_h;
}




static widget_appearence_function_set enterbox_appearence_functions=
(widget_appearence_function_set)
{
    .render =   enterbox_widget_render,
    .select =   enterbox_widget_select,
    .min_w  =   enterbox_widget_min_w,
    .min_h  =   enterbox_widget_min_h,
    .set_w  =   blank_widget_set_w,
    .set_h  =   blank_widget_set_h
};


widget * create_enterbox(int text_max_length,int text_min_visible,char * initial_text,widget_function func,void * data,bool activate_upon_deselect,bool free_data)
{
	widget * w=create_widget(ENTERBOX_WIDGET);

	w->enterbox.data=data;
	w->enterbox.func=func;

	w->enterbox.text_max_length=text_max_length;
	w->enterbox.text_min_visible=text_min_visible;

	w->enterbox.text=malloc(text_max_length+1);
	w->enterbox.upon_input=NULL;

    w->enterbox.activate_upon_deselect=activate_upon_deselect;
	w->enterbox.free_data=free_data;

	#warning remove next lines
	int i;
	for(i=0;i<text_max_length;i++)w->enterbox.text[i]='x';

	set_enterbox_text(w,initial_text);

	w->base.appearence_functions=&enterbox_appearence_functions;
	w->base.behaviour_functions=&enterbox_behaviour_functions;

	return w;
}




void set_enterbox_text(widget * w,char * text)
{
    if(text==NULL) w->enterbox.text[0]='\0';
    else strncpy(w->enterbox.text,text,w->enterbox.text_max_length);
    w->enterbox.text[w->enterbox.text_max_length]='\0';
    w->enterbox.visible_offset=0;
    w->enterbox.selection_begin=0;
    w->enterbox.selection_end=0;
}

void set_enterbox_text_using_int(widget * w,int v)
{
    char buffer[16];
    snprintf(buffer,16,"%d",v);
    set_enterbox_text(w,buffer);
}

int get_int_from_enterbox_text(widget * w)
{
    int r;
    sscanf(w->enterbox.text,"%d",&r);
    return r;
}

void set_enterbox_action_upon_input(widget * w,widget_function func)
{
    w->enterbox.upon_input=func;
}
