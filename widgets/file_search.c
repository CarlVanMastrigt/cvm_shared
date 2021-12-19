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








#warning have seperator character : / stored somewhere else and used for these functions ??, probably not needed as mac will likely be different
///below are OS dependant functions

static bool check_if_directory(char * filename)
{
    struct stat sb;
    if(stat(filename,&sb)) return false;
    return S_ISDIR(sb.st_mode);
}

static bool check_if_regular_file(char * filename)
{
    struct stat sb;
    if(stat(filename,&sb)) return false;
    return S_ISREG(sb.st_mode);
}

static bool check_if_file_exists(char * filename)
{
    struct stat sb;
    if(stat(filename,&sb)) return false;
    return true;
}

static bool check_if_directory_is_existent(char * filename)
{
    char * a=filename;
    char * last_slash=NULL;
    bool r=false;
    char tmp;

    while(*a)
    {
        if(*a == '/')
        {
            last_slash=a;
        }

        a++;
    }

    if(last_slash)
    {
        tmp=*last_slash;
        *last_slash='\0';
        r=check_if_directory(filename);
        *last_slash=tmp;
    }

    return r;
}

static void remove_file_search_layer(file_search_instance * fsi)
{
    char *e,*s;

    s=e=fsi->directory_buffer;
    while(*e)e++;
    e--;

    while((e>=s)&&(*e=='/'))e--;
    while((e>=s)&&(*e!='/'))e--;
    while((e>=s)&&(*e=='/'))e--;
    e[1]='/';
    e[2]='\0';
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
                if(w>directory)w--;///in case w==r ( meaning *w == '/' )
                while((w>directory)&&(*w!='/')) w--;
            }
            else if((r[1]=='.')&&(r[2]=='/'))r+=2;///remove ./
            else if(r[1]=='/')r++;/// remove multiple /
            else break;
        }

        *w=*r;//if(r!=w)

        r++;
        w++;
    }

    *w='\0';
}

static void set_file_search_directory(file_search_instance * fsi,char * directory)
{
    if(directory==NULL)directory=getenv("HOME");
    if(directory==NULL)directory="";
    int length=strlen(directory);

    while((length+1) >= fsi->directory_buffer_size)fsi->directory_buffer=realloc(fsi->directory_buffer,sizeof(char)*(fsi->directory_buffer_size*=2));
    strcpy(fsi->directory_buffer,directory);

    while((length>0)&&(fsi->directory_buffer[length-1]=='/'))length--;

    fsi->directory_buffer[length]='/';
    fsi->directory_buffer[length+1]='\0';

    clean_file_search_directory(fsi->directory_buffer);
}

static void concatenate_file_search_directory(file_search_instance * fsi,char * addition)
{
    if(addition==NULL)return;

    char * a;

    if(!fsi->show_directories)
    {
        a=addition;
        while(*a)
        {
            if(*a=='/')
            {
                return;
            }
            a++;
        }
    }

    #warning check for any issues caused by not sanatizing

    if(*addition=='\0')return;

    size_t length=strlen(fsi->directory_buffer)+strlen(addition);
    while((length+1) >= fsi->directory_buffer_size)fsi->directory_buffer=realloc(fsi->directory_buffer,sizeof(char)*(fsi->directory_buffer_size*=2));
    strcat(fsi->directory_buffer,addition);
    if(check_if_directory(fsi->directory_buffer))
    {
        fsi->directory_buffer[length]='/';
        fsi->directory_buffer[length+1]='\0';
    }

    clean_file_search_directory(fsi->directory_buffer);
}

static void load_file_search_directory_entries(file_search_instance * fsi)
{
    DIR * d;
    struct dirent * e;
    struct stat sb;

    int fn_len,dn_len;
    uint32_t type;


    fsi->entry_count=0;
    fsi->entry_name_data_count=0;

    d = opendir(fsi->directory_buffer);

    if(!d)
    {
        printf("SUPPLIED DIRECTORY INVALID :%s:\n",fsi->directory_buffer);
        set_file_search_directory(fsi,getenv("HOME"));

        d = opendir(fsi->directory_buffer);
        if(!d)
        {
            puts("HOME DIRECTORY INVALID");
            return;
        }
    }

    dn_len=strlen(fsi->directory_buffer);

    #warning alloc mem for each name and keep in entry itself (strdup)

    while((e=readdir(d)))
    {
        fn_len=strlen(e->d_name)+1;

        while((dn_len+fn_len+1) >= fsi->directory_buffer_size)fsi->directory_buffer=realloc(fsi->directory_buffer,sizeof(char)*(fsi->directory_buffer_size*=2));
        strcat(fsi->directory_buffer,e->d_name);

        if(stat(fsi->directory_buffer,&sb) == -1)
        {
            fsi->directory_buffer[dn_len]='\0';///undo filename append
            continue;
        }

        if(S_ISDIR(sb.st_mode))type=0;
        else if(S_ISREG(sb.st_mode))type=1;
        else type=-1;

        if(type>=0)
        {
            while((fsi->entry_name_data_count+fn_len)>fsi->entry_name_data_space)
            {
                fsi->entry_name_data=realloc(fsi->entry_name_data,(fsi->entry_name_data_space*=2));
            }

            if(fsi->entry_count==fsi->entry_space)
            {
                fsi->entries=realloc(fsi->entries,sizeof(widget_file_search_entry)*(fsi->entry_space*=2));
            }

            fsi->entries[fsi->entry_count].fsi=fsi;
            fsi->entries[fsi->entry_count].hidden=((e->d_name[0]=='.')&&(strcmp(e->d_name,"."))&&(strcmp(e->d_name,"..")));
            fsi->entries[fsi->entry_count].type=type;
            fsi->entries[fsi->entry_count].name_offset=fsi->entry_name_data_count;
            strcpy(fsi->entry_name_data+fsi->entry_name_data_count,e->d_name);

            fsi->entry_name_data_count+=fn_len;
            fsi->entry_count++;
        }

        fsi->directory_buffer[dn_len]='\0';///undo filename append
    }

    closedir(d);
}



