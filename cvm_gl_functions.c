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

    glf->glGenVertexArrays=SDL_GL_GetProcAddress("glGenVertexArrays");
    glf->glGenBuffers=SDL_GL_GetProcAddress("glGenBuffers");
    glf->glBindBuffer=SDL_GL_GetProcAddress("glBindBuffer");
    glf->glBufferData=SDL_GL_GetProcAddress("glBufferData");
    glf->glEnableVertexAttribArray=SDL_GL_GetProcAddress("glEnableVertexAttribArray");
    glf->glVertexAttribPointer=SDL_GL_GetProcAddress("glVertexAttribPointer");
    glf->glVertexAttribIPointer=SDL_GL_GetProcAddress("glVertexAttribIPointer");
    glf->glDisableVertexAttribArray=SDL_GL_GetProcAddress("glDisableVertexAttribArray");
    glf->glBufferSubData=SDL_GL_GetProcAddress("glBufferSubData");
    glf->glCreateShader=SDL_GL_GetProcAddress("glCreateShader");
    glf->glCreateProgram=SDL_GL_GetProcAddress("glCreateProgram");
    glf->glShaderSource=SDL_GL_GetProcAddress("glShaderSource");
    glf->glCompileShader=SDL_GL_GetProcAddress("glCompileShader");
    glf->glGetShaderiv=SDL_GL_GetProcAddress("glGetShaderiv");
    glf->glGetShaderInfoLog=SDL_GL_GetProcAddress("glGetShaderInfoLog");
    glf->glAttachShader=SDL_GL_GetProcAddress("glAttachShader");
    glf->glDetachShader=SDL_GL_GetProcAddress("glDetachShader");
    glf->glDeleteShader=SDL_GL_GetProcAddress("glDeleteShader");
    glf->glLinkProgram=SDL_GL_GetProcAddress("glLinkProgram");
    glf->glGetProgramiv=SDL_GL_GetProcAddress("glGetProgramiv");
    glf->glGetProgramInfoLog=SDL_GL_GetProcAddress("glGetProgramInfoLog");
    glf->glValidateProgram=SDL_GL_GetProcAddress("glValidateProgram");
    glf->glUseProgram=SDL_GL_GetProcAddress("glUseProgram");
    glf->glGetString=SDL_GL_GetProcAddress("glGetString");
    glf->glGenTextures=SDL_GL_GetProcAddress("glGenTextures");
    glf->glBindTexture=SDL_GL_GetProcAddress("glBindTexture");
    glf->glTexParameteri=SDL_GL_GetProcAddress("glTexParameteri");
    glf->glTexBuffer=SDL_GL_GetProcAddress("glTexBuffer");
    glf->glMapBufferRange=SDL_GL_GetProcAddress("glMapBufferRange");
    glf->glUnmapBuffer=SDL_GL_GetProcAddress("glUnmapBuffer");
    glf->glActiveTexture=SDL_GL_GetProcAddress("glActiveTexture");
    glf->glBindImageTexture=SDL_GL_GetProcAddress("glBindImageTexture");
    glf->glVertexAttribDivisor=SDL_GL_GetProcAddress("glVertexAttribDivisor");
    glf->glBindVertexArray=SDL_GL_GetProcAddress("glBindVertexArray");
    glf->glGetAttribLocation=SDL_GL_GetProcAddress("glGetAttribLocation");
    glf->glGenFramebuffers=SDL_GL_GetProcAddress("glGenFramebuffers");
    glf->glBindFramebuffer=SDL_GL_GetProcAddress("glBindFramebuffer");
    glf->glFramebufferTexture2D=SDL_GL_GetProcAddress("glFramebufferTexture2D");
    glf->glDrawBuffers=SDL_GL_GetProcAddress("glDrawBuffers");
    glf->glDrawBuffer=SDL_GL_GetProcAddress("glDrawBuffer");
    glf->glViewport=SDL_GL_GetProcAddress("glViewport");
    glf->glEnable=SDL_GL_GetProcAddress("glEnable");
    glf->glDisable=SDL_GL_GetProcAddress("glDisable");
    glf->glBlendFunc=SDL_GL_GetProcAddress("glBlendFunc");
    glf->glGetUniformLocation=SDL_GL_GetProcAddress("glGetUniformLocation");
    glf->glDrawArraysInstanced=SDL_GL_GetProcAddress("glDrawArraysInstanced");
    glf->glDrawArrays=SDL_GL_GetProcAddress("glDrawArrays");
    glf->glGetError=SDL_GL_GetProcAddress("glGetError");
    glf->glLineWidth=SDL_GL_GetProcAddress("glLineWidth");
    glf->glPointSize=SDL_GL_GetProcAddress("glPointSize");
    glf->glTexImage1D=SDL_GL_GetProcAddress("glTexImage1D");
    glf->glTexImage2D=SDL_GL_GetProcAddress("glTexImage2D");
    glf->glTexImage2DMultisample=SDL_GL_GetProcAddress("glTexImage2DMultisample");
    glf->glBufferStorage=SDL_GL_GetProcAddress("glBufferStorage");
    glf->glMultiDrawElementsIndirect=SDL_GL_GetProcAddress("glMultiDrawElementsIndirect");
    glf->glDrawElementsInstanced=SDL_GL_GetProcAddress("glDrawElementsInstanced");
    glf->glClearColor=SDL_GL_GetProcAddress("glClearColor");
    glf->glClear=SDL_GL_GetProcAddress("glClear");
    glf->glFinish=SDL_GL_GetProcAddress("glFinish");
    glf->glTexSubImage2D=SDL_GL_GetProcAddress("glTexSubImage2D");
    glf->glClipControl=SDL_GL_GetProcAddress("glClipControl");
    glf->glPointParameterf=SDL_GL_GetProcAddress("glPointParameterf");
    glf->glPointParameteri=SDL_GL_GetProcAddress("glPointParameteri");
    glf->glStencilOpSeparate=SDL_GL_GetProcAddress("glStencilOpSeparate");
    glf->glDepthRange=SDL_GL_GetProcAddress("glDepthRange");
    glf->glDepthMask=SDL_GL_GetProcAddress("glDepthMask");
    glf->glDepthFunc=SDL_GL_GetProcAddress("glDepthFunc");
    glf->glCullFace=SDL_GL_GetProcAddress("glCullFace");
    glf->glClearDepth=SDL_GL_GetProcAddress("glClearDepth");
    glf->glClearStencil=SDL_GL_GetProcAddress("glClearStencil");
    glf->glStencilFunc=SDL_GL_GetProcAddress("glStencilFunc");
    glf->glStencilOp=SDL_GL_GetProcAddress("glStencilOp");


    glf->glUniform1i=SDL_GL_GetProcAddress("glUniform1i");
    glf->glUniform2i=SDL_GL_GetProcAddress("glUniform2i");
    glf->glUniform3i=SDL_GL_GetProcAddress("glUniform3i");
    glf->glUniform4i=SDL_GL_GetProcAddress("glUniform4i");
    glf->glUniform1iv=SDL_GL_GetProcAddress("glUniform1iv");
    glf->glUniform2iv=SDL_GL_GetProcAddress("glUniform2iv");
    glf->glUniform3iv=SDL_GL_GetProcAddress("glUniform3iv");
    glf->glUniform4iv=SDL_GL_GetProcAddress("glUniform4iv");
    glf->glUniform1ui=SDL_GL_GetProcAddress("glUniform1ui");
    glf->glUniform2ui=SDL_GL_GetProcAddress("glUniform2ui");
    glf->glUniform3ui=SDL_GL_GetProcAddress("glUniform3ui");
    glf->glUniform4ui=SDL_GL_GetProcAddress("glUniform4ui");
    glf->glUniform1uiv=SDL_GL_GetProcAddress("glUniform1uiv");
    glf->glUniform2uiv=SDL_GL_GetProcAddress("glUniform2uiv");
    glf->glUniform3uiv=SDL_GL_GetProcAddress("glUniform3uiv");
    glf->glUniform4uiv=SDL_GL_GetProcAddress("glUniform4uiv");
    glf->glUniform1f=SDL_GL_GetProcAddress("glUniform1f");
    glf->glUniform2f=SDL_GL_GetProcAddress("glUniform2f");
    glf->glUniform3f=SDL_GL_GetProcAddress("glUniform3f");
    glf->glUniform4f=SDL_GL_GetProcAddress("glUniform4f");
    glf->glUniform1fv=SDL_GL_GetProcAddress("glUniform1fv");
    glf->glUniform2fv=SDL_GL_GetProcAddress("glUniform2fv");
    glf->glUniform3fv=SDL_GL_GetProcAddress("glUniform3fv");
    glf->glUniform4fv=SDL_GL_GetProcAddress("glUniform4fv");
    glf->glUniformMatrix2fv=SDL_GL_GetProcAddress("glUniformMatrix2fv");
    glf->glUniformMatrix3fv=SDL_GL_GetProcAddress("glUniformMatrix3fv");
    glf->glUniformMatrix4fv=SDL_GL_GetProcAddress("glUniformMatrix4fv");
    glf->glUniformMatrix2x3fv=SDL_GL_GetProcAddress("glUniformMatrix2x3fv");
    glf->glUniformMatrix3x2fv=SDL_GL_GetProcAddress("glUniformMatrix3x2fv");
    glf->glUniformMatrix2x4fv=SDL_GL_GetProcAddress("glUniformMatrix2x4fv");
    glf->glUniformMatrix4x2fv=SDL_GL_GetProcAddress("glUniformMatrix4x2fv");
    glf->glUniformMatrix3x4fv=SDL_GL_GetProcAddress("glUniformMatrix3x4fv");
    glf->glUniformMatrix4x3fv=SDL_GL_GetProcAddress("glUniformMatrix4x3fv");


    return glf;
}

void free_gl_functions(gl_functions * glf)
{
    ///perhaps relinquish resources/links if necessary
    free(glf);
}







