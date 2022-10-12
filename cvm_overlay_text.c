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
    int r=FT_Init_FreeType(&overlay_freetype_library);
    assert(!r);///COULD NOT INITIALISE FREETYPE LIBRARY
}

void cvm_overlay_close_freetype(void)
{
    int r=FT_Done_FreeType(overlay_freetype_library);
    assert(!r);///COULD NOT DESTROY FREETYPE LIBRARY
}

void cvm_overlay_create_font(cvm_overlay_font * font,char * filename,int pixel_size)
{
    int r;

    r=FT_New_Face(overlay_freetype_library,filename,0,&font->face);
    assert(!r || !fprintf(stderr,"COULD NOT LOAD FONT FACE FILE %s\n",filename));

    r=FT_Set_Pixel_Sizes(font->face,0,pixel_size);
    assert(!r || !fprintf(stderr,"COULD NOT SET FONT FACE SIZE %s %d\n",filename,pixel_size));

    font->glyph_size=pixel_size;

    font->space_character_index=FT_Get_Char_Index(font->face,' ');

    r=FT_Load_Glyph(font->face,font->space_character_index,0);
    assert(!r || !fprintf(stderr,"FONT DOES NOT CONTAIN SPACE CHARACTER %s\n",filename));

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
    int r=FT_Done_Face(font->face);
    assert(!r);///COULD NOT DESTROY FONT FACE

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
            assert((text[*incr]&0xC0) == 0x80);///ATTEMPTING TO RENDER AN INVALID UTF-8 STRING (TOP BIT MISMATCH)
            cp<<=6;
            cp|=text[*incr]&0x3F;
            (*incr)++;
        }
        cp|=(*text&((1<<(7-*incr))-1))<<(6u* *incr-6u);

        assert(*incr!=1);///ATTEMPTING TO RENDER AN INVALID UTF-8 STRING (INVALID LENGTH SPECIFIED)\n");

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

int cvm_overlay_utf8_count_glyphs_outside_range(char * text,char * begin,char * end)
{
    int i,c;
    c=0;
    while(*text)
    {
        c+= text<begin || text>=end;

        if(*text & 0x80)
        {
            for(i=1;*text & (0x80 >> i);i++);

            text+=i;
        }
        else text++;
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

            ///text+=3*(text[0]==0xEF && text[1]==0xB8 && (text[2]&0xF0)==0x80);///variation sequences don't count as a glyph
        }
        else text++;

        (*c)++;
    }
    return true;
}

char * cvm_overlay_utf8_get_previous_glyph(char * base,char * t)
{
    ///offset-= 3* (offset>2 && text[offset-3]==0xEF && text[offset-2]==0xB8 && (text[offset-1]&0xF0)==0x80);///skip over variation sequence

    if(t==base)return base;

    char *to,*tt;
    to=t;

    do t--;
    while(t!=base && (*t & 0xC0) == 0x80);
#ifndef	NDEBUG
    if(*t & 0x80)
    {
        for(tt=t+1;tt<to;tt++)///0 implicitly checked above
        {
            assert(*((uint8_t*)t)<<(tt-t) & 0x80);///GET PREVIOUS DETECTED INVALID UTF-8 CHAR IN STRING (INVALID LENGTH SPECIFIED)\n");
        }
    }
#endif
    return t;
}

char * cvm_overlay_utf8_get_next_glyph(char * t)
{
    if(!*t)return t;

    if(*t & 0x80)
    {
        int i;
        for(i=1;*t & (0x80 >> i);i++)
        {
            ///error check, dont put in release version
            assert((t[i]&0xC0) == 0x80);///GET NEXT DETECTED INVALID UTF-8 CHAR IN STRING (TOP BIT MISMATCH)
        }
        ///error check, dont put in release version
        assert(i!=1);///GET NEXT DETECTED INVALID UTF-8 CHAR IN STRING (INVALID LENGTH SPECIFIED)

        t+=i;

        ///offset+=3*(text[offset]==0xEF && text[offset+1]==0xB8 && (text[offset+2]&0xF0)==0x80);///skip over variation sequence
    }
    else t++;

    return t;
}

