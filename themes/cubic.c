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

/// how to handle non font "glyphs"? diy and actually use utf8 glyphs?
/// need way to render misc shapes (cubic parts here) dynamically, possibly just ensure they exist and create when necessary every frame? (is kind-of expensive, but may be best way)



/// "ring" (internal and external limit) variant
//static void cubic_create_shape(cvm_overlay_interactable_element * element,uint32_t ri,uint32_t ro)///change ri & r0 as 16ths of a pixel??
//{
//    uint32_t i,r,threshold,r4,x,y,xc,yc,xm,ym,t,r_min,r_max,rom,rim;
//    uint8_t *s;
//    uint8_t *data;
//
//    if(ro>640)
//    {
//        fprintf(stderr,"CUBIC OVERLAY ELEMENTS OF THIS SIZE NOT SUPPORTED\n");
//        exit(-1);
//    }
//
//    r4=(((ro>>4)-1)&~3)+4;///convert outer radius to pixels (not 1/16ths pixels) and round up to multiple of 4
//
//    element->tile=overlay_create_transparent_image_tile_with_staging(&s,r4*2,r4*2);
//
//    if(element->tile)
//    {
//        threshold=64;
//
//
//        data=s;
//        //data=malloc(sizeof(uint8_t)*r4*r4*4);
//
//        ro=ro*ro*ro;
//        ri=ri*ri*ri;
//
//        rom=ro*8;
//        rim=ri*8;
//
//        ro>>=12;
//        ri>>=12;
//
//
//
//        element->selection_grid=calloc((r4*r4)/4,sizeof(uint16_t));///r4*2*r4*2/16 == r4*r4/4
//
//        element->w=r4*2;
//        element->h=r4*2;
//
////        static struct timespec tso,tsn;
////        uint64_t dt;
////
////        clock_gettime(CLOCK_REALTIME,&tso);
//
//
//        for(x=0;x<r4;x++)
//        {
//            for(y=0;y<r4;y++)
//            {
//                t=0;
//                r_min=x*x*x + y*y*y;
//                r_max=(x+1)*(x+1)*(x+1) + (y+1)*(y+1)*(y+1);
//                if(r_min>=ro || r_max<=ri)t=0;
//                else if(r_max<=ro && r_min>=ri)t=255;
//                else for(xm=1;xm<32;xm+=2)for(ym=1;ym<32;ym+=2)///16 increments at pixel centres
//                {
//                    xc=(x<<5)+xm;
//                    yc=(y<<5)+ym;
//                    r=xc*xc*xc+yc*yc*yc;
//                    t+= r>rim && r<rom;
//                }
//                t-= t==256;
//
//                i=(r4+y)*r4*2+(r4+x);
//                data[i]=t;
//                element->selection_grid[i>>4]|=(t>threshold)<<(i&0x0F);
//
//                i=(r4+y)*r4*2+(r4-x-1);
//                data[i]=t;
//                element->selection_grid[i>>4]|=(t>threshold)<<(i&0x0F);
//
//                i=(r4-y-1)*r4*2+(r4+x);
//                data[i]=t;
//                element->selection_grid[i>>4]|=(t>threshold)<<(i&0x0F);
//
//                i=(r4-y-1)*r4*2+(r4-x-1);
//                data[i]=t;
//                element->selection_grid[i>>4]|=(t>threshold)<<(i&0x0F);
//            }
//        }
//
//        //memcpy(s,data,sizeof(uint8_t)*r4*r4*4);
//
////        clock_gettime(CLOCK_REALTIME,&tsn);
////
////        dt=(tsn.tv_sec-tso.tv_sec)*1000000000 + tsn.tv_nsec-tso.tv_nsec;
////
////        printf("CUBIC TIME = %lu      %u\n",dt,t);
//    }
//}


static void cubic_create_shape(cvm_vk_image_atlas_tile ** tile,uint16_t ** selection_grid,uint32_t r)///change ri & r0 as 16ths of a pixel??
{
    uint32_t i,rt,threshold,x,y,xc,yc,xm,ym,t,rm;
    uint8_t * data;
    uint16_t * grid;

    if(r>40)
    {
        fprintf(stderr,"CUBIC OVERLAY ELEMENTS OF THIS SIZE NOT SUPPORTED\n");
        exit(-1);
    }

    *tile=overlay_create_transparent_image_tile_with_staging((void*)(&data),r*2,r*2);

    if(*tile)
    {
        threshold=64;

        rt=r*r*r;

        rm=rt*32768;///32^3

        if(selection_grid)*selection_grid=grid=calloc((r*r+3)>>2,sizeof(uint16_t));///r*2*r*2/16 == r*r/4 == r*r>>2, but round up

        for(x=0;x<r;x++)
        {
            for(y=0;y<r;y++)
            {
                t=0;
                if((x*x*x + y*y*y)>=rt)t=0;
                else if(((x+1)*(x+1)*(x+1) + (y+1)*(y+1)*(y+1))<=rt)t=255;
                else
                    for(xm=1;xm<32;xm+=2)for(ym=1;ym<32;ym+=2)///16 increments at pixel centres
                {
                    xc=(x<<5)+xm;
                    yc=(y<<5)+ym;
                    t+= (xc*xc*xc+yc*yc*yc)<rm;
                }
                t-= t==256;

                i=(r+y)*r*2+(r+x);
                data[i]=t;
                if(selection_grid)grid[i>>4]|=(t>threshold)<<(i&0x0F);

                i=(r+y)*r*2+(r-x-1);
                data[i]=t;
                if(selection_grid)grid[i>>4]|=(t>threshold)<<(i&0x0F);

                i=(r-y-1)*r*2+(r+x);
                data[i]=t;
                if(selection_grid)grid[i>>4]|=(t>threshold)<<(i&0x0F);

                i=(r-y-1)*r*2+(r-x-1);
                data[i]=t;
                if(selection_grid)grid[i>>4]|=(t>threshold)<<(i&0x0F);
            }
        }
    }
}

