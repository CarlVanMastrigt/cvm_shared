/**
Copyright 2021 Carl van Mastrigt

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

static FT_Library overlay_freetype_library;

void cvm_overlay_open_freetype(void)
{
    if(FT_Init_FreeType(&overlay_freetype_library))
    {
        fprintf(stderr,"COULD NOT INITIALISE FREETYPE LIBRARY\n");
        exit(-1);
    }
}

void cvm_overlay_close_freetype(void)
{
    if(FT_Done_FreeType(overlay_freetype_library))
    {
        fprintf(stderr,"COULD NOT DESTROY FREETYPE LIBRARY\n");
        exit(-1);
    }
}

void cvm_overlay_create_font(cvm_overlay_font * font,char * filename,int pixel_size)
{
    if(FT_New_Face(overlay_freetype_library,filename,0,&font->face))
    {
        fprintf(stderr,"COULD NOT LOAD FONT FACE FILE %s\n",filename);
        exit(-1);
    }

    if(FT_Set_Pixel_Sizes(font->face,0,pixel_size))
    {
        fprintf(stderr,"COULD NOT SET FONT FACE SIZE %s %d\n",filename,pixel_size);
        exit(-1);
    }

    font->glyph_size=pixel_size;

    font->space_character_index=FT_Get_Char_Index(font->face,' ');

    if(FT_Load_Glyph(font->face,font->space_character_index,0))
    {
        fprintf(stderr,"FONT DOES NOT CONTAIN SPACE CHARACTER %s\n",filename);
        exit(-1);
    }

    font->space_advance=font->face->glyph->advance.x>>6;
    font->max_advance= font->face->size->metrics.max_advance>>6;
    font->line_spacing=font->face->size->metrics.height>>6;

    font->glyphs=malloc(4*sizeof(cvm_overlay_glyph));
    font->glyph_space=4;
    font->glyph_count=0;
}

void cvm_overlay_destroy_font(cvm_overlay_font * font)
{
    uint32_t i;
    if(FT_Done_Face(font->face))
    {
        fprintf(stderr,"COULD NOT DESTROY FONT FACE\n");
        exit(-1);
    }

    for(i=0;i<font->glyph_count;i++)
    {
        overlay_destroy_transparent_image_tile(font->glyphs[i].tile);
    }

    free(font->glyphs);
}





static inline uint32_t cvm_overlay_get_utf8_glyph_index(FT_Face face,uint8_t * text,uint32_t * incr)
{
    uint32_t cp;
    *incr=1;
    if(*text & 0x80)
    {
        cp=0;
        while(*text & (0x80 >> *incr))
        {
            ///error check, dont put in release version
            if((text[*incr]&0xC0) != 0x80)
            {
                fprintf(stderr,"ATTEMPTING TO RENDER AN INVALID UTF-8 STRING (TOP BIT MISMATCH)\n");
                exit(-1);
            }
            cp<<=6;
            cp|=text[*incr]&0x3F;
            (*incr)++;
        }
        cp|=(*text&((1<<(7-*incr))-1))<<(6u* *incr-6u);
        ///error check, dont put in release version
        if(*incr==1)
        {
            fprintf(stderr,"ATTEMPTING TO RENDER AN INVALID UTF-8 STRING (INVALID LENGTH SPECIFIED)\n");
            exit(-1);
        }

        /// check for variation sequence
        if(text[*incr]==0xEF && text[*incr+1]==0xB8 && (text[*incr+2]&0xF0)==0x80)
        {
            ///possibly convert colour to monochrome? alternatively could support colour emoji by allowing writing to/ use of colour image atlas
            *incr+=3;
            return FT_Face_GetCharVariantIndex(face,cp,0xFE00 | (text[*incr-1]&0x0F));
        }
        else return FT_Get_Char_Index(face,cp);
    }
    else return FT_Get_Char_Index(face,*text);
}

bool cvm_overlay_utf8_validate_string(char * text)
{
    int i;
    while(*text)
    {
        if(*text & 0x80)
        {
            for(i=1;*text & (0x80 >> i);i++)
            {
                if(i>4 || (text[i]&0xC0) != 0x80) return false;
            }
            text+=i;
        }
        else text++;
    }
    return true;
}

int cvm_overlay_utf8_count_glyphs(char * text)
{
    int i,c;
    c=0;
    while(*text)
    {
        if(*text & 0x80)
        {
            for(i=1;*text & (0x80 >> i);i++);
            text+=i;

            //text+=3*(text[0]==0xEF && text[1]==0xB8 && (text[2]&0xF0)==0x80);///variation sequences dont count as a glyph
        }
        else text++;

        c++;
    }
    return c;
}

int cvm_overlay_utf8_count_glyphs_outside_range(char * text,int begin,int end)
{
    int i,c,o;
    o=0;
    c=0;
    while(text[o])
    {
        if(text[o] & 0x80)
        {
            for(i=1;text[o] & (0x80 >> i);i++);

            //i+=3*(text[o]==0xEF && text[o+1]==0xB8 && (text[o+2]&0xF0)==0x80);///variation sequences dont count as a glyph
        }
        else i=1;

        c+= o<begin || o>=end;

        o+=i;
    }
    return c;
}

bool cvm_overlay_utf8_validate_string_and_count_glyphs(char * text,int * c)
{
    int i;
    *c=0;
    while(*text)
    {
        if(*text & 0x80)
        {
            for(i=1;*text & (0x80 >> i);i++)
            {
                if(i>4 || (text[i]&0xC0) != 0x80) return false;
            }
            text+=i;

            //text+=3*(text[0]==0xEF && text[1]==0xB8 && (text[2]&0xF0)==0x80);///variation sequences dont count as a glyph
        }
        else text++;

        (*c)++;
    }
    return true;
}

int cvm_overlay_utf8_get_previous_glyph(char * text,int offset)
{
    //offset-= 3* (offset>2 && text[offset-3]==0xEF && text[offset-2]==0xB8 && (text[offset-1]&0xF0)==0x80);///skip over variation sequence

    if(offset==0)return 0;

    int i,o=offset;///error checking, exclude from final build

    do offset--;
    while(offset && (text[offset]&0xC0) == 0x80);

    if(text[offset] & 0x80)///error checking, exclude from final build
    {
        o-=offset;
        for(i=1;i<o;i++)///0 implicitly checked above
        {
            if(!(((uint8_t*)text)[offset]<<i & 0x80))
            {
                fprintf(stderr,"GET PREVIOUS DETECTED INVALID UTF-8 CHAR IN STRING (INVALID LENGTH SPECIFIED)\n");
                exit(-1);
            }
        }
    }
    else if(text[offset] & 0x80)
    {
        fprintf(stderr,"GET PREVIOUS DETECTED INVALID UTF-8 CHAR IN STRING (LEAD CHARACTER NOT FOUND)\n");
        exit(-1);
    }

    return offset;
}

int cvm_overlay_utf8_get_next_glyph(char * text,int offset)
{
    if(text[offset]=='\0')return offset;

    if(text[offset] & 0x80)
    {
        int i;
        for(i=1;text[offset] & (0x80 >> i);i++)
        {
            ///error check, dont put in release version
            if((text[offset+i]&0xC0) != 0x80)
            {
                fprintf(stderr,"GET NEXT DETECTED INVALID UTF-8 CHAR IN STRING (TOP BIT MISMATCH)\n");
                exit(-1);
            }
        }
        ///error check, dont put in release version
        if(i==1)
        {
            fprintf(stderr,"GET NEXT DETECTED INVALID UTF-8 CHAR IN STRING (INVALID LENGTH SPECIFIED)\n");
            exit(-1);
        }

        offset+=i;

        //offset+=3*(text[offset]==0xEF && text[offset+1]==0xB8 && (text[offset+2]&0xF0)==0x80);///skip over variation sequence
    }
    else offset++;

    return offset;
}

int cvm_overlay_utf8_get_previous_word(char * text,int offset)
{
    /// could/should also match other whitespace characters?
    while(offset && text[offset-1]==' ')offset--;
    while(offset && text[offset-1]!=' ')offset--;
    return offset;
}

int cvm_overlay_utf8_get_next_word(char * text,int offset)
{
    /// could/should also match other whitespace characters?
    while(text[offset] && text[offset]==' ')offset++;
    while(text[offset] && text[offset]!=' ')offset++;
    return offset;
}

static inline cvm_overlay_glyph * cvm_overlay_find_glpyh(cvm_overlay_font * font,uint32_t gi)
{
    uint32_t s,e,i;

    s=0;
    e=font->glyph_count;
    while(1)
    {
        i=(s+e)>>1;
        if(i!=e && font->glyphs[i].glyph_index==gi) break;
        if(e-s<2)
        {
            ///does not exist, insert at i | i+1
            i+= i<e && font->glyphs[i].glyph_index<gi;

            if(font->glyph_count==font->glyph_space)font->glyphs=realloc(font->glyphs,sizeof(cvm_overlay_glyph)*(font->glyph_space*=2));

            memmove(font->glyphs+i+1,font->glyphs+i,(font->glyph_count-i)*sizeof(cvm_overlay_glyph));

            font->glyphs[i]=(cvm_overlay_glyph)
            {
                .tile=NULL,
                .glyph_index=gi,
                .usage_counter=0,
                .pos={0,0,0,0},
                .advance=0
            };

            font->glyph_count++;

            break;
        }
        else if(font->glyphs[i].glyph_index<gi) s=i+1;///already checked s, quicker convergence
        else e=i;///e is excluded from search
    }

    return font->glyphs+i;
}

static inline void cvm_overlay_prepare_glyph_render_data(cvm_overlay_element_render_buffer * erb,cvm_overlay_font * font,cvm_overlay_glyph * g)
{
    uint32_t w,h;
    FT_GlyphSlot gs;
    void * staging;

    if(g->tile==NULL)///if render doesnt already exist, create it
    {
        if(!FT_Load_Glyph(font->face,g->glyph_index,FT_LOAD_RENDER))
        {
            gs=font->face->glyph;
            w=gs->bitmap.width;
            h=gs->bitmap.rows;

            g->tile=overlay_create_transparent_image_tile_with_staging(erb,&staging,w,h);

            if(g->tile)
            {
                memcpy(staging,gs->bitmap.buffer,sizeof(unsigned char)*w*h);

                g->pos.x2=w+(g->pos.x1=gs->bitmap_left);
                g->pos.y2=h+(g->pos.y1=(font->face->size->metrics.ascender>>6) - gs->bitmap_top);
            }

            if(g->advance && g->advance!= gs->advance.x>>6)
            {
                fprintf(stderr,"ATTEMPTING TO RENDER AN INVALID UTF-8 STRING (TOP BIT MISMATCH)\n");
                exit(-1);
            }

            g->advance=gs->advance.x>>6;///probably NOT needed as never render before calculating widget size and calculating size requires knowing text size (may be edge cases though so leave this in
        }
    }
}

static inline int cvm_overlay_get_glyph_advance(cvm_overlay_font * font,cvm_overlay_glyph * g)
{
    FT_Fixed advance;

    if(!g->advance)
    {
        if(!FT_Get_Advance(font->face,g->glyph_index,0,&advance))g->advance=advance>>16;
        else
        {
            fprintf(stderr,"UNABLE TO GET GLYPH ADVANCE FOR CHARACTER POINT%u\n",g->glyph_index);
            exit(-1);
        }
        if(!g->advance)g->advance=1;///ensure nonzero advance if advance is actually zero
    }

    return g->advance;
}



int overlay_size_text_simple(cvm_overlay_font * font,char * text)
{
    uint32_t gi,prev_gi,incr;
    FT_Vector kern;
    cvm_overlay_glyph * g;
    int w;

    prev_gi=0;
    w=0;

    while(*text)
    {
        if(*text==' ')
        {
            w+=font->space_advance;
            prev_gi=font->space_character_index;
            text++;
            continue;
        }

        gi=cvm_overlay_get_utf8_glyph_index(font->face,(uint8_t*)text,&incr);

        g=cvm_overlay_find_glpyh(font,gi);

        if(prev_gi && !FT_Get_Kerning(font->face,prev_gi,gi,0,&kern)) w+=kern.x>>6;

        prev_gi=gi;

        w+=cvm_overlay_get_glyph_advance(font,g);

        text+=incr;
    }

    return w;
}

void overlay_render_text_simple(cvm_overlay_element_render_buffer * erb,cvm_overlay_font * font,char * text,int x,int y,rectangle bounds,overlay_colour_ colour)
{
    uint32_t gi,prev_gi,incr;
    FT_Vector kern;
    cvm_overlay_glyph * g;
    rectangle rb,rr;

    prev_gi=0;

    if(rectangle_has_positive_area(bounds))while(*text)
    {
        if(*text==' ')
        {
            x+=font->space_advance;
            prev_gi=font->space_character_index;
            text++;
            continue;
        }

        gi=cvm_overlay_get_utf8_glyph_index(font->face,(uint8_t*)text,&incr);

        g=cvm_overlay_find_glpyh(font,gi);

        cvm_overlay_prepare_glyph_render_data(erb,font,g);

        if(prev_gi && !FT_Get_Kerning(font->face,prev_gi,gi,0,&kern)) x+=kern.x>>6;

        prev_gi=gi;

        if(g->tile && erb->count != erb->space)///need to check again as can return null once again;
        {
            rb=rectangle_add_offset(g->pos,x,y);
            rr=get_rectangle_overlap(rb,bounds);

            if((rectangle_has_positive_area(rr))) erb->buffer[erb->count++]=(cvm_overlay_render_data)
            {
                {rr.x1,rr.y1,rr.x2,rr.y2},
                {(CVM_OVERLAY_ELEMENT_SHADED<<12)|(colour&0x0FFF),(g->tile->x_pos<<2)+rr.x1-rb.x1,(g->tile->y_pos<<2)+rr.y1-rb.y1,83},
            };
        }

        x+=cvm_overlay_get_glyph_advance(font,g);

        text+=incr;
    }
}

void overlay_render_text_selection_simple(cvm_overlay_element_render_buffer * erb,cvm_overlay_font * font,char * text,int x,int y,rectangle bounds,overlay_colour_ colour,char * selection_start,char * selection_end)
{
    uint32_t gi,prev_gi,incr;
    FT_Vector kern;
    cvm_overlay_glyph * g;
    rectangle rb,rr,rs;

    prev_gi=0;
    rs.y1=y;
    rs.y2=y+font->glyph_size;
    //rs.x1=rs.x2=x;

    while(*text)
    {
        if(text==selection_start) rs.x1=x;
        if(text==selection_end) rs.x2=x;

        if(*text==' ')
        {
            x+=font->space_advance;
            prev_gi=font->space_character_index;
            text++;
            continue;
        }

        gi=cvm_overlay_get_utf8_glyph_index(font->face,(uint8_t*)text,&incr);

        g=cvm_overlay_find_glpyh(font,gi);

        cvm_overlay_prepare_glyph_render_data(erb,font,g);

        if(prev_gi && !FT_Get_Kerning(font->face,prev_gi,gi,0,&kern))
        {
            x+=kern.x>>6;
            if(kern.x)
            {
                if(text==selection_start) rs.x1=x;
                if(text==selection_end) rs.x2=x;
            }
        }

        prev_gi=gi;

        if(g->tile && erb->count != erb->space)///need to check again as can return null once again;
        {
            rb=rectangle_add_offset(g->pos,x,y);
            rr=get_rectangle_overlap(rb,bounds);

            if((rectangle_has_positive_area(rr))) erb->buffer[erb->count++]=(cvm_overlay_render_data)
            {
                {rr.x1,rr.y1,rr.x2,rr.y2},
                {(CVM_OVERLAY_ELEMENT_SHADED<<12)|(colour&0x0FFF),(g->tile->x_pos<<2)+rr.x1-rb.x1,(g->tile->y_pos<<2)+rr.y1-rb.y1,83},
            };
        }

        x+=cvm_overlay_get_glyph_advance(font,g);

        text+=incr;
    }

    if(text==selection_start) rs.x1=x;
    if(text==selection_end) rs.x2=x;
    rs.x2+=selection_end==selection_start;

    if(erb->count != erb->space)
    {
        rs=get_rectangle_overlap(rs,bounds);

        if((rectangle_has_positive_area(rs))) erb->buffer[erb->count++]=(cvm_overlay_render_data)
        {
            {rs.x1,rs.y1,rs.x2,rs.y2},
            {(CVM_OVERLAY_ELEMENT_FILL<<12)|(OVERLAY_TEXT_HIGHLIGHT_COLOUR_&0x0FFF),0,0,83},
        };
    }
}

int overlay_text_find_offset_simple(cvm_overlay_font * font,char * text,int relative_x)
{
    uint32_t gi,prev_gi,incr;
    FT_Vector kern;
    cvm_overlay_glyph * g;
    char * text_start;
    int x,a;

    prev_gi=0;
    x=0;

    text_start=text;

    while(*text)
    {
        if(*text==' ')
        {
            if(x+font->space_advance/2 >= relative_x)break;
            x+=font->space_advance;
            prev_gi=font->space_character_index;
            text++;
            continue;
        }

        gi=cvm_overlay_get_utf8_glyph_index(font->face,(uint8_t*)text,&incr);

        g=cvm_overlay_find_glpyh(font,gi);

        if(prev_gi && !FT_Get_Kerning(font->face,prev_gi,gi,0,&kern)) x+=kern.x>>6;

        prev_gi=gi;

        a=cvm_overlay_get_glyph_advance(font,g);

        if(x+a/2 >= relative_x)break;

        x+=a;

        text+=incr;
    }

    return text-text_start;
}

cvm_overlay_glyph * overlay_get_glyph(cvm_overlay_element_render_buffer * erb,cvm_overlay_font * font,char * text)
{
    uint32_t gi,incr;
    cvm_overlay_glyph * g;

    g=NULL;

    if(*text && *text!=' ')
    {
        gi=cvm_overlay_get_utf8_glyph_index(font->face,(uint8_t*)text,&incr);

        g=cvm_overlay_find_glpyh(font,gi);

        cvm_overlay_prepare_glyph_render_data(erb,font,g);

        if(text[incr])
        {
            fprintf(stderr,"TRYING TO GET SINGLE GLYPH FOR STRING WITH MORE THAN ONE: %s\n",text);
        }
    }

    return g;
}

void overlay_process_multiline_text(cvm_overlay_font * font,cvm_overlay_text_block * block,char * text,int wrapping_width)
{
    uint32_t gi,prev_gi,incr;
    int w;
    char * word_start;
    bool line_start=true;
    FT_Vector kern;

    cvm_overlay_glyph * g;

    w=0;

    ///used for wrapping
    word_start=text;
    prev_gi=0;

    block->line_count=0;
    block->lines[block->line_count].start=text;

    while(*text)
    {
        if(*text==' ')
        {
            w+=font->space_advance;
            prev_gi=font->space_character_index;
            word_start= ++text;
            line_start=false;
            continue;
        }
        else if(*text=='\n')
        {
            if(block->line_count==block->line_space)block->lines=realloc(block->lines,sizeof(cvm_overlay_text_line)*(block->line_space*=2));
            block->lines[block->line_count++].finish=text;///non-inclusive_end
            block->lines[block->line_count].start=text+1;///dont include newline in next line

            w=0;
            prev_gi=0;
            word_start= ++text;
            line_start=true;
            continue;
        }

        gi=cvm_overlay_get_utf8_glyph_index(font->face,(uint8_t*)text,&incr);

        g=cvm_overlay_find_glpyh(font,gi);

        if(prev_gi && !FT_Get_Kerning(font->face,prev_gi,gi,0,&kern)) w+=kern.x>>6;

        prev_gi=gi;

        w+=cvm_overlay_get_glyph_advance(font,g);

        if(wrapping_width && w > wrapping_width)
        {
            if(line_start)
            {
                if(text==word_start) text+=incr;///force render if character takes up more than whole line

                if(block->line_count==block->line_space)block->lines=realloc(block->lines,sizeof(cvm_overlay_text_line)*(block->line_space*=2));
                block->lines[block->line_count++].finish=text;///non-inclusive_end
                block->lines[block->line_count].start=text;///dont include newline in next line

                w=0;
                word_start=text;
                prev_gi=0;
                line_start=true;/// need to "while" out any spaces here, or come up with smarter solution to wrapping on space...
            }
            else ///restart rendering this word on a new line, is a little awkward with overwriting elements in gpu memory though...
            {
                if(block->line_count==block->line_space)block->lines=realloc(block->lines,sizeof(cvm_overlay_text_line)*(block->line_space*=2));
                block->lines[block->line_count++].finish=word_start;///non-inclusive_end
                block->lines[block->line_count].start=word_start;///dont include newline in next line

                w=0;
                text=word_start;
                prev_gi=0;
                line_start=true;
            }
        }
        else
        {
            text+=incr;
        }
    }

    block->lines[block->line_count++].finish=text;
}

void overlay_render_multiline_text(cvm_overlay_element_render_buffer * erb,cvm_overlay_font * font,cvm_overlay_text_block * block,int x,int y,rectangle bounds,overlay_colour_ colour)
{
    uint32_t i,gi,prev_gi,incr;
    FT_Vector kern;
    cvm_overlay_glyph * g;
    rectangle rb,rr;
    char *text,*text_end;
    int x_start,first_line;

    prev_gi=0;
    x_start=x;

    first_line=(bounds.y1-y)/font->line_spacing;
    first_line*=first_line>0;
    y+=first_line*font->line_spacing;

    if(rectangle_has_positive_area(bounds))for(i=first_line;i<block->line_count && y<bounds.y2;i++)
    {
        text_end=block->lines[i].finish;

        for(text=block->lines[i].start;text<text_end;text+=incr)
        {
            if(*text==' ')
            {
                x+=font->space_advance;
                prev_gi=font->space_character_index;
                incr=1;
                continue;
            }

            gi=cvm_overlay_get_utf8_glyph_index(font->face,(uint8_t*)text,&incr);

            g=cvm_overlay_find_glpyh(font,gi);

            cvm_overlay_prepare_glyph_render_data(erb,font,g);

            if(prev_gi && !FT_Get_Kerning(font->face,prev_gi,gi,0,&kern)) x+=kern.x>>6;

            prev_gi=gi;

            if(g->tile && erb->count != erb->space)///need to check again as can return null once again;
            {
                rb=rectangle_add_offset(g->pos,x,y);
                rr=get_rectangle_overlap(rb,bounds);

                if((rectangle_has_positive_area(rr))) erb->buffer[erb->count++]=(cvm_overlay_render_data)
                {
                    {rr.x1,rr.y1,rr.x2,rr.y2},
                    {(CVM_OVERLAY_ELEMENT_SHADED<<12)|(colour&0x0FFF),(g->tile->x_pos<<2)+rr.x1-rb.x1,(g->tile->y_pos<<2)+rr.y1-rb.y1,83},
                };
            }

            x+=cvm_overlay_get_glyph_advance(font,g);
        }

        x=x_start;
        y+=font->line_spacing;
    }
}

void overlay_render_multiline_text_selection(cvm_overlay_element_render_buffer * erb,cvm_overlay_font * font,cvm_overlay_text_block * block,int x,int y,rectangle bounds,overlay_colour_ colour,char * selection_start,char * selection_end)
{
    uint32_t i,gi,prev_gi,incr;
    FT_Vector kern;
    cvm_overlay_glyph * g;
    rectangle rb,rr,rs;
    char *text,*text_end;
    int x_start,first_line;

    prev_gi=0;
    rs.x1=rs.x2=x_start=x;

    first_line=(bounds.y1-y)/font->line_spacing;
    first_line*=first_line>0;
    y+=first_line*font->line_spacing;

    if(rectangle_has_positive_area(bounds))for(i=first_line;i<block->line_count && y<bounds.y2;i++)
    {
        text_end=block->lines[i].finish;

        for(text=block->lines[i].start;text<text_end;text+=incr)
        {
            if(text==selection_start) rs.x1=x;
            if(text==selection_end) rs.x2=x;

            if(*text==' ')
            {
                x+=font->space_advance;
                prev_gi=font->space_character_index;
                incr=1;
                continue;
            }

            gi=cvm_overlay_get_utf8_glyph_index(font->face,(uint8_t*)text,&incr);

            g=cvm_overlay_find_glpyh(font,gi);

            cvm_overlay_prepare_glyph_render_data(erb,font,g);

            if(prev_gi && !FT_Get_Kerning(font->face,prev_gi,gi,0,&kern)) x+=kern.x>>6;

            prev_gi=gi;

            if(g->tile && erb->count != erb->space)///need to check again as can return null once again;
            {
                rb=rectangle_add_offset(g->pos,x,y);
                rr=get_rectangle_overlap(rb,bounds);

                if((rectangle_has_positive_area(rr))) erb->buffer[erb->count++]=(cvm_overlay_render_data)
                {
                    {rr.x1,rr.y1,rr.x2,rr.y2},
                    {(CVM_OVERLAY_ELEMENT_SHADED<<12)|(colour&0x0FFF),(g->tile->x_pos<<2)+rr.x1-rb.x1,(g->tile->y_pos<<2)+rr.y1-rb.y1,83},
                };
            }

            x+=cvm_overlay_get_glyph_advance(font,g);
        }

        if(text>selection_start && text<=selection_end)rs.x2=x;

        if(rs.x1!=rs.x2 && erb->count != erb->space)
        {
            rs.y1=y;
            rs.y2=y+font->glyph_size;
            rs=get_rectangle_overlap(rs,bounds);

            if((rectangle_has_positive_area(rs))) erb->buffer[erb->count++]=(cvm_overlay_render_data)
            {
                {rs.x1,rs.y1,rs.x2,rs.y2},
                {(CVM_OVERLAY_ELEMENT_FILL<<12)|(OVERLAY_TEXT_HIGHLIGHT_COLOUR_&0x0FFF),0,0,83},
            };

            rs.x1=rs.x2=x_start;
        }

        x=x_start;
        y+=font->line_spacing;
    }
}

int overlay_find_multiline_text_offset(cvm_overlay_font * font,cvm_overlay_text_block * block,char * text_base,int relative_x,int relative_y)
{
    uint32_t gi,prev_gi,incr;
    FT_Vector kern;
    cvm_overlay_glyph * g;
    char *text,*text_end;
    int x,line,a;

    prev_gi=0;
    x=0;

    if(relative_y<0)return 0;
    if(relative_y > (block->line_count-1)*font->line_spacing+font->glyph_size) return block->lines[block->line_count-1].finish-text_base;

    line=relative_y/font->line_spacing;

    text_end=block->lines[line].finish;

    for(text=block->lines[line].start;text<text_end;text+=incr)
    {
        if(*text==' ')
        {
            if(x+font->space_advance/2 >= relative_x)break;
            x+=font->space_advance;
            prev_gi=font->space_character_index;
            incr=1;
            continue;
        }

        gi=cvm_overlay_get_utf8_glyph_index(font->face,(uint8_t*)text,&incr);

        g=cvm_overlay_find_glpyh(font,gi);

        if(prev_gi && !FT_Get_Kerning(font->face,prev_gi,gi,0,&kern)) x+=kern.x>>6;

        prev_gi=gi;

        a=cvm_overlay_get_glyph_advance(font,g);

        if(x+a/2 >= relative_x)break;

        x+=a;
    }

    return text-text_base;
}