///above are OS dependant functions














static void load_file_search_directory(file_search_instance * fsi);

#warning better name for str_lower_cmp ??? use standardised function ???
bool str_lower_cmp(char * s1,char * s2)
{
    char c1,c2;

    do
    {
        c1=*(s1++);
        c2=*(s2++);

        if((c1>='A')&&(c1<='Z'))c1-='A'-'a';
        if((c2>='A')&&(c2<='Z'))c2-='A'-'a';

        if(c1!=c2)return false;
    }
    while((c1)&&(c2));

    return true;
}

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

static void clear_all_file_search_selection_information(file_search_instance * fsi)
{
//    fsi->selected_entry=NULL;
//    if(fsi->enterbox)set_enterbox_text(fsi->enterbox,NULL);
//    *fsi->file_box->contiguous_box.offset=fsi->file_box->contiguous_box.max_offset;
}

static void file_search_func(file_search_instance * fsi,bool use_only_enterbox_text)
{
    char * selected_text=NULL;
    char * enterbox_text=NULL;
    char * used_text;
    int prev_length=strlen(fsi->directory_buffer);

    if(fsi->selected_entry)selected_text=fsi->entry_name_data+fsi->entries[fsi->selected_entry->button.index].name_offset;
    if((fsi->enterbox)&&(strlen(fsi->enterbox->enterbox.text)))enterbox_text=fsi->enterbox->enterbox.text;

    if(use_only_enterbox_text)
    {
        if(enterbox_text) used_text=enterbox_text;//concatenate_file_search_directory(fsi,enterbox_text);
        else return;
    }
    else
    {
        if(selected_text) used_text=selected_text;//concatenate_file_search_directory(fsi,selected_text);
        else if(enterbox_text) used_text=enterbox_text;//concatenate_file_search_directory(fsi,enterbox_text);
        else return;
    }

    concatenate_file_search_directory(fsi,used_text);

    if(check_if_directory(fsi->directory_buffer)&&(fsi->show_directories))///load directory
    {
        ///dont call   clear_all_file_search_selection_information  because it clears enterbox always, changing directory should not clear enterbox (unless it was used to change dir)
        fsi->selected_entry=NULL;//even though load directory does this
        if((enterbox_text)&&(strcmp(enterbox_text,used_text)==0))set_enterbox_text(fsi->enterbox,NULL);///if enterbox text matches used text THEN clear enterbox

        load_file_search_directory(fsi);

        return;
    }

    //append_type_extension(fsi);

    if(check_if_regular_file(fsi->directory_buffer))
    {
        if(fsi->overwrite_popup)///of overwrite popup exists, then use it, otherwise assume no overwrite checking is needed (loading files/author allows automatic overwrite)
        {
            ///this popup uses selected_entry or enterbox (same as above) so don't clear either until after popup operation (this avoids displaying filename in textbar)
            fsi->directory_buffer[prev_length]='\0';
            toggle_exclusive_popup(fsi->overwrite_popup);
            return;
        }
        else fsi->error_type=fsi->action_function(fsi);
    }
    else if(check_if_directory(fsi->directory_buffer))
    {
        fsi->error_type=FILE_SEARCH_ERROR_CAN_NOT_CHANGE_DIRECTORY;
        puts("ERROR: DIRECTORY ERRONEOUSLY SELECTED");///if show_directories is false, otherwise managed above
    }
    else if(fsi->enterbox==NULL) /// (no enterbox but file is not listed) probably occurred because selected file has been deleted, ergo reload of directory is in order
    {
        fsi->error_type=FILE_SEARCH_ERROR_INVALID_FILE;
        puts("ERROR: SELECTED FILE NO LONGER EXISTS");
    }
    else if(check_if_file_exists(fsi->directory_buffer))///invalid : exists but is not a regular file or directory, ergo cannot load, revert, show error if applicable and return
    {
        fsi->error_type=FILE_SEARCH_ERROR_INVALID_FILE;
        puts("ERROR: FILE OF INVALID TYPE SELECTED");
    }
    else if(!check_if_directory_is_existent(fsi->directory_buffer))
    {
        fsi->error_type=FILE_SEARCH_ERROR_INVALID_DIRECTORY;
        puts("ERROR: INVALID DIRECTORY");
    }
    else
    {
        fsi->error_type=fsi->action_function(fsi);
        #warning need to perform op to ensure type is valid and append extension if necessary (probably above this if else block but after directory block ? )
    }

    fsi->directory_buffer[prev_length]='\0';
    load_file_search_directory(fsi);///for (fsi->enterbox==NULL) condition, or in case new file was added

    if(fsi->error_type)
    {
        if(fsi->error_popup)toggle_exclusive_popup(fsi->error_popup);
    }
    else
    {
        fsi->end_function(fsi);
        clear_all_file_search_selection_information(fsi);
        #warning update shared_file_search_data, selected_file and active_folder, set selected_file to used_file, active_folder to fsi->directory_buffer (file was cleared from this above)
        ///set update count to be same as shared_file_search_data after doing this
    }

    /// selected used above to update shared_file_search_data


    #warning check if appropriately typed file, before doing this in case selected with enterbox (compare typing with same function as when loading)

    #warning remember to manage automatic file extensions (& how they affect existing files/folders), possibly greyed out text in enterbox, or file with extension greyed out in textbar???
}

static void file_search_button_func(widget * w)
{
    file_search_instance * fsi=w->button.data;

    ensure_widget_in_contiguous_box_is_visible(w,fsi->file_box);

    if((fsi->selected_entry==w)&&(check_widget_double_clicked(w))) file_search_func(fsi,false);
    else
    {
        if((fsi->enterbox)&&(fsi->entries[w->button.index].type)) set_enterbox_text(fsi->enterbox,fsi->entry_name_data+fsi->entries[w->button.index].name_offset);
        fsi->selected_entry=w;
        //if(fsi->entries[w->button.index].type)fsi->selected_type=fsi->entries[w->button.index].type-1;
    }
}








