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


typedef struct widget_adjuster_pair_data
{
    widget * slider_bar;
    widget * enterbox;

    void * data;
    void(*func)(void*);
}
widget_adjuster_pair_data;






void adjuster_pair_slider_bar_function(widget * w)
{
    #warning perhaps instead allow enterbox to read/set its contents in render step when not active? (allows multiple enterboxes to represent same piece of data simultaneously)
    //set_enterbox_text_using_int(w->slider_bar.data,*w->slider_bar.value_ptr);
}

void adjuster_pair_enterbox_function(widget * w)
{
    /// could put contained data in some external structure, e.g. one to handle ints, one to handle floats?
    int r;
    if(sscanf(w->enterbox.text,"%d",&r)==1)
    {
        set_slider_bar_value(w->enterbox.data,r);
    }
}

void adjuster_pair_enterbox_update_contents_function(widget * w)
{
    widget * slider_bar=w->enterbox.data;

    char buffer[16];
    snprintf(buffer,16,"%d",*slider_bar->slider_bar.value_ptr);
    set_enterbox_text(w,buffer);
}

widget * create_adjuster_pair(struct widget_context* context, int * value_ptr,int min_value,int max_value,int text_space,int bar_fraction,int scroll_fraction)
{
    widget * box=create_box(context, WIDGET_HORIZONTAL, WIDGET_FIRST_DISTRIBUTED);


    char text[16];
    sprintf(text,"%d",*value_ptr);
    #warning revise adjuster_pair, does it have any real practical use, will probably always need extended functionality to what is provided, instead package above to be called by propper function
    /// specifically doesnt allow some specialised operation to be called upon update of value, may be useful in most cases though, just reading value externally whenever its needed/used

    widget * slider_bar=add_child_to_parent(box,create_slider_bar_fixed(context, value_ptr,min_value,max_value,bar_fraction,scroll_fraction,adjuster_pair_slider_bar_function,NULL,false));

	//slider_bar->slider_bar.data=add_child_to_parent(box,create_enterbox(text_space,text_space,text_space,text,adjuster_pair_enterbox_function,slider_bar,adjuster_pair_enterbox_update_contents_function,true,false));
	slider_bar->slider_bar.data=add_child_to_parent(box,create_enterbox_simple(context, text_space,text,adjuster_pair_enterbox_function,adjuster_pair_enterbox_update_contents_function,slider_bar,false,true));

    return box;
}









//void general_scroll_change(int * value,int min,int max,int magnitude,int delta)
//{
//    if(!magnitude)magnitude=abs(max - min)/16;
//    if(magnitude<0)magnitude=1;
//
//    *value+=magnitude*delta;
//
//    if(*value < min)*value=min;
//    if(*value > max)*value=max;
//}







void toggle_widget(widget * w)
{
    w->base.status^=WIDGET_ACTIVE;
    organise_toplevel_widget(w);
}

void toggle_widget_button_func(widget * w)
{
    toggle_widget(w->button.data);
}







static void empty_widget_render(overlay_theme * theme,widget * w,int16_t x_off,int16_t y_off,struct cvm_overlay_render_batch * restrict render_batch,rectangle bounds)
{

}


static widget_appearence_function_set empty_appearence_functions=
{
    .render =   empty_widget_render,
    .select =   blank_widget_select,
    .min_w  =   blank_widget_min_w,
    .min_h  =   blank_widget_min_h,
    .set_w  =   blank_widget_set_w,
    .set_h  =   blank_widget_set_h
};

widget * create_empty_widget(struct widget_context* context, int16_t min_w,int16_t min_h)
{
    widget * w = create_widget(context, sizeof(widget_base));///needs no extra data...

    w->base.appearence_functions=&empty_appearence_functions;

    w->base.min_w=min_w;
    w->base.min_h=min_h;

    return w;
}





static void separator_widget_min_w(overlay_theme * theme,widget * w)
{
	w->base.min_w=theme->separator_w;
}

static void separator_widget_min_h(overlay_theme * theme,widget * w)
{
	w->base.min_h=theme->separator_h;
}

static widget_appearence_function_set separator_appearence_functions=
{
    .render =   blank_widget_render,
    .select =   blank_widget_select,
    .min_w  =   separator_widget_min_w,
    .min_h  =   separator_widget_min_h,
    .set_w  =   blank_widget_set_w,
    .set_h  =   blank_widget_set_h
};

widget * create_separator_widget(struct widget_context* context)
{
    widget * w = create_widget(context, sizeof(widget_base));///needs no extra data, all behaviour comes from appearance functions and theme

    w->base.appearence_functions=&separator_appearence_functions;

    return w;
}



