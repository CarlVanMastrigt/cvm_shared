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

#ifndef CVM_SHARED_H
#include "cvm_shared.h"
#endif

#ifndef CVM_MANAGED_BUFFER_H
#define CVM_MANAGED_BUFFER_H


///@todo managed_buffer needs cleanup and renaming

typedef struct managed_buffer
{
    GLuint buffer;
    GLuint texture_buffer;
    uint32_t size;
    uint32_t size_delta;
    GLenum binding;
    GLenum texture_buffer_format;
    GLenum usage;
    bool mapped;
}
managed_buffer;

///change to instance buffer or w/e

void create_managed_buffer(managed_buffer * mb,GLenum usage,uint32_t size_delta);
void create_managed_texture_buffer(managed_buffer * mb,GLenum format,GLenum usage,uint32_t size_delta);
void delete_managed_buffer(managed_buffer * mb);
void ensure_managed_buffer_size(managed_buffer * mb,uint32_t size);
void upload_managed_buffer_data(managed_buffer * mb,uint32_t size,void * data);
void * map_managed_buffer_write(managed_buffer * mb,uint32_t size);
void * get_correct_managed_buffer_write_map(managed_buffer * mb,uint32_t size);
void unmap_managed_buffer(managed_buffer * mb);
void bind_managed_buffer(managed_buffer * mb);
void unbind_managed_buffer(managed_buffer * mb);
void bind_managed_texture_buffer(managed_buffer * mb,GLenum texture_id);
void unbind_managed_texture_buffer(managed_buffer * mb,GLenum texture_id);
void bind_managed_texture_buffer_as_image(managed_buffer * mb,GLuint unit,GLenum access);


/*typedef struct managed_ssbo
{
    GLuvoid ssbo;
    GLsizeiptr size;
    GLenum usage;

    GLsizeiptr delta_size;
}
managed_ssbo;

void create_managed_ssbo(managed_ssbo * ms,GLenum usage,GLsizeiptr delta_size);
void ensure_managed_ssbo_size(managed_ssbo * ms,GLsizeiptr size);
void upload_managed_ssbo_data(managed_ssbo * ms,GLsizeiptr size,void * data);
void bind_managed_ssbo(managed_ssbo * ms,GLuint binding_point);*/

#endif

