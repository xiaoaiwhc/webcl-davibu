/*
Copyright (c) 2009 David Bucciarelli (davibu@interfree.it)

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#define _USE_MATH_DEFINES
#endif
#include <math.h>

#if defined(__linux__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
#include <sys/time.h>
#elif defined (WIN32)
#include <windows.h>
#else
        Unsupported Platform !!!
#endif

#include "camera.h"
#include "geom.h"
#include "displayfunc.h"

extern void ReInit(const int);
extern void ReInitScene();
extern void UpdateRendering();
extern void UpdateCamera();

extern Camera camera;
extern Sphere *spheres;
extern unsigned int sphereCount;

int amiSmallptCPU;

int width = 512;
int height = 512;
unsigned int *pixels;
char captionBuffer[256];

static int printHelp = 1;
static int currentSphere;

#ifdef __EMSCRIPTEN__
	#include <emscripten/emscripten.h>
	#define glRasterPos2i(x,y)
	#define glRecti(x,y,z,w)
	#undef GLUT_BITMAP_HELVETICA_18
	#define GLUT_BITMAP_HELVETICA_18 (void*)""
#endif

double WallClockTime() {
#if defined(__linux__) || defined(__APPLE__)
	struct timeval t;
	gettimeofday(&t, NULL);

	return t.tv_sec + t.tv_usec / 1000000.0;
#elif defined (WIN32)
	return GetTickCount() / 1000.0;
#elif defined (__EMSCRIPTEN__)
	return (emscripten_get_now() / 1000.0);	
#else
	Unsupported Platform !!!
#endif
}

unsigned int TextureId;
static unsigned int TextureTarget = GL_TEXTURE_2D;
static unsigned int TextureInternal             = GL_RGBA;
static unsigned int TextureFormat               = GL_RGBA;
static unsigned int TextureType                 = GL_UNSIGNED_BYTE;
static unsigned int ActiveTextureUnit           = GL_TEXTURE1_ARB;
static size_t TextureTypeSize                   = sizeof(int);

static float VertexPos[4][2]            = { { -1.0f, -1.0f },
                                            { +1.0f, -1.0f },
                                            { +1.0f, +1.0f },
                                            { -1.0f, +1.0f } };
static float TexCoords[4][2];

static void CreateTexture(unsigned int width, unsigned int height)
{    
    
    printf("Creating Texture 1 %d x %d...\n", width, height);

    glActiveTexture(ActiveTextureUnit);
    
    glGenTextures( 1, &TextureId );
	glBindTexture(TextureTarget, TextureId);

    glTexParameteri(TextureTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(TextureTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexImage2D(TextureTarget, 0, TextureInternal, width, height, 0, TextureFormat, TextureType, 0);
    glBindTexture(TextureTarget, 0);

}

static int SetupGraphics(unsigned int width, unsigned int height)
{
    CreateTexture(width,height);

    glClearColor (0.0, 0.0, 0.0, 0.0);

    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);
    glViewport(0, 0,width,height);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    TexCoords[3][0] = 0.0f;
    TexCoords[3][1] = 0.0f;
    TexCoords[2][0] = width;
    TexCoords[2][1] = 0.0f;
    TexCoords[1][0] = width;
    TexCoords[1][1] = height;
    TexCoords[0][0] = 0.0f;
    TexCoords[0][1] = height;

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, VertexPos);
    glClientActiveTexture(GL_TEXTURE0);
    glTexCoordPointer(2, GL_FLOAT, 0, TexCoords);
    return GL_NO_ERROR;
}

static void PrintString(void *font, const char *string) {
	#ifdef __EMSCRIPTEN__
		printf("%s\n",string);
	#else
	int len, i;

	len = (int)strlen(string);
	for (i = 0; i < len; i++)
		glutBitmapCharacter(font, string[i]);
	#endif
}

static void PrintHelp() {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(0.f, 0.f, 0.5f, 0.5f);
	glRecti(40, 40, 600, 440);

	glColor3f(1.f, 1.f, 1.f);
	glRasterPos2i(300, 420);
	PrintString(GLUT_BITMAP_HELVETICA_18, "Help");

	glRasterPos2i(60, 390);
	PrintString(GLUT_BITMAP_HELVETICA_18, "h - toggle Help");
	glRasterPos2i(60, 360);
	PrintString(GLUT_BITMAP_HELVETICA_18, "arrow Keys - rotate camera left/right/up/down");
	glRasterPos2i(60, 330);
	PrintString(GLUT_BITMAP_HELVETICA_18, "a and d - move camera left and right");
	glRasterPos2i(60, 300);
	PrintString(GLUT_BITMAP_HELVETICA_18, "w and s - move camera forward and backward");
	glRasterPos2i(60, 270);
	PrintString(GLUT_BITMAP_HELVETICA_18, "r and f - move camera up and down");
	glRasterPos2i(60, 240);
	PrintString(GLUT_BITMAP_HELVETICA_18, "PageUp and PageDown - move camera target up and down");
	glRasterPos2i(60, 210);
	PrintString(GLUT_BITMAP_HELVETICA_18, "+ and - - to select next/previous object");
	glRasterPos2i(60, 180);
	PrintString(GLUT_BITMAP_HELVETICA_18, "2, 3, 4, 5, 6, 8, 9 - to move selected object");

	glDisable(GL_BLEND);
}

void ReadScene(char *fileName) {
	fprintf(stderr, "Reading scene: %s\n", fileName);

	FILE *f = fopen(fileName, "r");
	if (!f) {
		fprintf(stderr, "Failed to open file: %s\n", fileName);
		exit(-1);
	}

	/* Read the camera position */
	int c = fscanf(f,"camera %f %f %f  %f %f %f\n",
			&camera.orig.x, &camera.orig.y, &camera.orig.z,
			&camera.target.x, &camera.target.y, &camera.target.z);
	if (c != 6) {
		fprintf(stderr, "Failed to read 6 camera parameters: %d\n", c);
		exit(-1);
	}

	/* Read the sphere count */
	c = fscanf(f,"size %u\n", &sphereCount);
	if (c != 1) {
		fprintf(stderr, "Failed to read sphere count: %d\n", c);
		exit(-1);
	}
	fprintf(stderr, "Scene size: %d\n", sphereCount);

	/* Read all spheres */
	spheres = (Sphere *)malloc(sizeof(Sphere) * sphereCount);
	unsigned int i;
	for (i = 0; i < sphereCount; i++) {
		Sphere *s = &spheres[i];
		int mat;
		int c = fscanf(f,"sphere %f  %f %f %f  %f %f %f  %f %f %f  %d\n",
				&s->rad,
				&s->p.x, &s->p.y, &s->p.z,
				&s->e.x, &s->e.y, &s->e.z,
				&s->c.x, &s->c.y, &s->c.z,
				&mat);
		switch (mat) {
			case 0:
				s->refl = DIFF;
				break;
			case 1:
				s->refl = SPEC;
				break;
			case 2:
				s->refl = REFR;
				break;
			default:
				fprintf(stderr, "Failed to read material type for sphere #%d: %d\n", i, mat);
				exit(-1);
				break;
		}
		if (c != 11) {
			fprintf(stderr, "Failed to read sphere #%d: %d\n", i, c);
			exit(-1);
		}
	}

	fclose(f);
}