static void unit_separator_widget_min_w(overlay_theme * theme,widget * w)
{
	w->base.min_w=theme->base_unit_w;
}

static void unit_separator_widget_min_h(overlay_theme * theme,widget * w)
{
	w->base.min_h=theme->base_unit_h;
}

static widget_appearence_function_set unit_separator_appearence_functions=
{
    .render =   blank_widget_render,
    .select =   blank_widget_select,
    .min_w  =   unit_separator_widget_min_w,
    .min_h  =   unit_separator_widget_min_h,
    .set_w  =   blank_widget_set_w,
    .set_h  =   blank_widget_set_h
};

widget * create_unit_separator_widget(struct widget_context* context)
{
    widget * w = create_widget(context, sizeof(widget_base));///needs no extra data, all behaviour comes from appearance functions and theme

    w->base.appearence_functions=&unit_separator_appearence_functions;

    return w;
}






void window_toggle_button_func(widget * w)
{
    widget * rc;

    rc=w->button.data;
    rc->base.status^=WIDGET_ACTIVE;

    if(widget_active(rc))
    {
        organise_toplevel_widget(rc);
        move_toplevel_widget_to_front(rc);
    }
}


widget * create_window_widget(struct widget_context* context, widget ** box,char * title,bool resizable,widget_function custom_exit_function,void * custom_exit_data,bool free_custom_exit_data)
{
    widget *w,*sub_box;

    if(resizable) w=create_resize_constraint(context, WIDGET_RESIZABLE_FREE_MOVEMENT,true);
    else w=create_resize_constraint(context, WIDGET_RESIZABLE_LOCKED_HORIZONTAL|WIDGET_RESIZABLE_LOCKED_VERTICAL,false);

    *box=add_child_to_parent(add_child_to_parent(w,create_panel(context)),create_box(context, WIDGET_VERTICAL, WIDGET_LAST_DISTRIBUTED));

    sub_box=add_child_to_parent(*box,create_box(context, WIDGET_HORIZONTAL,WIDGET_FIRST_DISTRIBUTED));
    add_child_to_parent(sub_box,create_anchor(context, w, title));
    add_child_to_parent(sub_box,create_separator_widget(context));
    //add_child_to_parent(sub_box,create_unit_separator_widget());
    if(custom_exit_function)
    {
        add_child_to_parent(sub_box,create_icon_button(context, "🗙",custom_exit_data,free_custom_exit_data,custom_exit_function));
    }
    else
    {
        add_child_to_parent(sub_box,create_icon_button(context, "🗙",w,false,window_toggle_button_func));
    }

    add_child_to_parent(*box,create_separator_widget(context));

    return w;
}








void popup_button_toggle_func(widget * w)
{
    toggle_auto_close_popup(w->button.data);
}

///creates and returns button to toggle popup, also creates panel in popup in popup_container and adds panel_contents in the panel
widget * create_popup_panel_button(struct widget_context* context, widget* popup_container, widget* panel_contents, char* button_text)///input positioning????
{
    widget * popup=add_child_to_parent(popup_container,create_popup(context, WIDGET_POSITIONING_OFFSET_RIGHT,true));

    //widget * button=create_text_button(button_text,popup,popup_button_toggle_func);
    widget * button=create_text_highlight_toggle_button(context, button_text,popup,false,popup_button_toggle_func,button_widget_status_func);
    button->base.status&= ~WIDGET_CLOSE_POPUP_TREE;

    add_child_to_parent(add_child_to_parent(popup,create_panel(context)),panel_contents);

    set_popup_alignment_widgets(popup,panel_contents,button);
    set_popup_trigger_widget(popup,button);

    return button;
}




#warning probably want following as a single button, have special variant of text_icon h_bar button

widget * create_checkbox_button_pair(struct widget_context* context, char* text, void* data, widget_function func, widget_button_toggle_status_func toggle_status,bool free_data)
{
    widget * box=create_box(context, WIDGET_HORIZONTAL, WIDGET_FIRST_DISTRIBUTED);

    add_child_to_parent(box,create_text_button(context, text,data,false,func));
    add_child_to_parent(box,create_icon_toggle_button(context, "+", "-", data, free_data, func, toggle_status));

    return box;
}

widget * create_bool_checkbox_button_pair(struct widget_context* context, char* text, bool* bool_ptr)
{
    return create_checkbox_button_pair(context, text, bool_ptr, button_toggle_bool_func, button_bool_status_func, false);
}



