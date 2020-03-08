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

static overlay_sprite_data foreground_shape_sprite;
static overlay_sprite_data background_shape_sprite;
static overlay_sprite_data foreground_border_sprite;
static overlay_sprite_data small_foreground_shape_sprite;

static const int cubic_sprite_buffer_size=256;

static int set_cubic_shaded_sprite_data(uint8_t * buffer,int outer_r_x4,int inner_r_x4,int offset_x4,bool clear)
{
    int i,j,k,x,y,d,r,ri,ro;///

    d=((offset_x4+3)/4)*2;///diameter in pixels
    ri=inner_r_x4*inner_r_x4*inner_r_x4*8;///adjusted diameters for comparison (double to allow mid pixel testing)
    ro=outer_r_x4*outer_r_x4*outer_r_x4*8;

    if(clear) for(i=0;i<d;i++) memset(buffer+i*cubic_sprite_buffer_size,0,d*sizeof(uint8_t));

    for(i=-outer_r_x4;i<outer_r_x4;i++)
    {
        x=2*i+1;
        for(j=-outer_r_x4;j<outer_r_x4;j++)
        {
            y=2*j+1;
            r=abs(x*x*x)+abs(y*y*y);
            if((r<ro)&&(r>=ri))
            {
                ///each pixel is a 4x4 grid of tests,16 samples ergo 16 should be added to reach 256, with first addition being 15 to reach 255
                k=(i+offset_x4)/4 + cubic_sprite_buffer_size*((j+offset_x4)/4);
                if(buffer[k]) buffer[k]+=16;
                else buffer[k]+=15;
            }
        }
    }

    return d;
}

static overlay_sprite_data create_cubic_shaded_sprite(overlay_theme * theme,uint8_t * buffer,char * name,int outer_r_x4,int inner_r_x4)
{
    overlay_sprite_data osd;
    int d;

    d=set_cubic_shaded_sprite_data(buffer,outer_r_x4,inner_r_x4,outer_r_x4,true);

    osd=create_overlay_sprite(theme,name,d,d,OVERLAY_SHADED_SPRITE);
    set_overlay_sprite_image_data(theme,osd,buffer,cubic_sprite_buffer_size,d,d,0,0);

    return osd;
}

bool cubic_square_select(rectangle r,int x_in,int y_in,uint32_t status,overlay_theme * theme)
{
    r.x+=(r.w-20)/2-x_in;
    r.y+=(r.h-20)/2-y_in;
    r.w=r.h=20;

    return check_click_on_shaded_sprite(theme,r,foreground_shape_sprite.x_pos,foreground_shape_sprite.y_pos);
}

bool cubic_h_bar_select(rectangle r,int x_in,int y_in,uint32_t status,overlay_theme * theme)
{
    r.x-=x_in;
    r.y+=(r.h-20)/2-y_in;
    r.h=20;

    if(!(status&WIDGET_H_FIRST))///not the first
    {
        if(check_click_on_shaded_sprite(theme,(rectangle){.x=r.x+2,.y=r.y,.w=10,.h=20},foreground_shape_sprite.x_pos,foreground_shape_sprite.y_pos))return true;
        r.x+=12;
        r.w-=12;
    }

    if(!(status&WIDGET_H_LAST))///not the last
    {
        if(check_click_on_shaded_sprite(theme,(rectangle){.x=r.x+r.w-12,.y=r.y,.w=10,.h=20},foreground_shape_sprite.x_pos+10,foreground_shape_sprite.y_pos))return true;
        r.w-=12;
    }

    return rectangle_surrounds_origin(r);
}

bool cubic_v_bar_select(rectangle r,int x_in,int y_in,uint32_t status,overlay_theme * theme)
{
    r.x+=(r.w-20)/2-x_in;
    r.y+=12-y_in;
    r.w=20;
    r.h-=24;

    if(check_click_on_shaded_sprite(theme,(rectangle){.x=r.x,.y=r.y-10,.w=20,.h=10},foreground_shape_sprite.x_pos,foreground_shape_sprite.y_pos))return true;
    if(check_click_on_shaded_sprite(theme,(rectangle){.x=r.x,.y=r.y+r.h,.w=20,.h=10},foreground_shape_sprite.x_pos,foreground_shape_sprite.y_pos+10))return true;
    return rectangle_surrounds_origin(r);
}