char * cvm_overlay_utf8_get_previous_word(char * base,char * t)
{
    /// could/should also match other whitespace characters?
    while(t!=base && *(t-1)==' ')t--;
    while(t!=base && *(t-1)!=' ')t--;
    return t;
}

char * cvm_overlay_utf8_get_next_word(char * t)
{
    /// could/should also match other whitespace characters?
    while(*t && *t==' ')t++;
    while(*t && *t!=' ')t++;
    return t;
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

static inline void cvm_overlay_prepare_glyph_render_data(cvm_overlay_font * font,cvm_overlay_glyph * g)
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

            g->tile=overlay_create_transparent_image_tile_with_staging(&staging,w,h);

            if(g->tile)
            {
                memcpy(staging,gs->bitmap.buffer,sizeof(unsigned char)*w*h);

                g->pos.x2=w+(g->pos.x1=gs->bitmap_left);
                g->pos.y2=h+(g->pos.y1=(font->face->size->metrics.ascender>>6) - gs->bitmap_top);
            }

            assert(!g->advance || g->advance == gs->advance.x>>6);///ADVANCE ALREADY SET BUT WITH DIFFERENT VALUE

            g->advance=gs->advance.x>>6;///probably NOT needed as never render before calculating widget size and calculating size requires knowing text size (may be edge cases though so leave this in
        }
    }
}

static inline int cvm_overlay_get_glyph_advance(cvm_overlay_font * font,cvm_overlay_glyph * g)
{
    FT_Fixed advance;

    if(!g->advance)
    {
        advance=0;
        int r=FT_Get_Advance(font->face,g->glyph_index,0,&advance);
        assert(!r);
        g->advance=advance>>16;
        if(!g->advance)g->advance=1;///ensure nonzero advance if advance is actually zero
    }

    return g->advance;
}




///use complete code? and provide implementation more directly where possible?

#define SINGLE_LINE_RENDER_IMPLEMENTATION(NAME,SI,SS,SC,SR,FI,FS,BCI,GR)\
void NAME(cvm_overlay_element_render_buffer * restrict erb,overlay_theme * restrict theme,rectangle bounds,overlay_colour colour,const char * restrict text,int x,int y SI FI BCI)\
{\
    uint32_t gi,prev_gi,incr;\
    FT_Vector kern;\
    cvm_overlay_glyph * g;\
    SS \
    if(!text)return;\
    prev_gi=0;\
    FS \
    while(*text)\
    {\
        SC \
        if(*text==' ')\
        {\
            x+=theme->font_.space_advance;\
            prev_gi=theme->font_.space_character_index;\
            text++;\
            continue;\
        }\
        gi=cvm_overlay_get_utf8_glyph_index(theme->font_.face,(uint8_t*)text,&incr);\
        g=cvm_overlay_find_glpyh(&theme->font_,gi);\
        cvm_overlay_prepare_glyph_render_data(&theme->font_,g);\
        if(prev_gi && !FT_Get_Kerning(theme->font_.face,prev_gi,gi,0,&kern))\
        {\
            x+=kern.x>>6;\
            SC \
        }\
        prev_gi=gi;\
        if(g->tile) GR \
        x+=cvm_overlay_get_glyph_advance(&theme->font_,g);\
        text+=incr;\
    }\
    SC \
    SR \
}

#define SELECTION_INPUTS ,const char * restrict selection_begin,const char * restrict selection_end
#define SELECTION_SETUP int sb,se;
#define SELECTION_CHECK if(text==selection_begin) sb=x; if(text==selection_end) se=x;

#define FADING_INPUTS ,rectangle text_area,int text_length
#define FADING_SETUP rectangle fade_r=(rectangle){.x1=theme->h_text_fade_range*(text_area.x1>x),.y1=0,.x2=theme->h_text_fade_range*(text_area.x2<x+text_length),.y2=0};

#define BOX_CONSTRAINED_INPUTS ,rectangle box_r,uint32_t box_status