widget * create_icon_collapse_button(struct widget_context* context, char* icon_collapse, char* icon_expand, widget* widget_to_control, bool collapse)
{
    widget * button=create_icon_toggle_button(context, icon_collapse, icon_expand, widget_to_control, false, toggle_widget_button_func, button_widget_status_func);

    if(collapse)toggle_widget(widget_to_control);

    return button;
}





struct self_deleting_dialogue
{
    void* data;// strongly recommended to make this independent of as much else as possible, though this may be impossible
    void (*accept_function)(void*);
    void (*cancel_function)(void*);
    widget* dialogue;
};

static void self_deleting_dialogue_accept_button_func(widget* w)
{
    struct self_deleting_dialogue* ptr = w->button.data;
    struct self_deleting_dialogue sdd = *ptr;
    free(ptr);

    // this is deleting a widget from inside its own callstack, ergo it requires some nuance and care
    delete_widget(sdd.dialogue);

    sdd.accept_function(sdd.data);
}

static void self_deleting_dialogue_cancel_button_func(widget* w)
{
    struct self_deleting_dialogue* ptr = w->button.data;
    struct self_deleting_dialogue sdd = *ptr;
    free(ptr);

    // this is deleting a widget from inside its own callstack, ergo it requires some nuance and care
    delete_widget(sdd.dialogue);

    sdd.cancel_function(sdd.data);
}


#warning this could/should take a struct with some members being conditional??

#warning FUCK, this triggers inside a callstack, meaning the rest of the widgets may remain valid???
/// last clicked likely still valid FUCK
void create_self_deleting_dialogue(struct widget_context* context, widget* root_widget, const char* message_str, const char* accept_str, const char* cancel_str, void* data, void (*accept_function)(void*), void (*cancel_function)(void*))
{
    widget *box1,*box2,*panel;
    struct self_deleting_dialogue* sdd;

    sdd = malloc(sizeof(struct self_deleting_dialogue));
    sdd->data = data;
    sdd->accept_function = accept_function;
    sdd->cancel_function = cancel_function;


    #warning should this really be a popup?, is there a better paradigm (popup should be for navigating popup lists)
    sdd->dialogue = create_popup(context, WIDGET_POSITIONING_CENTRED, false);///replace with popup widget w/ appropriate relative positioning
    sdd->dialogue->base.status |= WIDGET_ACTIVE;

    add_child_to_parent(root_widget, sdd->dialogue);

    panel=add_child_to_parent(sdd->dialogue,create_panel(context));

    // add_child_to_parent(panel,create_empty_widget(context, 40,40));

    box1=add_child_to_parent(panel,create_box(context, WIDGET_VERTICAL, WIDGET_EVENLY_DISTRIBUTED));

    /*message bar*/add_child_to_parent(box1, create_static_text_bar(context, message_str));

    box1=add_child_to_parent(box1,create_box(context, WIDGET_HORIZONTAL,WIDGET_EVENLY_DISTRIBUTED));
    add_child_to_parent(box1,create_empty_widget(context, 0,0));
    box1=add_child_to_parent(box1,create_box(context, WIDGET_HORIZONTAL,WIDGET_LAST_DISTRIBUTED));
    box2=add_child_to_parent(box1,create_box(context, WIDGET_HORIZONTAL,WIDGET_ALL_SAME_DISTRIBUTED));
    add_child_to_parent(box1,create_empty_widget(context, 0,0));
    #warning above could (probably should) be a specialised centring widget

    /*accept button*/ add_child_to_parent(box2,create_text_button(context, accept_str, sdd, false,self_deleting_dialogue_accept_button_func));
    /*cancel button*/ add_child_to_parent(box2,create_text_button(context, cancel_str, sdd, false,self_deleting_dialogue_cancel_button_func));

    // #warning button or something else in hierarchy should be able to intercept escape key or invalidation input (is a good use/justification of popup)

    

    organise_toplevel_widget(sdd->dialogue);

    // move_toplevel_widget_to_front(popup); // shouldn't be necessary
    set_only_interactable_widget(context, sdd->dialogue);
    // organise_root_widget(root_widget, root_widget->base.r.x2, root_widget->base.r.y2);
}







/** remove
widget_contiguous_box
    ^ this may have utility for widgets
widget_file_list

make text_bar more dynamic or create a new widget type
*/