bool cubic_box_select(rectangle r,int x_in,int y_in,uint32_t status,overlay_theme * theme)
{
    r.x-=x_in;
    r.y+=1-y_in;
    r.h-=2;

    if(!(status&WIDGET_H_FIRST))///not the first
    {
        if(check_click_on_shaded_sprite(theme,(rectangle){.x=r.x+2,.y=r.y,.w=10,.h=10},foreground_shape_sprite.x_pos,foreground_shape_sprite.y_pos))return true;
        if(rectangle_surrounds_origin((rectangle){.x=r.x+2,.y=r.y+10,.w=10,.h=r.h-20}))return true;
        if(check_click_on_shaded_sprite(theme,(rectangle){.x=r.x+2,.y=r.y+r.h-10,.w=10,.h=10},foreground_shape_sprite.x_pos,foreground_shape_sprite.y_pos+10))return true;
        r.x+=12;
        r.w-=12;
    }

    if(!(status&WIDGET_H_LAST))///not the last
    {

        if(check_click_on_shaded_sprite(theme,(rectangle){.x=r.x+r.w-12,.y=r.y,.w=10,.h=20},foreground_shape_sprite.x_pos+10,foreground_shape_sprite.y_pos))return true;
        if(rectangle_surrounds_origin((rectangle){.x=r.x+r.w-12,.y=r.y+10,.w=10,.h=r.h-20}))return true;
        if(check_click_on_shaded_sprite(theme,(rectangle){.x=r.x+r.w-12,.y=r.y+r.h-10,.w=10,.h=10},foreground_shape_sprite.x_pos+10,foreground_shape_sprite.y_pos+10))return true;

        r.w-=12;
    }

    return rectangle_surrounds_origin(r);
}

bool cubic_panel_select(rectangle r,int x_in,int y_in,uint32_t status,overlay_theme * theme)
{
    rectangle r2;
    r.x-=x_in;
    r.y-=y_in;

    if(!(status&WIDGET_H_FIRST))
    {
        r2=r;
        r2.w=14;

        r.x+=14;
        r.w-=14;

        if(!(status&WIDGET_V_FIRST))
        {
            if(check_click_on_shaded_sprite(theme,(rectangle){.x=r2.x,.y=r2.y,.w=14,.h=14},background_shape_sprite.x_pos,background_shape_sprite.y_pos))return true;
            r2.y+=14;
            r2.h-=14;
        }
        if(!(status&WIDGET_V_LAST))
        {
            if(check_click_on_shaded_sprite(theme,(rectangle){.x=r2.x,.y=r2.y+r2.h-14,.w=14,.h=14},background_shape_sprite.x_pos,background_shape_sprite.y_pos+14))return true;
            r2.h-=14;
        }

        if(rectangle_surrounds_origin(r2))return true;
    }

    if(!(status&WIDGET_H_LAST))
    {
        r2=r;
        r2.w=14;
        r2.x+=r.w-14;

        r.w-=14;

        if(!(status&WIDGET_V_FIRST))
        {
            if(check_click_on_shaded_sprite(theme,(rectangle){.x=r2.x,.y=r2.y,.w=14,.h=14},background_shape_sprite.x_pos+14,background_shape_sprite.y_pos))return true;
            r2.y+=14;
            r2.h-=14;
        }
        if(!(status&WIDGET_V_LAST))
        {
            if(check_click_on_shaded_sprite(theme,(rectangle){.x=r2.x,.y=r2.y+r2.h-14,.w=14,.h=14},background_shape_sprite.x_pos+14,background_shape_sprite.y_pos+14))return true;
            r2.h-=14;
        }

        if(rectangle_surrounds_origin(r2))return true;
    }

    return rectangle_surrounds_origin(r);
}