static int file_search_button_widget_w(overlay_theme * theme)
{
//    return 16*theme->font.max_glyph_width+2*theme->h_bar_text_offset+theme->icon_bar_extra_w;
}

static int file_search_button_widget_h(overlay_theme * theme)
{
    return theme->contiguous_horizintal_bar_h;
}





static void file_search_button_widget_render(overlay_theme * theme,widget * w,int x_off,int y_off,cvm_overlay_element_render_buffer * erb,rectangle bounds)
{
    file_search_instance * fsi=w->button.data;
    //theme->h_text_bar_render(w->base.r,x_off,y_off,w->base.status,theme,od,bounds,OVERLAY_HIGHLIGHTING_COLOUR,w->button.text);

    char * icon_name;
    if(fsi->entries[w->button.index].type==0)icon_name="folder";
    else if(fsi->entries[w->button.index].type==1)icon_name="misc_file";
    else icon_name=fsi->sfsd->types[fsi->entries[w->button.index].type-2].icon_name;

//    theme->h_icon_text_bar_render(w->base.r,x_off,y_off,w->base.status,theme,od,bounds,
//        (fsi->selected_entry==w)?OVERLAY_HIGHLIGHTING_COLOUR:OVERLAY_NO_COLOUR,
//        fsi->entry_name_data+fsi->entries[w->button.index].name_offset,
//        icon_name,OVERLAY_TEXT_COLOUR_0);
}

static widget * file_search_button_widget_select(overlay_theme * theme,widget * w,int x_in,int y_in)
{
    if(theme->h_bar_select(rectangle_subtract_offset(w->base.r,x_in,y_in),w->base.status,theme))return w;
    return NULL;
}

static void file_search_button_widget_min_w(overlay_theme * theme,widget * w)
{
    w->base.min_w = file_search_button_widget_w(theme);///16 min characters
}

static void file_search_button_widget_min_h(overlay_theme * theme,widget * w)
{
    w->base.min_h = file_search_button_widget_h(theme);
}


static widget_appearence_function_set file_search_button_appearence_functions=
(widget_appearence_function_set)
{
    .render =   file_search_button_widget_render,
    .select =   file_search_button_widget_select,
    .min_w  =   file_search_button_widget_min_w,
    .min_h  =   file_search_button_widget_min_h,
    .set_w  =   blank_widget_set_w,
    .set_h  =   blank_widget_set_h
};

static widget * create_file_search_list_button(file_search_instance * fsi,int button_index)
{
    widget * w=create_button(fsi,file_search_button_func,false);
    w->button.index=button_index;

    w->base.appearence_functions=&file_search_button_appearence_functions;

    return w;
}



















//static bool test_ordering(widget_file_search_entry e1,widget_file_search_entry e2,char * entry_names)
//{
//    ///if e1 goes later return false
//    ///if e2 is later return true
//
//    char c1,c2,*s1,*s2;
//
//    if((!e1.type) != (!e2.type))return (!e1.type);
//
//    //return strcmp(entry_names+e1.name_offset,entry_names+e2.name_offset)<0;
//    //return strcoll(entry_names+e1.name_offset,entry_names+e2.name_offset)<0;
//
//    s1=entry_names+e1.name_offset;
//    s2=entry_names+e2.name_offset;
//
//    do
//    {
//        c1=*(s1++);
//        c2=*(s2++);
//
//        if((c1>='A')&&(c1<='Z'))c1-='A'-'a';
//        if((c2>='A')&&(c2<='Z'))c2-='A'-'a';
//
//        #warning check numbers as isolated units
//    }
//    while((c1==c2)&&(c1)&&(c2));
//
//    return c2>c1;
//}
//
//static void quicksort_file_search_entries(widget_file_search_entry * start,widget_file_search_entry * end,char * entry_names)
//{
//    widget_file_search_entry t,pv;
//
//    ///test_ordering is equivelant to (e1 < e2)
//
//    if(test_ordering(*end,*start,entry_names))
//    {
//        t = *start;
//        *start = *end;
//        *end = t;
//    }
//
//    if((start+2)>end)return;
//
//    widget_file_search_entry *e,*s=start+((end-start)>>1);
//
//    pv=*s;
//
//    if(test_ordering(*end,pv,entry_names))
//    {
//        pv = *end;
//        *end = *s;
//        *s = pv;
//    }
//    else if(test_ordering(pv,*start,entry_names))
//    {
//        pv = *start;
//        *start = *s;
//        *s = pv;
//    }
//
//    if((start+3)>end)
//    {
//        return;
//    }
//
//    s=start;
//    e=end;
//
//    while(1)
//    {
//        while(test_ordering(*(++s),pv,entry_names));
//        while(test_ordering(pv,*(--e),entry_names));
//
//        if(e<=s)break;
//
//        t = *s;
//        *s = *e;
//        *e = t;
//    }
//
//    s--;
//    e++;
//
//    if(start<s)quicksort_file_search_entries(start,s,entry_names);
//    if(e<end)quicksort_file_search_entries(e,end,entry_names);
//}



static bool file_search_file_currently_valid(file_search_instance * fsi,uint32_t i)
{
    char * name=fsi->entry_name_data+fsi->entries[i].name_offset;
    return
    (
        ((fsi->show_hidden)||(!fsi->entries[i].hidden))&&
        ((fsi->show_directories)||(fsi->entries[i].type!=0))&&
        ((fsi->show_misc_files)||(fsi->entries[i].type!=1))&&
        ((fsi->active_type_filter<0)||(fsi->active_type_filter==(fsi->entries[i].type-2))||(fsi->entries[i].type==0))&&
        ((fsi->show_control_entries)||((strcmp(name,"."))&&(strcmp(name,".."))))
    );
}