/// widget(multibox), x_off, y_off, render_batch, bounds, element/entry index
static void sol_directory_multibox_entry_render(widget* multibox, int16_t x_off, int16_t y_off, struct cvm_overlay_render_batch* render_batch, rectangle bounds, uint32_t entry_index)
{
    //     rectangle r=rectangle_add_offset(w->base.r, x_off, y_off);
    // theme->h_bar_render(render_batch, theme, bounds, r, w->base.status, OVERLAY_ALTERNATE_MAIN_COLOUR);

    // overlay_text_single_line_render_data text_render_data=
    // {
    //     .flags=0,
    //     .x=r.x1+theme->h_bar_text_offset,
    //     .y=(r.y1+r.y2-theme->font.glyph_size)>>1,
    //     .text=w->anchor.text,
    //     .bounds=bounds,
    //     .colour=OVERLAY_TEXT_COLOUR_0
    // };

    // overlay_text_single_line_render(render_batch, theme, &text_render_data);



    #warning should really try to minimise specailised work to render elements, sould be trivial to implement multibox for specialised purposes
    #warning probably want some extra metadata for directory RENDERING on top of the directory data itself (icons, min glyph count &c.)
    // file_list_entry * fle;
    // rectangle icon_r,r;
    // const file_list_type * file_types;
    // int32_t y,y_end,index,y_text_off;
    // const char * icon_glyph;

    // file_types=w->file_list.file_types;

    // r=rectangle_add_offset(w->base.r,x_off,y_off);

    // theme->box_render(render_batch, theme, bounds, r, w->base.status, OVERLAY_MAIN_COLOUR);


    // y_end=r.y2-theme->contiguous_box_y_offset;
    // index=w->file_list.offset/theme->base_contiguous_unit_h;
    // y=r.y1+index*theme->base_contiguous_unit_h-w->file_list.offset+theme->contiguous_box_y_offset;
    // y_text_off=(theme->base_contiguous_unit_h-theme->font.glyph_size)>>1;

    // ///make fade and constrained
    // overlay_text_single_line_render_data text_render_data=
    // {
    //     .flags=OVERLAY_TEXT_RENDER_BOX_CONSTRAINED|OVERLAY_TEXT_RENDER_FADING,
    //     .x=r.x1+theme->h_bar_text_offset+ w->file_list.render_type_icons*(theme->h_bar_icon_text_offset+theme->base_contiguous_unit_w),///base_contiguous_unit_w used for icon space/sizing
    //     //.y=,
    //     //.text=,
    //     .bounds=bounds,
    //     .colour=OVERLAY_TEXT_COLOUR_0,
    //     .box_r=r,
    //     .box_status=w->base.status,
    //     //.text_length=,
    // };

    // text_render_data.text_area=(rectangle){.x1=text_render_data.x,.y1=r.y1,.x2=r.x2-theme->h_bar_text_offset,.y2=r.y2};

    // icon_r.x1=r.x1+theme->h_bar_text_offset;
    // icon_r.x2=r.x1+theme->h_bar_text_offset+theme->base_contiguous_unit_w;

    // while(y<y_end && index<(int32_t)w->file_list.valid_entry_count)
    // {
    //     if(index==w->file_list.selected_entry_index)/// && is_currently_active_widget(w) not sure the currently selected widget thing is desirable, probably not, other programs don't do it
    //     {
    //         theme->h_bar_box_constrained_render(render_batch,theme,bounds,((rectangle){.x1=r.x1,.y1=y,.x2=r.x2,.y2=y+theme->base_contiguous_unit_h}),w->base.status,OVERLAY_HIGHLIGHTING_COLOUR,r,w->base.status);
    //     }

    //     fle=w->file_list.entries+index;

    //     if(w->file_list.render_type_icons)
    //     {
    //         ///render icon
    //         icon_r.y1=y;
    //         icon_r.y2=y+theme->base_contiguous_unit_h;

    //         if(fle->type_id==CVM_FL_DIRECTORY_TYPE_ID) icon_glyph="🗀";
    //         else if(fle->type_id==CVM_FL_MISCELLANEOUS_TYPE_ID) icon_glyph="🗎";
    //         else icon_glyph=file_types[fle->type_id-CVM_FL_CUSTOM_TYPE_OFFSET].icon;

    //         overlay_text_centred_glyph_box_constrained_render(render_batch,theme,bounds,icon_r,icon_glyph,OVERLAY_TEXT_COLOUR_0,r,w->base.status);
    //     }


    //     text_render_data.y=y+y_text_off;

    //     text_render_data.text=fle->filename;

    //     if(!fle->text_length_calculated)
    //     {
    //         fle->text_length=overlay_text_single_line_get_pixel_length(&theme->font,text_render_data.text);
    //         fle->text_length_calculated=true;
    //     }

    //     text_render_data.text_length=fle->text_length;

    //     overlay_text_single_line_render(render_batch,theme,&text_render_data);

    //     y+=theme->base_contiguous_unit_h;
    //     index++;
    // }
}

