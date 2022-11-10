/**
Copyright 2020,2021,2022 Carl van Mastrigt

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

#include <dirent.h>
#include <stdlib.h>



///implement basics of dirent for windows?




//static void append_type_extension(file_search_instance * fsi)
//{
//    if(fsi->selected_type)
//    {
//        char * s=fsi->directory_buffer;
//        char * t=NULL;
//
//        while(*s)
//        {
//            if(*s=='.')t=s;
//            s++;
//        }
//
//        s=fsi->types[fsi->selected_type-1].type_extension;
//        if((t)&&(str_lower_cmp(t+1,s)))return;///already has extension
//
//        int dlen=strlen(fsi->directory_buffer);
//        int elen=strlen(fsi->directory_buffer);
//
//        while((dlen+elen+1)>=(fsi->directory_buffer_size))fsi->directory_buffer=realloc(fsi->directory_buffer,sizeof(char)*(fsi->directory_buffer_size*=2));
//
//        t=fsi->directory_buffer+dlen;
//
//        *(t++)='.';
//        while(*s) *(t++)=*(s++);
//        *t='\0';
//    }
//}


static int file_search_entry_comparison(const void * a,const void * b)
{
    /// return a-b in terms of names
    const file_search_entry * fse1=a;
    const file_search_entry * fse2=b;
    char * entry_names=fse1->fsd->filename_buffer;

    char c1,c2,*s1,*s2,*n1,*n2,*e1,*e2,d;

    //if(fse1->type_id != fse2->type_id)return (((int)fse1->type_id)-((int)fse2->type_id));
    if(fse1->type_id != fse2->type_id && (fse1->type_id==CVM_FS_DIRECTORY_TYPE_ID || fse2->type_id==CVM_FS_DIRECTORY_TYPE_ID))return (((int)fse1->type_id)-((int)fse2->type_id));

    s1=entry_names+fse1->filename_offset;
    s2=entry_names+fse2->filename_offset;

    do
    {
        c1=*s1;
        c2=*s2;

        if(c1>='0' && c1<='9' && c2>='0' && c2<='9')///enter number analysis scheme
        {
            ///already know first is digit
            n1=s1+1;
            n2=s2+1;
            ///find end of number block
            while(*n1>='0' && *n1<='9')n1++;
            while(*n2>='0' && *n2<='9')n2++;
            e1=n1;
            e2=n2;
            d=0;///initially treat numbers as the same, let most significat different digit determine their difference
            while(1)
            {
                ///start with step back, last char, by definition, isnt a number
                n1--;
                n2--;
                if(n1<s1)
                {
                    while(n2>=s2) if(*n2-- != '0') return -1;///b is larger
                    if(d) return d;
                    else break;
                }
                if(n2<s2)
                {
                    while(n1>=s1) if(*n1-- != '0') return 1;///a is larger
                    if(d) return d;
                    else break;
                }
                if(*n1!=*n2) d=*n1-*n2;
            }
            ///skip to the point past the number block
            s1=e1;
            s2=e2;
            c1=*s1;
            c2=*s2;
        }
        else
        {
            s1++;
            s2++;
        }

        if(c1>='A' && c1<='Z')c1-='A'-'a';
        if(c2>='A' && c2<='Z')c2-='A'-'a';
    }
    while((c1==c2)&&(c1)&&(c2));

    return (c1-c2);
}

#warning better name for str_lower_cmp ??? use standardised function ???
int str_lower_cmp(const char * s1,const char * s2)
{
    char c1,c2;

    do
    {
        c1=*(s1++);
        c2=*(s2++);

        if((c1>='A')&&(c1<='Z'))c1-='A'-'a';
        if((c2>='A')&&(c2<='Z'))c2-='A'-'a';

        if(c1!=c2)return c1-c2;
    }
    while((c1)&&(c2));

    return 0;
}

static void clean_file_search_directory(char * directory)
{
    char *r,*w;

    r=w=directory;

    while(*r)
    {
        while(*r=='/')
        {
            if((r[1]=='.')&&(r[2]=='.')&&(r[3]=='/'))///go up a layer when encountering ../
            {
                r+=3;
                if(w>directory)w--;///reverse over '/'
                while((w>directory)&&(*w!='/')) w--;
            }
            else if((r[1]=='.')&&(r[2]=='/'))r+=2;///remove ./
            else if(r[1]=='/')r++;/// remove multiple /
            else break;
        }

        if(w!=r)*w=*r;

        r++;
        w++;
    }

    *w='\0';
}

static inline bool is_hidden_file(const char * filename)
{
    return filename && *filename=='.' && strcmp(filename,".") && strcmp(filename,"..");///exists, first entry is '.' and isnt just "." and isnt just ".."
}

static void set_file_search_directory(file_search_data * fsd,const char * directory)
{
    if(directory==NULL)directory=getenv("HOME");
    if(directory==NULL)directory="";
    uint32_t length=strlen(directory);

    while((length+1) >= fsd->directory_buffer_size)fsd->directory_buffer=realloc(fsd->directory_buffer,sizeof(char)*(fsd->directory_buffer_size*=2));
    strcpy(fsd->directory_buffer,directory);

    while((length>0)&&(fsd->directory_buffer[length-1]=='/'))length--;

    fsd->directory_buffer[length]='/';
    fsd->directory_buffer[length+1]='\0';

    clean_file_search_directory(fsd->directory_buffer);
}






#warning make static ??
void load_file_search_directory_entries(file_search_data * fsd)
{
    DIR * directory;
    struct dirent * entry;

    uint32_t i,filename_buffer_offset,filename_length;
    uint16_t type_id;
    bool valid;

    const char *filename,*ext,*type_ext;


    fsd->entry_count=0;
    filename_buffer_offset=0;

    directory = opendir(fsd->directory_buffer);

    if(!directory)
    {
        printf("SUPPLIED DIRECTORY INVALID :%s:\n",fsd->directory_buffer);
        set_file_search_directory(fsd,getenv("HOME"));

        directory = opendir(fsd->directory_buffer);
        if(!directory)
        {
            puts("HOME DIRECTORY INVALID");
            return;
        }
    }


    while((entry=readdir(directory)))
    {
        if(fsd->show_hidden || !is_hidden_file(entry->d_name))
        {

            valid=false;
            if(entry->d_type==DT_DIR)
            {
                type_id=CVM_FS_DIRECTORY_TYPE_ID;
                valid = fsd->show_control_entries || (strcmp(entry->d_name,".") && strcmp(entry->d_name,".."));/// dont show . or .. unless showing control entries
            }
            else if(entry->d_type==DT_REG)
            {
                type_id=CVM_FS_MISCELLANEOUS_TYPE_ID;

                ext=NULL;
                filename=entry->d_name;
                while(*filename)
                {
                    if(*filename=='.')ext=filename+1;
                    filename++;
                }

                if(ext) for(i=0;i<fsd->type_count && type_id==CVM_FS_MISCELLANEOUS_TYPE_ID;i++)
                {
                    type_ext=fsd->types[i].type_extensions;

                    while(*type_ext)
                    {
                        if(!str_lower_cmp(type_ext,ext))
                        {
                            type_id=i+CVM_FS_CUSTOM_TYPE_OFFSET;
                            break;
                        }
                        while(*type_ext++);///move to next in concatenated extension list
                    }
                }

                valid=fsd->show_misc_files || type_id!=CVM_FS_MISCELLANEOUS_TYPE_ID;
            }

            if(valid)
            {
                filename_length=strlen(entry->d_name)+1;

                while((filename_buffer_offset+filename_length)>fsd->filename_buffer_space)
                {
                    fsd->filename_buffer=realloc(fsd->filename_buffer,(fsd->filename_buffer_space*=2));
                }

                if(fsd->entry_count==fsd->entry_space)
                {
                    fsd->entries=realloc(fsd->entries,sizeof(file_search_entry)*(fsd->entry_space*=2));
                }

                fsd->entries[fsd->entry_count++]=(file_search_entry)
                {
                    .fsd=fsd,
                    .filename_offset=filename_buffer_offset,
                    .type_id=type_id,
                    .text_length_calculated=false,
                    .text_length=0,
                };

                strcpy(fsd->filename_buffer+filename_buffer_offset,entry->d_name);
                filename_buffer_offset+=filename_length;
            }
        }
    }

    qsort(fsd->entries,fsd->entry_count,sizeof(file_search_entry),file_search_entry_comparison);///sort based

    closedir(directory);
}



void initialise_file_search_data(file_search_data * fsd,const char * directory,const file_search_file_type * types,uint32_t type_count)
{
    fsd->filename_buffer_space=16;
    fsd->filename_buffer=malloc(sizeof(char)*fsd->filename_buffer_space);

    fsd->directory_buffer_size=16;
    fsd->directory_buffer=malloc(sizeof(char)*fsd->directory_buffer_size);

    fsd->entry_count=0;
    fsd->entry_space=8;
    fsd->entries=malloc(sizeof(file_search_entry)*fsd->entry_space);

    fsd->types=types;
    fsd->type_count=type_count;

    fsd->show_directories=true;
    fsd->show_misc_files=true;
    fsd->show_control_entries=false;
    fsd->show_hidden=true;

    set_file_search_directory(fsd,directory);
}





static inline void file_list_widget_recalculate_scroll_properties(overlay_theme * theme,widget * w)
{
    file_search_data * fsd=w->file_list.fsd;
    w->file_list.visible_height=w->base.r.y2-w->base.r.y1 - 2*theme->contiguous_box_y_offset;

    w->file_list.max_offset=fsd->entry_count*theme->base_contiguous_unit_h-w->file_list.visible_height;
    if(w->file_list.max_offset<0)w->file_list.max_offset=0;
    if(w->file_list.offset>w->file_list.max_offset)w->file_list.offset=w->file_list.max_offset;
}

static inline void file_list_widget_set_selected_entry(overlay_theme * theme,widget * w,int32_t selected_entry_index)
{
    int32_t o;
    file_search_data * fsd=w->file_list.fsd;

    o=selected_entry_index*theme->base_contiguous_unit_h;
    if(w->file_list.offset>o)w->file_list.offset=o;

    o-=w->base.r.y2-w->base.r.y1-2*theme->contiguous_box_y_offset-theme->base_contiguous_unit_h;
    if(w->file_list.offset<o)w->file_list.offset=o;

    w->file_list.selected_entry=selected_entry_index;

    ///cannot perform all ops here as they would overwrite, e.g. enterbox selects best match (potentially) but this would overwrite enterbox contents!
}




static bool file_list_widget_scroll(overlay_theme * theme,widget * w,int delta)
{
    w->file_list.offset-=theme->base_contiguous_unit_h*delta;
    if(w->file_list.offset<0)w->file_list.offset=0;
    if(w->file_list.offset>w->file_list.max_offset)w->file_list.offset=w->file_list.max_offset;

    return true;
}

static void file_list_widget_left_click(overlay_theme * theme,widget * w,int x,int y)
{
    file_search_data * fsd=w->file_list.fsd;
    rectangle r;
	int32_t index;
    adjust_coordinates_to_widget_local(w,&x,&y);

	index=(w->file_list.offset+y-theme->contiguous_box_y_offset)/theme->base_contiguous_unit_h;

    assert(index>=0);
    if(index<0)index=0;
    if(index>=fsd->entry_count)return;

    /// *could* check against bounding h_bar for each item, but i don't see the value

    file_list_widget_set_selected_entry(theme,w,index);

    if(fsd->entries[index].type_id == CVM_FS_DIRECTORY_TYPE_ID)
    {
        ///check double clicked
        if(check_widget_double_clicked(w))
        {
            puts("double clicked directory");
        }
    }
    else if(check_widget_double_clicked(w))
    {
        puts("double clicked file");
    }
    else ///update things
    {
        if(w->file_list.enterbox)
        {
            ///set enterbox text
            set_enterbox_text(w->file_list.enterbox,fsd->filename_buffer+fsd->entries[index].filename_offset);
        }
        if(w->file_list.directory_textbar)
        {
            ///
        }
    }
}

static bool file_list_widget_left_release(overlay_theme * theme,widget * clicked,widget * released,int x,int y)
{
    return true;
}

static widget_behaviour_function_set enterbox_behaviour_functions=
{
    .l_click        =   file_list_widget_left_click,
    .l_release      =   file_list_widget_left_release,
    .r_click        =   blank_widget_right_click,
    .m_move         =   blank_widget_mouse_movement,
    .scroll         =   file_list_widget_scroll,
    .key_down       =   blank_widget_key_down,
    .text_input     =   blank_widget_text_input,/// arrow key navigation, selection &c.
    .text_edit      =   blank_widget_text_edit,
    .click_away     =   blank_widget_click_away,///should invalidate current selection
    .add_child      =   blank_widget_add_child,
    .remove_child   =   blank_widget_remove_child,
    .wid_delete     =   blank_widget_delete ///requires nothing for the time being
};


static void file_list_widget_render(overlay_theme * theme,widget * w,int16_t x_off,int16_t y_off,cvm_overlay_element_render_buffer * erb,rectangle bounds)
{
    file_search_data * fsd=w->file_list.fsd;
    file_search_entry * fse;
    rectangle icon_r,r;
	int32_t y,y_end,index,y_text_off;
	char * icon_glyph;

	r=rectangle_add_offset(w->base.r,x_off,y_off);

	theme->box_render(erb,theme,bounds,r,w->base.status,OVERLAY_MAIN_COLOUR);


	y_end=r.y2-theme->contiguous_box_y_offset;
	index=w->file_list.offset/theme->base_contiguous_unit_h;
	y=r.y1+index*theme->base_contiguous_unit_h-w->file_list.offset+theme->contiguous_box_y_offset;
	y_text_off=(theme->base_contiguous_unit_h-theme->font.glyph_size)>>1;

    ///make fade and constrained
	overlay_text_single_line_render_data text_render_data=
	{
	    .flags=OVERLAY_TEXT_RENDER_BOX_CONSTRAINED|OVERLAY_TEXT_RENDER_FADING,
	    .x=r.x1+theme->h_bar_text_offset+theme->h_bar_icon_text_offset+theme->base_contiguous_unit_w,///base_contiguous_unit_w used for icon space/sizing
	    //.y=,
	    //.text=,
	    .bounds=bounds,
	    .colour=OVERLAY_TEXT_COLOUR_0,
	    .box_r=r,
	    .box_status=w->base.status,
	    //.text_length=,
	    .text_area={.x1=text_render_data.x,.y1=r.y1,.x2=r.x2-theme->h_bar_text_offset,.y2=r.y2}
	};

	icon_r.x1=r.x1+theme->h_bar_text_offset;
	icon_r.x2=r.x1+theme->h_bar_text_offset+theme->base_contiguous_unit_w;

	while(y<y_end && index<fsd->entry_count)
    {
        if(index==w->file_list.selected_entry)
        {
            theme->h_bar_box_constrained_render(erb,theme,bounds,((rectangle){.x1=r.x1,.y1=y,.x2=r.x2,.y2=y+theme->base_contiguous_unit_h}),w->base.status,OVERLAY_HIGHLIGHTING_COLOUR,r,w->base.status);
        }

        fse=fsd->entries+index;

        ///render icon
        icon_r.y1=y;
        icon_r.y2=y+theme->base_contiguous_unit_h;

        if(fse->type_id==CVM_FS_DIRECTORY_TYPE_ID) icon_glyph="🗀";
        else if(fse->type_id==CVM_FS_MISCELLANEOUS_TYPE_ID) icon_glyph="🗎";
        else icon_glyph=fsd->types[fse->type_id-CVM_FS_CUSTOM_TYPE_OFFSET].icon;

        overlay_text_centred_glyph_box_constrained_render(erb,&theme->font,bounds,icon_r,icon_glyph,OVERLAY_TEXT_COLOUR_0,r,w->base.status);


        text_render_data.y=y+y_text_off;

        text_render_data.text=fsd->filename_buffer+fse->filename_offset;

        if(!fse->text_length_calculated)
        {
            fse->text_length=overlay_text_single_line_get_pixel_length(&theme->font,text_render_data.text);
            fse->text_length_calculated=true;
        }

        text_render_data.text_length=fse->text_length;

        overlay_text_single_line_render(erb,theme,&text_render_data);

        y+=theme->base_contiguous_unit_h;
        index++;
    }
}

static widget * file_list_widget_select(overlay_theme * theme,widget * w,int16_t x_in,int16_t y_in)
{
    if(theme->box_select(theme,rectangle_subtract_offset(w->base.r,x_in,y_in),w->base.status))return w;
    return NULL;
}

static void file_list_widget_min_w(overlay_theme * theme,widget * w)
{
    w->base.min_w = 2*theme->h_bar_text_offset+theme->font.max_advance*w->file_list.min_visible_glyphs + theme->h_bar_icon_text_offset+theme->base_contiguous_unit_w;///base_contiguous_unit_w used for icon space/sizing
}

static void file_list_widget_min_h(overlay_theme * theme,widget * w)
{
    w->base.min_h=w->file_list.min_visible_rows*theme->base_contiguous_unit_h+2*theme->contiguous_box_y_offset;
}



static void file_list_widget_set_h(overlay_theme * theme,widget * w)
{
    file_list_widget_recalculate_scroll_properties(theme,w);
}

static widget_appearence_function_set file_list_appearence_functions=
{
    .render =   file_list_widget_render,
    .select =   file_list_widget_select,
    .min_w  =   file_list_widget_min_w,
    .min_h  =   file_list_widget_min_h,
    .set_w  =   blank_widget_set_w,
    .set_h  =   file_list_widget_set_h
};

widget * create_file_list(file_search_data * fsd,int16_t min_visible_rows,int16_t min_visible_glyphs)
{
    widget * w=create_widget();

    w->base.appearence_functions=&file_list_appearence_functions;
    w->base.behaviour_functions=&enterbox_behaviour_functions;

    w->file_list.fsd=fsd;

    w->file_list.min_visible_rows=min_visible_rows;
    w->file_list.min_visible_glyphs=min_visible_glyphs;

    w->file_list.directory_textbar=NULL;
    w->file_list.enterbox=NULL;
    w->file_list.parent_widget=NULL;
    w->file_list.error_popup=NULL;
    w->file_list.type_seclect_popup=NULL;

    w->file_list.selected_entry=-1;
    w->file_list.offset=0;
    w->file_list.max_offset=0;
    w->file_list.visible_height=0;

    w->file_list.selected_out_type=0;

    return w;
}

