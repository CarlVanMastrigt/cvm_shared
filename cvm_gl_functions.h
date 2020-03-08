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

///put ALL opengl function calls (these and those that are defined by default too) in struct that can be passed around to avoid global declarations and allow the function names to be correct (no _ptr appended)

    typedef void (* GL_GenVertexArrays_Func)(GLsizei n, GLuint * arrays);
    GL_GenVertexArrays_Func glGenVertexArrays_ptr;

    typedef void (* GL_GenBuffers_Func)(GLsizei n, GLuint * buffers);
    GL_GenBuffers_Func glGenBuffers_ptr;

    typedef void (* GL_BindBuffer_Func)(GLenum  target,  GLuint  buffer);
    GL_BindBuffer_Func glBindBuffer_ptr;

    typedef void (* GL_BufferData_Func)(GLenum  target,  GLsizeiptr  size,  const GLvoid *  data,  GLenum  usage);
    GL_BufferData_Func glBufferData_ptr;

    typedef void (* GL_EnableVertexAttribArray_Func)(GLuint index);
    GL_EnableVertexAttribArray_Func glEnableVertexAttribArray_ptr;

    typedef void (* GL_VertexAttribPointer_Func)(GLuint  index,  GLint  size,  GLenum  type,  GLboolean  normalized,  GLsizei  stride,  const GLvoid *  pointer);
    GL_VertexAttribPointer_Func glVertexAttribPointer_ptr;

    typedef void (* GL_DisableVertexAttribArray_Func)(GLuint index);
    GL_DisableVertexAttribArray_Func glDisableVertexAttribArray_ptr;

    typedef void (* GL_BufferSubData_Func)(GLenum  target, GLintptr offset,GLsizeiptr  size, const GLvoid *  data);
    GL_BufferSubData_Func glBufferSubData_ptr;

    typedef GLuint (* GL_CreateShader_Func)(GLenum  type);
    GL_CreateShader_Func glCreateShader_ptr;

    typedef GLuint (* GL_CreateProgram_Func)();
    GL_CreateProgram_Func glCreateProgram_ptr;

    typedef void (* GL_ShaderSource_Func)(GLuint shader,GLsizei count,const GLchar ** string,const GLint * length);
    GL_ShaderSource_Func glShaderSource_ptr;

    typedef void (* GL_CompileShader_Func)(GLuint shader);
    GL_CompileShader_Func glCompileShader_ptr;

    typedef void (* GL_GetShaderiv_Func)(GLuint shader,GLenum pname,GLint * params);
    GL_GetShaderiv_Func glGetShaderiv_ptr;

    typedef void (* GL_GetShaderInfoLog_Func)(GLuint shader,GLsizei maxLength,GLsizei * length,GLchar * infoLog);
    GL_GetShaderInfoLog_Func glGetShaderInfoLog_ptr;

    typedef void (* GL_AttachShader_Func)(GLuint program,GLuint shader);
    GL_AttachShader_Func glAttachShader_ptr;

    typedef void (* GL_DetachShader_Func)(GLuint program,GLuint shader);
    GL_DetachShader_Func glDetachShader_ptr;

    typedef void (* GL_DeleteShader_Func)(GLuint shader);
    GL_DeleteShader_Func glDeleteShader_ptr;

    typedef void (* GL_LinkProgram_Func)(GLuint program);
    GL_LinkProgram_Func glLinkProgram_ptr;

    typedef void (* GL_GetProgramiv_Func)(GLuint program,GLenum pname,GLint *params);
    GL_GetProgramiv_Func glGetProgramiv_ptr;

    typedef void (* GL_GetProgramInfoLog_Func)(GLuint program,GLsizei maxLength,GLsizei * length,GLchar * infoLog);
    GL_GetProgramInfoLog_Func glGetProgramInfoLog_ptr;

    typedef void (* GL_ValidateProgram_Func)(GLuint program);
    GL_ValidateProgram_Func glValidateProgram_ptr;

    typedef void (* GL_UseProgram_Func)(GLuint program);
    GL_UseProgram_Func glUseProgram_ptr;

    typedef GLint (* GL_GetUniformLocation_Func)(GLuint program,const GLchar * name);
    GL_GetUniformLocation_Func glGetUniformLocation_ptr;

    typedef void (* GL_UniformMatrix4fv_Func)(GLint location,GLsizei count,GLboolean  transpose,const GLfloat * value);
    GL_UniformMatrix4fv_Func glUniformMatrix4fv_ptr;

    typedef void (* GL_UniformMatrix3fv_Func)(GLint location,GLsizei count,GLboolean  transpose,const GLfloat * value);
    GL_UniformMatrix3fv_Func glUniformMatrix3fv_ptr;

    typedef void (* GL_Uniform4fv_Func)(GLint location,GLsizei count,const GLfloat * value);
    GL_Uniform4fv_Func glUniform4fv_ptr;

    typedef void (* GL_Uniform3fv_Func)(GLint location,GLsizei count,const GLfloat * value);
    GL_Uniform3fv_Func glUniform3fv_ptr;

    typedef void (* GL_Uniform1fv_Func)(GLint location,GLsizei count,const GLfloat * value);
    GL_Uniform1fv_Func glUniform1fv_ptr;

    typedef void (* GL_Uniform1iv_Func)(GLint location,GLsizei count,const GLint * value);
    GL_Uniform1iv_Func glUniform1iv_ptr;

    typedef void (* GL_Uniform1uiv_Func)(GLint location,GLsizei count,const GLuint * value);
    GL_Uniform1uiv_Func glUniform1uiv_ptr;

    typedef void (* GL_Uniform4uiv_Func)(GLint location,  GLsizei count,  const GLuint *value);
    GL_Uniform4uiv_Func glUniform4uiv_ptr;

    typedef void (* GL_Uniform1i_Func)(GLint location, GLint v0);
    GL_Uniform1i_Func glUniform1i_ptr;

    typedef void (* GL_Uniform1ui_Func)(GLint location, GLuint v0);
    GL_Uniform1ui_Func glUniform1ui_ptr;

    typedef void (* GL_Uniform4ui_Func)(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
    GL_Uniform4ui_Func glUniform4ui_ptr;

    typedef void (* GL_Uniform1f_Func)(GLint location, GLfloat v0);
    GL_Uniform1f_Func glUniform1f_ptr;

    typedef void (* GL_Uniform4f_Func)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
    GL_Uniform4f_Func glUniform4f_ptr;

    typedef void (* GL_Uniform3f_Func)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
    GL_Uniform3f_Func glUniform3f_ptr;

    typedef void (* GL_Uniform2f_Func)(GLint location, GLfloat v0, GLfloat v1);
    GL_Uniform2f_Func glUniform2f_ptr;

    typedef void (* GL_StencilOpSeparate_Func)(GLenum face, GLenum sfail,GLenum dpfail,GLenum dppass);
    GL_StencilOpSeparate_Func glStencilOpSeparate_ptr;

    typedef void (* GL_BindVertexArray_Func)(GLuint array);
    GL_BindVertexArray_Func glBindVertexArray_ptr;

    typedef void (* GL_DeleteBuffers_Func)(GLsizei n,const GLuint * buffers);
    GL_DeleteBuffers_Func glDeleteBuffers_ptr;

    typedef void (* GL_BindBufferRange_Func)(GLenum target,GLuint index,GLuint buffer,GLintptr offset,GLsizeiptr size);
    GL_BindBufferRange_Func glBindBufferRange_ptr;

    typedef void (* GL_ShaderStorageBlockBinding_Func)(GLuint  program,  GLuint  storageBlockIndex,  GLuint  storageBlockBinding);
    GL_ShaderStorageBlockBinding_Func glShaderStorageBlockBinding_ptr;

    typedef void (* GL_BindBufferBase_Func)(GLenum target,GLuint index,GLuint buffer);
    GL_BindBufferBase_Func glBindBufferBase_ptr;

    typedef void (* GL_VertexAttribIPointer_Func)(GLuint index,GLint size,GLenum type,GLsizei stride,const GLvoid * pointer);
    GL_VertexAttribIPointer_Func glVertexAttribIPointer_ptr;

    typedef void (* GL_GenFramebuffers_Func)(GLsizei  n,  GLuint * ids);
    GL_GenFramebuffers_Func glGenFramebuffers_ptr;

    typedef void (* GL_BindFramebuffer_Func)(GLenum  target,  GLuint ids);
    GL_BindFramebuffer_Func glBindFramebuffer_ptr;

    typedef void (* GL_DeleteFramebuffers_Func)(GLsizei  n,  GLuint * ids);
    GL_DeleteFramebuffers_Func glDeleteFramebuffers_ptr;

    typedef void (* GL_FramebufferTexture2D_Func)(GLenum  target,  GLenum  attachment,  GLenum  textarget,  GLuint  texture,  GLint  level);
    GL_FramebufferTexture2D_Func glFramebufferTexture2D_ptr;

    typedef void (* GL_FramebufferTexture3D_Func)(GLenum  target,  GLenum  attachment,  GLenum  textarget,  GLuint  texture,  GLint  level,  GLint layer);
    GL_FramebufferTexture3D_Func glFramebufferTexture3D_ptr;

    typedef void (* GL_BlitFramebuffer_Func)(GLint  srcX0,  GLint  srcY0,  GLint  srcX1,  GLint  srcY1,  GLint  dstX0,  GLint  dstY0,  GLint  dstX1,  GLint  dstY1,  GLbitfield  mask,  GLenum  filter);
    GL_BlitFramebuffer_Func glBlitFramebuffer_ptr;

    typedef void (* GL_DrawBuffers_Func)(GLsizei  n,  const GLenum * bufs);
    GL_DrawBuffers_Func glDrawBuffers_ptr;

    typedef void (* GL_Uniform2fv_Func)(GLint  location,  GLsizei  count,  const GLfloat * value);
    GL_Uniform2fv_Func glUniform2fv_ptr;

    typedef void (* GL_BindImageTexture_Func)(GLuint  unit,  GLuint  texture,  GLint  level,  GLboolean  layered,  GLint  layer,  GLenum  access,  GLenum  format);
    GL_BindImageTexture_Func glBindImageTexture_ptr;

    typedef void (* GL_DrawElementsBaseVertex_Func)(GLenum mode,  GLsizei count,  GLenum type,  GLvoid *indices,  GLint basevertex);
    GL_DrawElementsBaseVertex_Func glDrawElementsBaseVertex_ptr;

    typedef void (* GL_VertexAttribDivisor_Func)(GLuint index,GLuint divisor);
    GL_VertexAttribDivisor_Func glVertexAttribDivisor_ptr;

    typedef void (* GL_DrawElementsInstanced_Func)(GLenum mode,  GLsizei count,  GLenum type,  const void * indices,  GLsizei primcount);
    GL_DrawElementsInstanced_Func glDrawElementsInstanced_ptr;

    typedef void (* GL_DrawElementsInstancedBaseVertex_Func)(GLenum mode,  GLsizei count,  GLenum type,  const void * indices,  GLsizei primcount,GLint basevertex);
    GL_DrawElementsInstancedBaseVertex_Func glDrawElementsInstancedBaseVertex_ptr;

    typedef void (* GL_TexBuffer_Func)(GLenum target,GLenum internalFormat,GLuint buffer);
    GL_TexBuffer_Func glTexBuffer_ptr;

    typedef void* (* GL_MapBufferRange_Func)(GLenum target,GLint offset,GLsizeiptr length,GLbitfield access);
    GL_MapBufferRange_Func glMapBufferRange_ptr;

    typedef void* (* GL_MemoryBarrier_Func)(GLbitfield barriers);
    GL_MemoryBarrier_Func glMemoryBarrier_ptr;

    typedef void (* GL_UnmapBuffer_Func)(GLenum target);
    GL_UnmapBuffer_Func glUnmapBuffer_ptr;

    typedef void (* GL_DrawElementsInstancedBaseInstance_Func)(GLenum mode,  GLsizei count,  GLenum type,  const void * indices,  GLsizei primcount,GLuint baseinstance);
    GL_DrawElementsInstancedBaseInstance_Func glDrawElementsInstancedBaseInstance_ptr;

//    typedef void (APIENTRY * GL_DrawElementsInstancedBaseVertexBaseInstance_Func)(GLenum mode,  GLsizei count,  GLenum type,  const void * indices,  GLsizei primcount,GLint basevertex,GLuint baseinstance);
//    GL_DrawElementsInstancedBaseVertexBaseInstance_Func glDrawElementsInstancedBaseVertexBaseInstance_ptr;

//    typedef void (* GL_UniformSubroutinesuiv_Func)(GLenum shadertype,GLsizei count,const GLuint * indicies);
//    GL_UniformSubroutinesuiv_Func glUniformSubroutinesuiv_ptr;

    typedef void (* GL_GenTransformFeedbacks_Func)(GLsizei n, GLuint *ids);
    GL_GenTransformFeedbacks_Func glGenTransformFeedbacks_ptr;

    typedef void (* GL_BindTransformFeedback_Func)(GLenum target, GLuint id);
    GL_BindTransformFeedback_Func glBindTransformFeedback_ptr;

    typedef void (* GL_BeginTransformFeedback_Func)(GLenum primitiveMode);
    GL_BeginTransformFeedback_Func glBeginTransformFeedback_ptr;

    typedef void (* GL_TransformFeedbackVaryings_Func)(GLuint program,GLsizei count,const char ** varyings,GLenum bufferMode);
    GL_TransformFeedbackVaryings_Func glTransformFeedbackVaryings_ptr;

    typedef void (* GL_EndTransformFeedback_Func)();
    GL_EndTransformFeedback_Func glEndTransformFeedback_ptr;

    typedef void (* GL_DrawTransformFeedback_Func)(GLenum mode,GLuint id);
    GL_DrawTransformFeedback_Func glDrawTransformFeedback_ptr;

    typedef void (* GL_MultiDrawElementsIndirect_Func)(GLenum mode,GLenum type,const void *indirect,GLsizei drawcount,GLsizei stride);
    GL_MultiDrawElementsIndirect_Func glMultiDrawElementsIndirect_ptr;

    typedef void (* GL_DrawElementsIndirect_Func)(GLenum mode,GLenum type,const void *indirect);
    GL_DrawElementsIndirect_Func glDrawElementsIndirect_ptr;

    typedef void (* GL_BufferStorage_Func)(GLenum target,GLsizeiptr size,const GLvoid * data,GLbitfield flags);
    GL_BufferStorage_Func glBufferStorage_ptr;

    typedef void (* GL_FramebufferParameteri_Func)(GLenum target,GLenum pname,GLint param);
    GL_FramebufferParameteri_Func glFramebufferParameteri_ptr;

    typedef void (* GL_DrawArraysInstanced_Func)(GLenum mode,GLint first,GLsizei count,GLsizei primcount);
    GL_DrawArraysInstanced_Func glDrawArraysInstanced_ptr;

    typedef void (* GL_InvalidateBufferData_Func)(GLuint buffer);
    GL_InvalidateBufferData_Func glInvalidateBufferData_ptr;

    typedef void (* GL_BlendFuncSeparate_Func)(GLenum srcRGB,GLenum dstRGB,GLenum srcA,GLenum dstA);
    GL_BlendFuncSeparate_Func glBlendFuncSeparate_ptr;

    typedef void (* GL_TexImage2DMultisample_Func)(GLenum target,GLsizei samples,GLint internalformat,GLsizei width,GLsizei height,GLboolean fixedsamplelocations);
    GL_TexImage2DMultisample_Func glTexImage2DMultisample_ptr;

    typedef void (* GL_TexImage3DMultisample_Func)(GLenum target,GLsizei samples,GLint internalformat,GLsizei width,GLsizei height,GLsizei depth,GLboolean fixedsamplelocations);
    GL_TexImage3DMultisample_Func glTexImage3DMultisample_ptr;

    typedef void (* GL_ClearTexImage_Func)(GLuint texture,GLint level,GLenum format,GLenum type,const void * data);
    GL_ClearTexImage_Func glClearTexImage_ptr;

    typedef void (* GL_ClipControl_Func)(GLenum origin,GLenum depth);
    GL_ClipControl_Func glClipControl_ptr;

    typedef GLuint (APIENTRY * GL_GetAttribLocation_Func)(GLuint program,const GLchar * name);
    GL_GetAttribLocation_Func glGetAttribLocation_ptr;

    typedef void (* GL_DrawArraysInstancedBaseInstance_Func)(GLenum mode,GLint first,GLsizei count,GLsizei primcount,GLuint baseinstance);
    GL_DrawArraysInstancedBaseInstance_Func glDrawArraysInstancedBaseInstance_ptr;

    typedef void (* GL_GenQueries_Func)(GLsizei n,GLuint * ids);
    GL_GenQueries_Func glGenQueries_ptr;

    typedef void (* GL_BeginQuery_Func)(GLenum target,GLuint id);
    GL_BeginQuery_Func glBeginQuery_ptr;

    typedef void (* GL_EndQuery_Func)(GLenum target);
    GL_EndQuery_Func glEndQuery_ptr;

    typedef void (* GL_GetQueryObjectiv_Func)(GLuint id,GLenum pname,GLint * params);
    GL_GetQueryObjectiv_Func glGetQueryObjectiv_ptr;

    typedef void (* GL_UniformMatrix2fv_Func)(GLuint location,GLsizei count,GLboolean tramspose,const GLfloat * value);
    GL_UniformMatrix2fv_Func glUniformMatrix2fv_ptr;

//    typedef void (* GL_TextureBarrier_Func)(void);
//    GL_TextureBarrier_Func glTextureBarrier_ptr;

    typedef void (* GL_glPointParameteri_Func)(GLenum pname,GLint param);
    GL_glPointParameteri_Func glPointParameteri_ptr;

    typedef void (* GL_glPointParameterf_Func)(GLenum pname,GLfloat param);
    GL_glPointParameterf_Func glPointParameterf_ptr;



    int load_contextual_resources(void);


static inline void get_gl_error(void) ///move to c file to avoid using default function (non pointer func)
{
    GLenum x=glGetError();
    if(x)
    {
        printf("gl error : %d\n",x);
    }
}




#endif