static void populate_file_search_buttons(file_search_instance * fsi)
{
//    if(fsi->file_box==NULL)return;
//
//    //widget * file_box=fsi->file_box;
//    widget * current=fsi->file_box->contiguous_box.contained_box->container.first;
//
//
//    fsi->selected_entry=NULL;
//
//    uint32_t i;
//
//    for(i=0;i < fsi->entry_count;i++)
//    {
//        if(file_search_file_currently_valid(fsi,i))
//        {
//            if(current) current->button.index=i;
//            else current=add_child_to_parent(fsi->file_box,create_file_search_list_button(fsi,i));
//
//            current->base.status|=WIDGET_ACTIVE;
//            current->base.appearence_functions=&file_search_button_appearence_functions;
//
//            current=current->base.next;
//        }
//    }
//
//    while(current)
//    {
//        current->base.status&= ~WIDGET_ACTIVE;
//        current=current->base.next;
//    }
//
//    *fsi->file_box->contiguous_box.offset=100000;///will be reverted to contiguous_box.max_offset when actually organised (following will not be fully called if parent widget inactive)
//
//    organise_toplevel_widget(fsi->file_box);
}

void set_entry_type(file_search_instance * fsi,uint32_t entry_index)
{
    file_search_file_type * types=fsi->sfsd->types;
    char ** f;
    char *s,*t;
    uint32_t j;

    if(fsi->entries[entry_index].type)
    {
        s=fsi->entry_name_data+fsi->entries[entry_index].name_offset;
        t=NULL;

        while(*s)
        {
            if(*s=='.')t=s;
            s++;
        }

        if(t)
        {
            t++;

            for(j=0;types[j].type_extensions;j++)
            {
                f=types[j].type_extensions;

                while(*f)
                {
                    if(str_lower_cmp(t,*f))
                    {
                        fsi->entries[entry_index].type=j+2;
                        return;
                    }
                    f++;
                }
            }
        }
    }
}



static int file_search_entry_comparison(const void * a,const void * b)
{
    ///if e1 goes later return false
    ///if e2 is later return true

    const widget_file_search_entry * e1=a;
    const widget_file_search_entry * e2=b;
    char * entry_names=e1->fsi->entry_name_data;

    char c1,c2,*s1,*s2;

    if(e1->type != e2->type)return (((int)e1->type)-((int)e2->type));

    s1=entry_names+e1->name_offset;
    s2=entry_names+e2->name_offset;

    do
    {
        c1=*(s1++);
        c2=*(s2++);

        if((c1>='A')&&(c1<='Z'))c1-='A'-'a';
        if((c2>='A')&&(c2<='Z'))c2-='A'-'a';

        #warning check numbers as isolated units
    }
    while((c1==c2)&&(c1)&&(c2));

    return (c1-c2);
}



#warning break following function up
static void load_file_search_directory(file_search_instance * fsi)
{
    load_file_search_directory_entries(fsi);///need different versions for different implementations

    qsort(fsi->entries,(size_t)fsi->entry_count,sizeof(widget_file_search_entry),file_search_entry_comparison);

    uint32_t i;
    if(fsi->sfsd->types)for(i=0;i<fsi->entry_count;i++)set_entry_type(fsi,i);

    populate_file_search_buttons(fsi);
}

file_search_instance * create_file_search_instance(shared_file_search_data * sfsd,int * type_filters,char ** export_formats)
{
    file_search_instance * fsi=malloc(sizeof(file_search_instance));

    fsi->next=sfsd->first;
    sfsd->first=fsi;

    fsi->sfsd=sfsd;




    fsi->directory_buffer_size=4;
    fsi->directory_buffer=malloc(sizeof(char)*fsi->directory_buffer_size);

    //if(directory==NULL)directory=getenv("HOME");

    set_file_search_directory(fsi,sfsd->directory_buffer);

    fsi->type_filters=type_filters;
    fsi->active_type_filter=-1;
    //fsi->contents_changed=false;
    //fsi->button_func=button_func;
    fsi->export_formats=export_formats;
    fsi->active_export_format=0;
//    if(export_formats) fsi->active_export_format=0;
//    else fsi->active_export_format=-1;


    fsi->entry_name_data=malloc(sizeof(char)*4096);
    fsi->entry_name_data_space=4096;
    fsi->entry_name_data_count=0;

    fsi->entries=malloc(sizeof(widget_file_search_entry));
    fsi->entry_space=1;
    fsi->entry_count=0;

    fsi->file_box=NULL;
    fsi->selected_entry=NULL;
    fsi->enterbox=NULL;
    fsi->overwrite_popup=NULL;
    fsi->error_popup=NULL;
    fsi->type_filter_popup=NULL;
    fsi->type_export_popup=NULL;

    fsi->error_type=FILE_SEARCH_NO_ERROR;

    fsi->action_function=NULL;
    fsi->action_data=NULL;

    fsi->end_function=NULL;
    fsi->end_data=NULL;


    fsi->free_action_data=false;
    fsi->free_end_data=false;
    fsi->show_hidden=false;
    fsi->show_directories=true;
    fsi->show_misc_files=true;
    fsi->show_control_entries=false;


    fsi->shared_data_update_count=0;

    return fsi;
}


void set_file_search_instance_action_variables(file_search_instance * fsi,file_search_action_function action_function,void * action_data,bool free_action_data)
{
    fsi->action_function=action_function;
    fsi->action_data=action_data;
    fsi->free_action_data=free_action_data;
}

void set_file_search_instance_end_variables(file_search_instance * fsi,file_search_end_function end_function,void * end_data,bool free_end_data)
{
    fsi->end_function=end_function;
    fsi->end_data=end_data;
    fsi->free_end_data=free_end_data;
}


void set_file_search_instance_settings(file_search_instance * fsi,bool show_hidden,bool show_directories,bool show_misc_files,bool show_control_entries)///move to declaration of fsi ?????
{
    fsi->show_hidden=show_hidden;
    fsi->show_directories=show_directories;
    fsi->show_misc_files=show_misc_files;
    fsi->show_control_entries=show_control_entries;

    if((!show_misc_files)&&(fsi->type_filters)&&(fsi->sfsd->types))fsi->active_type_filter=fsi->type_filters[0];
}




