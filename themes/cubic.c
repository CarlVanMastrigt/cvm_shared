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


static void cubic_create_shape(cvm_overlay_element_render_buffer * erb,cvm_vk_image_atlas_tile ** tile,uint16_t ** selection_grid,uint32_t r)///change ri & r0 as 16ths of a pixel??
{
    uint32_t i,rt,threshold,x,y,xc,yc,xm,ym,t,rm;
    uint8_t * data;
    uint16_t * grid;

    if(r>40)
    {
        fprintf(stderr,"CUBIC OVERLAY ELEMENTS OF THIS SIZE NOT SUPPORTED\n");
        exit(-1);
    }

    *tile=overlay_create_transparent_image_tile_with_staging(erb,(void*)(&data),r*2,r*2);

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

bool cubic_box_select(rectangle_ r,uint32_t status,overlay_theme * theme)
{
    int i,y,x,y1,y2;
    cubic_theme_data * cubic;

    cubic=theme->other_data;

    if(!cubic->foreground_selection_grid)return false;

    r.x1+=cubic->foreground_offset_x;
    r.x2-=cubic->foreground_offset_x;
    r.y1+=cubic->foreground_offset_y;
    r.y2-=cubic->foreground_offset_y;

    if(!(status&WIDGET_H_FIRST))
    {
        x= -r.x1;
        y1=r.y1;
        y2=r.y2;
        r.x1+=cubic->foreground_r;

        if(x>=0 && x<cubic->foreground_r)
        {
            if(!(status&WIDGET_V_FIRST))
            {
                y= -y1;

                if(y>=0 && y<cubic->foreground_r)
                {
                    i=y*cubic->foreground_d+x;
                    if(cubic->foreground_selection_grid[i>>4] & (1<<(i&0x0F))) return true;
                }

                y1+=cubic->foreground_r;
            }

            if(!(status&WIDGET_V_LAST))
            {
                y= cubic->foreground_d-y2;

                if(y>=cubic->foreground_r && y<cubic->foreground_d)
                {
                    i=y*cubic->foreground_d+x;
                    if(cubic->foreground_selection_grid[i>>4] & (1<<(i&0x0F))) return true;
                }

                y2-=cubic->foreground_r;
            }

            return y1<=0 && y2>0;
        }
    }

    if(!(status&WIDGET_H_LAST))
    {
        x= cubic->foreground_d-r.x2;
        y1=r.y1;
        y2=r.y2;
        r.x2-=cubic->foreground_r;

        if(x>=cubic->foreground_r && x<cubic->foreground_d)
        {
            if(!(status&WIDGET_V_FIRST))
            {
                y= -y1;

                if(y>=0 && y<cubic->foreground_r)
                {
                    i=y*cubic->foreground_d+x;
                    if(cubic->foreground_selection_grid[i>>4] & (1<<(i&0x0F))) return true;
                }

                y1+=cubic->foreground_r;
            }

            if(!(status&WIDGET_V_LAST))
            {
                y= cubic->foreground_d-y2;

                if(y>=cubic->foreground_r && y<cubic->foreground_d)
                {
                    i=y*cubic->foreground_d+x;
                    if(cubic->foreground_selection_grid[i>>4] & (1<<(i&0x0F))) return true;
                }

                y2-=cubic->foreground_r;
            }

            return y1<=0 && y2>0;
        }
    }

    return rectangle_surrounds_origin_(r);
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

    cvm_overlay_element_render_buffer * erb=od;///HACK, temporary measure before switching types

    g=overlay_get_glyph(erb,&theme->font_,icon_glyph);

    if(!cubic->foreground_image_tile)cubic_create_shape(erb,&cubic->foreground_image_tile,&cubic->foreground_selection_grid,cubic->foreground_r);
    if(!cubic->foreground_image_tile || !g->tile)return;

    rg=r;

    r.y1=(r.y1+r.y2-cubic->foreground_d)>>1;
    r.y2=r.y1+cubic->foreground_d;
    o=(r.x2-r.x1-cubic->foreground_d)>>1;


    if(!(status&WIDGET_H_FIRST))
    {
        r.x1+= o;

        if(erb->count != erb->space)
        {
            rb=r;
            rb.x2=rb.x1+cubic->foreground_r;
            rr=get_rectangle_overlap_(rb,bounds);

            if((rectangle_has_positive_area(rr))) erb->buffer[erb->count++]=(cvm_overlay_render_data)
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

        if(erb->count != erb->space)
        {
            rb=r;
            rb.x1=rb.x2-cubic->foreground_r;
            rr=get_rectangle_overlap_(rb,bounds);

            if((rectangle_has_positive_area(rr))) erb->buffer[erb->count++]=(cvm_overlay_render_data)
            {
                {rr.x1,rr.y1,rr.x2,rr.y2},
                {(CVM_OVERLAY_ELEMENT_SHADED<<12)|(OVERLAY_MAIN_COLOUR_&0x0FFF),(cubic->foreground_image_tile->x_pos<<2)+rr.x1-rb.x1+cubic->foreground_r,(cubic->foreground_image_tile->y_pos<<2)+rr.y1-rb.y1,83},
            };
        }

        r.x2-=cubic->foreground_r;
    }

    if(erb->count != erb->space)
    {
        rr=get_rectangle_overlap_(r,bounds);

        if((rectangle_has_positive_area(rr))) erb->buffer[erb->count++]=(cvm_overlay_render_data)
        {
            {rr.x1,rr.y1,rr.x2,rr.y2},
            {(CVM_OVERLAY_ELEMENT_FILL<<12)|(OVERLAY_MAIN_COLOUR_&0x0FFF),0,0,83},
        };
    }

    rg.y1=(rg.y1+rg.y2 - g->pos.y2+g->pos.y1)>>1;
    rg.y2=rg.y1+g->pos.y2-g->pos.y1;
    rg.x1=(rg.x1+rg.x2 - g->pos.x2+g->pos.x1)>>1;
    rg.x2=rg.x1+g->pos.x2-g->pos.x1;

    if(erb->count != erb->space)
    {
        rr=get_rectangle_overlap_(rg,bounds);

        if((rectangle_has_positive_area(rr))) erb->buffer[erb->count++]=(cvm_overlay_render_data)
        {
            {rr.x1,rr.y1,rr.x2,rr.y2},
            {(CVM_OVERLAY_ELEMENT_SHADED<<12)|(OVERLAY_TEXT_COLOUR_0_&0x0FFF),(g->tile->x_pos<<2)+rr.x1-rg.x1,(g->tile->y_pos<<2)+rr.y1-rg.y1,83},
        };
    }
}

void cubic_h_bar_render(rectangle_ r,uint32_t status,overlay_theme * theme,overlay_data * od,rectangle_ bounds,overlay_colour_ colour)
{
    rectangle_ rb,rr;
    cubic_theme_data * cubic;

    cubic=theme->other_data;

    cvm_overlay_element_render_buffer * erb=od;///HACK, temporary measure before switching types

    if(!cubic->foreground_image_tile)cubic_create_shape(erb,&cubic->foreground_image_tile,&cubic->foreground_selection_grid,cubic->foreground_r);
    if(!cubic->foreground_image_tile)return;

    r.y1=(r.y1+r.y2-cubic->foreground_d)>>1;
    r.y2=r.y1+cubic->foreground_d;


    if(!(status&WIDGET_H_FIRST))
    {
        r.x1+= cubic->foreground_offset_x;

        if(erb->count != erb->space)
        {
            rb=r;
            rb.x2=rb.x1+cubic->foreground_r;
            rr=get_rectangle_overlap_(rb,bounds);

            if((rectangle_has_positive_area(rr))) erb->buffer[erb->count++]=(cvm_overlay_render_data)
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

        if(erb->count != erb->space)
        {
            rb=r;
            rb.x1=rb.x2-cubic->foreground_r;
            rr=get_rectangle_overlap_(rb,bounds);

            if((rectangle_has_positive_area(rr))) erb->buffer[erb->count++]=(cvm_overlay_render_data)
            {
                {rr.x1,rr.y1,rr.x2,rr.y2},
                {(CVM_OVERLAY_ELEMENT_SHADED<<12)|(colour&0x0FFF),(cubic->foreground_image_tile->x_pos<<2)+rr.x1-rb.x1+cubic->foreground_r,(cubic->foreground_image_tile->y_pos<<2)+rr.y1-rb.y1,83},
            };
        }

        r.x2-=cubic->foreground_r;
    }

    if(erb->count != erb->space)
    {
        rr=get_rectangle_overlap_(r,bounds);

        if((rectangle_has_positive_area(rr))) erb->buffer[erb->count++]=(cvm_overlay_render_data)
        {
            {rr.x1,rr.y1,rr.x2,rr.y2},
            {(CVM_OVERLAY_ELEMENT_FILL<<12)|(colour&0x0FFF),0,0,83},
        };
    }
}

void cubic_v_bar_render(rectangle_ r,uint32_t status,overlay_theme * theme,overlay_data * od,rectangle_ bounds,overlay_colour_ colour)
{
    rectangle_ rb,rr;
    cubic_theme_data * cubic;

    cubic=theme->other_data;

    cvm_overlay_element_render_buffer * erb=od;///HACK, temporary measure before switching types

    if(!cubic->foreground_image_tile)cubic_create_shape(erb,&cubic->foreground_image_tile,&cubic->foreground_selection_grid,cubic->foreground_r);
    if(!cubic->foreground_image_tile)return;

    r.y1=(r.y1+r.y2-cubic->foreground_d)>>1;
    r.y2=r.y1+cubic->foreground_d;


    if(!(status&WIDGET_H_FIRST))
    {
        r.x1+= cubic->foreground_offset_x;

        if(erb->count != erb->space)
        {
            rb=r;
            rb.x2=rb.x1+cubic->foreground_r;
            rr=get_rectangle_overlap_(rb,bounds);

            if((rectangle_has_positive_area(rr))) erb->buffer[erb->count++]=(cvm_overlay_render_data)
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

        if(erb->count != erb->space)
        {
            rb=r;
            rb.x1=rb.x2-cubic->foreground_r;
            rr=get_rectangle_overlap_(rb,bounds);

            if((rectangle_has_positive_area(rr))) erb->buffer[erb->count++]=(cvm_overlay_render_data)
            {
                {rr.x1,rr.y1,rr.x2,rr.y2},
                {(CVM_OVERLAY_ELEMENT_SHADED<<12)|(colour&0x0FFF),(cubic->foreground_image_tile->x_pos<<2)+rr.x1-rb.x1+cubic->foreground_r,(cubic->foreground_image_tile->y_pos<<2)+rr.y1-rb.y1,83},
            };
        }

        r.x2-=cubic->foreground_r;
    }

    if(erb->count != erb->space)
    {
        rr=get_rectangle_overlap_(r,bounds);

        if((rectangle_has_positive_area(rr))) erb->buffer[erb->count++]=(cvm_overlay_render_data)
        {
            {rr.x1,rr.y1,rr.x2,rr.y2},
            {(CVM_OVERLAY_ELEMENT_FILL<<12)|(colour&0x0FFF),0,0,83},
        };
    }
}

void cubic_h_bar_slider_render(rectangle_ r,uint32_t status,overlay_theme * theme,overlay_data * od,rectangle_ bounds,overlay_colour_ colour,int range,int value,int bar)
{
    rectangle_ rb,rr;
    cubic_theme_data * cubic;

    cubic=theme->other_data;

    cvm_overlay_element_render_buffer * erb=od;///HACK, temporary measure before switching types

    //cubic_h_bar_render(r,status,theme,od,bounds,colour);

    if(!cubic->internal_image_tile)cubic_create_shape(erb,&cubic->internal_image_tile,NULL,cubic->internal_r);
    if(!cubic->internal_image_tile)return;


    if(!(status&WIDGET_H_FIRST))
    {
        r.x1+= cubic->foreground_offset_x;
    }

    if(!(status&WIDGET_H_LAST))
    {
        r.x2-= cubic->foreground_offset_x;
    }

    r.y1=(r.y1+r.y2-cubic->internal_d)>>1;
    r.y2=r.y1+cubic->internal_d;

    r.x1+=cubic->foreground_r;
    r.x2-=cubic->foreground_r;

    if(bar<0)///works like a flag, perhaps better to put in reserved status bit?
    {
        bar=(r.x2-r.x1)/ -bar;
        r.x1+=((r.x2-r.x1-bar)*value)/range;
        r.x2=r.x1+bar;
    }
    else
    {
        r.x1+=((r.x2-r.x1)*value)/(range+bar);
        r.x2-=((r.x2-r.x1)*(range-value))/(range+bar);
    }

    if(erb->count != erb->space)
    {
        rb=r;
        rb.x2=rb.x1;
        rb.x1-=cubic->internal_r;
        rr=get_rectangle_overlap_(rb,bounds);

        if((rectangle_has_positive_area(rr))) erb->buffer[erb->count++]=(cvm_overlay_render_data)
        {
            {rr.x1,rr.y1,rr.x2,rr.y2},
            {(CVM_OVERLAY_ELEMENT_SHADED<<12)|(colour&0x0FFF),(cubic->internal_image_tile->x_pos<<2)+rr.x1-rb.x1,(cubic->internal_image_tile->y_pos<<2)+rr.y1-rb.y1,83},
        };
    }

    if(erb->count != erb->space)
    {
        rb=r;
        rb.x1=rb.x2;
        rb.x2+=cubic->internal_r;
        rr=get_rectangle_overlap_(rb,bounds);

        if((rectangle_has_positive_area(rr))) erb->buffer[erb->count++]=(cvm_overlay_render_data)
        {
            {rr.x1,rr.y1,rr.x2,rr.y2},
            {(CVM_OVERLAY_ELEMENT_SHADED<<12)|(colour&0x0FFF),(cubic->internal_image_tile->x_pos<<2)+rr.x1-rb.x1+cubic->internal_r,(cubic->internal_image_tile->y_pos<<2)+rr.y1-rb.y1,83},
        };
    }

    if(erb->count != erb->space)
    {
        rr=get_rectangle_overlap_(r,bounds);

        if((rectangle_has_positive_area(rr))) erb->buffer[erb->count++]=(cvm_overlay_render_data)
        {
            {rr.x1,rr.y1,rr.x2,rr.y2},
            {(CVM_OVERLAY_ELEMENT_FILL<<12)|(colour&0x0FFF),0,0,83},
        };
    }
}

void cubic_box_render(rectangle_ r,uint32_t status,overlay_theme * theme,overlay_data * od,rectangle_ bounds,overlay_colour_ colour)
{
    rectangle_ rv,re,rr;
    cubic_theme_data * cubic;

    cubic=theme->other_data;

    cvm_overlay_element_render_buffer * erb=od;///HACK, temporary measure before switching types

    if(!cubic->foreground_image_tile)cubic_create_shape(erb,&cubic->foreground_image_tile,&cubic->foreground_selection_grid,cubic->foreground_r);
    if(!cubic->foreground_image_tile)return;

    r.x1+=cubic->foreground_offset_x;
    r.x2-=cubic->foreground_offset_x;
    r.y1+=cubic->foreground_offset_y;
    r.y2-=cubic->foreground_offset_y;

    if(!(status&WIDGET_H_FIRST))
    {
        rv=r;
        rv.x2=r.x1+=cubic->foreground_r;

        if(!(status&WIDGET_V_FIRST))
        {
            if(erb->count != erb->space)
            {
                re=rv;
                re.y2=re.y1+cubic->foreground_r;
                rr=get_rectangle_overlap_(re,bounds);

                if((rectangle_has_positive_area(rr))) erb->buffer[erb->count++]=(cvm_overlay_render_data)
                {
                    {rr.x1,rr.y1,rr.x2,rr.y2},
                    {(CVM_OVERLAY_ELEMENT_SHADED<<12)|(colour&0x0FFF),(cubic->foreground_image_tile->x_pos<<2)+rr.x1-re.x1,(cubic->foreground_image_tile->y_pos<<2)+rr.y1-re.y1,83},
                };
            }

            rv.y1+=cubic->foreground_r;
        }

        if(!(status&WIDGET_V_LAST))
        {
            if(erb->count != erb->space)
            {
                re=rv;
                re.y1=re.y2-cubic->foreground_r;
                rr=get_rectangle_overlap_(re,bounds);

                if((rectangle_has_positive_area(rr))) erb->buffer[erb->count++]=(cvm_overlay_render_data)
                {
                    {rr.x1,rr.y1,rr.x2,rr.y2},
                    {(CVM_OVERLAY_ELEMENT_SHADED<<12)|(colour&0x0FFF),(cubic->foreground_image_tile->x_pos<<2)+rr.x1-re.x1,(cubic->foreground_image_tile->y_pos<<2)+rr.y1-re.y1+cubic->foreground_r,83},
                };
            }

            rv.y2-=cubic->foreground_r;
        }

        if(erb->count != erb->space)///need to check again as can return null once again;
        {
            rr=get_rectangle_overlap_(rv,bounds);

            if((rectangle_has_positive_area(rr))) erb->buffer[erb->count++]=(cvm_overlay_render_data)
            {
                {rr.x1,rr.y1,rr.x2,rr.y2},
                {(CVM_OVERLAY_ELEMENT_FILL<<12)|(colour&0x0FFF),0,0,83},
            };
        }
    }

    if(!(status&WIDGET_H_LAST))
    {
        rv=r;
        rv.x1=r.x2-=cubic->foreground_r;

        if(!(status&WIDGET_V_FIRST))
        {
            if(erb->count != erb->space)
            {
                re=rv;
                re.y2=re.y1+cubic->foreground_r;
                rr=get_rectangle_overlap_(re,bounds);

                if((rectangle_has_positive_area(rr))) erb->buffer[erb->count++]=(cvm_overlay_render_data)
                {
                    {rr.x1,rr.y1,rr.x2,rr.y2},
                    {(CVM_OVERLAY_ELEMENT_SHADED<<12)|(colour&0x0FFF),(cubic->foreground_image_tile->x_pos<<2)+rr.x1-re.x1+cubic->foreground_r,(cubic->foreground_image_tile->y_pos<<2)+rr.y1-re.y1,83},
                };
            }

            rv.y1+=cubic->foreground_r;
        }

        if(!(status&WIDGET_V_LAST))
        {
            if(erb->count != erb->space)
            {
                re=rv;
                re.y1=re.y2-cubic->foreground_r;
                rr=get_rectangle_overlap_(re,bounds);

                if((rectangle_has_positive_area(rr))) erb->buffer[erb->count++]=(cvm_overlay_render_data)
                {
                    {rr.x1,rr.y1,rr.x2,rr.y2},
                    {(CVM_OVERLAY_ELEMENT_SHADED<<12)|(colour&0x0FFF),(cubic->foreground_image_tile->x_pos<<2)+rr.x1-re.x1+cubic->foreground_r,(cubic->foreground_image_tile->y_pos<<2)+rr.y1-re.y1+cubic->foreground_r,83},
                };
            }

            rv.y2-=cubic->foreground_r;
        }

        if(erb->count != erb->space)///need to check again as can return null once again;
        {
            rr=get_rectangle_overlap_(rv,bounds);

            if((rectangle_has_positive_area(rr))) erb->buffer[erb->count++]=(cvm_overlay_render_data)
            {
                {rr.x1,rr.y1,rr.x2,rr.y2},
                {(CVM_OVERLAY_ELEMENT_FILL<<12)|(colour&0x0FFF),0,0,83},
            };
        }
    }

    if(erb->count != erb->space)///need to check again as can return null once again;
    {
        rr=get_rectangle_overlap_(r,bounds);

        if((rectangle_has_positive_area(rr))) erb->buffer[erb->count++]=(cvm_overlay_render_data)
        {
            {rr.x1,rr.y1,rr.x2,rr.y2},
            {(CVM_OVERLAY_ELEMENT_FILL<<12)|(colour&0x0FFF),0,0,83},
        };
    }
}

void cubic_panel_render(rectangle_ r,uint32_t status,overlay_theme * theme,overlay_data * od,rectangle_ bounds,overlay_colour_ colour)
{
    rectangle_ rv,re,rr;
    cubic_theme_data * cubic;

    cubic=theme->other_data;

    cvm_overlay_element_render_buffer * erb=od;///HACK, temporary measure before switching types

    if(!cubic->background_image_tile)cubic_create_shape(erb,&cubic->background_image_tile,&cubic->background_selection_grid,cubic->background_r);
    if(!cubic->background_image_tile)return;

    if(!(status&WIDGET_H_FIRST))
    {
        rv=r;
        rv.x2=r.x1+=cubic->background_r;

        if(!(status&WIDGET_V_FIRST))
        {
            if(erb->count != erb->space)
            {
                re=rv;
                re.y2=re.y1+cubic->background_r;
                rr=get_rectangle_overlap_(re,bounds);

                if((rectangle_has_positive_area(rr))) erb->buffer[erb->count++]=(cvm_overlay_render_data)
                {
                    {rr.x1,rr.y1,rr.x2,rr.y2},
                    {(CVM_OVERLAY_ELEMENT_SHADED<<12)|(colour&0x0FFF),(cubic->background_image_tile->x_pos<<2)+rr.x1-re.x1,(cubic->background_image_tile->y_pos<<2)+rr.y1-re.y1,83},
                };
            }

            rv.y1+=cubic->background_r;
        }

        if(!(status&WIDGET_V_LAST))
        {
            if(erb->count != erb->space)
            {
                re=rv;
                re.y1=re.y2-cubic->background_r;
                rr=get_rectangle_overlap_(re,bounds);

                if((rectangle_has_positive_area(rr))) erb->buffer[erb->count++]=(cvm_overlay_render_data)
                {
                    {rr.x1,rr.y1,rr.x2,rr.y2},
                    {(CVM_OVERLAY_ELEMENT_SHADED<<12)|(colour&0x0FFF),(cubic->background_image_tile->x_pos<<2)+rr.x1-re.x1,(cubic->background_image_tile->y_pos<<2)+rr.y1-re.y1+cubic->background_r,83},
                };
            }

            rv.y2-=cubic->background_r;
        }

        if(erb->count != erb->space)///need to check again as can return null once again;
        {
            rr=get_rectangle_overlap_(rv,bounds);

            if((rectangle_has_positive_area(rr))) erb->buffer[erb->count++]=(cvm_overlay_render_data)
            {
                {rr.x1,rr.y1,rr.x2,rr.y2},
                {(CVM_OVERLAY_ELEMENT_FILL<<12)|(colour&0x0FFF),0,0,83},
            };
        }
    }

    if(!(status&WIDGET_H_LAST))
    {
        rv=r;
        rv.x1=r.x2-=cubic->background_r;

        if(!(status&WIDGET_V_FIRST))
        {
            if(erb->count != erb->space)
            {
                re=rv;
                re.y2=re.y1+cubic->background_r;
                rr=get_rectangle_overlap_(re,bounds);

                if((rectangle_has_positive_area(rr))) erb->buffer[erb->count++]=(cvm_overlay_render_data)
                {
                    {rr.x1,rr.y1,rr.x2,rr.y2},
                    {(CVM_OVERLAY_ELEMENT_SHADED<<12)|(colour&0x0FFF),(cubic->background_image_tile->x_pos<<2)+rr.x1-re.x1+cubic->background_r,(cubic->background_image_tile->y_pos<<2)+rr.y1-re.y1,83},
                };
            }

            rv.y1+=cubic->background_r;
        }

        if(!(status&WIDGET_V_LAST))
        {
            if(erb->count != erb->space)
            {
                re=rv;
                re.y1=re.y2-cubic->background_r;
                rr=get_rectangle_overlap_(re,bounds);

                if((rectangle_has_positive_area(rr))) erb->buffer[erb->count++]=(cvm_overlay_render_data)
                {
                    {rr.x1,rr.y1,rr.x2,rr.y2},
                    {(CVM_OVERLAY_ELEMENT_SHADED<<12)|(colour&0x0FFF),(cubic->background_image_tile->x_pos<<2)+rr.x1-re.x1+cubic->background_r,(cubic->background_image_tile->y_pos<<2)+rr.y1-re.y1+cubic->background_r,83},
                };
            }

            rv.y2-=cubic->background_r;
        }

        if(erb->count != erb->space)///need to check again as can return null once again;
        {
            rr=get_rectangle_overlap_(rv,bounds);

            if((rectangle_has_positive_area(rr))) erb->buffer[erb->count++]=(cvm_overlay_render_data)
            {
                {rr.x1,rr.y1,rr.x2,rr.y2},
                {(CVM_OVERLAY_ELEMENT_FILL<<12)|(colour&0x0FFF),0,0,83},
            };
        }
    }

    if(erb->count != erb->space)///need to check again as can return null once again;
    {
        rr=get_rectangle_overlap_(r,bounds);

        if((rectangle_has_positive_area(rr))) erb->buffer[erb->count++]=(cvm_overlay_render_data)
        {
            {rr.x1,rr.y1,rr.x2,rr.y2},
            {(CVM_OVERLAY_ELEMENT_FILL<<12)|(colour&0x0FFF),0,0,83},
        };
    }
}

overlay_theme * create_cubic_theme(void)
{
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

    theme->h_bar_text_offset=16;

    theme->h_slider_bar_lost_w=24;

    theme->x_box_offset=16;/// ??? figure out what this should be (text and
    theme->y_box_offset=7;/// ??? figure out what this should be

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
    theme->h_bar_render=cubic_h_bar_render;
    theme->h_bar_slider_render=cubic_h_bar_slider_render;
    theme->box_render=cubic_box_render;
    theme->panel_render=cubic_panel_render;


    theme->square_select=cubic_square_select;
    theme->h_bar_select=cubic_h_bar_select;
    theme->box_select=cubic_box_select;
    theme->panel_select=cubic_panel_select;

    cubic->foreground_image_tile=NULL;
    cubic->foreground_selection_grid=NULL;
    cubic->foreground_offset_x=2;
    cubic->foreground_offset_y=1;
    cubic->foreground_r=10;
    cubic->foreground_d=20;

    cubic->background_image_tile=NULL;
    cubic->background_selection_grid=NULL;
    cubic->background_r=14;
    cubic->background_d=28;

    cubic->internal_image_tile=NULL;
    cubic->internal_r=8;
    cubic->internal_d=16;

    cvm_overlay_create_font(&theme->font_,"cvm_shared/resources/cvm_font_1.ttf",16);

/// ↑←↓→ 🗙 🔄 💾 📁 📷 📄 🎵 ➕ ➖ 👻 🖋 🔓 🔗 🖼 ✂ 👁 ⚙
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




