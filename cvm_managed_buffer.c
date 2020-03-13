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

void create_managed_buffer(gl_functions * glf,managed_buffer * mb,GLenum usage,uint32_t size_delta)
{
    mb->size=0;
    mb->binding=GL_ARRAY_BUFFER;
    mb->usage=usage;
    mb->size_delta=size_delta;

    mb->buffer=0;
    glf->glGenBuffers(1,&mb->buffer);

    mb->mapped=false;
}

void create_managed_texture_buffer(gl_functions * glf,managed_buffer * mb,GLenum format,GLenum usage,uint32_t size_delta)
{
    mb->size=size_delta;
    mb->binding=GL_TEXTURE_BUFFER;
    mb->texture_buffer_format=format;
    mb->usage=usage;
    mb->size_delta=size_delta;

    mb->buffer=0;
    glf->glGenBuffers(1,&mb->buffer);
    glf->glBindBuffer(mb->binding,mb->buffer);
    glf->glBufferData(mb->binding,size_delta,NULL,mb->usage);
    glf->glBindBuffer(mb->binding,0);

    mb->texture_buffer=0;
    glf->glGenTextures(1,&mb->texture_buffer);

    glf->glBindTexture(mb->binding,mb->texture_buffer);
    glf->glTexBuffer(mb->binding,mb->texture_buffer_format,mb->buffer);
    glf->glBindTexture(mb->binding,0);

    mb->mapped=false;
}

void delete_managed_buffer(gl_functions * glf,managed_buffer * mb)
{
    ///Finish
}

void ensure_managed_buffer_size(gl_functions * glf,managed_buffer * mb,uint32_t size)
{
    if(size>mb->size)
    {
        mb->size=(((size-1)/mb->size_delta)+2)*mb->size_delta;

        glf->glBindBuffer(mb->binding,mb->buffer);
        glf->glBufferData(mb->binding,mb->size,NULL,mb->usage);
        glf->glBindBuffer(mb->binding,0);
    }
}

void upload_managed_buffer_data(gl_functions * glf,managed_buffer * mb,uint32_t size,void * data)
{
    if(size>mb->size)
    {
        mb->size=(((size-1)/mb->size_delta)+2)*mb->size_delta;

        glf->glBindBuffer(mb->binding,mb->buffer);
        glf->glBufferData(mb->binding,mb->size,NULL,mb->usage);
        glf->glBufferSubData(mb->binding,0,size,data);
        glf->glBindBuffer(mb->binding,0);
    }
    else
    {
        glf->glBindBuffer(mb->binding,mb->buffer);
        glf->glBufferSubData(mb->binding,0,size,data);
        glf->glBindBuffer(mb->binding,0);
    }
}

void * map_managed_buffer_write(gl_functions * glf,managed_buffer * mb,uint32_t size)///update to only map buffer amount that is needed:      ,uint32_t required_size);
{
    void * ptr;

    if(size==0)
    {
        printf("error: trying to map empty managed_buffer\n");
        return NULL;
    }

    if(mb->mapped)
    {
        printf("error: trying to map mapped managed_buffer\n");
    }

    glf->glBindBuffer(mb->binding, mb->buffer);
    ptr=glf->glMapBufferRange(mb->binding,0,size,GL_MAP_WRITE_BIT);
    glf->glBindBuffer(mb->binding,0);
    mb->mapped=true;

    return ptr;
}

void * get_correct_managed_buffer_write_map(gl_functions * glf,managed_buffer * mb,uint32_t size)
{
    ensure_managed_buffer_size(glf,mb,size);
    return map_managed_buffer_write(glf,mb,size);
}

void unmap_managed_buffer(gl_functions * glf,managed_buffer * mb)
{
    if(mb->mapped)
    {
        glf->glBindBuffer(mb->binding, mb->buffer);
        glf->glUnmapBuffer(mb->binding);
        glf->glBindBuffer(mb->binding,0);

        mb->mapped=false;
    }
    else
    {
        printf("error: trying to unmap unmapped managed_buffer\n");
    }
}

void bind_managed_buffer(gl_functions * glf,managed_buffer * mb)
{
    glf->glBindBuffer(mb->binding, mb->buffer);
}

void unbind_managed_buffer(gl_functions * glf,managed_buffer * mb)
{
    glf->glBindBuffer(mb->binding, 0);
}

void bind_managed_texture_buffer(gl_functions * glf,managed_buffer * mb,GLenum texture_id)
{
    ensure_managed_buffer_size(glf,mb,4);

    glf->glActiveTexture(texture_id);
    glf->glBindTexture(mb->binding,mb->texture_buffer);
}

void unbind_managed_texture_buffer(gl_functions * glf,managed_buffer * mb,GLenum texture_id)
{
    glf->glActiveTexture(texture_id);
    glf->glBindTexture(mb->binding,0);
}

void bind_managed_texture_buffer_as_image(gl_functions * glf,managed_buffer * mb,GLuint unit,GLenum access)
{
    glf->glBindImageTexture(unit,mb->texture_buffer,0,GL_FALSE,0,access,mb->texture_buffer_format);
}