widget * create_file_search_file_list(file_search_instance * fsi)
{
    fsi->file_box=create_contiguous_box(WIDGET_VERTICAL,8);
    set_contiguous_box_default_contained_dimensions(fsi->file_box,file_search_button_widget_w,file_search_button_widget_h);
    return fsi->file_box;
}



void file_search_up_button_func(widget * w)
{
    file_search_instance * fsi=w->button.data;
    if(fsi->show_directories)
    {
        remove_file_search_layer(fsi);
        load_file_search_directory(fsi);
    }
}

widget * create_file_search_up_button(file_search_instance * fsi)
{
    return create_icon_button("directory_up",fsi,false,file_search_up_button_func);
}



void file_search_toggle_hidden_button_func(widget * w)
{
    file_search_instance * fsi=w->button.data;
    fsi->show_hidden^=1;
    populate_file_search_buttons(fsi);
}

widget * create_file_search_toggle_hidden_button(file_search_instance * fsi)
{
    return create_icon_button("directory_hidden",fsi,false,file_search_toggle_hidden_button_func);
}



void file_search_refresh_button_func(widget * w)
{
    file_search_instance * fsi=w->button.data;
    load_file_search_directory(fsi);
}

widget * create_file_search_refresh_button(file_search_instance * fsi)
{
    return create_icon_button("directory_refresh",fsi,false,file_search_refresh_button_func);
}





static void file_search_text_bar_func(widget * w)
{
    file_search_instance * fsi=w->text_bar.data;
//    puts("aa");
//    puts(fsi->directory_buffer);
    w->text_bar.text=fsi->directory_buffer;
//    puts("bb");
}

widget * create_file_search_text_bar(file_search_instance * fsi)
{
    return create_dynamic_text_bar(24,file_search_text_bar_func,false,fsi,false,WIDGET_TEXT_RIGHT_ALIGNED);
}








//widget * create_file_search_scrollbar(file_search_instance * fsi)
//{
//    return create_contiguous_box_scrollbar(fsi->file_search.file_box);
//}



static void file_search_enterbox_func(widget * w)
{
    file_search_instance * fsi=w->enterbox.data;

    file_search_func(fsi,true);
}

static void file_search_enterbox_input_func(widget * w)
{
    file_search_instance * fsi=w->enterbox.data;

    widget * current=fsi->file_box->contiguous_box.contained_box->container.first;

//    file_search_file_type * types=fsi->types;
//    uint32_t i;
//
//    char * s=w->enterbox.text;
//    char * t=NULL;
//
//    while(*s)
//    {
//        if(*s=='.')t=s;
//        s++;
//    }
//
//    if(t)
//    {
//        t++;
//
//        for(i=0;types[i].type_extension;i++)
//        {
//            if(str_lower_cmp(types[i].type_extension,t))
//            {
//                fsi->selected_type=i+1;
//                return;
//            }
//        }
//    }

    while(current)
    {
        /// dont allow selection if type isnt appropriate (equivalent to not listed in contiguous box files)
        #warning using only displayed files works for this but relies on contiguous box existing, for file search that doesnt use entries and file_search_file_currently_valid instead

        if(strcmp(w->enterbox.text,fsi->entry_name_data+fsi->entries[current->button.index].name_offset)==0)
        {
            //if(fsi->entries[current->button.index].type)
            ensure_widget_in_contiguous_box_is_visible(current,fsi->file_box);
            fsi->selected_entry=current;
            return;
        }

        current=current->base.next;
    }

    fsi->selected_entry=NULL;
}

widget * create_file_search_enterbox(file_search_instance * fsi)
{
    widget * w=create_enterbox(255,255,16,NULL,file_search_enterbox_func,fsi,NULL,false,false);

    set_enterbox_action_upon_input(w,file_search_enterbox_input_func);

    fsi->enterbox=w;

    return w;
}



void file_search_cancel_button_func(widget * w)
{
    file_search_instance * fsi=w->button.data;

    fsi->end_function(fsi);
    clear_all_file_search_selection_information(fsi);
}

widget * create_file_search_cancel_button(file_search_instance * fsi,char * text)
{
    return create_text_button(text,fsi,false,file_search_cancel_button_func);
}



void file_search_accept_button_func(widget * w)
{
    file_search_func(w->button.data,false);
}

widget * create_file_search_accept_button(file_search_instance * fsi,char * text)
{
    return create_text_button(text,fsi,false,file_search_accept_button_func);
}






void file_search_overwrite_accept_button_func(widget * w)
{
    file_search_instance * fsi=w->button.data;

    char * selected_text=NULL;
    char * enterbox_text=NULL;
    char * used_text=NULL;
    int prev_length=strlen(fsi->directory_buffer);

    if(fsi->selected_entry)selected_text=fsi->entry_name_data+fsi->entries[fsi->selected_entry->button.index].name_offset;
    if((fsi->enterbox)&&(strlen(fsi->enterbox->enterbox.text)))enterbox_text=fsi->enterbox->enterbox.text;

    if(selected_text) used_text=enterbox_text;//concatenate_file_search_directory(fsi,selected_text);
    else if(enterbox_text) used_text=enterbox_text;//concatenate_file_search_directory(fsi,enterbox_text);

    concatenate_file_search_directory(fsi,used_text);

    if((check_if_regular_file(fsi->directory_buffer))||(!check_if_file_exists(fsi->directory_buffer)))///overwrite if regular file or if file was deleted since overwrite was initiated (file no longer exists)
    {
        fsi->error_type=fsi->action_function(fsi);
    }
    else
    {
        ///invalid : exists but is not a regular file, ergo should not overwrite, revert, show error if applicable and return
        puts("ERROR: INVALID FILE SELECTED");
        fsi->error_type=FILE_SEARCH_ERROR_INVALID_FILE;
    }

    fsi->directory_buffer[prev_length]='\0';
    load_file_search_directory(fsi);/// in case new file was added

    if(fsi->overwrite_popup)toggle_exclusive_popup(fsi->overwrite_popup);

    if(fsi->error_type)
    {
        if(fsi->error_popup)toggle_exclusive_popup(fsi->error_popup);
    }
    else
    {
        fsi->end_function(fsi);
        clear_all_file_search_selection_information(fsi);/// selected used above to update shared_file_search_data

        #warning update shared_file_search_data, selected_file and active_folder, set selected_file to used_file, active_folder to fsi->directory_buffer (file was cleared from this above)
        ///set update count to be same as shared_file_search_data after doing this
    }
}