void cubic_square_icon_render(rectangle r,int x_off,int y_off,uint32_t status,overlay_theme * theme,overlay_data * od,rectangle bounds,overlay_colour colour,char * icon_name,overlay_colour icon_colour)
{
    r.x+=x_off;
    r.y+=y_off+(r.h-20)/2;
    r.h=20;

    int half_w=r.w/2;

    if(status&WIDGET_H_FIRST) render_rectangle(od,(rectangle){.x=r.x,.y=r.y,.w=half_w,.h=20},bounds,colour);
    else render_overlay_shade(od,(rectangle){.x=r.x+half_w-10,.y=r.y,.w=10,.h=20},bounds,foreground_shape_sprite.x_pos,foreground_shape_sprite.y_pos,colour);

    r.x+=half_w;
    r.w-=half_w;

    if(status&WIDGET_H_LAST) render_rectangle(od,r,bounds,colour);
    else render_overlay_shade(od,(rectangle){.x=r.x,.y=r.y,.w=10,.h=20},bounds,foreground_shape_sprite.x_pos+10,foreground_shape_sprite.y_pos,colour);

    if(icon_name)
    {
        r.x-=8;
        r.y+=2;
        r.w=r.h=16;///icon_size=16

        overlay_sprite_data icon=get_overlay_sprite_data(theme,icon_name);
        render_overlay_shade(od,r,bounds,icon.x_pos,icon.y_pos,icon_colour);
    }
}

void cubic_h_text_bar_render(rectangle r,int x_off,int y_off,uint32_t status,overlay_theme * theme,overlay_data * od,rectangle bounds,overlay_colour colour,char * text)
{
    r.x+=x_off;
    r.y+=y_off+(r.h-20)/2;
    r.h=20;

    /// use x_off and y_off here for cleanlyness/legibility purposes ??? also for text placement when on border

    x_off=r.x+theme->h_bar_text_offset;
    y_off=r.y+(r.h-theme->fonts[0].font_height)/2;
    int text_w=r.w-2*theme->h_bar_text_offset;

    if(!(status&WIDGET_H_FIRST))///not the first
    {
        render_overlay_shade(od,(rectangle){.x=r.x+2,.y=r.y,.w=10,.h=20},bounds,foreground_shape_sprite.x_pos,foreground_shape_sprite.y_pos,colour);
        r.x+=12;
        r.w-=12;
    }

    if(!(status&WIDGET_H_LAST))///not the last
    {
        render_overlay_shade(od,(rectangle){.x=r.x+r.w-12,.y=r.y,.w=10,.h=20},bounds,foreground_shape_sprite.x_pos+10,foreground_shape_sprite.y_pos,colour);
        r.w-=12;
    }

    render_rectangle(od,r,bounds,colour);

    if((text)&&(get_rectangle_overlap(&bounds,(rectangle){.x=x_off,.y=y_off,.w=text_w,.h=theme->fonts[0].font_height})))render_overlay_text(od,theme,text,x_off,y_off,bounds,0,0);
}

void cubic_h_icon_text_bar_render(rectangle r,int x_off,int y_off,uint32_t status,overlay_theme * theme,overlay_data * od,rectangle bounds,overlay_colour colour,char * text,char * icon_name,overlay_colour icon_colour)
{
    r.x+=x_off;
    r.y+=y_off+(r.h-20)/2;
    r.h=20;

    /// use x_off and y_off here for cleanlyness/legibility purposes ??? also for text placement when on border

    x_off=r.x+theme->h_bar_text_offset+theme->icon_bar_extra_w;
    y_off=r.y+(r.h-theme->fonts[0].font_height)/2;
    int text_w=r.w-2*theme->h_bar_text_offset- theme->icon_bar_extra_w;

    if(!(status&WIDGET_H_FIRST))///not the first
    {
        render_overlay_shade(od,(rectangle){.x=r.x+2,.y=r.y,.w=10,.h=20},bounds,foreground_shape_sprite.x_pos,foreground_shape_sprite.y_pos,colour);
        r.x+=12;
        r.w-=12;
    }

    if(!(status&WIDGET_H_LAST))///not the last
    {
        render_overlay_shade(od,(rectangle){.x=r.x+r.w-12,.y=r.y,.w=10,.h=20},bounds,foreground_shape_sprite.x_pos+10,foreground_shape_sprite.y_pos,colour);
        r.w-=12;
    }

    render_rectangle(od,r,bounds,colour);

    if(text)
    {
        rectangle text_bounds=bounds;
        if(get_rectangle_overlap(&text_bounds,(rectangle){.x=x_off,.y=y_off,.w=text_w,.h=theme->fonts[0].font_height}))render_overlay_text(od,theme,text,x_off,y_off,text_bounds,0,0);
    }

    if(icon_name)
    {
        x_off-=theme->h_bar_text_offset+16;///16=icon_size
        y_off=r.y+(r.h-16)/2;///16=icon_size

        overlay_sprite_data icon=get_overlay_sprite_data(theme,icon_name);
        render_overlay_shade(od,(rectangle){.x=x_off,.y=y_off,.w=16,.h=16},bounds,icon.x_pos,icon.y_pos,icon_colour);
    }
}