#define GLYPH_RENDER cvm_render_shaded_overlay_element(erb,bounds,rectangle_add_offset(g->pos,x,y),colour,g->tile->x_pos<<2,g->tile->y_pos<<2);
#define GLYPH_RENDER_FADING cvm_render_shaded_fading_overlay_element(erb,bounds,rectangle_add_offset(g->pos,x,y),colour,g->tile->x_pos<<2,g->tile->y_pos<<2,text_area,fade_r);
#define GLYPH_RENDER_BOX_CONSTRAINED theme->shaded_box_constrained_render(erb,theme,bounds,rectangle_add_offset(g->pos,x,y),colour,g->tile->x_pos<<2,g->tile->y_pos<<2,box_r,box_status);
#define GLYPH_RENDER_FADING_BOX_CONSTRAINED theme->shaded_fading_box_constrained_render(erb,theme,bounds,rectangle_add_offset(g->pos,x,y),colour,g->tile->x_pos<<2,g->tile->y_pos<<2,text_area,fade_r,box_r,box_status);

#define SELECTION_RENDER cvm_render_fill_overlay_element(erb,bounds,((rectangle){.x1=sb,.y1=y,.x2=se+(selection_end==selection_begin),.y2=y+theme->font_.glyph_size}),OVERLAY_TEXT_HIGHLIGHT_COLOUR);
#define SELECTION_RENDER_FADING cvm_render_fill_fading_overlay_element(erb,bounds,((rectangle){.x1=sb,.y1=y,.x2=se+(selection_end==selection_begin),.y2=y+theme->font_.glyph_size}),OVERLAY_TEXT_HIGHLIGHT_COLOUR,text_area,fade_r);
#define SELECTION_RENDER_BOX_CONSTRAINED theme->fill_box_constrained_render(erb,theme,bounds,((rectangle){.x1=sb,.y1=y,.x2=se+(selection_end==selection_begin),.y2=y+theme->font_.glyph_size}),OVERLAY_TEXT_HIGHLIGHT_COLOUR,box_r,box_status);
#define SELECTION_RENDER_FADING_BOX_CONSTRAINED theme->fill_fading_box_constrained_render(erb,theme,bounds,((rectangle){.x1=sb,.y1=y,.x2=se+(selection_end==selection_begin),.y2=y+theme->font_.glyph_size}),OVERLAY_TEXT_HIGHLIGHT_COLOUR,text_area,fade_r,box_r,box_status);


SINGLE_LINE_RENDER_IMPLEMENTATION(overlay_text_single_line_render_,,,,,,,,GLYPH_RENDER)
SINGLE_LINE_RENDER_IMPLEMENTATION(overlay_text_single_line_render_selection,SELECTION_INPUTS,SELECTION_SETUP,SELECTION_CHECK,SELECTION_RENDER,,,,GLYPH_RENDER)
SINGLE_LINE_RENDER_IMPLEMENTATION(overlay_text_single_line_render_fading,,,,,FADING_INPUTS,FADING_SETUP,,GLYPH_RENDER_FADING)
SINGLE_LINE_RENDER_IMPLEMENTATION(overlay_text_single_line_render_selection_fading,SELECTION_INPUTS,SELECTION_SETUP,SELECTION_CHECK,SELECTION_RENDER_FADING,FADING_INPUTS,FADING_SETUP,,GLYPH_RENDER_FADING)
SINGLE_LINE_RENDER_IMPLEMENTATION(overlay_text_single_line_render_box_constrained,,,,,,,BOX_CONSTRAINED_INPUTS,GLYPH_RENDER_BOX_CONSTRAINED)
SINGLE_LINE_RENDER_IMPLEMENTATION(overlay_text_single_line_render_selection_box_constrained,SELECTION_INPUTS,SELECTION_SETUP,SELECTION_CHECK,SELECTION_RENDER_BOX_CONSTRAINED,,,BOX_CONSTRAINED_INPUTS,GLYPH_RENDER_BOX_CONSTRAINED)
SINGLE_LINE_RENDER_IMPLEMENTATION(overlay_text_single_line_render_fading_box_constrained,,,,,FADING_INPUTS,FADING_SETUP,BOX_CONSTRAINED_INPUTS,GLYPH_RENDER_FADING_BOX_CONSTRAINED)
SINGLE_LINE_RENDER_IMPLEMENTATION(overlay_text_single_line_render_selection_fading_box_constrained,SELECTION_INPUTS,SELECTION_SETUP,SELECTION_CHECK,SELECTION_RENDER_FADING_BOX_CONSTRAINED,FADING_INPUTS,FADING_SETUP,BOX_CONSTRAINED_INPUTS,GLYPH_RENDER_FADING_BOX_CONSTRAINED)