void UpdateCamera() {
	vsub(camera.dir, camera.target, camera.orig);
	vnorm(camera.dir);

	const Vec up = {0.f, 1.f, 0.f};
	const float fov = (M_PI / 180.f) * 45.f;
	vxcross(camera.x, camera.dir, up);
	vnorm(camera.x);
	vsmul(camera.x, width * fov / height, camera.x);

	vxcross(camera.y, camera.x, camera.dir);
	vnorm(camera.y);
	vsmul(camera.y, fov, camera.y);
}

void idleFunc(void) {
	UpdateRendering();

	glutPostRedisplay();
}

void displayFunc(void) {
	glClear(GL_COLOR_BUFFER_BIT);


	#ifndef __EMSCRIPTEN__
		glRasterPos2i(0, 0);
		glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	#else
	 	glEnable( TextureTarget );
	    glBindTexture( TextureTarget, TextureId );

	    if(pixels) {
	        glTexSubImage2D(TextureTarget, 0, 0, 0,  width, height, TextureFormat, TextureType, pixels);
	    }

		glBegin(GL_TRIANGLE_STRIP);
		glColor3f( 1, 1, 1 );
	    glTexCoord2i( 0, 0 ); glVertex3f( 0, 0, 0 );
	    glColor3f( 1, 1, 1 );
	    glTexCoord2i( 0, 1 ); glVertex3f( 0, height, 0 );
	    glColor3f( 1, 1, 1 );
	    glTexCoord2i( 1, 0 ); glVertex3f( width, 0, 0 );
	    glColor3f( 1, 1, 1 );
	    glTexCoord2i( 1, 1 ); glVertex3f( width, height, 0 );
	    glEnd();

	    glDisable( TextureTarget );
	    glBindTexture( TextureTarget, 0 );
	#endif

	// Title
	glColor3f(1.f, 1.f, 1.f);
	glRasterPos2i(4, height - 16);
	if (amiSmallptCPU)
		PrintString(GLUT_BITMAP_HELVETICA_18, "SmallptCPU v1.6 (Written by David Bucciarelli)");
	else
		PrintString(GLUT_BITMAP_HELVETICA_18, "SmallptGPU v1.6 (Written by David Bucciarelli)");

	// Caption line 0
	glColor3f(1.f, 1.f, 1.f);
	glRasterPos2i(4, 10);
	PrintString(GLUT_BITMAP_HELVETICA_18, captionBuffer);

	if (printHelp) {
		glPushMatrix();
		glLoadIdentity();
		glOrtho(-0.5, 639.5, -0.5, 479.5, -1.0, 1.0);

		PrintHelp();

		glPopMatrix();
	}

	glutSwapBuffers();
}