bool cubic_square_select(rectangle_ r,uint32_t status,overlay_theme * theme)
{
    int i,y,x,o;
    cubic_theme_data * cubic;

    cubic=theme->other_data;

    if(!cubic->foreground_selection_grid)return false;

    y=(cubic->foreground_d-r.y1-r.y2)>>1;
    o=(r.x2-r.x1-cubic->foreground_d)>>1;

    if(y<0 || y>=cubic->foreground_d)return false;

    if(!(status&WIDGET_H_FIRST))///not the first
    {
        x= -r.x1-o;

        if(x>=0 && x<cubic->foreground_r)
        {
            i=y*cubic->foreground_d+x;
            if(cubic->foreground_selection_grid[i>>4] & (1<<(i&0x0F))) return true;
        }

        r.x1+=o+cubic->foreground_r;
    }

    if(!(status&WIDGET_H_LAST))///not the last
    {
        x= cubic->foreground_d+o-r.x2;

        if(x>=cubic->foreground_r && x<cubic->foreground_d)
        {
            i=y*cubic->foreground_d+x;
            if(cubic->foreground_selection_grid[i>>4] & (1<<(i&0x0F))) return true;
        }

        r.x2-=o+cubic->foreground_r;
    }

    return r.x1<=0 && r.x2>0;
}

bool cubic_h_bar_select(rectangle_ r,uint32_t status,overlay_theme * theme)
{
    int i,y,x;
    cubic_theme_data * cubic;

    cubic=theme->other_data;

    if(!cubic->foreground_selection_grid)return false;

    y=(cubic->foreground_d-r.y1-r.y2)>>1;

    if(y<0 || y>=cubic->foreground_d)return false;

    if(!(status&WIDGET_H_FIRST))///not the first
    {
        x= -r.x1-cubic->foreground_offset_x;

        if(x>=0 && x<cubic->foreground_r)
        {
            i=y*cubic->foreground_d+x;
            if(cubic->foreground_selection_grid[i>>4] & (1<<(i&0x0F))) return true;
        }

        r.x1+=cubic->foreground_offset_x+cubic->foreground_r;
    }

    if(!(status&WIDGET_H_LAST))///not the last
    {
        x= cubic->foreground_d+cubic->foreground_offset_x-r.x2;

        if(x>=cubic->foreground_r && x<cubic->foreground_d)
        {
            i=y*cubic->foreground_d+x;
            if(cubic->foreground_selection_grid[i>>4] & (1<<(i&0x0F))) return true;
        }

        r.x2-=cubic->foreground_offset_x+cubic->foreground_r;
    }

    return r.x1<=0 && r.x2>0;
}

bool cubic_v_bar_select(rectangle_ r,uint32_t status,overlay_theme * theme)
{
//    r.x+=(r.w-20)/2-x_in;
//    r.y+=12-y_in;
//    r.w=20;
//    r.h-=24;
//
//    if(check_click_on_shaded_sprite(theme,(rectangle){.x=r.x,.y=r.y-10,.w=20,.h=10},foreground_shape_sprite.x_pos,foreground_shape_sprite.y_pos))return true;
//    if(check_click_on_shaded_sprite(theme,(rectangle){.x=r.x,.y=r.y+r.h,.w=20,.h=10},foreground_shape_sprite.x_pos,foreground_shape_sprite.y_pos+10))return true;
//    return rectangle_surrounds_origin(r);
}

bool cubic_box_select(rectangle_ r,uint32_t status,overlay_theme * theme)
{
//    r.x-=x_in;
//    r.y+=1-y_in;
//    r.h-=2;
//
//    if(!(status&WIDGET_H_FIRST))///not the first
//    {
//        if(check_click_on_shaded_sprite(theme,(rectangle){.x=r.x+2,.y=r.y,.w=10,.h=10},foreground_shape_sprite.x_pos,foreground_shape_sprite.y_pos))return true;
//        if(rectangle_surrounds_origin((rectangle){.x=r.x+2,.y=r.y+10,.w=10,.h=r.h-20}))return true;
//        if(check_click_on_shaded_sprite(theme,(rectangle){.x=r.x+2,.y=r.y+r.h-10,.w=10,.h=10},foreground_shape_sprite.x_pos,foreground_shape_sprite.y_pos+10))return true;
//        r.x+=12;
//        r.w-=12;
//    }
//
//    if(!(status&WIDGET_H_LAST))///not the last
//    {
//
//        if(check_click_on_shaded_sprite(theme,(rectangle){.x=r.x+r.w-12,.y=r.y,.w=10,.h=20},foreground_shape_sprite.x_pos+10,foreground_shape_sprite.y_pos))return true;
//        if(rectangle_surrounds_origin((rectangle){.x=r.x+r.w-12,.y=r.y+10,.w=10,.h=r.h-20}))return true;
//        if(check_click_on_shaded_sprite(theme,(rectangle){.x=r.x+r.w-12,.y=r.y+r.h-10,.w=10,.h=10},foreground_shape_sprite.x_pos+10,foreground_shape_sprite.y_pos+10))return true;
//
//        r.w-=12;
//    }
//
//    return rectangle_surrounds_origin(r);
}

bool cubic_panel_select(rectangle_ r,uint32_t status,overlay_theme * theme)
{
    int i,y,x,y1,y2;
    cubic_theme_data * cubic;

    cubic=theme->other_data;

    if(!cubic->background_selection_grid)return false;

    ///panel has no internal offsets

    if(!(status&WIDGET_H_FIRST))
    {
        x= -r.x1;
        y1=r.y1;
        y2=r.y2;
        r.x1+=cubic->background_r;

        if(x>=0 && x<cubic->background_r)
        {
            if(!(status&WIDGET_V_FIRST))
            {
                y= -y1;

                if(y>=0 && y<cubic->background_r)
                {
                    i=y*cubic->background_d+x;
                    if(cubic->background_selection_grid[i>>4] & (1<<(i&0x0F))) return true;
                }

                y1+=cubic->background_r;
            }

            if(!(status&WIDGET_V_LAST))
            {
                y= cubic->background_d-y2;

                if(y>=cubic->background_r && y<cubic->background_d)
                {
                    i=y*cubic->background_d+x;
                    if(cubic->background_selection_grid[i>>4] & (1<<(i&0x0F))) return true;
                }

                y2-=cubic->background_r;
            }

            return y1<=0 && y2>0;
        }
    }

    if(!(status&WIDGET_H_LAST))
    {
        x= cubic->background_d-r.x2;
        y1=r.y1;
        y2=r.y2;
        r.x2-=cubic->background_r;

        if(x>=cubic->background_r && x<cubic->background_d)
        {
            if(!(status&WIDGET_V_FIRST))
            {
                y= -y1;

                if(y>=0 && y<cubic->background_r)
                {
                    i=y*cubic->background_d+x;
                    if(cubic->background_selection_grid[i>>4] & (1<<(i&0x0F))) return true;
                }

                y1+=cubic->background_r;
            }

            if(!(status&WIDGET_V_LAST))
            {
                y= cubic->background_d-y2;

                if(y>=cubic->background_r && y<cubic->background_d)
                {
                    i=y*cubic->background_d+x;
                    if(cubic->background_selection_grid[i>>4] & (1<<(i&0x0F))) return true;
                }

                y2-=cubic->background_r;
            }

            return y1<=0 && y2>0;
        }
    }

    return rectangle_surrounds_origin_(r);
}

