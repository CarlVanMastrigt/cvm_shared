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

void create_managed_buffer(managed_buffer * mb,GLenum usage,uint32_t size_delta)
{
    mb->size=0;
    mb->binding=GL_ARRAY_BUFFER;
    mb->usage=usage;
    mb->size_delta=size_delta;

    mb->buffer=0;
    glGenBuffers_ptr(1,&mb->buffer);

    mb->mapped=false;
}

void create_managed_texture_buffer(managed_buffer * mb,GLenum format,GLenum usage,uint32_t size_delta)
{
    mb->size=size_delta;
    mb->binding=GL_TEXTURE_BUFFER;
    mb->texture_buffer_format=format;
    mb->usage=usage;
    mb->size_delta=size_delta;

    mb->buffer=0;
    glGenBuffers_ptr(1,&mb->buffer);
    glBindBuffer_ptr(mb->binding,mb->buffer);
    glBufferData_ptr(mb->binding,size_delta,NULL,mb->usage);
    glBindBuffer_ptr(mb->binding,0);

    mb->texture_buffer=0;
    glGenTextures(1,&mb->texture_buffer);

    glBindTexture(mb->binding,mb->texture_buffer);
    glTexBuffer_ptr(mb->binding,mb->texture_buffer_format,mb->buffer);
    glBindTexture(mb->binding,0);

    mb->mapped=false;
}

void delete_managed_buffer(managed_buffer * mb)
{
    ///Finish
}

void ensure_managed_buffer_size(managed_buffer * mb,uint32_t size)
{
    if(size>mb->size)
    {
        mb->size=(((size-1)/mb->size_delta)+2)*mb->size_delta;

        glBindBuffer_ptr(mb->binding,mb->buffer);
        glBufferData_ptr(mb->binding,mb->size,NULL,mb->usage);
        glBindBuffer_ptr(mb->binding,0);
    }
}

void upload_managed_buffer_data(managed_buffer * mb,uint32_t size,void * data)
{
    if(size>mb->size)
    {
        mb->size=(((size-1)/mb->size_delta)+2)*mb->size_delta;

        glBindBuffer_ptr(mb->binding,mb->buffer);
        glBufferData_ptr(mb->binding,mb->size,NULL,mb->usage);
        glBufferSubData_ptr(mb->binding,0,size,data);
        glBindBuffer_ptr(mb->binding,0);
    }
    else
    {
        glBindBuffer_ptr(mb->binding,mb->buffer);
        glBufferSubData_ptr(mb->binding,0,size,data);
        glBindBuffer_ptr(mb->binding,0);
    }
}

void * map_managed_buffer_write(managed_buffer * mb,uint32_t size)///update to only map buffer amount that is needed:      ,uint32_t required_size);
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

    glBindBuffer_ptr(mb->binding, mb->buffer);
    ptr=glMapBufferRange_ptr(mb->binding,0,size,GL_MAP_WRITE_BIT);
    glBindBuffer_ptr(mb->binding, 0);
    mb->mapped=true;

    return ptr;
}

void * get_correct_managed_buffer_write_map(managed_buffer * mb,uint32_t size)
{
    ensure_managed_buffer_size(mb,size);
    return map_managed_buffer_write(mb,size);
}

void unmap_managed_buffer(managed_buffer * mb)
{
    if(mb->mapped)
    {
        glBindBuffer_ptr(mb->binding, mb->buffer);
        glUnmapBuffer_ptr(mb->binding);
        glBindBuffer_ptr(mb->binding,0);

        mb->mapped=false;
    }
    else
    {
        printf("error: trying to unmap unmapped managed_buffer\n");
    }
}

void bind_managed_buffer(managed_buffer * mb)
{
    glBindBuffer_ptr(mb->binding, mb->buffer);
}

void unbind_managed_buffer(managed_buffer * mb)
{
    glBindBuffer_ptr(mb->binding, 0);
}

void bind_managed_texture_buffer(managed_buffer * mb,GLenum texture_id)
{
    ensure_managed_buffer_size(mb,4);

    glActiveTexture(texture_id);
    glBindTexture(mb->binding,mb->texture_buffer);
}

void unbind_managed_texture_buffer(managed_buffer * mb,GLenum texture_id)
{
    glActiveTexture(texture_id);
    glBindTexture(mb->binding,0);
}

void bind_managed_texture_buffer_as_image(managed_buffer * mb,GLuint unit,GLenum access)
{
    glBindImageTexture_ptr(unit,mb->texture_buffer,0,GL_FALSE,0,access,mb->texture_buffer_format);
}





















/*void create_managed_ssbo(managed_ssbo * ms,GLenum usage,GLsizeiptr delta_size)
{
    delta_size--;
    delta_size |= delta_size >> 1;
    delta_size |= delta_size >> 2;
    delta_size |= delta_size >> 4;
    delta_size |= delta_size >> 8;
    delta_size |= delta_size >> 16;
    delta_size |= delta_size >> 32;
    delta_size++;

    ms->size=delta_size;
    ms->usage=usage;
    ms->delta_size=delta_size;

    glGenBuffers_ptr(1,&ms->ssbo);
    glBindBuffer_ptr(GL_SHADER_STORAGE_BUFFER,ms->ssbo);
    glBufferData_ptr(GL_SHADER_STORAGE_BUFFER,delta_size,NULL,usage);
    glBindBuffer_ptr(GL_SHADER_STORAGE_BUFFER,0);
}

void ensure_managed_ssbo_size(managed_ssbo * ms,GLsizeiptr size)
{
    if(size>ms->size)
    {
        size--;
        size=((size/ms->delta_size)+2)*ms->delta_size;
        ms->size=size;

        glBindBuffer_ptr(GL_SHADER_STORAGE_BUFFER,ms->ssbo);
        glBufferData_ptr(GL_SHADER_STORAGE_BUFFER,size,NULL,ms->usage);
        glBindBuffer_ptr(GL_SHADER_STORAGE_BUFFER,0);
    }
}

void upload_managed_ssbo_data(managed_ssbo * ms,GLsizeiptr size,void * data)
{
    if(size>ms->size)
    {
        ms->size=(((size-1)/ms->delta_size)+2)*ms->delta_size;

        glBindBuffer_ptr(GL_SHADER_STORAGE_BUFFER,ms->ssbo);
        glBufferData_ptr(GL_SHADER_STORAGE_BUFFER,ms->size,NULL,ms->usage);
        glBufferSubData_ptr(GL_SHADER_STORAGE_BUFFER,0,size,data);
        glBindBuffer_ptr(GL_SHADER_STORAGE_BUFFER,0);
    }
    else
    {
        glBindBuffer_ptr(GL_SHADER_STORAGE_BUFFER,ms->ssbo);
        glBufferSubData_ptr(GL_SHADER_STORAGE_BUFFER,0,size,data);
        glBindBuffer_ptr(GL_SHADER_STORAGE_BUFFER,0);
    }
}

void bind_managed_ssbo(managed_ssbo * ms,GLuint binding_point)
{
    glBindBuffer_ptr(GL_SHADER_STORAGE_BUFFER,ms->ssbo);
    glBindBufferBase_ptr(GL_SHADER_STORAGE_BUFFER,binding_point,ms->ssbo);
    glBindBuffer_ptr(GL_SHADER_STORAGE_BUFFER,0);
}*/















