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



static inline void enterbox_delete_selection(widget * w,int s_begin,int s_end)
{
    if(s_begin!=s_end)
    {
        memmove(w->enterbox.text + s_begin,w->enterbox.text + s_end,strlen(w->enterbox.text + s_end) + 1);/// +1 for null terminating character
        w->enterbox.selection_begin = w->enterbox.selection_end = s_begin;
    }
}

static void enterbox_copy_selection_to_clipboard(widget * w)
{
    int s_begin,s_end;
    char tmp;

    if(w->enterbox.selection_end > w->enterbox.selection_begin) s_begin=w->enterbox.selection_begin, s_end=w->enterbox.selection_end;
    else s_begin=w->enterbox.selection_end, s_end=w->enterbox.selection_begin;


    /// copy all contents if nothing selected?
    if(s_begin!=s_end)
    {
        tmp=w->enterbox.text[s_end];
        w->enterbox.text[s_end]='\0';

        SDL_SetClipboardText(w->enterbox.text+s_begin);

        w->enterbox.text[s_end]=tmp;
    }
    else if(*w->enterbox.text)///not empty
    {
        SDL_SetClipboardText(w->enterbox.text);
    }
}

/// perhaps return pass/fail such that enterbox can flash/fade a colour upon failure
static void enterbox_enter_text(widget * w,char * text)
{
    int s_begin,s_end,new_glyph_count,new_strlen;

    if(w->enterbox.selection_end > w->enterbox.selection_begin) s_begin=w->enterbox.selection_begin, s_end=w->enterbox.selection_end;
    else s_begin=w->enterbox.selection_end, s_end=w->enterbox.selection_begin;

    if( text && (new_strlen=strlen(text)) > 0 && cvm_overlay_utf8_validate_string_and_count_glyphs(text,&new_glyph_count) &&
        ///new text exists, isnt empty and is valid unicode (also get glyph count of input text)
        cvm_overlay_utf8_count_glyphs_outside_range(w->enterbox.text,s_begin,s_end) + new_glyph_count <= w->enterbox.max_glyphs &&
        /// adding the new glyphs while replacing the old ones wont exceed limit
        strlen(w->enterbox.text) + s_begin - s_end + new_strlen <= w->enterbox.max_strlen)
        /// adding the new chars while replacing the old ones wont exceed limit
    {
        memmove(w->enterbox.text + s_begin + new_strlen,w->enterbox.text + s_end,strlen(w->enterbox.text + s_end) + 1);/// +1 for null terminating character
        memcpy(w->enterbox.text + s_begin,text,new_strlen);
        w->enterbox.selection_begin = w->enterbox.selection_end = s_begin + new_strlen;

        ///if visible<max ensure visibility here (possibly do check inside ensure visibility function)
    }
    //return true;
}

static void enterbox_check_visible_offset(widget * w,overlay_theme * theme)
{
//    int i,text_length,caret_offset=0,e=w->enterbox.selection_end;
//    char prev=0;
//
//    cvm_font * font= &theme->font;
//    char * text=w->enterbox.text;
//
//    int side_space=2*theme->h_bar_text_offset;
//
//    text_length=calculate_text_length(theme,text,0);
//    if((text_length+1)<(w->base.r.w-side_space))///+1 for caret
//    {
//        w->enterbox.visible_offset=0;
//        return;
//    }
//
//    if((text_length-w->enterbox.visible_offset)<(w->base.r.w-side_space-1))///1 extra for caret
//    {
//        w->enterbox.visible_offset=text_length-w->base.r.w+side_space+1;///1 extra for caret
//    }
//
//    for(i=0;text[i];i++)
//    {
//        if(i==e)
//        {
//            break;
//        }
//        caret_offset=get_new_text_offset(font,prev,text[i],caret_offset);
//        prev=text[i];
//        ///combine calculate text  length with this
//    }
//
//    if(caret_offset<w->enterbox.visible_offset)
//    {
//        w->enterbox.visible_offset=caret_offset;
//    }
//
//    if(caret_offset>=(w->enterbox.visible_offset+w->base.r.w-side_space))
//    {
//        w->enterbox.visible_offset=caret_offset-w->base.r.w+side_space+1;///caret is 1 wide ergo 1 extra space
//    }
//
//    if(w->enterbox.visible_offset<0)
//    {
//        w->enterbox.visible_offset=0;
//    }
}


