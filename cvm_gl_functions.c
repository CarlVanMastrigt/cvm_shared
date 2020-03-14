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




gl_functions * get_gl_functions(void)
{
    gl_functions * glf=malloc(sizeof(gl_functions));

    if((glf->glGenVertexArrays=SDL_GL_GetProcAddress("glGenVertexArrays"))==NULL)puts("Can't load glGenVertexArrays");
    if((glf->glGenBuffers=SDL_GL_GetProcAddress("glGenBuffers"))==NULL)puts("Can't load glGenBuffers");
    if((glf->glBindBuffer=SDL_GL_GetProcAddress("glBindBuffer"))==NULL)puts("Can't load glBindBuffer");
    if((glf->glBufferData=SDL_GL_GetProcAddress("glBufferData"))==NULL)puts("Can't load glBufferData");
    if((glf->glEnableVertexAttribArray=SDL_GL_GetProcAddress("glEnableVertexAttribArray"))==NULL)puts("Can't load glEnableVertexAttribArray");
    if((glf->glVertexAttribPointer=SDL_GL_GetProcAddress("glVertexAttribPointer"))==NULL)puts("Can't load glVertexAttribPointer");
    if((glf->glVertexAttribIPointer=SDL_GL_GetProcAddress("glVertexAttribIPointer"))==NULL)puts("Can't load glVertexAttribIPointer");
    if((glf->glDisableVertexAttribArray=SDL_GL_GetProcAddress("glDisableVertexAttribArray"))==NULL)puts("Can't load glDisableVertexAttribArray");
    if((glf->glBufferSubData=SDL_GL_GetProcAddress("glBufferSubData"))==NULL)puts("Can't load glBufferSubData");
    if((glf->glCreateShader=SDL_GL_GetProcAddress("glCreateShader"))==NULL)puts("Can't load glCreateShader");
    if((glf->glCreateProgram=SDL_GL_GetProcAddress("glCreateProgram"))==NULL)puts("Can't load glCreateProgram");
    if((glf->glShaderSource=SDL_GL_GetProcAddress("glShaderSource"))==NULL)puts("Can't load glShaderSource");
    if((glf->glCompileShader=SDL_GL_GetProcAddress("glCompileShader"))==NULL)puts("Can't load glCompileShader");
    if((glf->glGetShaderiv=SDL_GL_GetProcAddress("glGetShaderiv"))==NULL)puts("Can't load glGetShaderiv");
    if((glf->glGetShaderInfoLog=SDL_GL_GetProcAddress("glGetShaderInfoLog"))==NULL)puts("Can't load glGetShaderInfoLog");
    if((glf->glAttachShader=SDL_GL_GetProcAddress("glAttachShader"))==NULL)puts("Can't load glAttachShader");
    if((glf->glDetachShader=SDL_GL_GetProcAddress("glDetachShader"))==NULL)puts("Can't load glDetachShader");
    if((glf->glDeleteShader=SDL_GL_GetProcAddress("glDeleteShader"))==NULL)puts("Can't load glDeleteShader");
    if((glf->glLinkProgram=SDL_GL_GetProcAddress("glLinkProgram"))==NULL)puts("Can't load glLinkProgram");
    if((glf->glGetProgramiv=SDL_GL_GetProcAddress("glGetProgramiv"))==NULL)puts("Can't load glGetProgramiv");
    if((glf->glGetProgramInfoLog=SDL_GL_GetProcAddress("glGetProgramInfoLog"))==NULL)puts("Can't load glGetProgramInfoLog");
    if((glf->glValidateProgram=SDL_GL_GetProcAddress("glValidateProgram"))==NULL)puts("Can't load glValidateProgram");
    if((glf->glUseProgram=SDL_GL_GetProcAddress("glUseProgram"))==NULL)puts("Can't load glUseProgram");
    if((glf->glGetString=SDL_GL_GetProcAddress("glGetString"))==NULL)puts("Can't load glGetString");
    if((glf->glGenTextures=SDL_GL_GetProcAddress("glGenTextures"))==NULL)puts("Can't load glGenTextures");
    if((glf->glBindTexture=SDL_GL_GetProcAddress("glBindTexture"))==NULL)puts("Can't load glBindTexture");
    if((glf->glTexParameteri=SDL_GL_GetProcAddress("glTexParameteri"))==NULL)puts("Can't load glTexParameteri");
    if((glf->glTexBuffer=SDL_GL_GetProcAddress("glTexBuffer"))==NULL)puts("Can't load glTexBuffer");
    if((glf->glMapBufferRange=SDL_GL_GetProcAddress("glMapBufferRange"))==NULL)puts("Can't load glMapBufferRange");
    if((glf->glUnmapBuffer=SDL_GL_GetProcAddress("glUnmapBuffer"))==NULL)puts("Can't load glUnmapBuffer");
    if((glf->glActiveTexture=SDL_GL_GetProcAddress("glActiveTexture"))==NULL)puts("Can't load glActiveTexture");
    if((glf->glBindImageTexture=SDL_GL_GetProcAddress("glBindImageTexture"))==NULL)puts("Can't load glBindImageTexture");
    if((glf->glVertexAttribDivisor=SDL_GL_GetProcAddress("glVertexAttribDivisor"))==NULL)puts("Can't load glVertexAttribDivisor");
    if((glf->glBindVertexArray=SDL_GL_GetProcAddress("glBindVertexArray"))==NULL)puts("Can't load glBindVertexArray");
    if((glf->glGetAttribLocation=SDL_GL_GetProcAddress("glGetAttribLocation"))==NULL)puts("Can't load glGetAttribLocation");
    if((glf->glGenFramebuffers=SDL_GL_GetProcAddress("glGenFramebuffers"))==NULL)puts("Can't load glGenFramebuffers");
    if((glf->glBindFramebuffer=SDL_GL_GetProcAddress("glBindFramebuffer"))==NULL)puts("Can't load glBindFramebuffer");
    if((glf->glFramebufferTexture2D=SDL_GL_GetProcAddress("glFramebufferTexture2D"))==NULL)puts("Can't load glFramebufferTexture2D");
    if((glf->glDrawBuffers=SDL_GL_GetProcAddress("glDrawBuffers"))==NULL)puts("Can't load glDrawBuffers");
    if((glf->glDrawBuffer=SDL_GL_GetProcAddress("glDrawBuffer"))==NULL)puts("Can't load glDrawBuffer");
    if((glf->glViewport=SDL_GL_GetProcAddress("glViewport"))==NULL)puts("Can't load glViewport");
    if((glf->glEnable=SDL_GL_GetProcAddress("glEnable"))==NULL)puts("Can't load glEnable");
    if((glf->glDisable=SDL_GL_GetProcAddress("glDisable"))==NULL)puts("Can't load glDisable");
    if((glf->glBlendFunc=SDL_GL_GetProcAddress("glBlendFunc"))==NULL)puts("Can't load glBlendFunc");
    if((glf->glGetUniformLocation=SDL_GL_GetProcAddress("glGetUniformLocation"))==NULL)puts("Can't load glGetUniformLocation");
    if((glf->glDrawArraysInstanced=SDL_GL_GetProcAddress("glDrawArraysInstanced"))==NULL)puts("Can't load glDrawArraysInstanced");
    if((glf->glDrawArrays=SDL_GL_GetProcAddress("glDrawArrays"))==NULL)puts("Can't load glDrawArrays");
    if((glf->glGetError=SDL_GL_GetProcAddress("glGetError"))==NULL)puts("Can't load glGetError");
    if((glf->glLineWidth=SDL_GL_GetProcAddress("glLineWidth"))==NULL)puts("Can't load glLineWidth");
    if((glf->glPointSize=SDL_GL_GetProcAddress("glPointSize"))==NULL)puts("Can't load glPointSize");
    if((glf->glTexImage1D=SDL_GL_GetProcAddress("glTexImage1D"))==NULL)puts("Can't load glTexImage1D");
    if((glf->glTexImage2D=SDL_GL_GetProcAddress("glTexImage2D"))==NULL)puts("Can't load glTexImage2D");
    if((glf->glTexImage2DMultisample=SDL_GL_GetProcAddress("glTexImage2DMultisample"))==NULL)puts("Can't load glTexImage2DMultisample");
    if((glf->glBufferStorage=SDL_GL_GetProcAddress("glBufferStorage"))==NULL)puts("Can't load glBufferStorage");
    if((glf->glMultiDrawElementsIndirect=SDL_GL_GetProcAddress("glMultiDrawElementsIndirect"))==NULL)puts("Can't load glMultiDrawElementsIndirect");
    if((glf->glDrawElementsInstanced=SDL_GL_GetProcAddress("glDrawElementsInstanced"))==NULL)puts("Can't load glDrawElementsInstanced");
    if((glf->glClearColor=SDL_GL_GetProcAddress("glClearColor"))==NULL)puts("Can't load glClearColor");
    if((glf->glClear=SDL_GL_GetProcAddress("glClear"))==NULL)puts("Can't load glClear");


    if((glf->glUniform1i=SDL_GL_GetProcAddress("glUniform1i"))==NULL)puts("Can't load glUniform1i");
    if((glf->glUniform2i=SDL_GL_GetProcAddress("glUniform2i"))==NULL)puts("Can't load glUniform2i");
    if((glf->glUniform3i=SDL_GL_GetProcAddress("glUniform3i"))==NULL)puts("Can't load glUniform3i");
    if((glf->glUniform4i=SDL_GL_GetProcAddress("glUniform4i"))==NULL)puts("Can't load glUniform4i");
    if((glf->glUniform1iv=SDL_GL_GetProcAddress("glUniform1iv"))==NULL)puts("Can't load glUniform1iv");
    if((glf->glUniform2iv=SDL_GL_GetProcAddress("glUniform2iv"))==NULL)puts("Can't load glUniform2iv");
    if((glf->glUniform3iv=SDL_GL_GetProcAddress("glUniform3iv"))==NULL)puts("Can't load glUniform3iv");
    if((glf->glUniform4iv=SDL_GL_GetProcAddress("glUniform4iv"))==NULL)puts("Can't load glUniform4iv");
    if((glf->glUniform1ui=SDL_GL_GetProcAddress("glUniform1ui"))==NULL)puts("Can't load glUniform1ui");
    if((glf->glUniform2ui=SDL_GL_GetProcAddress("glUniform2ui"))==NULL)puts("Can't load glUniform2ui");
    if((glf->glUniform3ui=SDL_GL_GetProcAddress("glUniform3ui"))==NULL)puts("Can't load glUniform3ui");
    if((glf->glUniform4ui=SDL_GL_GetProcAddress("glUniform4ui"))==NULL)puts("Can't load glUniform4ui");
    if((glf->glUniform1uiv=SDL_GL_GetProcAddress("glUniform1uiv"))==NULL)puts("Can't load glUniform1uiv");
    if((glf->glUniform2uiv=SDL_GL_GetProcAddress("glUniform2uiv"))==NULL)puts("Can't load glUniform2uiv");
    if((glf->glUniform3uiv=SDL_GL_GetProcAddress("glUniform3uiv"))==NULL)puts("Can't load glUniform3uiv");
    if((glf->glUniform4uiv=SDL_GL_GetProcAddress("glUniform4uiv"))==NULL)puts("Can't load glUniform4uiv");
    if((glf->glUniform1f=SDL_GL_GetProcAddress("glUniform1f"))==NULL)puts("Can't load glUniform1f");
    if((glf->glUniform2f=SDL_GL_GetProcAddress("glUniform2f"))==NULL)puts("Can't load glUniform2f");
    if((glf->glUniform3f=SDL_GL_GetProcAddress("glUniform3f"))==NULL)puts("Can't load glUniform3f");
    if((glf->glUniform4f=SDL_GL_GetProcAddress("glUniform4f"))==NULL)puts("Can't load glUniform4f");
    if((glf->glUniform1fv=SDL_GL_GetProcAddress("glUniform1fv"))==NULL)puts("Can't load glUniform1fv");
    if((glf->glUniform2fv=SDL_GL_GetProcAddress("glUniform2fv"))==NULL)puts("Can't load glUniform2fv");
    if((glf->glUniform3fv=SDL_GL_GetProcAddress("glUniform3fv"))==NULL)puts("Can't load glUniform3fv");
    if((glf->glUniform4fv=SDL_GL_GetProcAddress("glUniform4fv"))==NULL)puts("Can't load glUniform4fv");


    return glf;
}

