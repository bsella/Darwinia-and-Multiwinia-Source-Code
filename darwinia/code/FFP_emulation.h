#pragma once

#include <GL/gl.h>

#if 0
#define FFP_ENABLE_EMULATION
#endif

namespace ffp_emulation
{
    void init();
    void quit();

    void create_vertex_buffers(GLuint& vao, GLuint& vbo);
    void draw_buffer(GLuint vao, GLenum primitive_mode, GLint first, GLsizei count);

    void glBegin(GLenum);
    void glEnd();

    void glMatrixMode(GLenum);
    void glLoadIdentity();
    void glMultMatrixf(const GLfloat *m);
    void glPushMatrix();
    void glPopMatrix();
    void glLoadMatrixd(const GLdouble *m);
    void glScalef(GLfloat x, GLfloat y, GLfloat z);
    void glTranslatef(GLfloat x, GLfloat y, GLfloat z);
    void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);

    void glGetIntegerv(GLenum pname, GLint *params );
    void glGetDoublev(GLenum pname, GLdouble *params);

    void gluLookAt(GLdouble pos_x, GLdouble pos_y, GLdouble pos_z,
                   GLdouble forwards_x, GLdouble forwards_y, GLdouble forwards_z,
                   GLdouble up_x, GLdouble up_y, GLdouble up_z);

    void gluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);

    void gluOrtho2D(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top);

    void glVertex2f( GLfloat x, GLfloat y );
    void glVertex2i( GLint x, GLint y );
    void glVertex3d( GLdouble x, GLdouble y, GLdouble z );
    void glVertex3f( GLfloat x, GLfloat y, GLfloat z );
    void glVertex2fv( const GLfloat *v );
    void glVertex3fv( const GLfloat *v );
    void glNormal3f( GLfloat nx, GLfloat ny, GLfloat nz );
    void glNormal3fv( const GLfloat *v );
    void glColor3f( GLfloat red, GLfloat green, GLfloat blue );
    void glColor3ub( GLubyte red, GLubyte green, GLubyte blue );
    void glColor4f( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );
    void glColor4ub( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha );
    void glColor3fv( const GLfloat *v );
    void glColor3ubv( const GLubyte *v );
    void glColor4fv( const GLfloat *v );
    void glColor4ubv( const GLubyte *v );
    void glTexCoord2f( GLfloat s, GLfloat t );
    void glTexCoord2i( GLint s, GLint t );

    void glAlphaFunc(GLenum func, GLclampf ref);

    void glColorMaterial(GLenum face, GLenum mode);
    void glMaterialfv(GLenum face, GLenum pname, const GLfloat *params);

    GLuint glGenLists(GLsizei range);
    void glNewList(GLuint list, GLenum mode);
    void glEndList();
    void glCallList(GLuint list);

    void glFogf(GLenum pname, GLfloat param);
    void glFogi(GLenum pname, GLint param);
    void glFogfv(GLenum pname, const GLfloat* params);
    void glFogiv(GLenum pname, const GLint* params);

    void glLightfv(GLenum  light, GLenum  pname, const GLfloat *params);
    void glLightModelf(GLenum  pname, GLfloat *param);
    void glLightModelfv(GLenum  pname, const GLfloat *params);

    void glShadeModel(GLenum mode);

    void _glActiveTexture(GLenum texture);
    void glBindTexture(GLenum target, GLuint texture);
    void glTexEnvi(GLenum target, GLenum pname, GLint param);
    void glTexEnviv(GLenum target, GLenum pname, const GLint* params);
    void glTexEnvf(GLenum target, GLenum pname, GLfloat param);

    void glEnable(GLenum cap);
    void glDisable(GLenum cap);

    void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);

    void glDisableClientState(GLenum array);
    void glEnableClientState(GLenum array);

    void glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer);
    void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
    void glVertexPointer(GLint size, GLenum type, GLsizei stride, GLvoid *pointer);

    void _glMultiTexCoord2fARB(GLenum target, GLfloat s, GLfloat t);
}

#ifdef FFP_ENABLE_EMULATION

#define glBegin(mode) ffp_emulation::glBegin(mode)
#define glEnd()       ffp_emulation::glEnd()

#define glMatrixMode(mode)        ffp_emulation::glMatrixMode(mode)
#define glLoadIdentity()          ffp_emulation::glLoadIdentity()
#define glMultMatrixf(m)          ffp_emulation::glMultMatrixf(m)
#define glPushMatrix()            ffp_emulation::glPushMatrix()
#define glPopMatrix()             ffp_emulation::glPopMatrix()
#define glLoadMatrixd(m)          ffp_emulation::glLoadMatrixd(m)
#define glScalef(x, y, z)         ffp_emulation::glScalef(x, y, z)
#define glTranslatef(x, y, z)     ffp_emulation::glTranslatef(x, y, z)
#define glRotatef(angle, x, y, z) ffp_emulation::glRotatef(angle, x, y, z)

