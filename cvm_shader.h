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


#ifndef CVM_SHADER_H
#define CVM_SHADER_H

#define MAX_SHADER_INCLUDES 32



void set_shader_version_and_defines(char * svad);
void free_shader_version_and_defines(void);
GLuint initialise_shader_program(gl_functions * glf,const char * defines,const char * vert_file,const char * geom_file,const char * frag_file);


static inline void set_instanced_attribute_f(gl_functions * glf,GLuint index,GLint size,GLsizei stride,const GLvoid * pointer)
{
    glf->glEnableVertexAttribArray(index);
    glf->glVertexAttribPointer(index,size,GL_FLOAT,GL_FALSE,stride,pointer);
    glf->glVertexAttribDivisor(index,1);
}

static inline void set_instanced_attribute_i(gl_functions * glf,GLuint index,GLint size,GLsizei stride,const GLvoid * pointer,GLenum type)
{
    glf->glEnableVertexAttribArray(index);
    glf->glVertexAttribIPointer(index,size,type,stride,pointer);
    glf->glVertexAttribDivisor(index,1);
}




#endif