void cubic_square_icon_render(rectangle_ r,uint32_t status,overlay_theme * theme,overlay_data * od,rectangle_ bounds,overlay_colour_ colour,char * icon_glyph,overlay_colour_ icon_colour)
{
    rectangle_ rg,rb,rr;
    int o;
    cubic_theme_data * cubic;
    cvm_overlay_glyph * g;

    cubic=theme->other_data;

    g=overlay_get_glyph(&theme->font_,icon_glyph);

    if(!cubic->foreground_image_tile)cubic_create_shape(&cubic->foreground_image_tile,&cubic->foreground_selection_grid,cubic->foreground_r);
    if(!cubic->foreground_image_tile || !g->tile)return;

    cvm_overlay_element_render_buffer * element_render_buffer=od;///HACK, temporary measure before switching types

    rg=r;

    r.y1=(r.y1+r.y2-cubic->foreground_d)>>1;
    r.y2=r.y1+cubic->foreground_d;
    o=(r.x2-r.x1-cubic->foreground_d)>>1;


    if(!(status&WIDGET_H_FIRST))
    {
        r.x1+= o;

        if(element_render_buffer->count != element_render_buffer->space)
        {
            rb=r;
            rb.x2=rb.x1+cubic->foreground_r;
            rr=get_rectangle_overlap_(rb,bounds);

            if((rectangle_has_positive_area(rr))) element_render_buffer->buffer[element_render_buffer->count++]=(cvm_overlay_render_data)
            {
                {rr.x1,rr.y1,rr.x2,rr.y2},
                {(CVM_OVERLAY_ELEMENT_SHADED<<12)|(OVERLAY_MAIN_COLOUR_&0x0FFF),(cubic->foreground_image_tile->x_pos<<2)+rr.x1-rb.x1,(cubic->foreground_image_tile->y_pos<<2)+rr.y1-rb.y1,83},
            };
        }

        r.x1+=cubic->foreground_r;
    }

    if(!(status&WIDGET_H_LAST))
    {
        r.x2-= o;

        if(element_render_buffer->count != element_render_buffer->space)
        {
            rb=r;
            rb.x1=rb.x2-cubic->foreground_r;
            rr=get_rectangle_overlap_(rb,bounds);

            if((rectangle_has_positive_area(rr))) element_render_buffer->buffer[element_render_buffer->count++]=(cvm_overlay_render_data)
            {
                {rr.x1,rr.y1,rr.x2,rr.y2},
                {(CVM_OVERLAY_ELEMENT_SHADED<<12)|(OVERLAY_MAIN_COLOUR_&0x0FFF),(cubic->foreground_image_tile->x_pos<<2)+rr.x1-rb.x1+cubic->foreground_r,(cubic->foreground_image_tile->y_pos<<2)+rr.y1-rb.y1,83},
            };
        }

        r.x2-=cubic->foreground_r;
    }

    if(element_render_buffer->count != element_render_buffer->space)
    {
        rr=get_rectangle_overlap_(r,bounds);

        if((rectangle_has_positive_area(rr))) element_render_buffer->buffer[element_render_buffer->count++]=(cvm_overlay_render_data)
        {
            {rr.x1,rr.y1,rr.x2,rr.y2},
            {(CVM_OVERLAY_ELEMENT_FILL<<12)|(OVERLAY_MAIN_COLOUR_&0x0FFF),0,0,83},
        };
    }

    rg.y1=(rg.y1+rg.y2 - g->pos.y2+g->pos.y1)>>1;
    rg.y2=rg.y1+g->pos.y2-g->pos.y1;
    rg.x1=(rg.x1+rg.x2 - g->pos.x2+g->pos.x1)>>1;
    rg.x2=rg.x1+g->pos.x2-g->pos.x1;

    if(element_render_buffer->count != element_render_buffer->space)
    {
        rr=get_rectangle_overlap_(rg,bounds);

        if((rectangle_has_positive_area(rr))) element_render_buffer->buffer[element_render_buffer->count++]=(cvm_overlay_render_data)
        {
            {rr.x1,rr.y1,rr.x2,rr.y2},
            {(CVM_OVERLAY_ELEMENT_SHADED<<12)|(OVERLAY_TEXT_COLOUR_0_&0x0FFF),(g->tile->x_pos<<2)+rr.x1-rg.x1,(g->tile->y_pos<<2)+rr.y1-rg.y1,83},
        };
    }
}