void reshapeFunc(int newWidth, int newHeight) {
	width = newWidth;
	height = newHeight;

	glViewport(0, 0, width, height);
	glLoadIdentity();
	glOrtho(0.f, width - 1.f, 0.f, height - 1.f, -1.f, 1.f);

	ReInit(1);

	glutPostRedisplay();
}

#define MOVE_STEP 10.0f
#define ROTATE_STEP (2.f * M_PI / 180.f)
void keyFunc(unsigned char key, int x, int y) {
	switch (key) {
		case 'p': {
			FILE *f = fopen("image.ppm", "w"); // Write image to PPM file.
			if (!f) {
				fprintf(stderr, "Failed to open image file: image.ppm\n");
			} else {
				fprintf(f, "P3\n%d %d\n%d\n", width, height, 255);

				int x, y;
				for (y = height - 1; y >= 0; --y) {
					unsigned char *p = (unsigned char *)(&pixels[y * width]);
					for (x = 0; x < width; ++x, p += 4)
						fprintf(f, "%d %d %d ", p[0], p[1], p[2]);
				}

				fclose(f);
			}
			break;
		}
		case 27: /* Escape key */
			fprintf(stderr, "Done.\n");
			exit(0);
			break;
		case ' ': /* Refresh display */
			ReInit(1);
			break;
		case 'a': {
			Vec dir = camera.x;
			vnorm(dir);
			vsmul(dir, -MOVE_STEP, dir);
			vadd(camera.orig, camera.orig, dir);
			vadd(camera.target, camera.target, dir);
			ReInit(0);
			break;
		}
		case 'd': {
			Vec dir = camera.x;
			vnorm(dir);
			vsmul(dir, MOVE_STEP, dir);
			vadd(camera.orig, camera.orig, dir);
			vadd(camera.target, camera.target, dir);
			ReInit(0);
			break;
		}
		case 'w': {
			Vec dir = camera.dir;
			vsmul(dir, MOVE_STEP, dir);
			vadd(camera.orig, camera.orig, dir);
			vadd(camera.target, camera.target, dir);
			ReInit(0);
			break;
		}
		case 's': {
			Vec dir = camera.dir;
			vsmul(dir, -MOVE_STEP, dir);
			vadd(camera.orig, camera.orig, dir);
			vadd(camera.target, camera.target, dir);
			ReInit(0);
			break;
		}
		case 'r':
			camera.orig.y += MOVE_STEP;
			camera.target.y += MOVE_STEP;
			ReInit(0);
			break;
		case 'f':
			camera.orig.y -= MOVE_STEP;
			camera.target.y -= MOVE_STEP;
			ReInit(0);
			break;
		case '+':
			currentSphere = (currentSphere + 1) % sphereCount;
			fprintf(stderr, "Selected sphere %d (%f %f %f)\n", currentSphere,
					spheres[currentSphere].p.x, spheres[currentSphere].p.y, spheres[currentSphere].p.z);
			ReInitScene();
			break;
		case '-':
			currentSphere = (currentSphere + (sphereCount - 1)) % sphereCount;
			fprintf(stderr, "Selected sphere %d (%f %f %f)\n", currentSphere,
					spheres[currentSphere].p.x, spheres[currentSphere].p.y, spheres[currentSphere].p.z);
			ReInitScene();
			break;
		case '4':
			spheres[currentSphere].p.x -= 0.5f * MOVE_STEP;
			ReInitScene();
			break;
		case '6':
			spheres[currentSphere].p.x += 0.5f * MOVE_STEP;
			ReInitScene();
			break;
		case '8':
			spheres[currentSphere].p.z -= 0.5f * MOVE_STEP;
			ReInitScene();
			break;
		case '2':
			spheres[currentSphere].p.z += 0.5f * MOVE_STEP;
			ReInitScene();
			break;
		case '9':
			spheres[currentSphere].p.y += 0.5f * MOVE_STEP;
			ReInitScene();
			break;
		case '3':
			spheres[currentSphere].p.y -= 0.5f * MOVE_STEP;
			ReInitScene();
			break;
		case 'h':
			printHelp = (!printHelp);
			break;
		default:
			break;
	}
}