void file_search_overwrite_cancel_button_func(widget * w)
{
    file_search_instance * fsi=w->button.data;

    if(fsi->overwrite_popup)toggle_exclusive_popup(fsi->overwrite_popup);
}

void create_file_search_overwrite_popup(widget * menu_widget,file_search_instance * fsi,widget * external_alignment_widget,char * overwrite_message,char * affirmative_text,char * negative_text)
{
//    widget *box_0,*box_1;
//
//    fsi->overwrite_popup=add_child_to_parent(menu_widget,create_popup(WIDGET_POSITIONING_CENTRED,false));
//
//    box_0=add_child_to_parent(add_child_to_parent(fsi->overwrite_popup,create_panel()),create_box(WIDGET_VERTICAL,WIDGET_EVENLY_DISTRIBUTED));
//    add_child_to_parent(box_0,create_static_text_bar(0,overwrite_message,WIDGET_TEXT_LEFT_ALIGNED));
//    box_1=add_child_to_parent(box_0,create_box(WIDGET_HORIZONTAL,WIDGET_ALL_SAME_DISTRIBUTED));
//    add_child_to_parent(box_1,create_text_button(affirmative_text,fsi,file_search_overwrite_accept_button_func));
//    add_child_to_parent(box_1,create_text_button(negative_text,fsi,file_search_overwrite_cancel_button_func));
//
//    set_popup_alignment_widgets(fsi->overwrite_popup,box_0,external_alignment_widget);

    widget *box_0,*box_1,*box_2,*box_3;

    fsi->overwrite_popup=add_child_to_parent(menu_widget,create_popup(WIDGET_POSITIONING_CENTRED,false));

    box_0=add_child_to_parent(add_child_to_parent(fsi->overwrite_popup,create_panel()),create_box(WIDGET_VERTICAL,WIDGET_EVENLY_DISTRIBUTED));
    add_child_to_parent(box_0,create_static_text_bar(0,overwrite_message,WIDGET_TEXT_LEFT_ALIGNED));
    box_1=add_child_to_parent(box_0,create_box(WIDGET_HORIZONTAL,WIDGET_EVENLY_DISTRIBUTED));
    add_child_to_parent(box_1,create_empty_widget(0,0));
    box_2=add_child_to_parent(box_1,create_box(WIDGET_HORIZONTAL,WIDGET_LAST_DISTRIBUTED));
    box_3=add_child_to_parent(box_2,create_box(WIDGET_HORIZONTAL,WIDGET_ALL_SAME_DISTRIBUTED));
    add_child_to_parent(box_2,create_empty_widget(0,0));
    add_child_to_parent(box_3,create_text_button(affirmative_text,fsi,false,file_search_overwrite_accept_button_func));
    add_child_to_parent(box_3,create_text_button(negative_text,fsi,false,file_search_overwrite_cancel_button_func));

    set_popup_alignment_widgets(fsi->overwrite_popup,box_0,external_alignment_widget);
}








void error_popup_text_box_func(widget * w)
{
    file_search_instance * fsi=w->text_bar.data;

    if(fsi->error_type) w->text_bar.text=fsi->sfsd->error_messages[fsi->error_type];
    else w->text_bar.text=NULL;
}

void error_popup_button_func(widget * w)
{
    file_search_instance * fsi=w->button.data;

    if(fsi->error_popup)toggle_exclusive_popup(fsi->error_popup);

    fsi->error_type=FILE_SEARCH_NO_ERROR;
}

void create_file_search_error_popup(widget * menu_widget,file_search_instance * fsi,widget * external_alignment_widget,char * affirmative_text)
{
    #warning all triggers should be checked to see that they work as intended

    widget *box_0,*box_1,*box_2;

    fsi->error_popup=add_child_to_parent(menu_widget,create_popup(WIDGET_POSITIONING_CENTRED,false));

    box_0=add_child_to_parent(add_child_to_parent(fsi->error_popup,create_panel()),create_box(WIDGET_VERTICAL,WIDGET_EVENLY_DISTRIBUTED));
    add_child_to_parent(box_0,create_dynamic_text_bar(0,error_popup_text_box_func,false,fsi,false,WIDGET_TEXT_LEFT_ALIGNED));
    box_1=add_child_to_parent(box_0,create_box(WIDGET_HORIZONTAL,WIDGET_EVENLY_DISTRIBUTED));
    add_child_to_parent(box_1,create_empty_widget(0,0));
    box_2=add_child_to_parent(box_1,create_box(WIDGET_HORIZONTAL,WIDGET_LAST_DISTRIBUTED));
    add_child_to_parent(box_2,create_text_button(affirmative_text,fsi,false,error_popup_button_func));
    add_child_to_parent(box_2,create_empty_widget(0,0));

    set_popup_alignment_widgets(fsi->error_popup,box_0,external_alignment_widget);
}