void cubic_h_text_bar_render(rectangle_ r,uint32_t status,overlay_theme * theme,overlay_data * od,rectangle_ bounds,overlay_colour_ colour,char * text,overlay_colour_ text_colour)
{
    rectangle_ rb,rt,rr;
    cubic_theme_data * cubic;

    cubic=theme->other_data;

    if(!cubic->foreground_image_tile)cubic_create_shape(&cubic->foreground_image_tile,&cubic->foreground_selection_grid,cubic->foreground_r);
    if(!cubic->foreground_image_tile)return;

    cvm_overlay_element_render_buffer * element_render_buffer=od;///HACK, temporary measure before switching types

    rt=r;

    r.y1=(r.y1+r.y2-cubic->foreground_d)>>1;
    r.y2=r.y1+cubic->foreground_d;


    if(!(status&WIDGET_H_FIRST))
    {
        r.x1+= cubic->foreground_offset_x;

        if(element_render_buffer->count != element_render_buffer->space)
        {
            rb=r;
            rb.x2=rb.x1+cubic->foreground_r;
            rr=get_rectangle_overlap_(rb,bounds);

            if((rectangle_has_positive_area(rr))) element_render_buffer->buffer[element_render_buffer->count++]=(cvm_overlay_render_data)
            {
                {rr.x1,rr.y1,rr.x2,rr.y2},
                {(CVM_OVERLAY_ELEMENT_SHADED<<12)|(colour&0x0FFF),(cubic->foreground_image_tile->x_pos<<2)+rr.x1-rb.x1,(cubic->foreground_image_tile->y_pos<<2)+rr.y1-rb.y1,83},
            };
        }

        r.x1+=cubic->foreground_r;
    }

    if(!(status&WIDGET_H_LAST))
    {
        r.x2-= cubic->foreground_offset_x;

        if(element_render_buffer->count != element_render_buffer->space)
        {
            rb=r;
            rb.x1=rb.x2-cubic->foreground_r;
            rr=get_rectangle_overlap_(rb,bounds);

            if((rectangle_has_positive_area(rr))) element_render_buffer->buffer[element_render_buffer->count++]=(cvm_overlay_render_data)
            {
                {rr.x1,rr.y1,rr.x2,rr.y2},
                {(CVM_OVERLAY_ELEMENT_SHADED<<12)|(colour&0x0FFF),(cubic->foreground_image_tile->x_pos<<2)+rr.x1-rb.x1+cubic->foreground_r,(cubic->foreground_image_tile->y_pos<<2)+rr.y1-rb.y1,83},
            };
        }

        r.x2-=cubic->foreground_r;
    }

    if(element_render_buffer->count != element_render_buffer->space)
    {
        rr=get_rectangle_overlap_(r,bounds);

        if((rectangle_has_positive_area(rr))) element_render_buffer->buffer[element_render_buffer->count++]=(cvm_overlay_render_data)
        {
            {rr.x1,rr.y1,rr.x2,rr.y2},
            {(CVM_OVERLAY_ELEMENT_FILL<<12)|(colour&0x0FFF),0,0,83},
        };
    }

    if(text)
    {
        rt.y1=(rt.y1+rt.y2-theme->font_.glyph_size)>>1;
        rt.y2=rt.y1+theme->font_.glyph_size;
        rt.x1+=theme->h_bar_text_offset;
        rt.x2-=theme->h_bar_text_offset;

        rb=get_rectangle_overlap_(rt,bounds);

        if(rectangle_has_positive_area(rb))overlay_render_text_simple(element_render_buffer,&theme->font_,text,rt.x1,rt.y1,&rb,text_colour);
    }
}

void cubic_h_icon_text_bar_render(rectangle r,int x_off,int y_off,uint32_t status,overlay_theme * theme,overlay_data * od,rectangle bounds,overlay_colour colour,char * text,char * icon_name,overlay_colour icon_colour)
{
//    r.x+=x_off;
//    r.y+=y_off+(r.h-20)/2;
//    r.h=20;
//
//    /// use x_off and y_off here for cleanlyness/legibility purposes ??? also for text placement when on border
//
//    x_off=r.x+theme->h_bar_text_offset+theme->icon_bar_extra_w;
//    y_off=r.y+(r.h-theme->font.font_height)/2;
//    int text_w=r.w-2*theme->h_bar_text_offset- theme->icon_bar_extra_w;
//
//    if(!(status&WIDGET_H_FIRST))///not the first
//    {
//        render_overlay_shade(od,(rectangle){.x=r.x+2,.y=r.y,.w=10,.h=20},bounds,foreground_shape_sprite.x_pos,foreground_shape_sprite.y_pos,colour);
//        r.x+=12;
//        r.w-=12;
//    }
//
//    if(!(status&WIDGET_H_LAST))///not the last
//    {
//        render_overlay_shade(od,(rectangle){.x=r.x+r.w-12,.y=r.y,.w=10,.h=20},bounds,foreground_shape_sprite.x_pos+10,foreground_shape_sprite.y_pos,colour);
//        r.w-=12;
//    }
//
//    render_rectangle(od,r,bounds,colour);
//
//    if(text)
//    {
//        rectangle text_bounds=bounds;
//        if(get_rectangle_overlap(&text_bounds,(rectangle){.x=x_off,.y=y_off,.w=text_w,.h=theme->font.font_height}))render_overlay_text(od,theme,text,x_off,y_off,text_bounds,0,0);
//    }
//
//    if(icon_name)
//    {
//        x_off-=theme->h_bar_text_offset+16;///16=icon_size
//        y_off=r.y+(r.h-16)/2;///16=icon_size
//
//        overlay_sprite_data icon=get_overlay_sprite_data(theme,icon_name);
//        render_overlay_shade(od,(rectangle){.x=x_off,.y=y_off,.w=16,.h=16},bounds,icon.x_pos,icon.y_pos,icon_colour);
//    }
}