void specialFunc(int key, int x, int y) {
	switch (key) {
		case GLUT_KEY_UP: {
			Vec t = camera.target;
			vsub(t, t, camera.orig);
			t.y = t.y * cos(-ROTATE_STEP) + t.z * sin(-ROTATE_STEP);
			t.z = -t.y * sin(-ROTATE_STEP) + t.z * cos(-ROTATE_STEP);
			vadd(t, t, camera.orig);
			camera.target = t;
			ReInit(0);
			break;
		}
		case GLUT_KEY_DOWN: {
			Vec t = camera.target;
			vsub(t, t, camera.orig);
			t.y = t.y * cos(ROTATE_STEP) + t.z * sin(ROTATE_STEP);
			t.z = -t.y * sin(ROTATE_STEP) + t.z * cos(ROTATE_STEP);
			vadd(t, t, camera.orig);
			camera.target = t;
			ReInit(0);
			break;
		}
		case GLUT_KEY_LEFT: {
			Vec t = camera.target;
			vsub(t, t, camera.orig);
			t.x = t.x * cos(-ROTATE_STEP) - t.z * sin(-ROTATE_STEP);
			t.z = t.x * sin(-ROTATE_STEP) + t.z * cos(-ROTATE_STEP);
			vadd(t, t, camera.orig);
			camera.target = t;
			ReInit(0);
			break;
		}
		case GLUT_KEY_RIGHT: {
			Vec t = camera.target;
			vsub(t, t, camera.orig);
			t.x = t.x * cos(ROTATE_STEP) - t.z * sin(ROTATE_STEP);
			t.z = t.x * sin(ROTATE_STEP) + t.z * cos(ROTATE_STEP);
			vadd(t, t, camera.orig);
			camera.target = t;
			ReInit(0);
			break;
		}
		case GLUT_KEY_PAGE_UP:
			camera.target.y += MOVE_STEP;
			ReInit(0);
			break;
		case GLUT_KEY_PAGE_DOWN:
			camera.target.y -= MOVE_STEP;
			ReInit(0);
			break;
		default:
			break;
	}
}

void InitGlut(int argc, char *argv[], char *windowTittle) {
    glutInitWindowSize(width, height);
    glutInitWindowPosition(0,0);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInit(&argc, argv);

	glutCreateWindow(windowTittle);

	SetupGraphics(width, height);

    glutReshapeFunc(reshapeFunc);
    glutKeyboardFunc(keyFunc);
    glutSpecialFunc(specialFunc);
    glutDisplayFunc(displayFunc);
	glutIdleFunc(idleFunc);

	glViewport(0, 0, width, height);
	glLoadIdentity();
	glOrtho(0.f, width - 1.f, 0.f, height - 1.f, -1.f, 1.f);
}
