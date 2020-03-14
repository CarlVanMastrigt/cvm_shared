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

static char * shader_version_and_defines;
static int shader_version_and_defines_length;

void set_shader_version_and_defines(char * svad)
{
    shader_version_and_defines=malloc(strlen(svad)+1);
    strcpy(shader_version_and_defines,svad);

    shader_version_and_defines_length=strlen(svad);
}

void free_shader_version_and_defines(void)
{
    free(shader_version_and_defines);
}

static int load_shader_file(const char * filename,char ** code,int * lengths)///returns number of new files
{
    char c;
    FILE * f_in;
    int i,new_files=0,length=0;




    f_in=fopen(filename,"r");c='a';
    if(f_in==NULL){printf(">could not load %s\n",filename);exit(-1);}
    while(c!=EOF)
    {
        c=fgetc(f_in);
        length++;
    }
    length--;
    fclose(f_in);

    char * new_string=malloc(sizeof(char)*length);



    length=0;
    char line[65536];
    char fn[256];
    f_in=fopen(filename,"r");
    if(f_in==NULL){printf(">could not load %s\n",filename);exit(-1);}
    while(fgets(line,65536,f_in)!=NULL)
    {
        i=0;
        if((line[0]=='#')&&(line[1]=='i')&&(line[2]=='n')&&(line[3]=='c')&&(line[4]=='l')&&(line[5]=='u')&&(line[6]=='d')&&(line[7]=='e'))
        {
            while(line[i+10])
            {
                fn[i]=line[i+10];
                i++;
            }
            fn[i-2]='\0';

            new_files+=load_shader_file(fn,code+new_files,lengths+new_files);
        }
        else while(line[i]) new_string[length++]=line[i++];
    }
    fclose(f_in);

    lengths[new_files]=length;
    code[new_files]=new_string;


    return new_files+1;
}

static int load_shader(gl_functions * glf,GLuint shader_object,const char * file,const char * defines)
{
    int i,part_count;

    int length[MAX_SHADER_INCLUDES+2];
    char * code[MAX_SHADER_INCLUDES+2];


    part_count=0;

    length[part_count]=shader_version_and_defines_length;
    code[part_count++]=shader_version_and_defines;

    if(defines!=NULL)
    {
        length[part_count]=0;
        while(defines[length[part_count]])length[part_count]++;
        code[part_count++]=(char*)defines;
    }

    part_count+=load_shader_file(file,code+part_count,length+part_count);



    glf->glShaderSource(shader_object,part_count,(const GLchar**)code,length);
    glf->glCompileShader(shader_object);
    GLint sucess;


    glf->glGetShaderiv(shader_object, GL_COMPILE_STATUS, &sucess);

    if (!sucess)
    {
        printf("\nerror>  %s\n",file);
        char info[65536];
        glf->glGetShaderInfoLog(shader_object,65536,NULL,info);
        fprintf(stderr,"\nError compiling shader : '%s'\n",info);
        exit(1);
    }

    for(i=1;i<part_count;i++) if(code[i] != defines) free(code[i]);

    return 0;
}



GLuint initialise_shader_program(gl_functions * glf,const char * defines,const char * vert_file,const char * geom_file,const char * frag_file)
{
    GLuint sp;
    GLuint vs,gs,fs;

    sp=glf->glCreateProgram();


    if(vert_file!=NULL)
    {
        vs=glf->glCreateShader(GL_VERTEX_SHADER);
        load_shader(glf,vs,vert_file,defines);
        glf->glAttachShader(sp,vs);
    }

    if(geom_file!=NULL)
    {
        gs=glf->glCreateShader(GL_GEOMETRY_SHADER);
        load_shader(glf,gs,geom_file,defines);
        glf->glAttachShader(sp,gs);
    }

    if(frag_file!=NULL)
    {
        fs=glf->glCreateShader(GL_FRAGMENT_SHADER);
        load_shader(glf,fs,frag_file,defines);
        glf->glAttachShader(sp,fs);
    }



    GLint success = 0;
    GLchar info_log[65536];



    glf->glLinkProgram(sp);
    glf->glGetProgramiv(sp, GL_LINK_STATUS, &success);
    if(!success)
    {
        glf->glGetProgramInfoLog(sp, sizeof(info_log),NULL,info_log);
        fprintf(stderr,"Error linking shader program: '%s'\n",info_log);
        exit(1);
    }

    glf->glValidateProgram(sp);
    glf->glGetProgramiv(sp, GL_VALIDATE_STATUS, &success);
    if(!success)
    {
        glf->glGetProgramInfoLog(sp, sizeof(info_log),NULL,info_log);
        fprintf(stderr,"Invalid shader program: '%s'\n",info_log);
        exit(1);
    }

    if(vert_file!=NULL)
    {
        glf->glDetachShader(sp,vs);
        glf->glDeleteShader(vs);
    }

    if(geom_file!=NULL)
    {
        glf->glDetachShader(sp,gs);
        glf->glDeleteShader(gs);
    }

    if(frag_file!=NULL)
    {
        glf->glDetachShader(sp,fs);
        glf->glDeleteShader(fs);
    }

    return sp;
}






/**

///=== Surfaces

int initialise_colour_surfacess()
{
    return 0;
}

int activate_colour_surfacess()
{
    return 0;
}

int initialise_shadow_surfaces()
{
    return 0;
}

int activate_shadow_surfaces()
{
    return 0;
}
*/