void cubic_h_text_icon_bar_render(rectangle r,int x_off,int y_off,uint32_t status,overlay_theme * theme,overlay_data * od,rectangle bounds,overlay_colour colour,char * text,char * icon_name,overlay_colour icon_colour)
{
//    r.x+=x_off;
//    r.y+=y_off+(r.h-20)/2;
//    r.h=20;
//
//    /// use x_off and y_off here for cleanlyness/legibility purposes ??? also for text placement when on border
//
//    x_off=r.x+theme->h_bar_text_offset;
//    y_off=r.y+(r.h-theme->font.font_height)/2;
//    int text_w=r.w-2*theme->h_bar_text_offset-theme->icon_bar_extra_w;
//
//    if(!(status&WIDGET_H_FIRST))///not the first
//    {
//        render_overlay_shade(od,(rectangle){.x=r.x+2,.y=r.y,.w=10,.h=20},bounds,foreground_shape_sprite.x_pos,foreground_shape_sprite.y_pos,colour);
//        r.x+=12;
//        r.w-=12;
//    }
//
//    if(!(status&WIDGET_H_LAST))///not the last
//    {
//        render_overlay_shade(od,(rectangle){.x=r.x+r.w-12,.y=r.y,.w=10,.h=20},bounds,foreground_shape_sprite.x_pos+10,foreground_shape_sprite.y_pos,colour);
//        r.w-=12;
//    }
//
//    render_rectangle(od,r,bounds,colour);
//
//    if(text)
//    {
//        rectangle text_bounds=bounds;
//        if(get_rectangle_overlap(&text_bounds,(rectangle){.x=x_off,.y=y_off,.w=text_w,.h=theme->font.font_height}))render_overlay_text(od,theme,text,x_off,y_off,text_bounds,0,0);
//    }
//
//    if(icon_name)
//    {
//        x_off+=text_w+theme->h_bar_text_offset;
//        y_off=r.y+(r.h-16)/2;///16=icon_size
//
//        overlay_sprite_data icon=get_overlay_sprite_data(theme,icon_name);
//        render_overlay_shade(od,(rectangle){.x=x_off,.y=y_off,.w=16,.h=16},bounds,icon.x_pos,icon.y_pos,icon_colour);
//    }
}

void cubic_h_slider_bar_render(rectangle_ r,uint32_t status,overlay_theme * theme,overlay_data * od,rectangle_ bounds,overlay_colour_ colour,int range,int value,int bar,overlay_colour_ bar_colour)
{
    //puts("cubic slider render");

    //int
    rectangle_ rb,rr,rs;
    cubic_theme_data * cubic;

    cubic=theme->other_data;

    if(!cubic->foreground_image_tile)cubic_create_shape(&cubic->foreground_image_tile,&cubic->foreground_selection_grid,cubic->foreground_r);
    if(!cubic->foreground_image_tile)return;

    if(!cubic->internal_image_tile)cubic_create_shape(&cubic->internal_image_tile,NULL,cubic->internal_r);
    if(!cubic->internal_image_tile)return;

    cvm_overlay_element_render_buffer * element_render_buffer=od;///HACK, temporary measure before switching types

    rs=r;

    r.y1=(r.y1+r.y2-cubic->foreground_d)>>1;
    r.y2=r.y1+cubic->foreground_d;


    if(!(status&WIDGET_H_FIRST))
    {
        r.x1+= cubic->foreground_offset_x;
        rs.x1+= cubic->foreground_offset_x;

        if(element_render_buffer->count != element_render_buffer->space)
        {
            rb=r;
            rb.x2=rb.x1+cubic->foreground_r;
            rr=get_rectangle_overlap_(rb,bounds);

            if((rectangle_has_positive_area(rr))) element_render_buffer->buffer[element_render_buffer->count++]=(cvm_overlay_render_data)
            {
                {rr.x1,rr.y1,rr.x2,rr.y2},
                {(CVM_OVERLAY_ELEMENT_SHADED<<12)|(colour&0x0FFF),(cubic->foreground_image_tile->x_pos<<2)+rr.x1-rb.x1,(cubic->foreground_image_tile->y_pos<<2)+rr.y1-rb.y1,83},
            };
        }

        r.x1+=cubic->foreground_r;
    }

    if(!(status&WIDGET_H_LAST))
    {
        r.x2-= cubic->foreground_offset_x;
        rs.x2-= cubic->foreground_offset_x;

        if(element_render_buffer->count != element_render_buffer->space)
        {
            rb=r;
            rb.x1=rb.x2-cubic->foreground_r;
            rr=get_rectangle_overlap_(rb,bounds);

            if((rectangle_has_positive_area(rr))) element_render_buffer->buffer[element_render_buffer->count++]=(cvm_overlay_render_data)
            {
                {rr.x1,rr.y1,rr.x2,rr.y2},
                {(CVM_OVERLAY_ELEMENT_SHADED<<12)|(colour&0x0FFF),(cubic->foreground_image_tile->x_pos<<2)+rr.x1-rb.x1+cubic->foreground_r,(cubic->foreground_image_tile->y_pos<<2)+rr.y1-rb.y1,83},
            };
        }

        r.x2-=cubic->foreground_r;
    }

    if(element_render_buffer->count != element_render_buffer->space)
    {
        rr=get_rectangle_overlap_(r,bounds);

        if((rectangle_has_positive_area(rr))) element_render_buffer->buffer[element_render_buffer->count++]=(cvm_overlay_render_data)
        {
            {rr.x1,rr.y1,rr.x2,rr.y2},
            {(CVM_OVERLAY_ELEMENT_FILL<<12)|(colour&0x0FFF),0,0,83},
        };
    }

    rs.y1=(rs.y1+rs.y2-cubic->internal_d)>>1;
    rs.y2=rs.y1+cubic->internal_d;

    rs.x1+=cubic->foreground_r;
    rs.x2-=cubic->foreground_r;

    if(bar<0)///works like a flag, perhaps better to put in reserved status bit?
    {
        bar=(rs.x2-rs.x1)/ -bar;
        rs.x1+=((rs.x2-rs.x1-bar)*value)/range;
        rs.x2=rs.x1+bar;
    }
    else
    {
        rs.x1+=((rs.x2-rs.x1)*value)/(range+bar);
        rs.x2-=((rs.x2-rs.x1)*(range-value))/(range+bar);
    }

    if(element_render_buffer->count != element_render_buffer->space)
    {
        rb=rs;
        rb.x2=rb.x1;
        rb.x1-=cubic->internal_r;
        rr=get_rectangle_overlap_(rb,bounds);

        if((rectangle_has_positive_area(rr))) element_render_buffer->buffer[element_render_buffer->count++]=(cvm_overlay_render_data)
        {
            {rr.x1,rr.y1,rr.x2,rr.y2},
            {(CVM_OVERLAY_ELEMENT_SHADED<<12)|(bar_colour&0x0FFF),(cubic->internal_image_tile->x_pos<<2)+rr.x1-rb.x1,(cubic->internal_image_tile->y_pos<<2)+rr.y1-rb.y1,83},
        };
    }

    if(element_render_buffer->count != element_render_buffer->space)
    {
        rb=rs;
        rb.x1=rb.x2;
        rb.x2+=cubic->internal_r;
        rr=get_rectangle_overlap_(rb,bounds);

        if((rectangle_has_positive_area(rr))) element_render_buffer->buffer[element_render_buffer->count++]=(cvm_overlay_render_data)
        {
            {rr.x1,rr.y1,rr.x2,rr.y2},
            {(CVM_OVERLAY_ELEMENT_SHADED<<12)|(bar_colour&0x0FFF),(cubic->internal_image_tile->x_pos<<2)+rr.x1-rb.x1+cubic->internal_r,(cubic->internal_image_tile->y_pos<<2)+rr.y1-rb.y1,83},
        };
    }

    if(element_render_buffer->count != element_render_buffer->space)
    {
        rr=get_rectangle_overlap_(rs,bounds);

        if((rectangle_has_positive_area(rr))) element_render_buffer->buffer[element_render_buffer->count++]=(cvm_overlay_render_data)
        {
            {rr.x1,rr.y1,rr.x2,rr.y2},
            {(CVM_OVERLAY_ELEMENT_FILL<<12)|(bar_colour&0x0FFF),0,0,83},
        };
    }

    ///perhaps slightly smaller and disregard edge when not first??

//    r.x+=x_off+12;
//    r.y+=y_off+(r.h-20)/2;
//    r.w-=theme->h_sliderbar_lost_w;
//    r.h=20;
//
//    render_overlay_shade(od,(rectangle){.x=r.x-10,.y=r.y,.w=10,.h=20},bounds,foreground_border_sprite.x_pos,foreground_border_sprite.y_pos,colour);
//    render_overlay_shade(od,(rectangle){.x=r.x+r.w,.y=r.y,.w=10,.h=20},bounds,foreground_border_sprite.x_pos+10,foreground_border_sprite.y_pos,colour);
//    render_border(od,r,bounds,(rectangle){.x=r.x,.y=r.y+1,.w=r.w,.h=18},colour);
//
//    r.y+=3;
//    r.h-=6;
//
//    if(bar<0)
//    {
//        bar=r.w/(-bar);
//        r.x+=((r.w-bar)*value)/range;
//        r.w=bar;
//    }
//    else
//    {
//        r.x+=(r.w*value)/(range+bar);
//        r.w-=(r.w*range)/(range+bar);
//    }
//
//    render_overlay_shade(od,(rectangle){.x=r.x-7,.y=r.y,.w=7,.h=14},bounds,small_foreground_shape_sprite.x_pos,small_foreground_shape_sprite.y_pos,colour);
//    render_overlay_shade(od,(rectangle){.x=r.x+r.w,.y=r.y,.w=7,.h=14},bounds,small_foreground_shape_sprite.x_pos+7,small_foreground_shape_sprite.y_pos,colour);
//    render_rectangle(od,r,bounds,colour);
}