static int16_t sol_directory_multibox_entry_min_w(widget* multibox)
{
    struct overlay_theme* theme = multibox->base.context->theme;
    return 2 * theme->h_bar_text_offset + 16 * theme->font.max_advance;// require at least 16 visible characters
}

static int16_t sol_directory_multibox_entry_min_h(widget* multibox)
{
    struct overlay_theme* theme = multibox->base.context->theme;
    return theme->base_contiguous_unit_h;
}


// directory multibox functions
const struct multibox_functions directory_multibox_functions = (struct multibox_functions)
{
    .entry_render = &sol_directory_multibox_entry_render,
    .entry_min_w  = &sol_directory_multibox_entry_min_w,
    .entry_min_h  = &sol_directory_multibox_entry_min_h,
};




// multibox specific

static void multibox_widget_render(overlay_theme * theme, widget * w, int16_t x_off, int16_t y_off, struct cvm_overlay_render_batch * restrict render_batch, rectangle bounds)
{
    rectangle r = rectangle_add_offset(w->base.r, x_off, y_off);
    theme->box_render(render_batch, theme, bounds, r, w->base.status, OVERLAY_MAIN_COLOUR);

    // render entries at theme->x_box_offset. theme->y_box_offset

}

static widget * multibox_widget_select(overlay_theme * theme,widget * w,int16_t x_in,int16_t y_in)
{
    rectangle r = rectangle_subtract_offset(w->base.r, x_in, y_in);
    if(theme->box_select(theme, r, w->base.status))
    {
        return w;
    }
    return NULL;
}

static void multibox_widget_min_w(overlay_theme * theme,widget * w)
{
    int16_t entry_w = w->multibox.functions->entry_min_w(w);

    if(w->multibox.min_displayed_count_x)
    {
        w->base.min_w = entry_w * w->multibox.min_displayed_count_x;
    }
    else
    {
        w->base.min_w = entry_w;
    }

    #warning could use dimensions directly, then have something to force overlap ?
    // unsure regarding this
    w->base.min_w += theme->contiguous_box_x_offset * 2;
}

static void multibox_widget_min_h(overlay_theme * theme,widget * w)
{
    int16_t entry_h = w->multibox.functions->entry_min_h(w);

    if(w->multibox.min_displayed_count_y)
    {
        w->base.min_h = entry_h * w->multibox.min_displayed_count_y;
    }
    else
    {
        w->base.min_h = entry_h;
    }

    // unsure regarding this
    w->base.min_h += theme->contiguous_box_y_offset * 2;
}

static void multibox_widget_set_h(overlay_theme * theme,widget * w)
{
    // w->textbox.wheel_delta= -theme->font.line_spacing;///w->textbox.font_index
    // w->textbox.visible_size= w->base.r.y2-w->base.r.y1-2*theme->y_box_offset;

    // if(w->textbox.min_visible_lines)
    // {
    //     w->textbox.max_offset=(w->textbox.text_block.line_count-1)*theme->font.line_spacing+theme->font.glyph_size - w->textbox.visible_size;
    //     if(w->textbox.max_offset < 0)w->textbox.max_offset=0;
    //     if(w->textbox.y_offset > w->textbox.max_offset)w->textbox.y_offset = w->textbox.max_offset;
    // }
}


static widget_appearence_function_set textbox_appearence_functions=
{
    .render = multibox_widget_render,
    .select = multibox_widget_select,
    .min_w  = multibox_widget_min_w,
    .min_h  = multibox_widget_min_h,
    .set_w  = blank_widget_set_w,
    .set_h  = multibox_widget_set_h
};




#warning tabs are a good use case for contiguous box though (CONTIGUOUS BOX INCOMPLETE, WTH!)

#warning is there a way to COMBINE contiguous box and multibox?  -- NO!
/** ^^^
 * button would need: awareness of EVERY entry or every entry would actually need to exist!
 * ^ could do specialised button for file entries, basically need to anyway, then we just have button * count
 *   ^ HOWEVER as entry count is so dynamic it would be REALLY difficult to make it act as if static
 *     ^ for tab bar; scrollable box container it's actually quite simple in concept, the contents hold the complexity, so is actually okay?
 *       ^ similarly applicable logic for file list; as long as its simple, an indexed multibox is fine, contents hold the complexity
 *
 *      better to provide solid foundations and instead have custom BS where necessary
*/

#warning retain/release widget references simplifies context held widgets and deletion!

#warning deletion should be a function passed in anongside `void* data` something custom for applicable custom data
// ^ allows retain/release instead of flat out free