#undef SELECTION_RENDER_FADING_BOX_CONSTRAINED
#undef SELECTION_RENDER_BOX_CONSTRAINED
#undef SELECTION_RENDER_FADING
#undef SELECTION_RENDER
#undef GLYPH_RENDER_FADING_BOX_CONSTRAINED
#undef GLYPH_RENDER_BOX_CONSTRAINED
#undef GLYPH_RENDER_FADING
#undef GLYPH_RENDER
#undef BOX_CONSTRAINED_INPUTS
#undef FADING_SETUP
#undef FADING_INPUTS
#undef SELECTION_CHECK
#undef SELECTION_SETUP
#undef SELECTION_INPUTS
#undef SINGLE_LINE_RENDER_IMPLEMENTATION


int overlay_text_single_line_get_pixel_length(cvm_overlay_font * font,char * text)
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

char * overlay_text_single_line_find_offset(cvm_overlay_font * font,char * text,int relative_x)
{
    uint32_t gi,prev_gi,incr;
    FT_Vector kern;
    cvm_overlay_glyph * g;
    int x,a;

    prev_gi=0;
    x=0;

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

    return text;
}

void overlay_text_multiline_processing(cvm_overlay_font * font,cvm_overlay_text_block * block,char * text,int wrapping_width)
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

void overlay_text_multiline_render(cvm_overlay_element_render_buffer * erb,cvm_overlay_font * font,rectangle bounds,cvm_overlay_text_block * block,int x,int y,overlay_colour colour)
{
    uint32_t i,gi,prev_gi,incr;
    FT_Vector kern;
    cvm_overlay_glyph * g;
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

            cvm_overlay_prepare_glyph_render_data(font,g);

            if(prev_gi && !FT_Get_Kerning(font->face,prev_gi,gi,0,&kern)) x+=kern.x>>6;

            prev_gi=gi;

            if(g->tile)cvm_render_shaded_overlay_element(erb,bounds,rectangle_add_offset(g->pos,x,y),colour,g->tile->x_pos<<2,g->tile->y_pos<<2);

            x+=cvm_overlay_get_glyph_advance(font,g);
        }

        x=x_start;
        y+=font->line_spacing;
    }
}

void overlay_text_multiline_selection_render(cvm_overlay_element_render_buffer * erb,cvm_overlay_font * font,rectangle bounds,cvm_overlay_text_block * block,int x,int y,overlay_colour colour,char * selection_start,char * selection_end)
{
    uint32_t i,gi,prev_gi,incr;
    FT_Vector kern;
    cvm_overlay_glyph * g;
    char *text,*text_end;
    int x_start,first_line,ss,se;

    prev_gi=0;
    ss=se=x_start=x;

    first_line=(bounds.y1-y)/font->line_spacing;
    first_line*=first_line>0;
    y+=first_line*font->line_spacing;

    if(rectangle_has_positive_area(bounds))for(i=first_line;i<block->line_count && y<bounds.y2;i++)
    {
        text_end=block->lines[i].finish;

        for(text=block->lines[i].start;text<text_end;text+=incr)
        {
            if(text==selection_start) ss=x;
            if(text==selection_end) se=x;

            if(*text==' ')
            {
                x+=font->space_advance;
                prev_gi=font->space_character_index;
                incr=1;
                continue;
            }

            gi=cvm_overlay_get_utf8_glyph_index(font->face,(uint8_t*)text,&incr);

            g=cvm_overlay_find_glpyh(font,gi);

            cvm_overlay_prepare_glyph_render_data(font,g);

            if(prev_gi && !FT_Get_Kerning(font->face,prev_gi,gi,0,&kern))
            {
                x+=kern.x>>6;
                if(kern.x)
                {
                    if(text==selection_start) ss=x;
                    if(text==selection_end) se=x;
                }
            }

            prev_gi=gi;

            if(g->tile)cvm_render_shaded_overlay_element(erb,bounds,rectangle_add_offset(g->pos,x,y),colour,g->tile->x_pos<<2,g->tile->y_pos<<2);

            x+=cvm_overlay_get_glyph_advance(font,g);
        }

        if(text>selection_start && text<=selection_end)se=x;

        if(ss!=se)
        {
            cvm_render_fill_overlay_element(erb,bounds,((rectangle){.x1=ss,.y1=y,.x2=se,.y2=y+font->glyph_size}),OVERLAY_TEXT_HIGHLIGHT_COLOUR);
            ss=se=x_start;
        }

        x=x_start;
        y+=font->line_spacing;
    }
}

