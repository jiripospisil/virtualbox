/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef CLEAR_H
#define CLEAR_H


#include "glheader.h"


extern void GLAPIENTRY
_mesa_ClearIndex( GLfloat c );

extern void GLAPIENTRY
_mesa_ClearColor( GLclampf red, GLclampf green,
                  GLclampf blue, GLclampf alpha );

extern void GLAPIENTRY
_mesa_ClearColorIiEXT(GLint r, GLint g, GLint b, GLint a);

extern void GLAPIENTRY
_mesa_ClearColorIuiEXT(GLuint r, GLuint g, GLuint b, GLuint a);

void GLAPIENTRY
_mesa_Clear_no_error(GLbitfield mask);

extern void GLAPIENTRY
_mesa_Clear( GLbitfield mask );

void GLAPIENTRY
_mesa_ClearBufferiv_no_error(GLenum buffer, GLint drawbuffer,
                             const GLint *value);

extern void GLAPIENTRY
_mesa_ClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint *value);

extern void GLAPIENTRY
_mesa_ClearNamedFramebufferiv(GLuint framebuffer, GLenum buffer,
                              GLint drawbuffer, const GLint *value);

void GLAPIENTRY
_mesa_ClearBufferuiv_no_error(GLenum buffer, GLint drawbuffer,
                              const GLuint *value);

extern void GLAPIENTRY
_mesa_ClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint *value);

extern void GLAPIENTRY
_mesa_ClearNamedFramebufferuiv(GLuint framebuffer, GLenum buffer,
                               GLint drawbuffer, const GLuint *value);

void GLAPIENTRY
_mesa_ClearBufferfv_no_error(GLenum buffer, GLint drawbuffer,
                             const GLfloat *value);

extern void GLAPIENTRY
_mesa_ClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat *value);

extern void GLAPIENTRY
_mesa_ClearNamedFramebufferfv(GLuint framebuffer, GLenum buffer,
                              GLint drawbuffer, const GLfloat *value);

void GLAPIENTRY
_mesa_ClearBufferfi_no_error(GLenum buffer, GLint drawbuffer,
                             GLfloat depth, GLint stencil);

extern void GLAPIENTRY
_mesa_ClearBufferfi(GLenum buffer, GLint drawbuffer,
                    GLfloat depth, GLint stencil);

extern void GLAPIENTRY
_mesa_ClearNamedFramebufferfi(GLuint framebuffer, GLenum buffer,
                              GLint drawbuffer, GLfloat depth, GLint stencil);

#endif