static bool enterbox_key_actions(overlay_theme * theme,widget * w,SDL_Keycode keycode)
{
    int s_begin,s_end;
    bool accepted;
    SDL_Keymod mod;

    if(w->enterbox.selection_end > w->enterbox.selection_begin) s_begin=w->enterbox.selection_begin, s_end=w->enterbox.selection_end;
    else s_begin=w->enterbox.selection_end, s_end=w->enterbox.selection_begin;

    mod=SDL_GetModState();
    accepted=false;

    /// KMOD_SHIFT KMOD_CTRL KMOD_ALT
    switch(keycode)
    {
        case SDLK_c:
        if(mod&KMOD_CTRL)
        {
            enterbox_copy_selection_to_clipboard(w);
            accepted=true;
        }
        break;

        case SDLK_x:
        if(mod&KMOD_CTRL)
        {
            enterbox_copy_selection_to_clipboard(w);
            enterbox_delete_selection(w,s_begin,s_end);
            accepted=true;
        }
        break;

        case SDLK_v:
        if(mod&KMOD_CTRL)
        {
            if(SDL_HasClipboardText())
            {
                enterbox_enter_text(w,SDL_GetClipboardText());
            }
            accepted=true;
        }
        break;

        case SDLK_a:
        if(mod&KMOD_CTRL)
        {
            w->enterbox.selection_begin=0;
            w->enterbox.selection_end=strlen(w->enterbox.text);
            accepted=true;
        }
        break;

        case SDLK_BACKSPACE:
        if(s_begin==s_end) s_begin=cvm_overlay_utf8_get_previous(w->enterbox.text,s_begin);
        enterbox_delete_selection(w,s_begin,s_end);
        accepted=true;
        break;

        case SDLK_DELETE:
        if(s_begin==s_end) s_end=cvm_overlay_utf8_get_next(w->enterbox.text,s_end);
        enterbox_delete_selection(w,s_begin,s_end);
        accepted=true;
        break;

        default:;
    }
    return accepted;





    bool caps=((mod&KMOD_CAPS)==KMOD_CAPS);
    bool shift=(((mod&KMOD_RSHIFT)==KMOD_RSHIFT)||((mod&KMOD_LSHIFT)==KMOD_LSHIFT));///KMOD_SHIFT

    #warning have last input time here


    if(mod&KMOD_CTRL)
    {
        switch(keycode)
        {
            case SDLK_c:
            enterbox_copy_selection_to_clipboard(w);
            break;

            case 'x':
            enterbox_copy_selection_to_clipboard(w);
            delete_selected_enterbox_contents(w);
            break;

            case 'v':
            if(SDL_HasClipboardText())
            {
                //enterbox_input_text(w,SDL_GetClipboardText());
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
//        if((w->enterbox.selection_begin==w->enterbox.selection_end)&&(w->enterbox.selection_end<w->enterbox.text_max_length))w->enterbox.selection_end++;///generates appropriate selection to remove if "selection" == caret
//        delete_selected_enterbox_contents(w);
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
        if((w->enterbox.activation_func)&&(!w->enterbox.activate_upon_deselect))/// !activate_upon_deselect because the set_currently_active_widget(NULL) will call click_away which executes func if activate_upon_deselect
        {
            w->enterbox.activation_func(w);
        }
        set_currently_active_widget(NULL);
        break;
    }


    /// actual characters

//    if((keycode<' ')||(keycode>'~'))
//    {
//        return false;
//    }

//    if((keycode>='a')&&(keycode<='z'))
//    {
//        //enterbox_input_character(w,keycode+(caps^shift)*('A'-'a'));
//        return true;
//    }

//    if(!shift)
//    {
//        //enterbox_input_character(w,keycode);
//        return true;
//    }

//    switch(keycode)
//    {
//        case '`':keycode='~';break;
//        case '1':keycode='!';break;
//        case '2':keycode='@';break;
//        case '3':keycode='#';break;
//        case '4':keycode='$';break;
//        case '5':keycode='%';break;
//        case '6':keycode='^';break;
//        case '7':keycode='&';break;
//        case '8':keycode='*';break;
//        case '9':keycode='(';break;
//        case '0':keycode=')';break;
//        case '-':keycode='_';break;
//        case '=':keycode='+';break;
//        case '[':keycode='{';break;
//        case ']':keycode='}';break;
//        case '\\':keycode='|';break;
//        case ';':keycode=':';break;
//        case '\'':keycode='"';break;
//        case ',':keycode='<';break;
//        case '.':keycode='>';break;
//        case '/':keycode='?';break;
//        default:return false;
//    }

    //enterbox_input_character(w,keycode);

    return true;
}

//static bool handle_enterbox_key(overlay_theme * theme,widget * w,SDL_Keycode keycode)
//{
//    bool rtrn=enterbox_key_actions(theme,w,keycode);
//
//    if(w->enterbox.upon_input!=NULL)
//    {
//        w->enterbox.upon_input(w);
//    }
//
//    return rtrn;
//}



#warning need ctrl and alt to disable textinput
/// ^ this is done automatically it turns out

/// U+ input not supported...

///other implementations stop rendering text correctly when selection changes (possibly errantly? no way to handle?)
/// otherwise would need to track current edit text (is useful for underline...) as well as selected text and move both around in tandem, removing and placing edited test as necessary (inelegant)
///or render composition over the top of other text (i do like this paradigm) place over current caret and disregard overlaid text in selection (do limit to within window/current bounding box though?)
///     ^ give composition a special box colour? make active and if enterbox was active (probably the case) make it inactive
///     ^ negates real need for underline as its obvious what is being entered and removes need for sizing enterbox to accomodate
///     ^ make space sufficient to accommodate max unicode length (including variation sequence)
///     ^ also allows unicode to elegantly replace selected text while still showing what will be replaced!
///     ^ theme specific composition offset?

/// !! rendering over the top in popup window isnt really valid! need to ensure no subsequent glyphs get rendered over the top. instead change enterbox colour and replace contents as simple/temporary solution?

/// text editing (having popup be present) should disable navigation (arrow keys) -- system does this automatically!

///do on key press/release, need proper check to is active to stop/start correctly if clicking away while keys still pressed

static void enterbox_widget_left_click(overlay_theme * theme,widget * w,int x,int y)
{
    if(!SDL_IsTextInputActive())
    {
        SDL_Rect r={0,0,100,20};
        SDL_SetTextInputRect(&r);
        SDL_StartTextInput();///need to only call if not already active...
    }
    /// currently active widget is set before l_click is called, so checking against that is not a way to test id this has already been called
    //puts("start text");

//    w->enterbox.selection_begin=w->enterbox.selection_end=find_enterbox_character_index_from_position(theme,w,x,y);

    adjust_coordinates_to_widget_local(w,&x,&y);
    w->enterbox.selection_begin=w->enterbox.selection_end=overlay_text_find_offset_simple(&theme->font_,w->enterbox.text,x-theme->h_bar_text_offset-w->enterbox.visible_offset);

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
    adjust_coordinates_to_widget_local(w,&x,&y);
    w->enterbox.selection_end=overlay_text_find_offset_simple(&theme->font_,w->enterbox.text,x-theme->h_bar_text_offset-w->enterbox.visible_offset);
    printf(">emb: %d\n",w->enterbox.selection_end);

    enterbox_check_visible_offset(w,theme);
}

static bool enterbox_widget_key_down(overlay_theme * theme,widget * w,SDL_Keycode keycode)
{
//    bool rtrn=handle_enterbox_key(theme,w,keycode);
//
//    enterbox_check_visible_offset(w,theme);
//
//    return rtrn;
    return enterbox_key_actions(theme,w,keycode);
}

static bool enterbox_widget_text_input(overlay_theme * theme,widget * w,char * text)
{
    printf("enterbox input: %s\n",text);
    enterbox_enter_text(w,text);
    return true;
}

static bool enterbox_widget_text_edit(overlay_theme * theme,widget * w,char * text,int start,int length)
{
    strncpy(w->enterbox.composition_text,text,CVM_OVERLAY_MAX_COMPOSITION_BYTES);
    w->enterbox.composition_text[CVM_OVERLAY_MAX_COMPOSITION_BYTES-1]='\0';
    ///put this in popup?
    printf("enterbox edit: %s %d %d\n",text,start,length);
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






static void render_enterbox_text_highlighting(overlay_data * od,overlay_theme * theme,widget * w,int x_off,int y_off,rectangle bounds,int font_index)
{
//    if(!is_currently_active_widget(w))return;
//
//    int i;
//
//    cvm_font * font= &theme->font;
//    char * text=w->enterbox.text;
//
//    int x1,x2;
//    bool highlighting=false;
//
//    int sb=w->enterbox.selection_begin;
//    int se=w->enterbox.selection_end;
//
//    int x=0;
//    char prev=0;
//
//    x1=x2=0;
//
//
//    if(sb==se)
//    {
//        for(i=0;text[i];i++)
//        {
//            if((i==sb))
//            {
//                break;
//            }
//
//            x=get_new_text_offset(font,prev,text[i],x);
//            prev=text[i];
//        }
//
//        render_rectangle(od,(rectangle){.x=x+x_off,.y=y_off,.w=1,.h=theme->font.font_height},bounds,OVERLAY_TEXT_COLOUR_0);
//    }
//    else
//    {
//        for(i=0;text[i];i++)
//        {
//            if((i==sb)||(i==se))
//            {
//                if(highlighting)
//                {
//                    x2=x;
//                    highlighting=false;
//                }
//                else
//                {
//                    x1=x;
//                    highlighting=true;
//                }
//            }
//
//            x=get_new_text_offset(font,prev,text[i],x);
//            prev=text[i];
//        }
//
//        if(highlighting)x2=x;
//
//        render_rectangle(od,(rectangle){.x=x1+x_off,.y=y_off,.w=x2-x1,.h=theme->font.font_height},bounds,OVERLAY_TEXT_HIGHLIGHT_COLOUR);
//    }
}












static void enterbox_widget_render(overlay_data * od,overlay_theme * theme,widget * w,int x_off,int y_off,rectangle bounds)
{
    if(w->enterbox.update_contents_func && !is_currently_active_widget(w))w->enterbox.update_contents_func(w);
	//theme->h_text_bar_render(w->base.r,x_off,y_off,w->base.status,theme,od,bounds,OVERLAY_MAIN_COLOUR,NULL);
	overlay_colour_ c=OVERLAY_TEXT_COLOUR_0_;
	char * t=w->enterbox.text;

	if(*w->enterbox.composition_text)
    {
        c=OVERLAY_BACKGROUND_COLOUR_;
        t=w->enterbox.composition_text;
    }

	theme->h_text_bar_render(rectangle_add_offset(rectangle_new_conversion(w->base.r),x_off,y_off),w->base.status,theme,od,rectangle_new_conversion(bounds),OVERLAY_MAIN_COLOUR_,t,c);
//    y_off+=w->base.r.y+(w->base.r.h-theme->font.font_height)/2;
//    x_off+=w->base.r.x+theme->h_bar_text_offset;
//    get_rectangle_overlap(&bounds,(rectangle){.x=x_off,.y=y_off,.w=w->base.r.w-2*theme->h_bar_text_offset,.h=theme->font.font_height});
//
//    x_off-=w->enterbox.visible_offset;
//
//    render_overlay_text(od,theme,w->enterbox.text,x_off,y_off,bounds,0,0);
//	render_enterbox_text_highlighting(od,theme,w,x_off,y_off,bounds,0);

///need appropriate way to handle text input/editing, new paradigm?
}

static widget * enterbox_widget_select(overlay_theme * theme,widget * w,int x_in,int y_in)
{
    if(theme->h_bar_select(rectangle_subtract_offset(rectangle_new_conversion(w->base.r),x_in,y_in),w->base.status,theme))return w;

    return NULL;
}

static void enterbox_widget_min_w(overlay_theme * theme,widget * w)
{
    w->base.min_w = 2*theme->h_bar_text_offset+theme->font_.max_advance*w->enterbox.min_glyphs_visible;///+1 for caret, only necessary when bearingX is 0
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


//widget * create_enterbox(int text_max_length,int text_min_visible,char * initial_text,widget_function activation_func,void * data,widget_function update_contents_func,bool activate_upon_deselect,bool free_data)


widget * create_enterbox(int max_strlen,int max_glyphs,int min_glyphs_visible,char * initial_text,widget_function activation_func,void * data,widget_function update_contents_func,bool activate_upon_deselect,bool free_data)
{
	widget * w=create_widget(ENTERBOX_WIDGET);

	w->enterbox.data=data;
	w->enterbox.activation_func=activation_func;
	w->enterbox.update_contents_func=update_contents_func;

	w->enterbox.max_strlen=max_strlen;/// +1 ?
	w->enterbox.max_glyphs=max_glyphs;
	w->enterbox.min_glyphs_visible=min_glyphs_visible;

	w->enterbox.text=malloc(max_strlen+1);///multiply by max unicode character size (including variation sequences) ?
	w->enterbox.upon_input=NULL;

    w->enterbox.activate_upon_deselect=activate_upon_deselect;
	w->enterbox.free_data=free_data;

	set_enterbox_text(w,initial_text);

	w->enterbox.composition_text[0]='\0';///use first character as key as to whether text is present/valid

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
    w->enterbox.selection_begin=0;
    w->enterbox.selection_end=0;
}

void set_enterbox_action_upon_input(widget * w,widget_function func)
{
    w->enterbox.upon_input=func;
}