#define glGetIntegerv(pname, params) ffp_emulation::glGetIntegerv(pname, params)
#define glGetDoublev(pname, params) ffp_emulation::glGetDoublev(pname, params)

#define gluLookAt(pos_x, pos_y, pos_z, forwards_x, forwards_y, forwards_z, up_x, up_y, up_z) ffp_emulation::gluLookAt(pos_x, pos_y, pos_z, forwards_x, forwards_y, forwards_z, up_x, up_y, up_z)
#define gluPerspective(fovy, aspect, zNear, zFar) ffp_emulation::gluPerspective(fovy, aspect, zNear, zFar)
#define gluOrtho2D(left, right, bottom, top) ffp_emulation::gluOrtho2D(left, right, bottom, top)

#define glVertex2f( x, y ) ffp_emulation::glVertex2f(x, y)
#define glVertex2i( x, y ) ffp_emulation::glVertex2i(x, y)
#define glVertex3d( x, y, z ) ffp_emulation::glVertex3d(x, y, z)
#define glVertex3f( x, y, z ) ffp_emulation::glVertex3f(x, y, z)
#define glVertex2fv( v ) ffp_emulation::glVertex2fv(v)
#define glVertex3fv( v ) ffp_emulation::glVertex3fv(v)
#define glNormal3f( nx, ny, nz ) ffp_emulation::glNormal3f(nx, ny, nz)
#define glNormal3fv( v ) ffp_emulation::glNormal3fv(v)
#define glColor3f( red, green, blue ) ffp_emulation::glColor3f(red, green, blue)
#define glColor3ub( red, green, blue ) ffp_emulation::glColor3ub(red, green, blue)
#define glColor4f( red, green, blue, alpha ) ffp_emulation::glColor4f(red, green, blue, alpha)
#define glColor4ub( red, green, blue, alpha ) ffp_emulation::glColor4ub(red, green, blue, alpha)
#define glColor3fv( v ) ffp_emulation::glColor3fv(v)
#define glColor3ubv( v ) ffp_emulation::glColor3ubv(v)
#define glColor4fv( v ) ffp_emulation::glColor4fv(v)
#define glColor4ubv( v ) ffp_emulation::glColor4ubv(v)
#define glTexCoord2f( s, t ) ffp_emulation::glTexCoord2f(s, t)
#define glTexCoord2i( s, t ) ffp_emulation::glTexCoord2i(s, t)

#define glAlphaFunc(func, ref) ffp_emulation::glAlphaFunc(func, ref)

#define glColorMaterial(face, mode)       ffp_emulation::glColorMaterial(face, mode)
#define glMaterialfv(face, pname, params) ffp_emulation::glMaterialfv(face, pname, params)

#define glGenLists(range)     ffp_emulation::glGenLists(range)
#define glNewList(list, mode) ffp_emulation::glNewList(list, mode)
#define glEndList()           ffp_emulation::glEndList()
#define glCallList(list)      ffp_emulation::glCallList(list)

#define glFogf(pname, param)   ffp_emulation::glFogf(pname, param)
#define glFogi(pname, param)   ffp_emulation::glFogi(pname, param)
#define glFogfv(pname, params) ffp_emulation::glFogfv(pname, params)
#define glFogiv(pname, params) ffp_emulation::glFogiv(pname, params)

#define glLightfv(light, pname, params) ffp_emulation::glLightfv(light, pname, params)
#define glLightModelf(pname, param)     ffp_emulation::glLightModelf(pname, param)
#define glLightModelfv(pname, params)   ffp_emulation::glLightModelfv(pname, params)

#define glShadeModel(mode) ffp_emulation::glShadeModel(mode)

#define glActiveTexture(texture)          ffp_emulation::_glActiveTexture(texture)
#define glBindTexture(target, texture)    ffp_emulation::glBindTexture(target, texture)
#define glTexEnvi(target, pname, param)   ffp_emulation::glTexEnvi(target, pname, param)
#define glTexEnviv(target, pname, params) ffp_emulation::glTexEnviv(target, pname, params)
#define glTexEnvf(target, pname, param)   ffp_emulation::glTexEnvf(target, pname, param)

#define glEnable(cap)  ffp_emulation::glEnable(cap)
#define glDisable(cap) ffp_emulation::glDisable(cap)

#define glColorPointer(size, type, stride, pointer) ffp_emulation::glColorPointer(size, type, stride, pointer)

#define glDisableClientState(array) ffp_emulation::glDisableClientState(array)
#define glEnableClientState(array)  ffp_emulation::glEnableClientState(array)

#define glNormalPointer(type, stride, pointer)         ffp_emulation::glNormalPointer(type, stride, pointer)
#define glTexCoordPointer(size, type, stride, pointer) ffp_emulation::glTexCoordPointer(size, type, stride, pointer)
#define glVertexPointer(size, type, stride, pointer)   ffp_emulation::glVertexPointer(size, type, stride, pointer)

#define glMultiTexCoord2fARB(target, s, t) ffp_emulation::_glMultiTexCoord2fARB(target, s, t)

#endif