void free_gl_functions(gl_functions * glf)
{
    ///perhaps relinquish resources/links if necessary
    free(glf);
}


int load_contextual_resources()
{
//    glGenVertexArrays_ptr= 0;
//    glGenVertexArrays_ptr=(GL_GenVertexArrays_Func) SDL_GL_GetProcAddress("glGenVertexArrays");
//    if (glGenVertexArrays_ptr==NULL){printf("Can't load glGenVertexArrays");}
//
//    glGenBuffers_ptr= 0;
//    glGenBuffers_ptr=(GL_GenBuffers_Func) SDL_GL_GetProcAddress("glGenBuffers");
//    if (glGenVertexArrays_ptr==NULL){printf("Can't load glGenBuffers");}
//
//    glBindBuffer_ptr= 0;
//    glBindBuffer_ptr=(GL_BindBuffer_Func) SDL_GL_GetProcAddress("glBindBuffer");
//    if (glBindBuffer_ptr==NULL){printf("Can't load glBindBuffer");}
//
//    glBufferData_ptr= 0;
//    glBufferData_ptr=(GL_BufferData_Func) SDL_GL_GetProcAddress("glBufferData");
//    if (glBufferData_ptr==NULL){printf("Can't load glBufferData");}
//
//    glEnableVertexAttribArray_ptr= 0;
//    glEnableVertexAttribArray_ptr=(GL_EnableVertexAttribArray_Func) SDL_GL_GetProcAddress("glEnableVertexAttribArray");
//    if (glEnableVertexAttribArray_ptr==NULL){printf("Can't load glEnableVertexAttribArray");}
//
//    glVertexAttribPointer_ptr= 0;
//    glVertexAttribPointer_ptr=(GL_VertexAttribPointer_Func) SDL_GL_GetProcAddress("glVertexAttribPointer");
//    if (glVertexAttribPointer_ptr==NULL){printf("Can't load glVertexAttribPointer");}
//
//    glDisableVertexAttribArray_ptr= 0;
//    glDisableVertexAttribArray_ptr=(GL_DisableVertexAttribArray_Func) SDL_GL_GetProcAddress("glDisableVertexAttribArray");
//    if (glDisableVertexAttribArray_ptr==NULL){printf("Can't load glDisableVertexAttribArray");}
//
//    glBufferSubData_ptr= 0;
//    glBufferSubData_ptr=(GL_BufferSubData_Func) SDL_GL_GetProcAddress("glBufferSubData");
//    if (glBufferSubData_ptr==NULL){printf("Can't load glBufferSubData");}
//
//    glCreateShader_ptr= 0;
//    glCreateShader_ptr=(GL_CreateShader_Func) SDL_GL_GetProcAddress("glCreateShader");
//    if (glCreateShader_ptr==NULL){printf("Can't load glCreateShader");}
//
//    glCreateProgram_ptr= 0;
//    glCreateProgram_ptr=(GL_CreateProgram_Func) SDL_GL_GetProcAddress("glCreateProgram");
//    if (glCreateProgram_ptr==NULL){printf("Can't load glCreateProgram");}
//
//    glShaderSource_ptr= 0;
//    glShaderSource_ptr=(GL_ShaderSource_Func) SDL_GL_GetProcAddress("glShaderSource");
//    if (glShaderSource_ptr==NULL){printf("Can't load glShaderSource");}
//
//    glCompileShader_ptr= 0;
//    glCompileShader_ptr=(GL_CompileShader_Func) SDL_GL_GetProcAddress("glCompileShader");
//    if (glCompileShader_ptr==NULL){printf("Can't load glCompileShader");}
//
//    glGetShaderiv_ptr= 0;
//    glGetShaderiv_ptr=(GL_GetShaderiv_Func) SDL_GL_GetProcAddress("glGetShaderiv");
//    if (glGetShaderiv_ptr==NULL){printf("Can't load glGetShaderiv");}
//
//    glGetShaderInfoLog_ptr= 0;
//    glGetShaderInfoLog_ptr=(GL_GetShaderInfoLog_Func) SDL_GL_GetProcAddress("glGetShaderInfoLog");
//    if (glGetShaderInfoLog_ptr==NULL){printf("Can't load glGetShaderInfoLog");}
//
//    glAttachShader_ptr= 0;
//    glAttachShader_ptr=(GL_AttachShader_Func) SDL_GL_GetProcAddress("glAttachShader");
//    if (glAttachShader_ptr==NULL){printf("Can't load glAttachShader");}
//
//    glDetachShader_ptr= 0;
//    glDetachShader_ptr=(GL_DetachShader_Func) SDL_GL_GetProcAddress("glDetachShader");
//    if (glDetachShader_ptr==NULL){printf("Can't load glDetachShader");}
//
//    glDeleteShader_ptr= 0;
//    glDeleteShader_ptr=(GL_DeleteShader_Func) SDL_GL_GetProcAddress("glDeleteShader");
//    if (glDeleteShader_ptr==NULL){printf("Can't load glDeleteShader");}
//
//    glLinkProgram_ptr= 0;
//    glLinkProgram_ptr=(GL_LinkProgram_Func) SDL_GL_GetProcAddress("glLinkProgram");
//    if (glLinkProgram_ptr==NULL){printf("Can't load glLinkProgram");}
//
//    glGetProgramiv_ptr= 0;
//    glGetProgramiv_ptr=(GL_GetProgramiv_Func) SDL_GL_GetProcAddress("glGetProgramiv");
//    if (glGetProgramiv_ptr==NULL){printf("Can't load glGetProgramiv");}
//
//    glGetProgramInfoLog_ptr= 0;
//    glGetProgramInfoLog_ptr=(GL_GetProgramInfoLog_Func) SDL_GL_GetProcAddress("glGetProgramInfoLog");
//    if (glGetProgramInfoLog_ptr==NULL){printf("Can't load glGetProgramInfoLog");}
//
//    glValidateProgram_ptr= 0;
//    glValidateProgram_ptr=(GL_ValidateProgram_Func) SDL_GL_GetProcAddress("glValidateProgram");
//    if (glValidateProgram_ptr==NULL){printf("Can't load glValidateProgram");}
//
//    glUseProgram_ptr= 0;
//    glUseProgram_ptr=(GL_UseProgram_Func) SDL_GL_GetProcAddress("glUseProgram");
//    if (glUseProgram_ptr==NULL){printf("Can't load glUseProgram");}
//
//    glGetUniformLocation_ptr=0;
//    glGetUniformLocation_ptr=(GL_GetUniformLocation_Func) SDL_GL_GetProcAddress("glGetUniformLocation");
//    if (glGetUniformLocation_ptr==NULL){printf("Can't load glGetUniformLocation");}
//
//    glUniformMatrix4fv_ptr=0;
//    glUniformMatrix4fv_ptr=(GL_UniformMatrix4fv_Func) SDL_GL_GetProcAddress("glUniformMatrix4fv");
//    if (glUniformMatrix4fv_ptr==NULL){printf("Can't load glUniformMatrix4fv");}
//
//    glUniformMatrix3fv_ptr=0;
//    glUniformMatrix3fv_ptr=(GL_UniformMatrix3fv_Func) SDL_GL_GetProcAddress("glUniformMatrix3fv");
//    if (glUniformMatrix3fv_ptr==NULL){printf("Can't load glUniformMatrix3fv");}
//
//    glUniform4fv_ptr=0;
//    glUniform4fv_ptr=(GL_Uniform4fv_Func) SDL_GL_GetProcAddress("glUniform4fv");
//    if (glUniform4fv_ptr==NULL){printf("Can't load glUniform4fv");}
//
//    glUniform3fv_ptr=0;
//    glUniform3fv_ptr=(GL_Uniform3fv_Func) SDL_GL_GetProcAddress("glUniform3fv");
//    if (glUniform3fv_ptr==NULL){printf("Can't load glUniform3fv");}
//
//    glUniform1fv_ptr=0;
//    glUniform1fv_ptr=(GL_Uniform1fv_Func) SDL_GL_GetProcAddress("glUniform1fv");
//    if (glUniform1fv_ptr==NULL){printf("Can't load glUniform1fv");}
//
//    glUniform1iv_ptr=0;
//    glUniform1iv_ptr=(GL_Uniform1iv_Func) SDL_GL_GetProcAddress("glUniform1iv");
//    if (glUniform1iv_ptr==NULL){printf("Can't load glUniform1iv");}
//
//    glUniform1uiv_ptr=0;
//    glUniform1uiv_ptr=(GL_Uniform1uiv_Func) SDL_GL_GetProcAddress("glUniform1uiv");
//    if (glUniform1uiv_ptr==NULL){printf("Can't load glUniform1uiv");}
//
//    glUniform4uiv_ptr=0;
//    glUniform4uiv_ptr=(GL_Uniform4uiv_Func) SDL_GL_GetProcAddress("glUniform4uiv");
//    if (glUniform4uiv_ptr==NULL){printf("Can't load glUniform4uiv");}
//
//    glUniform1i_ptr=0;
//    glUniform1i_ptr=(GL_Uniform1i_Func) SDL_GL_GetProcAddress("glUniform1i");
//    if (glUniform1i_ptr==NULL){printf("Can't load glUniform1i");}
//
//    glUniform1ui_ptr=0;
//    glUniform1ui_ptr=(GL_Uniform1ui_Func) SDL_GL_GetProcAddress("glUniform1ui");
//    if (glUniform1ui_ptr==NULL){printf("Can't load glUniform1ui");}
//
//    glUniform4ui_ptr=0;
//    glUniform4ui_ptr=(GL_Uniform4ui_Func) SDL_GL_GetProcAddress("glUniform4ui");
//    if (glUniform4ui_ptr==NULL){printf("Can't load glUniform4ui");}
//
//    glUniform1f_ptr=0;
//    glUniform1f_ptr=(GL_Uniform1f_Func) SDL_GL_GetProcAddress("glUniform1f");
//    if (glUniform1f_ptr==NULL){printf("Can't load glUniform1f");}
//
//    glUniform4f_ptr=0;
//    glUniform4f_ptr=(GL_Uniform4f_Func) SDL_GL_GetProcAddress("glUniform4f");
//    if (glUniform4f_ptr==NULL){printf("Can't load glUniform4f");}
//
//    glUniform3f_ptr=0;
//    glUniform3f_ptr=(GL_Uniform3f_Func) SDL_GL_GetProcAddress("glUniform3f");
//    if (glUniform3f_ptr==NULL){printf("Can't load glUniform3f");}
//
//    glUniform2f_ptr=0;
//    glUniform2f_ptr=(GL_Uniform2f_Func) SDL_GL_GetProcAddress("glUniform2f");
//    if (glUniform2f_ptr==NULL){printf("Can't load glUniform2f");}
//
//    glStencilOpSeparate_ptr=0;
//    glStencilOpSeparate_ptr=(GL_StencilOpSeparate_Func) SDL_GL_GetProcAddress("glStencilOpSeparate");
//    if (glStencilOpSeparate_ptr==NULL){printf("Can't load glStencilOpSeparate");}
//
//    glBindVertexArray_ptr=0;
//    glBindVertexArray_ptr=(GL_BindVertexArray_Func) SDL_GL_GetProcAddress("glBindVertexArray");
//    if (glBindVertexArray_ptr==NULL){printf("Can't load glBindVertexArray");}
//
//    glDeleteBuffers_ptr=0;
//    glDeleteBuffers_ptr=(GL_DeleteBuffers_Func) SDL_GL_GetProcAddress("glDeleteBuffers");
//    if (glDeleteBuffers_ptr==NULL){printf("Can't load glDeleteBuffers");}
//
//    glBindBufferRange_ptr=0;
//    glBindBufferRange_ptr=(GL_BindBufferRange_Func) SDL_GL_GetProcAddress("glBindBufferRange");
//    if (glBindBufferRange_ptr==NULL){printf("Can't load glBindBufferRange");}
//
//    glShaderStorageBlockBinding_ptr=0;
//    glShaderStorageBlockBinding_ptr=(GL_ShaderStorageBlockBinding_Func) SDL_GL_GetProcAddress("glShaderStorageBlockBinding");
//    if (glShaderStorageBlockBinding_ptr==NULL){printf("Can't load glShaderStorageBlockBinding");}
//
//    glBindBufferBase_ptr=0;
//    glBindBufferBase_ptr=(GL_BindBufferBase_Func) SDL_GL_GetProcAddress("glBindBufferBase");
//    if (glBindBufferBase_ptr==NULL){printf("Can't load glBindBufferBase");}
//
//    glVertexAttribIPointer_ptr=0;
//    glVertexAttribIPointer_ptr=(GL_VertexAttribIPointer_Func) SDL_GL_GetProcAddress("glVertexAttribIPointer");
//    if (glVertexAttribIPointer_ptr==NULL){printf("Can't load glVertexAttribIPointer");}
//
//    glGenFramebuffers_ptr=0;
//    glGenFramebuffers_ptr=(GL_GenFramebuffers_Func) SDL_GL_GetProcAddress("glGenFramebuffers");
//    if (glGenFramebuffers_ptr==NULL){printf("Can't load glGenFramebuffers");}
//
//    glBindFramebuffer_ptr=0;
//    glBindFramebuffer_ptr=(GL_BindFramebuffer_Func) SDL_GL_GetProcAddress("glBindFramebuffer");
//    if (glBindFramebuffer_ptr==NULL){printf("Can't load glBindFramebuffer");}
//
//    glDeleteFramebuffers_ptr=0;
//    glDeleteFramebuffers_ptr=(GL_DeleteFramebuffers_Func) SDL_GL_GetProcAddress("glDeleteFramebuffers");
//    if (glDeleteFramebuffers_ptr==NULL){printf("Can't load glDeleteFramebuffers");}
//
//    glFramebufferTexture2D_ptr=0;
//    glFramebufferTexture2D_ptr=(GL_FramebufferTexture2D_Func) SDL_GL_GetProcAddress("glFramebufferTexture2D");
//    if (glFramebufferTexture2D_ptr==NULL){printf("Can't load glFramebufferTexture2D");}
//
//    glFramebufferTexture3D_ptr=0;
//    glFramebufferTexture3D_ptr=(GL_FramebufferTexture3D_Func) SDL_GL_GetProcAddress("glFramebufferTexture3D");
//    if (glFramebufferTexture3D_ptr==NULL){printf("Can't load glFramebufferTexture3D");}
//
//    glBlitFramebuffer_ptr=0;
//    glBlitFramebuffer_ptr=(GL_BlitFramebuffer_Func) SDL_GL_GetProcAddress("glBlitFramebuffer");
//    if (glBlitFramebuffer_ptr==NULL){printf("Can't load glBlitFramebuffer");}
//
//    glDrawBuffers_ptr=0;
//    glDrawBuffers_ptr=(GL_DrawBuffers_Func) SDL_GL_GetProcAddress("glDrawBuffers");
//    if (glDrawBuffers_ptr==NULL){printf("Can't load glDrawBuffers");}
//
//    glUniform2fv_ptr=0;
//    glUniform2fv_ptr=(GL_Uniform2fv_Func) SDL_GL_GetProcAddress("glUniform2fv");
//    if (glUniform2fv_ptr==NULL){printf("Can't load glUniform2fv");}
//
//    glBindImageTexture_ptr=0;
//    glBindImageTexture_ptr=(GL_BindImageTexture_Func) SDL_GL_GetProcAddress("glBindImageTexture");
//    if (glBindImageTexture_ptr==NULL){printf("Can't load glBindImageTexture");}
//
//    glDrawElementsBaseVertex_ptr=0;
//    glDrawElementsBaseVertex_ptr=(GL_DrawElementsBaseVertex_Func) SDL_GL_GetProcAddress("glDrawElementsBaseVertex");
//    if (glDrawElementsBaseVertex_ptr==NULL){printf("Can't load glDrawElementsBaseVertex");}
//
//    glVertexAttribDivisor_ptr=0;
//    glVertexAttribDivisor_ptr=(GL_VertexAttribDivisor_Func) SDL_GL_GetProcAddress("glVertexAttribDivisor");
//    if (glVertexAttribDivisor_ptr==NULL){printf("Can't load glVertexAttribDivisor");}
//
//    glDrawElementsInstanced_ptr=0;
//    glDrawElementsInstanced_ptr=(GL_DrawElementsInstanced_Func) SDL_GL_GetProcAddress("glDrawElementsInstanced");
//    if (glDrawElementsInstanced_ptr==NULL){printf("Can't load glDrawElementsInstanced");}
//
//    glDrawElementsInstancedBaseVertex_ptr=0;
//    glDrawElementsInstancedBaseVertex_ptr=(GL_DrawElementsInstancedBaseVertex_Func) SDL_GL_GetProcAddress("glDrawElementsInstancedBaseVertex");
//    if (glDrawElementsInstancedBaseVertex_ptr==NULL){printf("Can't load glDrawElementsInstancedBaseVertex");}
//
//    glTexBuffer_ptr=0;
//    glTexBuffer_ptr=(GL_TexBuffer_Func) SDL_GL_GetProcAddress("glTexBuffer");
//    if (glTexBuffer_ptr==NULL){printf("Can't load glTexBuffer");}
//
//    glMapBufferRange_ptr=0;
//    glMapBufferRange_ptr=(GL_MapBufferRange_Func) SDL_GL_GetProcAddress("glMapBufferRange");
//    if (glMapBufferRange_ptr==NULL){printf("Can't load glMapBufferRange");}
//
//    glMemoryBarrier_ptr=0;
//    glMemoryBarrier_ptr=(GL_MemoryBarrier_Func) SDL_GL_GetProcAddress("glMemoryBarrier");
//    if (glMemoryBarrier_ptr==NULL){printf("Can't load glMemoryBarrier");}
//
//    glUnmapBuffer_ptr=0;
//    glUnmapBuffer_ptr=(GL_UnmapBuffer_Func) SDL_GL_GetProcAddress("glUnmapBuffer");
//    if (glUnmapBuffer_ptr==NULL){printf("Can't load glUnmapBuffer");}
//
//    glDrawElementsInstancedBaseInstance_ptr=0;
//    glDrawElementsInstancedBaseInstance_ptr=(GL_DrawElementsInstancedBaseInstance_Func) SDL_GL_GetProcAddress("glDrawElementsInstancedBaseInstance");
//    if (glDrawElementsInstancedBaseInstance_ptr==NULL){printf("Can't load glDrawElementsInstancedBaseInstance");}
//
////    glDrawElementsInstancedBaseVertexBaseInstance_ptr=0;
////    glDrawElementsInstancedBaseVertexBaseInstance_ptr=(GL_DrawElementsInstancedBaseVertexBaseInstance_Func) SDL_GL_GetProcAddress("glDrawElementsInstancedBaseVertexBaseInstance");
////    if (glDrawElementsInstancedBaseVertexBaseInstance_ptr==NULL){printf("Can't load glDrawElementsInstancedBaseVertexBaseInstance");}
//
////    glUniformSubroutinesuiv_ptr=0;
////    glUniformSubroutinesuiv_ptr=(GL_UniformSubroutinesuiv_Func) SDL_GL_GetProcAddress("glUniformSubroutinesuiv");
////    if (glUniformSubroutinesuiv_ptr==NULL){printf("Can't load glUniformSubroutinesuiv");}
//
//    glGenTransformFeedbacks_ptr=0;
//    glGenTransformFeedbacks_ptr=(GL_GenTransformFeedbacks_Func) SDL_GL_GetProcAddress("glGenTransformFeedbacks");
//    if (glGenTransformFeedbacks_ptr==NULL){printf("Can't load glGenTransformFeedbacks");}
//
//    glBindTransformFeedback_ptr=0;
//    glBindTransformFeedback_ptr=(GL_BindTransformFeedback_Func) SDL_GL_GetProcAddress("glBindTransformFeedback");
//    if (glBindTransformFeedback_ptr==NULL){printf("Can't load glBindTransformFeedback");}
//
//    glBeginTransformFeedback_ptr=0;
//    glBeginTransformFeedback_ptr=(GL_BeginTransformFeedback_Func) SDL_GL_GetProcAddress("glBeginTransformFeedback");
//    if (glBeginTransformFeedback_ptr==NULL){printf("Can't load glBeginTransformFeedback");}
//
//    glTransformFeedbackVaryings_ptr=0;
//    glTransformFeedbackVaryings_ptr=(GL_TransformFeedbackVaryings_Func) SDL_GL_GetProcAddress("glTransformFeedbackVaryings");
//    if (glTransformFeedbackVaryings_ptr==NULL){printf("Can't load glTransformFeedbackVaryings");}
//
//    glEndTransformFeedback_ptr=0;
//    glEndTransformFeedback_ptr=(GL_EndTransformFeedback_Func) SDL_GL_GetProcAddress("glEndTransformFeedback");
//    if (glEndTransformFeedback_ptr==NULL){printf("Can't load glEndTransformFeedback");}
//
//    glDrawTransformFeedback_ptr=0;
//    glDrawTransformFeedback_ptr=(GL_DrawTransformFeedback_Func) SDL_GL_GetProcAddress("glDrawTransformFeedback");
//    if (glDrawTransformFeedback_ptr==NULL){printf("Can't load glDrawTransformFeedback");}
//
//    glMultiDrawElementsIndirect_ptr=0;
//    glMultiDrawElementsIndirect_ptr=(GL_MultiDrawElementsIndirect_Func) SDL_GL_GetProcAddress("glMultiDrawElementsIndirect");
//    if (glMultiDrawElementsIndirect_ptr==NULL){printf("Can't load glMultiDrawElementsIndirect");}
//
//    glDrawElementsIndirect_ptr=0;
//    glDrawElementsIndirect_ptr=(GL_DrawElementsIndirect_Func) SDL_GL_GetProcAddress("glDrawElementsIndirect");
//    if (glDrawElementsIndirect_ptr==NULL){printf("Can't load glDrawElementsIndirect");}
//
//    glBufferStorage_ptr=0;
//    glBufferStorage_ptr=(GL_BufferStorage_Func) SDL_GL_GetProcAddress("glBufferStorage");
//    if (glBufferStorage_ptr==NULL){printf("Can't load glBufferStorage");}
//
//    glFramebufferParameteri_ptr=0;
//    glFramebufferParameteri_ptr=(GL_FramebufferParameteri_Func) SDL_GL_GetProcAddress("glFramebufferParameteri");
//    if (glFramebufferParameteri_ptr==NULL){printf("Can't load glFramebufferParameteri");}
//
//    glDrawArraysInstanced_ptr=0;
//    glDrawArraysInstanced_ptr=(GL_DrawArraysInstanced_Func) SDL_GL_GetProcAddress("glDrawArraysInstanced");
//    if (glDrawArraysInstanced_ptr==NULL){printf("Can't load glDrawArraysInstanced");}
//
//    glInvalidateBufferData_ptr=0;
//    glInvalidateBufferData_ptr=(GL_InvalidateBufferData_Func) SDL_GL_GetProcAddress("glInvalidateBufferData");
//    if (glInvalidateBufferData_ptr==NULL){printf("Can't load glInvalidateBufferData");}
//
//    glBlendFuncSeparate_ptr=0;
//    glBlendFuncSeparate_ptr=(GL_BlendFuncSeparate_Func) SDL_GL_GetProcAddress("glBlendFuncSeparate");
//    if (glBlendFuncSeparate_ptr==NULL){printf("Can't load glBlendFuncSeparate");}
//
//    glTexImage2DMultisample_ptr=0;
//    glTexImage2DMultisample_ptr=(GL_TexImage2DMultisample_Func) SDL_GL_GetProcAddress("glTexImage2DMultisample");
//    if (glTexImage2DMultisample_ptr==NULL){printf("Can't load glTexImage2DMultisample");}
//
//    glTexImage3DMultisample_ptr=0;
//    glTexImage3DMultisample_ptr=(GL_TexImage3DMultisample_Func) SDL_GL_GetProcAddress("glTexImage3DMultisample");
//    if (glTexImage3DMultisample_ptr==NULL){printf("Can't load glTexImage3DMultisample");}
//
//    glClearTexImage_ptr=0;
//    glClearTexImage_ptr=(GL_ClearTexImage_Func) SDL_GL_GetProcAddress("glClearTexImage");
//    if (glClearTexImage_ptr==NULL){printf("Can't load glClearTexImage");}
//
//    glClipControl_ptr=0;
//    glClipControl_ptr=(GL_ClipControl_Func) SDL_GL_GetProcAddress("glClipControl");
//    if (glClipControl_ptr==NULL){printf("Can't load glClipControl");}
//
//    glGetAttribLocation_ptr=0;
//    glGetAttribLocation_ptr=(GL_GetAttribLocation_Func) SDL_GL_GetProcAddress("glGetAttribLocation");
//    if (glGetAttribLocation_ptr==NULL){printf("Can't load glGetAttribLocation");}
//
//    glDrawArraysInstancedBaseInstance_ptr=0;
//    glDrawArraysInstancedBaseInstance_ptr=(GL_DrawArraysInstancedBaseInstance_Func) SDL_GL_GetProcAddress("glDrawArraysInstancedBaseInstance");
//    if (glDrawArraysInstancedBaseInstance_ptr==NULL){printf("Can't load glDrawArraysInstancedBaseInstance");}
//
//    glGenQueries_ptr=0;
//    glGenQueries_ptr=(GL_GenQueries_Func) SDL_GL_GetProcAddress("glGenQueries");
//    if (glGenQueries_ptr==NULL){printf("Can't load glGenQueries");}
//
//    glBeginQuery_ptr=0;
//    glBeginQuery_ptr=(GL_BeginQuery_Func) SDL_GL_GetProcAddress("glBeginQuery");
//    if (glBeginQuery_ptr==NULL){printf("Can't load glBeginQuery");}
//
//    glEndQuery_ptr=0;
//    glEndQuery_ptr=(GL_EndQuery_Func) SDL_GL_GetProcAddress("glEndQuery");
//    if (glEndQuery_ptr==NULL){printf("Can't load glEndQuery");}
//
//    glGetQueryObjectiv_ptr=0;
//    glGetQueryObjectiv_ptr=(GL_GetQueryObjectiv_Func) SDL_GL_GetProcAddress("glGetQueryObjectiv");
//    if (glGetQueryObjectiv_ptr==NULL){printf("Can't load glGetQueryObjectiv");}
//
//    glUniformMatrix2fv_ptr=0;
//    glUniformMatrix2fv_ptr=(GL_UniformMatrix2fv_Func) SDL_GL_GetProcAddress("glUniformMatrix2fv");
//    if (glUniformMatrix2fv_ptr==NULL){printf("Can't load glUniformMatrix2fv");}
//
//    glPointParameteri_ptr=0;
//    glPointParameteri_ptr=(GL_glPointParameteri_Func) SDL_GL_GetProcAddress("glPointParameteri");
//    if (glPointParameteri_ptr==NULL){printf("Can't load glPointParameteri");}
//
//    glPointParameterf_ptr=0;
//    glPointParameterf_ptr=(GL_glPointParameterf_Func) SDL_GL_GetProcAddress("glPointParameterf");
//    if (glPointParameterf_ptr==NULL){printf("Can't load glPointParameterf");}
//
//    /*glTextureBarrier_ptr=0;
//    glTextureBarrier_ptr=(GL_TextureBarrier_Func) SDL_GL_GetProcAddress("glTextureBarrier");
//    if (glTextureBarrier_ptr==NULL){printf("Can't load glTextureBarrier");}*/
//


    return 0;
}