void cubic_v_slider_bar_render(rectangle r,int x_off,int y_off,uint32_t status,overlay_theme * theme,overlay_data * od,rectangle bounds,overlay_colour colour,int range,int value,int bar)
{
//    r.x+=x_off+(r.w-20)/2;
//    r.y+=y_off+11;
//    r.w=20;
//    r.h-=theme->v_sliderbar_lost_h;
//
//    render_overlay_shade(od,(rectangle){.x=r.x,.y=r.y-10,.w=20,.h=10},bounds,foreground_border_sprite.x_pos,foreground_border_sprite.y_pos,colour);
//    render_overlay_shade(od,(rectangle){.x=r.x,.y=r.y+r.h,.w=20,.h=10},bounds,foreground_border_sprite.x_pos,foreground_border_sprite.y_pos+10,colour);
//    render_border(od,r,bounds,(rectangle){.x=r.x+1,.y=r.y,.w=18,.h=r.h},colour);
//
//    r.x+=3;
//    r.w-=6;
//
//    if(bar<0)
//    {
//        bar=r.h/(-bar);
//        r.y+=((r.h-bar)*value)/range;
//        r.h=bar;
//    }
//    else
//    {
//        r.y+=(r.h*value)/(range+bar);
//        r.h-=(r.h*range)/(range+bar);
//    }
//
//    render_overlay_shade(od,(rectangle){.x=r.x,.y=r.y-7,.w=14,.h=7},bounds,small_foreground_shape_sprite.x_pos,small_foreground_shape_sprite.y_pos,colour);
//    render_overlay_shade(od,(rectangle){.x=r.x,.y=r.y+r.h,.w=14,.h=7},bounds,small_foreground_shape_sprite.x_pos,small_foreground_shape_sprite.y_pos+7,colour);
//    render_rectangle(od,r,bounds,colour);
}

void cubic_box_render(rectangle r,int x_off,int y_off,uint32_t status,overlay_theme * theme,overlay_data * od,rectangle bounds,overlay_colour colour)
{
//    r.x+=x_off;
//    r.y+=y_off+1;
//    r.h-=2;
//
//    if(!(status&WIDGET_H_FIRST))///not the first
//    {
//        render_overlay_shade(od,(rectangle){.x=r.x+2,.y=r.y,.w=10,.h=10},bounds,foreground_shape_sprite.x_pos,foreground_shape_sprite.y_pos,colour);
//        render_rectangle(od,(rectangle){.x=r.x+2,.y=r.y+10,.w=10,.h=r.h-20},bounds,colour);
//        render_overlay_shade(od,(rectangle){.x=r.x+2,.y=r.y+r.h-10,.w=10,.h=10},bounds,foreground_shape_sprite.x_pos,foreground_shape_sprite.y_pos+10,colour);
//        r.x+=12;
//        r.w-=12;
//    }
//
//    if(!(status&WIDGET_H_LAST))///not the last
//    {
//        render_overlay_shade(od,(rectangle){.x=r.x+r.w-12,.y=r.y,.w=10,.h=10},bounds,foreground_shape_sprite.x_pos+10,foreground_shape_sprite.y_pos,colour);
//        render_rectangle(od,(rectangle){.x=r.x+r.w-12,.y=r.y+10,.w=10,.h=r.h-20},bounds,colour);
//        render_overlay_shade(od,(rectangle){.x=r.x+r.w-12,.y=r.y+r.h-10,.w=10,.h=10},bounds,foreground_shape_sprite.x_pos+10,foreground_shape_sprite.y_pos+10,colour);
//        r.w-=12;
//    }
//
//    render_rectangle(od,r,bounds,colour);
}