void cubic_h_text_icon_bar_render(rectangle r,int x_off,int y_off,uint32_t status,overlay_theme * theme,overlay_data * od,rectangle bounds,overlay_colour colour,char * text,char * icon_name,overlay_colour icon_colour)
{
    r.x+=x_off;
    r.y+=y_off+(r.h-20)/2;
    r.h=20;

    /// use x_off and y_off here for cleanlyness/legibility purposes ??? also for text placement when on border

    x_off=r.x+theme->h_bar_text_offset;
    y_off=r.y+(r.h-theme->fonts[0].font_height)/2;
    int text_w=r.w-2*theme->h_bar_text_offset-theme->icon_bar_extra_w;

    if(!(status&WIDGET_H_FIRST))///not the first
    {
        render_overlay_shade(od,(rectangle){.x=r.x+2,.y=r.y,.w=10,.h=20},bounds,foreground_shape_sprite.x_pos,foreground_shape_sprite.y_pos,colour);
        r.x+=12;
        r.w-=12;
    }

    if(!(status&WIDGET_H_LAST))///not the last
    {
        render_overlay_shade(od,(rectangle){.x=r.x+r.w-12,.y=r.y,.w=10,.h=20},bounds,foreground_shape_sprite.x_pos+10,foreground_shape_sprite.y_pos,colour);
        r.w-=12;
    }

    render_rectangle(od,r,bounds,colour);

    if(text)
    {
        rectangle text_bounds=bounds;
        if(get_rectangle_overlap(&text_bounds,(rectangle){.x=x_off,.y=y_off,.w=text_w,.h=theme->fonts[0].font_height}))render_overlay_text(od,theme,text,x_off,y_off,text_bounds,0,0);
    }

    if(icon_name)
    {
        x_off+=text_w+theme->h_bar_text_offset;
        y_off=r.y+(r.h-16)/2;///16=icon_size

        overlay_sprite_data icon=get_overlay_sprite_data(theme,icon_name);
        render_overlay_shade(od,(rectangle){.x=x_off,.y=y_off,.w=16,.h=16},bounds,icon.x_pos,icon.y_pos,icon_colour);
    }
}

void cubic_h_slider_bar_render(rectangle r,int x_off,int y_off,uint32_t status,overlay_theme * theme,overlay_data * od,rectangle bounds,overlay_colour colour,int range,int value,int bar)
{
    r.x+=x_off+12;
    r.y+=y_off+(r.h-20)/2;
    r.w-=theme->h_sliderbar_lost_w;
    r.h=20;

    render_overlay_shade(od,(rectangle){.x=r.x-10,.y=r.y,.w=10,.h=20},bounds,foreground_border_sprite.x_pos,foreground_border_sprite.y_pos,colour);
    render_overlay_shade(od,(rectangle){.x=r.x+r.w,.y=r.y,.w=10,.h=20},bounds,foreground_border_sprite.x_pos+10,foreground_border_sprite.y_pos,colour);
    render_border(od,r,bounds,(rectangle){.x=r.x,.y=r.y+1,.w=r.w,.h=18},colour);

    r.y+=3;
    r.h-=6;

    if(bar<0)
    {
        bar=r.w/(-bar);
        r.x+=((r.w-bar)*value)/range;
        r.w=bar;
    }
    else
    {
        r.x+=(r.w*value)/(range+bar);
        r.w-=(r.w*range)/(range+bar);
    }

    render_overlay_shade(od,(rectangle){.x=r.x-7,.y=r.y,.w=7,.h=14},bounds,small_foreground_shape_sprite.x_pos,small_foreground_shape_sprite.y_pos,colour);
    render_overlay_shade(od,(rectangle){.x=r.x+r.w,.y=r.y,.w=7,.h=14},bounds,small_foreground_shape_sprite.x_pos+7,small_foreground_shape_sprite.y_pos,colour);
    render_rectangle(od,r,bounds,colour);
}