static void file_search_filter_type_button_widget_render(overlay_theme * theme,widget * w,int x_off,int y_off,cvm_overlay_element_render_buffer * erb,rectangle bounds)
{
    file_search_instance * fsi=w->button.data;

    char * text="All Files";
    if(fsi->active_type_filter>=0) text=fsi->sfsd->types[fsi->active_type_filter].name;
    if(widget_active(fsi->type_filter_popup))text=NULL;

    rectangle r=rectangle_add_offset(w->base.r,x_off,y_off);
	theme->h_bar_render(r,w->base.status,theme,erb,bounds,OVERLAY_MAIN_COLOUR_);

    r=overlay_simple_text_rectangle(r,theme->font_.glyph_size,theme->h_bar_text_offset);
    rectangle b=get_rectangle_overlap(r,bounds);
    if(rectangle_has_positive_area(b))overlay_render_text_simple(erb,&theme->font_,text,r.x1,r.y1,b,OVERLAY_TEXT_COLOUR_0_);
}

static widget * file_search_filter_type_button_widget_select(overlay_theme * theme,widget * w,int x_in,int y_in)
{
    if(theme->h_bar_select(rectangle_subtract_offset(w->base.r,x_in,y_in),w->base.status,theme))return w;
    return NULL;
}

static void file_search_filter_type_button_widget_min_w(overlay_theme * theme,widget * w)
{
    file_search_instance * fsi=w->button.data;
    if(fsi->show_misc_files) w->base.min_w=overlay_size_text_simple(&theme->font_,"All Files");//calculate_text_length(theme,"All Files",0);
    else w->base.min_w=0;
    int width;

    file_search_file_type * types=fsi->sfsd->types;
    int * filter=fsi->type_filters;

    if((types)&&(filter))while(*filter >= 0)
    {
        width=overlay_size_text_simple(&theme->font_,types[*filter].name);//calculate_text_length(theme,types[*filter].name,0);

        if(width>w->base.min_w)w->base.min_w=width;

        filter++;
    }

    w->base.min_w+=2*theme->h_bar_text_offset;
}

static void file_search_filter_type_button_widget_min_h(overlay_theme * theme,widget * w)
{
    w->base.min_h = theme->base_unit_h;
}


static widget_appearence_function_set file_search_filter_type_button_appearence_functions=
(widget_appearence_function_set)
{
    .render =   file_search_filter_type_button_widget_render,
    .select =   file_search_filter_type_button_widget_select,
    .min_w  =   file_search_filter_type_button_widget_min_w,
    .min_h  =   file_search_filter_type_button_widget_min_h,
    .set_w  =   blank_widget_set_w,
    .set_h  =   blank_widget_set_h
};

static void file_search_filter_type_button_func(widget * w)
{
    file_search_instance * fsi=w->button.data;

    if(fsi->type_filter_popup)toggle_auto_close_popup(fsi->type_filter_popup);
}

static void file_search_filter_type_list_button_func(widget * w)
{
    file_search_instance * fsi=w->button.data;
    if(fsi->active_type_filter!=w->button.index)
    {
        fsi->active_type_filter=w->button.index;
        fsi->type_filter_popup->popup.internal_alignment_widget=w;
        populate_file_search_buttons(fsi);
    }
}

widget * create_file_search_filter_type_button(widget * menu_widget,file_search_instance * fsi)
{
    fsi->type_filter_popup=add_child_to_parent(menu_widget,create_popup(WIDGET_POSITIONING_CENTRED,true));

    widget * cb=add_child_to_parent(add_child_to_parent(fsi->type_filter_popup,create_panel()),create_contiguous_box(WIDGET_VERTICAL,0));

    widget *tmp,*internal=NULL;



    if(fsi->show_misc_files)
    {
        internal=add_child_to_parent(cb,create_contiguous_text_highlight_toggle_button("All Files",fsi,false,file_search_filter_type_list_button_func,is_potential_interaction_widget));
        internal->button.index=-1;
    }

    file_search_file_type * types=fsi->sfsd->types;
    int * filter=fsi->type_filters;

    if((types)&&(filter))while(*filter >= 0)
    {
        tmp=add_child_to_parent(cb,create_contiguous_text_highlight_toggle_button(types[*filter].name,fsi,false,file_search_filter_type_list_button_func,is_potential_interaction_widget));
        tmp->button.index=*filter;
        if(internal==NULL)internal=tmp;

        filter++;
    }

    widget * button=create_button(fsi,file_search_filter_type_button_func,false);
    button->base.status&= ~WIDGET_CLOSE_POPUP_TREE;

    set_popup_alignment_widgets(fsi->type_filter_popup,internal,button);

    button->base.appearence_functions=&file_search_filter_type_button_appearence_functions;

    return button;
}




static void file_search_export_type_button_widget_render(overlay_theme * theme,widget * w,int x_off,int y_off,cvm_overlay_element_render_buffer * erb,rectangle bounds)
{
    file_search_instance * fsi=w->button.data;

    char * text=NULL;

    if(fsi->export_formats) text=fsi->export_formats[fsi->active_export_format];

    rectangle r=rectangle_add_offset(w->base.r,x_off,y_off);
	theme->h_bar_render(r,w->base.status,theme,erb,bounds,OVERLAY_MAIN_COLOUR_);

    r=overlay_simple_text_rectangle(r,theme->font_.glyph_size,theme->h_bar_text_offset);
    rectangle b=get_rectangle_overlap(r,bounds);
    if(rectangle_has_positive_area(b))overlay_render_text_simple(erb,&theme->font_,text,r.x1,r.y1,b,OVERLAY_TEXT_COLOUR_0_);
}

static widget * file_search_export_type_button_widget_select(overlay_theme * theme,widget * w,int x_in,int y_in)
{
    if(theme->h_bar_select(rectangle_subtract_offset(w->base.r,x_in,y_in),w->base.status,theme))return w;
    return NULL;
}

static void file_search_export_type_button_widget_min_w(overlay_theme * theme,widget * w)
{
    file_search_instance * fsi=w->button.data;
    w->base.min_w=0;
    int width;

    char ** formats=fsi->export_formats;

    if(formats)while(*formats)
    {
        width=overlay_size_text_simple(&theme->font_,*formats);//calculate_text_length(theme,*formats,0);

        if(width>w->base.min_w)w->base.min_w=width;

        formats++;
    }

    w->base.min_w+=2*theme->h_bar_text_offset;
}