void cubic_panel_render(rectangle_ r,uint32_t status,overlay_theme * theme,overlay_data * od,rectangle_ bounds,overlay_colour_ colour)
{
    rectangle_ rv,re,rr;
    cubic_theme_data * cubic;

    cubic=theme->other_data;

    if(!cubic->background_image_tile)cubic_create_shape(&cubic->background_image_tile,&cubic->background_selection_grid,cubic->background_r);
    if(!cubic->background_image_tile)return;

    cvm_overlay_element_render_buffer * element_render_buffer=od;///HACK, temporary measure before switching types

    if(!(status&WIDGET_H_FIRST))
    {
        rv=r;
        rv.x2=r.x1+=cubic->background_r;

        if(!(status&WIDGET_V_FIRST))
        {
            if(element_render_buffer->count != element_render_buffer->space)
            {
                re=rv;
                re.y2=re.y1+cubic->background_r;
                rr=get_rectangle_overlap_(re,bounds);

                if((rectangle_has_positive_area(rr))) element_render_buffer->buffer[element_render_buffer->count++]=(cvm_overlay_render_data)
                {
                    {rr.x1,rr.y1,rr.x2,rr.y2},
                    {(CVM_OVERLAY_ELEMENT_SHADED<<12)|(OVERLAY_BACKGROUND_COLOUR_&0x0FFF),(cubic->background_image_tile->x_pos<<2)+rr.x1-re.x1,(cubic->background_image_tile->y_pos<<2)+rr.y1-re.y1,83},
                };
            }

            rv.y1+=cubic->background_r;
        }

        if(!(status&WIDGET_V_LAST))
        {
            if(element_render_buffer->count != element_render_buffer->space)
            {
                re=rv;
                re.y1=re.y2-cubic->background_r;
                rr=get_rectangle_overlap_(re,bounds);

                if((rectangle_has_positive_area(rr))) element_render_buffer->buffer[element_render_buffer->count++]=(cvm_overlay_render_data)
                {
                    {rr.x1,rr.y1,rr.x2,rr.y2},
                    {(CVM_OVERLAY_ELEMENT_SHADED<<12)|(OVERLAY_BACKGROUND_COLOUR_&0x0FFF),(cubic->background_image_tile->x_pos<<2)+rr.x1-re.x1,(cubic->background_image_tile->y_pos<<2)+rr.y1-re.y1+cubic->background_r,83},
                };
            }

            rv.y2-=cubic->background_r;
        }

        if(element_render_buffer->count != element_render_buffer->space)///need to check again as can return null once again;
        {
            rr=get_rectangle_overlap_(rv,bounds);

            if((rectangle_has_positive_area(rr))) element_render_buffer->buffer[element_render_buffer->count++]=(cvm_overlay_render_data)
            {
                {rr.x1,rr.y1,rr.x2,rr.y2},
                {(CVM_OVERLAY_ELEMENT_FILL<<12)|(OVERLAY_BACKGROUND_COLOUR_&0x0FFF),0,0,83},
            };
        }
    }

    if(!(status&WIDGET_H_LAST))
    {
        rv=r;
        rv.x1=r.x2-=cubic->background_r;

        if(!(status&WIDGET_V_FIRST))
        {
            if(element_render_buffer->count != element_render_buffer->space)
            {
                re=rv;
                re.y2=re.y1+cubic->background_r;
                rr=get_rectangle_overlap_(re,bounds);

                if((rectangle_has_positive_area(rr))) element_render_buffer->buffer[element_render_buffer->count++]=(cvm_overlay_render_data)
                {
                    {rr.x1,rr.y1,rr.x2,rr.y2},
                    {(CVM_OVERLAY_ELEMENT_SHADED<<12)|(OVERLAY_BACKGROUND_COLOUR_&0x0FFF),(cubic->background_image_tile->x_pos<<2)+rr.x1-re.x1+cubic->background_r,(cubic->background_image_tile->y_pos<<2)+rr.y1-re.y1,83},
                };
            }

            rv.y1+=cubic->background_r;
        }

        if(!(status&WIDGET_V_LAST))
        {
            if(element_render_buffer->count != element_render_buffer->space)
            {
                re=rv;
                re.y1=re.y2-cubic->background_r;
                rr=get_rectangle_overlap_(re,bounds);

                if((rectangle_has_positive_area(rr))) element_render_buffer->buffer[element_render_buffer->count++]=(cvm_overlay_render_data)
                {
                    {rr.x1,rr.y1,rr.x2,rr.y2},
                    {(CVM_OVERLAY_ELEMENT_SHADED<<12)|(OVERLAY_BACKGROUND_COLOUR_&0x0FFF),(cubic->background_image_tile->x_pos<<2)+rr.x1-re.x1+cubic->background_r,(cubic->background_image_tile->y_pos<<2)+rr.y1-re.y1+cubic->background_r,83},
                };
            }

            rv.y2-=cubic->background_r;
        }

        if(element_render_buffer->count != element_render_buffer->space)///need to check again as can return null once again;
        {
            rr=get_rectangle_overlap_(rv,bounds);

            if((rectangle_has_positive_area(rr))) element_render_buffer->buffer[element_render_buffer->count++]=(cvm_overlay_render_data)
            {
                {rr.x1,rr.y1,rr.x2,rr.y2},
                {(CVM_OVERLAY_ELEMENT_FILL<<12)|(OVERLAY_BACKGROUND_COLOUR_&0x0FFF),0,0,83},
            };
        }
    }

    if(element_render_buffer->count != element_render_buffer->space)///need to check again as can return null once again;
    {
        rr=get_rectangle_overlap_(r,bounds);

        if((rectangle_has_positive_area(rr))) element_render_buffer->buffer[element_render_buffer->count++]=(cvm_overlay_render_data)
        {
            {rr.x1,rr.y1,rr.x2,rr.y2},
            {(CVM_OVERLAY_ELEMENT_FILL<<12)|(OVERLAY_BACKGROUND_COLOUR_&0x0FFF),0,0,83},
        };
    }
}