void cubic_v_slider_bar_render(rectangle r,int x_off,int y_off,uint32_t status,overlay_theme * theme,overlay_data * od,rectangle bounds,overlay_colour colour,int range,int value,int bar)
{
    r.x+=x_off+(r.w-20)/2;
    r.y+=y_off+11;
    r.w=20;
    r.h-=theme->v_sliderbar_lost_h;

    render_overlay_shade(od,(rectangle){.x=r.x,.y=r.y-10,.w=20,.h=10},bounds,foreground_border_sprite.x_pos,foreground_border_sprite.y_pos,colour);
    render_overlay_shade(od,(rectangle){.x=r.x,.y=r.y+r.h,.w=20,.h=10},bounds,foreground_border_sprite.x_pos,foreground_border_sprite.y_pos+10,colour);
    render_border(od,r,bounds,(rectangle){.x=r.x+1,.y=r.y,.w=18,.h=r.h},colour);

    r.x+=3;
    r.w-=6;

    if(bar<0)
    {
        bar=r.h/(-bar);
        r.y+=((r.h-bar)*value)/range;
        r.h=bar;
    }
    else
    {
        r.y+=(r.h*value)/(range+bar);
        r.h-=(r.h*range)/(range+bar);
    }

    render_overlay_shade(od,(rectangle){.x=r.x,.y=r.y-7,.w=14,.h=7},bounds,small_foreground_shape_sprite.x_pos,small_foreground_shape_sprite.y_pos,colour);
    render_overlay_shade(od,(rectangle){.x=r.x,.y=r.y+r.h,.w=14,.h=7},bounds,small_foreground_shape_sprite.x_pos,small_foreground_shape_sprite.y_pos+7,colour);
    render_rectangle(od,r,bounds,colour);
}

void cubic_box_render(rectangle r,int x_off,int y_off,uint32_t status,overlay_theme * theme,overlay_data * od,rectangle bounds,overlay_colour colour)
{
    r.x+=x_off;
    r.y+=y_off+1;
    r.h-=2;

    if(!(status&WIDGET_H_FIRST))///not the first
    {
        render_overlay_shade(od,(rectangle){.x=r.x+2,.y=r.y,.w=10,.h=10},bounds,foreground_shape_sprite.x_pos,foreground_shape_sprite.y_pos,colour);
        render_rectangle(od,(rectangle){.x=r.x+2,.y=r.y+10,.w=10,.h=r.h-20},bounds,colour);
        render_overlay_shade(od,(rectangle){.x=r.x+2,.y=r.y+r.h-10,.w=10,.h=10},bounds,foreground_shape_sprite.x_pos,foreground_shape_sprite.y_pos+10,colour);
        r.x+=12;
        r.w-=12;
    }

    if(!(status&WIDGET_H_LAST))///not the last
    {
        render_overlay_shade(od,(rectangle){.x=r.x+r.w-12,.y=r.y,.w=10,.h=10},bounds,foreground_shape_sprite.x_pos+10,foreground_shape_sprite.y_pos,colour);
        render_rectangle(od,(rectangle){.x=r.x+r.w-12,.y=r.y+10,.w=10,.h=r.h-20},bounds,colour);
        render_overlay_shade(od,(rectangle){.x=r.x+r.w-12,.y=r.y+r.h-10,.w=10,.h=10},bounds,foreground_shape_sprite.x_pos+10,foreground_shape_sprite.y_pos+10,colour);
        r.w-=12;
    }

    render_rectangle(od,r,bounds,colour);
}