static void file_search_export_type_button_widget_min_h(overlay_theme * theme,widget * w)
{
    w->base.min_h = theme->base_unit_h;
}


static widget_appearence_function_set file_search_export_type_button_appearence_functions=
(widget_appearence_function_set)
{
    .render =   file_search_export_type_button_widget_render,
    .select =   file_search_export_type_button_widget_select,
    .min_w  =   file_search_export_type_button_widget_min_w,
    .min_h  =   file_search_export_type_button_widget_min_h,
    .set_w  =   blank_widget_set_w,
    .set_h  =   blank_widget_set_h
};

static void file_search_export_type_button_func(widget * w)
{
    file_search_instance * fsi=w->button.data;

    if(fsi->type_export_popup)toggle_auto_close_popup(fsi->type_export_popup);
}

static void file_search_export_type_list_button_func(widget * w)
{
    file_search_instance * fsi=w->button.data;
    if(fsi->active_export_format!=w->button.index)
    {
        fsi->active_export_format=w->button.index;
        fsi->type_export_popup->popup.internal_alignment_widget=w;
    }
}

widget * create_file_search_export_type_button(widget * menu_widget,file_search_instance * fsi)
{
    fsi->type_export_popup=add_child_to_parent(menu_widget,create_popup(WIDGET_POSITIONING_CENTRED,true));

    widget * cb=add_child_to_parent(add_child_to_parent(fsi->type_export_popup,create_panel()),create_contiguous_box(WIDGET_VERTICAL,0));


    widget *tmp,*internal=NULL;
    char ** formats=fsi->export_formats;
    int i;

    if(formats)for(i=0;formats[i];i++)
    {
        tmp=add_child_to_parent(cb,create_contiguous_text_highlight_toggle_button(formats[i],fsi,false,file_search_export_type_list_button_func,is_potential_interaction_widget));
        tmp->button.index=i;
        if(i==fsi->active_export_format)internal=tmp;
    }

    widget * button=create_button(fsi,file_search_export_type_button_func,false);
    button->base.status&= ~WIDGET_CLOSE_POPUP_TREE;

    set_popup_alignment_widgets(fsi->type_export_popup,internal,button);

    button->base.appearence_functions=&file_search_export_type_button_appearence_functions;

    return button;
}











static void set_shared_file_search_data_directory(shared_file_search_data * sfsd,char * directory)
{
    if(directory==NULL)directory=getenv("HOME");
    if(directory==NULL)directory="";
    int length=strlen(directory);

    while((length+1) >= sfsd->directory_buffer_size)sfsd->directory_buffer=realloc(sfsd->directory_buffer,sizeof(char)*(sfsd->directory_buffer_size*=2));
    strcpy(sfsd->directory_buffer,directory);

    while((length>0)&&(sfsd->directory_buffer[length-1]=='/'))length--;

    sfsd->directory_buffer[length]='/';
    sfsd->directory_buffer[length+1]='\0';
}


shared_file_search_data * create_shared_file_search_data(char * initial_directory,file_search_file_type * types)
{
    shared_file_search_data * sfsd=malloc(sizeof(shared_file_search_data));

    sfsd->update_count=1;

    sfsd->types=types;

    sfsd->directory_buffer_size=4;
    sfsd->directory_buffer=malloc(sizeof(char)*sfsd->directory_buffer_size);

    //sfsd->filename_buffer_size=4;
    //sfsd->filename_buffer=malloc(sizeof(char)*sfsd->filename_buffer_size);

    sfsd->error_count=FILE_SEARCH_ERROR_COUNT;
    sfsd->error_messages=malloc(sizeof(char*)*FILE_SEARCH_ERROR_COUNT);
    sfsd->error_messages[FILE_SEARCH_NO_ERROR]=NULL;
    sfsd->error_messages[FILE_SEARCH_ERROR_INVALID_FILE]=strdup("Invalid file selected");
    sfsd->error_messages[FILE_SEARCH_ERROR_INVALID_DIRECTORY]=strdup("Invalid directory selected");
    sfsd->error_messages[FILE_SEARCH_ERROR_CAN_NOT_CHANGE_DIRECTORY]=strdup("Directory change not permitted");
    sfsd->error_messages[FILE_SEARCH_ERROR_CAN_NOT_SAVE_FILE]=strdup("Could not save file");
    sfsd->error_messages[FILE_SEARCH_ERROR_CAN_NOT_LOAD_FILE]=strdup("Could not load file");

    sfsd->first=NULL;

    set_shared_file_search_data_directory(sfsd,initial_directory);

    return sfsd;
}

void delete_shared_file_search_data(shared_file_search_data * sfsd)
{
    int i;

    free(sfsd->directory_buffer);
    //free(sfsd->filename_buffer);

    file_search_instance *tmp,*fsi;
    fsi=sfsd->first;

    while(fsi)
    {
        free(fsi->directory_buffer);
        free(fsi->entries);
        free(fsi->entry_name_data);
        if(fsi->free_action_data)free(fsi->action_data);
        if(fsi->free_end_data)free(fsi->end_data);

        tmp=fsi->next;
        free(fsi);
        fsi=tmp;
    }

    for(i=0;i<sfsd->error_count;i++)
    {
        free(sfsd->error_messages[i]);
    }
    free(sfsd->error_messages);

    free(sfsd);
}

void update_shared_file_search_data_directory_use(shared_file_search_data * sfsd)
{
    file_search_instance *fsi=sfsd->first;

    while(fsi)
    {
        if(fsi->shared_data_update_count!=sfsd->update_count)
        {
            #warning set/change directory appropriately
            #warning if a file in that directory matches filename_buffer select it

            load_file_search_directory(fsi);
            fsi->shared_data_update_count=sfsd->update_count;
        }

        fsi=fsi->next;
    }
}
