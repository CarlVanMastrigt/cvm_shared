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

#ifndef CVM_GL_FUNCTIONS_H
#define CVM_GL_FUNCTIONS_H


typedef struct gl_functions
{
    void(*glGenVertexArrays)(GLsizei n,GLuint * arrays);
    void(*glGenBuffers)(GLsizei n,GLuint * buffers);
    void(*glDeleteBuffers)(GLsizei n,GLuint * buffers);
    void(*glBindBuffer)(GLenum target,GLuint buffer);
    void(*glBufferData)(GLenum target,GLsizeiptr size,const GLvoid * data,GLenum usage);
    void(*glEnableVertexAttribArray)(GLuint index);
    void(*glVertexAttribPointer)(GLuint index,GLint size,GLenum type,GLboolean normalized,GLsizei stride,const GLvoid * pointer);
    void(*glVertexAttribIPointer)(GLuint index,GLint size,GLenum type,GLsizei stride,const GLvoid * pointer);
    void(*glDisableVertexAttribArray)(GLuint index);
    void(*glBufferSubData)(GLenum target,GLintptr offset,GLsizeiptr size,const GLvoid * data);
    GLuint(*glCreateShader)(GLenum type);
    GLuint(*glCreateProgram)();
    void(*glShaderSource)(GLuint shader,GLsizei count,const GLchar ** string,const GLint * length);
    void(*glCompileShader)(GLuint shader);
    void(*glGetShaderiv)(GLuint shader,GLenum pname,GLint * params);
    void(*glGetShaderInfoLog)(GLuint shader,GLsizei maxLength,GLsizei * length,GLchar * infoLog);
    void(*glAttachShader)(GLuint program,GLuint shader);
    void(*glDetachShader)(GLuint program,GLuint shader);
    void(*glDeleteShader)(GLuint shader);
    void(*glLinkProgram)(GLuint program);
    void(*glGetProgramiv)(GLuint program,GLenum pname,GLint *params);
    void(*glGetProgramInfoLog)(GLuint program,GLsizei maxLength,GLsizei * length,GLchar * infoLog);
    void(*glValidateProgram)(GLuint program);
    void(*glUseProgram)(GLuint program);
    GLubyte*(*glGetString)(GLenum name);
    void(*glGenTextures)(GLsizei n,GLuint * textures);
    void(*glDeleteTextures)(GLsizei n,GLuint * textures);
    void(*glBindTexture)(GLenum target,GLuint texture);
    void(*glTexParameteri)(GLenum target,GLenum pname,GLint param);
    void(*glTexBuffer)(GLenum target,GLenum internalFormat,GLuint buffer);
    void*(*glMapBufferRange)(GLenum target,GLint offset,GLsizeiptr length,GLbitfield access);
    void(*glUnmapBuffer)(GLenum target);
    void(*glActiveTexture)(GLenum texture);
    void(*glBindImageTexture)(GLuint unit,GLuint texture,GLint level,GLboolean layered,GLint layer,GLenum access,GLenum format);
    void(*glVertexAttribDivisor)(GLuint index,GLuint divisor);
    void(*glBindVertexArray)(GLuint array);
    GLuint(*glGetAttribLocation)(GLuint program,const GLchar * name);
    void(*glGenFramebuffers)(GLsizei n,GLuint * ids);
    void(*glBindFramebuffer)(GLenum target,GLuint framebuffer);
    void(*glFramebufferTexture2D)(GLenum target,GLenum attachment,GLenum textarget,GLuint texture,GLint level);
    void(*glDrawBuffers)(GLsizei n,const GLenum * bufs);
    void(*glDrawBuffer)(GLenum buf);
    void(*glViewport)(GLint x,GLint y,GLsizei width,GLsizei height);
    void(*glEnable)(GLenum cap);
    void(*glDisable)(GLenum cap);
    void(*glBlendFunc)(GLenum sfactor,GLenum dfactor);
    GLint(*glGetUniformLocation)(GLuint program,const GLchar * name);
    void(*glDrawArraysInstanced)(GLenum mode,GLint first,GLsizei count,GLsizei primcount);
    void(*glDrawArrays)(GLenum mode,GLint first,GLsizei count);
    GLenum(*glGetError)(void);
    void(*glLineWidth)(GLfloat width);
    void(*glPointSize)(GLfloat size);
    void(*glTexImage1D)(GLenum target,GLint level,GLint internalformat,GLsizei width,GLint border,GLenum format,GLenum type,const void * data);
    void(*glTexImage2D)(GLenum target,GLint level,GLint internalformat,GLsizei width,GLsizei height,GLint border,GLenum format,GLenum type,const void * data);
    void(*glTexImage2DMultisample)(GLenum target,GLsizei samples,GLint internalformat,GLsizei width,GLsizei height,GLboolean fixedsamplelocations);
    void(*glBufferStorage)(GLenum target,GLsizeiptr size,const GLvoid * data,GLbitfield flags);
    void(*glMultiDrawElementsIndirect)(GLenum mode,GLenum type,const void *indirect,GLsizei drawcount,GLsizei stride);
    void(*glDrawElementsInstanced)(GLenum mode,GLsizei count,GLenum type,const void * indices,GLsizei primcount);
    void(*glClearColor)(GLclampf red,GLclampf green,GLclampf blue,GLclampf alpha);
    void(*glClear)(GLbitfield mask);
    void(*glFinish)(void);
    void(*glTexSubImage2D)(GLenum target,GLint level,GLint xoffset,GLint yoffset,GLsizei width,GLsizei height,GLenum format,GLenum type,const void * pixels);
    void(*glClipControl)(GLenum origin,GLenum depth);
    void(*glPointParameterf)(GLenum pname,GLfloat param);
    void(*glPointParameteri)(GLenum pname,GLint param);
    void(*glStencilOpSeparate)(GLenum face,GLenum sfail,GLenum dpfail,GLenum dppass);
    void(*glDepthRange)(GLdouble nearVal,GLdouble farVal);
    void(*glDepthMask)(GLboolean flag);
    void(*glDepthFunc)(GLenum func);
    void(*glCullFace)(GLenum mode);
    void(*glClearDepth)(GLdouble depth);
    void(*glClearStencil)(GLint s);
    void(*glStencilFunc)(GLenum func,GLint ref,GLuint mask);
    void(*glStencilOp)(GLenum sfail,GLenum dpfail,GLenum dppass);


    void(*glUniform1i)(GLint location,GLint v0);
    void(*glUniform2i)(GLint location,GLint v0,GLint v1);
    void(*glUniform3i)(GLint location,GLint v0,GLint v1,GLint v2);
    void(*glUniform4i)(GLint location,GLint v0,GLint v1,GLint v2,GLint v3);
    void(*glUniform1iv)(GLint location,GLsizei count,const GLint * value);
    void(*glUniform2iv)(GLint location,GLsizei count,const GLint * value);
    void(*glUniform3iv)(GLint location,GLsizei count,const GLint * value);
    void(*glUniform4iv)(GLint location,GLsizei count,const GLint * value);
    void(*glUniform1ui)(GLint location,GLuint v0);
    void(*glUniform2ui)(GLint location,GLuint v0,GLuint v1);
    void(*glUniform3ui)(GLint location,GLuint v0,GLuint v1,GLuint v2);
    void(*glUniform4ui)(GLint location,GLuint v0,GLuint v1,GLuint v2,GLuint v3);
    void(*glUniform1uiv)(GLint location,GLsizei count,const GLuint * value);
    void(*glUniform2uiv)(GLint location,GLsizei count,const GLuint * value);
    void(*glUniform3uiv)(GLint location,GLsizei count,const GLuint * value);
    void(*glUniform4uiv)(GLint location,GLsizei count,const GLuint * value);
    void(*glUniform1f)(GLint location,GLfloat v0);
    void(*glUniform2f)(GLint location,GLfloat v0,GLfloat v1);
    void(*glUniform3f)(GLint location,GLfloat v0,GLfloat v1,GLfloat v2);
    void(*glUniform4f)(GLint location,GLfloat v0,GLfloat v1,GLfloat v2,GLfloat v3);
    void(*glUniform1fv)(GLint location,GLsizei count,const GLfloat * value);
    void(*glUniform2fv)(GLint location,GLsizei count,const GLfloat * value);
    void(*glUniform3fv)(GLint location,GLsizei count,const GLfloat * value);
    void(*glUniform4fv)(GLint location,GLsizei count,const GLfloat * value);
    void(*glUniformMatrix2fv)(GLint location,GLsizei count,GLboolean transpose,const GLfloat *value);
    void(*glUniformMatrix3fv)(GLint location,GLsizei count,GLboolean transpose,const GLfloat *value);
    void(*glUniformMatrix4fv)(GLint location,GLsizei count,GLboolean transpose,const GLfloat *value);
    void(*glUniformMatrix2x3fv)(GLint location,GLsizei count,GLboolean transpose,const GLfloat *value);
    void(*glUniformMatrix3x2fv)(GLint location,GLsizei count,GLboolean transpose,const GLfloat *value);
    void(*glUniformMatrix2x4fv)(GLint location,GLsizei count,GLboolean transpose,const GLfloat *value);
    void(*glUniformMatrix4x2fv)(GLint location,GLsizei count,GLboolean transpose,const GLfloat *value);
    void(*glUniformMatrix3x4fv)(GLint location,GLsizei count,GLboolean transpose,const GLfloat *value);
    void(*glUniformMatrix4x3fv)(GLint location,GLsizei count,GLboolean transpose,const GLfloat *value);
}
gl_functions;

gl_functions * get_gl_functions(void);
void free_gl_functions(gl_functions * glf);

static inline void check_for_gl_error(gl_functions * glf) ///move to c file to avoid using default function (non pointer func)
{
    GLenum x=glf->glGetError();
    if(x) printf("gl error : %d\n",x);
}




#endif