void cubic_panel_render(rectangle r,int x_off,int y_off,uint32_t status,overlay_theme * theme,overlay_data * od,rectangle bounds,overlay_colour colour)
{
    r.x+=x_off;
    r.y+=y_off;

    rectangle r2;

    if(!(status&WIDGET_H_FIRST))
    {
        r2=r;
        r2.w=14;

        r.x+=14;
        r.w-=14;

        if(!(status&WIDGET_V_FIRST))
        {
            render_overlay_shade(od,(rectangle){.x=r2.x,.y=r2.y,.w=14,.h=14},bounds,background_shape_sprite.x_pos,background_shape_sprite.y_pos,colour);
            r2.y+=14;
            r2.h-=14;
        }
        if(!(status&WIDGET_V_LAST))
        {
            render_overlay_shade(od,(rectangle){.x=r2.x,.y=r2.y+r2.h-14,.w=14,.h=14},bounds,background_shape_sprite.x_pos,background_shape_sprite.y_pos+14,colour);
            r2.h-=14;
        }

        render_rectangle(od,r2,bounds,colour);
    }

    if(!(status&WIDGET_H_LAST))
    {
        r2=r;
        r2.w=14;
        r2.x+=r.w-14;

        r.w-=14;

        if(!(status&WIDGET_V_FIRST))
        {
            render_overlay_shade(od,(rectangle){.x=r2.x,.y=r2.y,.w=14,.h=14},bounds,background_shape_sprite.x_pos+14,background_shape_sprite.y_pos,colour);
            r2.y+=14;
            r2.h-=14;
        }
        if(!(status&WIDGET_V_LAST))
        {
            render_overlay_shade(od,(rectangle){.x=r2.x,.y=r2.y+r2.h-14,.w=14,.h=14},bounds,background_shape_sprite.x_pos+14,background_shape_sprite.y_pos+14,colour);
            r2.h-=14;
        }

        render_rectangle(od,r2,bounds,colour);
    }

    render_rectangle(od,r,bounds,colour);
}