overlay_theme * create_cubic_theme(void)
{
//    uint8_t * shaded_sprite_buffer=malloc(sizeof(uint8_t)*cubic_sprite_buffer_size*cubic_sprite_buffer_size);
//    overlay_sprite_data osd;
//    int d;
//
//    load_font_to_overlay(glf,theme,"resources/CVM_font_1.ttf",16);
//    SDL_Surface * icons_surface=IMG_Load("resources/icons_alpha.png");
//
//    if(icons_surface==NULL)
//    {
//        puts("THEME ICONS FILE NOT FOUND");
//        exit(-1);
//    }
//
//
    overlay_theme * theme;
    cubic_theme_data * cubic;


    theme=malloc(sizeof(overlay_theme));
    theme->other_data=cubic=malloc(sizeof(cubic_theme_data));

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

    cubic->foreground_image_tile=NULL;
    cubic->foreground_selection_grid=NULL;
    cubic->foreground_offset_x=2;
    cubic->foreground_r=10;
    cubic->foreground_d=20;

    cubic->background_image_tile=NULL;
    cubic->background_selection_grid=NULL;
    cubic->background_r=14;
    cubic->background_d=28;

    cubic->internal_image_tile=NULL;
    //cubic->internal_selection_grid=NULL;
    cubic->internal_r=8;
    cubic->internal_d=16;

    cvm_overlay_create_font(&theme->font_,"cvm_shared/resources/cvm_font_1.ttf",16);
//
//
//    foreground_shape_sprite=create_cubic_shaded_sprite(theme,shaded_sprite_buffer,"foreground_shape",40,0);///r=10 ergo rx4=40
//    background_shape_sprite=create_cubic_shaded_sprite(theme,shaded_sprite_buffer,"background_shape",56,0);///r=14 ergo rx4=56
//    foreground_border_sprite=create_cubic_shaded_sprite(theme,shaded_sprite_buffer,"foreground_border",40,36);///r=10 ergo rx4=40  &  r=9 ergo rx4=36
//    small_foreground_shape_sprite=create_cubic_shaded_sprite(theme,shaded_sprite_buffer,"small_foreground_shape",28,0);///r=7 ergo rx4=28
//
//    create_overlay_sprite_from_sdl_surface(theme,"exit",icons_surface,16,16,0,0,OVERLAY_SHADED_SPRITE);
//    create_overlay_sprite_from_sdl_surface(theme,"directory_up",icons_surface,16,16,16,0,OVERLAY_SHADED_SPRITE);
//    create_overlay_sprite_from_sdl_surface(theme,"directory_refresh",icons_surface,16,16,32,0,OVERLAY_SHADED_SPRITE);
//    create_overlay_sprite_from_sdl_surface(theme,"directory_hidden",icons_surface,16,16,48,0,OVERLAY_SHADED_SPRITE);
//    create_overlay_sprite_from_sdl_surface(theme,"expand",icons_surface,16,16,0,16,OVERLAY_SHADED_SPRITE);
//    create_overlay_sprite_from_sdl_surface(theme,"collapse",icons_surface,16,16,16,16,OVERLAY_SHADED_SPRITE);
//    create_overlay_sprite_from_sdl_surface(theme,"folder",icons_surface,16,16,32,16,OVERLAY_SHADED_SPRITE);
//    create_overlay_sprite_from_sdl_surface(theme,"misc_file",icons_surface,16,16,48,16,OVERLAY_SHADED_SPRITE);
//    create_overlay_sprite_from_sdl_surface(theme,"image_file",icons_surface,16,16,0,32,OVERLAY_SHADED_SPRITE);
//    create_overlay_sprite_from_sdl_surface(theme,"music_file",icons_surface,16,16,16,32,OVERLAY_SHADED_SPRITE);
//    create_overlay_sprite_from_sdl_surface(theme,"vector_file",icons_surface,16,16,32,32,OVERLAY_SHADED_SPRITE);
//
///// ↑←↓→ 🗙 🔄 💾 📁 📷 📄 🎵 ➕ ➖ 👻 🖋 🔓 🔗 🖼 ✂ 👁 ⚙
//
//    d=set_cubic_shaded_sprite_data(shaded_sprite_buffer,28,24,32,true);
//    osd=create_overlay_sprite(theme,"false",d,d,OVERLAY_SHADED_SPRITE);
//    set_overlay_sprite_image_data(theme,osd,shaded_sprite_buffer,cubic_sprite_buffer_size,d,d,0,0);
//
//    d=set_cubic_shaded_sprite_data(shaded_sprite_buffer,16,0,32,false);
//    osd=create_overlay_sprite(theme,"true",d,d,OVERLAY_SHADED_SPRITE);
//    set_overlay_sprite_image_data(theme,osd,shaded_sprite_buffer,cubic_sprite_buffer_size,d,d,0,0);
//
//
//
//    SDL_FreeSurface(icons_surface);
//    free(shaded_sprite_buffer);
}

void destroy_cubic_theme(overlay_theme * theme)
{
    cubic_theme_data * cubic;
    cubic=theme->other_data;

    if(cubic->foreground_image_tile)overlay_destroy_transparent_image_tile(cubic->foreground_image_tile);
    if(cubic->foreground_selection_grid)free(cubic->foreground_selection_grid);

    if(cubic->background_image_tile)overlay_destroy_transparent_image_tile(cubic->background_image_tile);
    if(cubic->background_selection_grid)free(cubic->background_selection_grid);

    if(cubic->internal_image_tile)overlay_destroy_transparent_image_tile(cubic->internal_image_tile);

    cvm_overlay_destroy_font(&theme->font_);

    free(theme->other_data);

    free(theme);
}

///make themes union/punned type, relying on base theme but extending as onther type of struct with extra needed data