char * overlay_text_multiline_find_offset(cvm_overlay_font * font,cvm_overlay_text_block * block,int relative_x,int relative_y)
{
    uint32_t gi,prev_gi,incr;
    FT_Vector kern;
    cvm_overlay_glyph * g;
    char *text,*text_end;
    int x,line,a;

    prev_gi=0;
    x=0;

    if(relative_y<0)return 0;
    if(relative_y > (block->line_count-1)*font->line_spacing+font->glyph_size) return block->lines[block->line_count-1].finish;

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

    return text;
}



cvm_overlay_glyph * overlay_get_glyph(cvm_overlay_font * font,char * text)
{
    uint32_t gi,incr;
    cvm_overlay_glyph * g;

    g=NULL;

    if(text && *text && *text!=' ')
    {
        gi=cvm_overlay_get_utf8_glyph_index(font->face,(uint8_t*)text,&incr);

        g=cvm_overlay_find_glpyh(font,gi);

        cvm_overlay_prepare_glyph_render_data(font,g);

        if(text[incr])
        {
            fprintf(stderr,"TRYING TO GET SINGLE GLYPH FOR STRING WITH MORE THAN ONE: %s\n",text);
        }
    }

    return g;
}

void overlay_text_centred_glyph_render(cvm_overlay_element_render_buffer * erb,cvm_overlay_font * font,rectangle bounds,rectangle r,char * icon_glyph,overlay_colour colour)
{
    cvm_overlay_glyph * g;

    g=overlay_get_glyph(font,icon_glyph);

    if(!g || !g->tile)return;

    r.y1=((r.y1+r.y2)>>1) - ((g->pos.y2-g->pos.y1+1)>>1);
    r.y2=r.y1+g->pos.y2-g->pos.y1;
    r.x1=((r.x1+r.x2)>>1) - ((g->pos.x2-g->pos.x1+1)>>1);
    r.x2=r.x1+g->pos.x2-g->pos.x1;

    cvm_render_shaded_overlay_element(erb,bounds,r,colour,g->tile->x_pos<<2,g->tile->y_pos<<2);
}

void overlay_text_centred_glyph_box_constrained_render(cvm_overlay_element_render_buffer * erb,overlay_theme * theme,rectangle bounds,rectangle r,char * icon_glyph,overlay_colour colour,rectangle box_r,uint32_t box_status)
{
    cvm_overlay_glyph * g;

    g=overlay_get_glyph(&theme->font_,icon_glyph);

    if(!g || !g->tile)return;

    r.y1=((r.y1+r.y2)>>1) - ((g->pos.y2-g->pos.y1+1)>>1);
    r.y2=r.y1+g->pos.y2-g->pos.y1;
    r.x1=((r.x1+r.x2)>>1) - ((g->pos.x2-g->pos.x1+1)>>1);
    r.x2=r.x1+g->pos.x2-g->pos.x1;

    theme->shaded_box_constrained_render(erb,theme,bounds,r,colour,g->tile->x_pos<<2,g->tile->y_pos<<2,box_r,box_status);
}






