#!/usr/bin/python
from __future__ import division, print_function

import sys
from math import cos

def perror(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)
	
try:
    from OpenGL.GLUT import *
    from OpenGL.GL import *
except ImportError as err:
    perror('Error: PyOpenGL not installed.')
  
    sys.exit(1)


movement = 0.0
MOVE_INC = 0.003

# Radius, stacks, slices
SPHERE_RADIUS = 0.6
SPHERE_STACKS = 50
SPHERE_SLICES = 50

#  Initialize material property and light source.
def init():
    light_ambient =  [0.0, 0.0, 0.0, 1.0]
    light_diffuse =  [1.0, 1.0, 1.0, 1.0]
    light_specular =  [1.0, 1.0, 1.0, 1.0]
#  light_position is NOT default value
    light_position =  [1.0, 1.0, 1.0, 0.0]

    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient)
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse)
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular)
    glLightfv(GL_LIGHT0, GL_POSITION, light_position)
    
    glEnable(GL_LIGHTING)
    glEnable(GL_LIGHT0)
    glEnable(GL_DEPTH_TEST)
   
    glMatrixMode(GL_MODELVIEW)

def display():
    global movement
    global MOVE_INC
    global SPHERE_RADIUS
    global SPHERE_STACKS
    global SPHERE_SLICES

    movement += MOVE_INC

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)

    glPushMatrix()
    glTranslatef(0.0, 3.0 * cos(movement), 1.0)
    glutSolidSphere(SPHERE_RADIUS, SPHERE_STACKS, SPHERE_SLICES)
    glPopMatrix()

    glFlush()


def reshape(w, h):
    glViewport(0, 0, w, h)
    glMatrixMode (GL_PROJECTION)
    glLoadIdentity()
    if w <= h:
        glOrtho(-2.5, 2.5, -2.5*h/w, 
                 2.5*h/w, -10.0, 10.0)
    else: 
        glOrtho(-2.5*w/h, 
                 2.5*w/h, -2.5, 2.5, -10.0, 10.0)
    glMatrixMode(GL_MODELVIEW)
    glLoadIdentity()


def keyboard(key, x, y):
    escape_key = chr(27)
    if key == escape_key:
        sys.exit(0)
   

def main():
    glutInit(sys.argv)
    glutInitDisplayMode (GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH)
    glutInitWindowSize (500, 500)
    glutCreateWindow('scene')
    init()
    glutReshapeFunc(reshape)
    glutKeyboardFunc(keyboard)
    glutDisplayFunc(display)
    glutIdleFunc(display)
    glutMainLoop()


if __name__ == "__main__":
    main()