void initialise_cubic_theme(overlay_theme * theme)
{
    uint8_t * shaded_sprite_buffer=malloc(sizeof(uint8_t)*cubic_sprite_buffer_size*cubic_sprite_buffer_size);
    overlay_sprite_data osd;
    int d;

    load_font_to_overlay(theme,"resources/CVM_font_1.ttf",16,0);
    SDL_Surface * icons_surface=IMG_Load("resources/icons_alpha.png");

    if(icons_surface==NULL)
    {
        puts("THEME ICONS FILE NOT FOUND");
        exit(-1);
    }


    theme->base_unit_w=24;
    theme->base_unit_h=22;

    theme->x_panel_offset=2;
    theme->x_panel_offset_side=0;
    theme->y_panel_offset=3;
    theme->y_panel_offset_side=3;

    theme->h_bar_minimum_w=0;///WTF IS THIS ???
    theme->h_bar_text_offset=16;

    theme->h_sliderbar_lost_w=24;
    theme->v_sliderbar_lost_h=22;

    theme->x_box_offset=16;/// ??? figure out what this should be (text and
    theme->y_box_offset=7;/// ??? figure out what this should be

    //theme->icon_bar_close_extra_w=20;///16(icon_size)+ 2(offset of end) +2 ((bar_height(20)-icon_size(16))/2)
    theme->icon_bar_extra_w=32;///16(icon_size)+ text_offset(16) e.g. same offset as text bar
    theme->separator_w=8;
    theme->separator_h=4;
    theme->popup_separation=8;

    theme->border_resize_selection_range=14;

    theme->contiguous_all_box_x_offset=0;
    theme->contiguous_all_box_y_offset=1;///possibly 1 ??
    theme->contiguous_some_box_x_offset=12;
    theme->contiguous_some_box_y_offset=1;///possibly 1 ??
    theme->contiguous_horizintal_bar_h=20;


    theme->square_icon_render=cubic_square_icon_render;
    theme->h_text_bar_render=cubic_h_text_bar_render;
    theme->h_text_icon_bar_render=cubic_h_text_icon_bar_render;
    theme->h_icon_text_bar_render=cubic_h_icon_text_bar_render;
    theme->h_slider_bar_render=cubic_h_slider_bar_render;
    theme->v_slider_bar_render=cubic_v_slider_bar_render;
    theme->box_render=cubic_box_render;
    theme->panel_render=cubic_panel_render;


    theme->square_select=cubic_square_select;
    theme->h_bar_select=cubic_h_bar_select;
    theme->v_bar_select=cubic_v_bar_select;
    theme->box_select=cubic_box_select;
    theme->panel_select=cubic_panel_select;


    foreground_shape_sprite=create_cubic_shaded_sprite(theme,shaded_sprite_buffer,"foreground_shape",40,0);///r=10 ergo rx4=40
    background_shape_sprite=create_cubic_shaded_sprite(theme,shaded_sprite_buffer,"background_shape",56,0);///r=14 ergo rx4=56
    foreground_border_sprite=create_cubic_shaded_sprite(theme,shaded_sprite_buffer,"foreground_border",40,36);///r=10 ergo rx4=40  &  r=9 ergo rx4=36
    small_foreground_shape_sprite=create_cubic_shaded_sprite(theme,shaded_sprite_buffer,"small_foreground_shape",28,0);///r=7 ergo rx4=28

    create_overlay_sprite_from_sdl_surface(theme,"exit",icons_surface,16,16,0,0,OVERLAY_SHADED_SPRITE);
    create_overlay_sprite_from_sdl_surface(theme,"directory_up",icons_surface,16,16,16,0,OVERLAY_SHADED_SPRITE);
    create_overlay_sprite_from_sdl_surface(theme,"directory_refresh",icons_surface,16,16,32,0,OVERLAY_SHADED_SPRITE);
    create_overlay_sprite_from_sdl_surface(theme,"directory_hidden",icons_surface,16,16,48,0,OVERLAY_SHADED_SPRITE);
    create_overlay_sprite_from_sdl_surface(theme,"expand",icons_surface,16,16,0,16,OVERLAY_SHADED_SPRITE);
    create_overlay_sprite_from_sdl_surface(theme,"collapse",icons_surface,16,16,16,16,OVERLAY_SHADED_SPRITE);
    create_overlay_sprite_from_sdl_surface(theme,"folder",icons_surface,16,16,32,16,OVERLAY_SHADED_SPRITE);
    create_overlay_sprite_from_sdl_surface(theme,"misc_file",icons_surface,16,16,48,16,OVERLAY_SHADED_SPRITE);
    create_overlay_sprite_from_sdl_surface(theme,"image_file",icons_surface,16,16,0,32,OVERLAY_SHADED_SPRITE);
    create_overlay_sprite_from_sdl_surface(theme,"music_file",icons_surface,16,16,16,32,OVERLAY_SHADED_SPRITE);
    create_overlay_sprite_from_sdl_surface(theme,"vector_file",icons_surface,16,16,32,32,OVERLAY_SHADED_SPRITE);



    d=set_cubic_shaded_sprite_data(shaded_sprite_buffer,28,24,32,true);
    osd=create_overlay_sprite(theme,"false",d,d,OVERLAY_SHADED_SPRITE);
    set_overlay_sprite_image_data(theme,osd,shaded_sprite_buffer,cubic_sprite_buffer_size,d,d,0,0);

    d=set_cubic_shaded_sprite_data(shaded_sprite_buffer,16,0,32,false);
    osd=create_overlay_sprite(theme,"true",d,d,OVERLAY_SHADED_SPRITE);
    set_overlay_sprite_image_data(theme,osd,shaded_sprite_buffer,cubic_sprite_buffer_size,d,d,0,0);



    SDL_FreeSurface(icons_surface);
    free(shaded_sprite_buffer);
}
