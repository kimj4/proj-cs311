/* On macOS, compile with...
    clang 500openGLODE.c -lglfw -framework OpenGL
*/

#include <stdio.h>
#include <GLFW/glfw3.h>
#include <ode/ode.h>



#define DENSITY (5.0)

typedef struct MyObject MyObject;
	struct MyObject {
    	dBodyID body;
		dGeomID geom;
	};

dReal radius = 0.25;
dReal length = 1.0;
dReal sides[3] = {0.5, 0.5, 1.0};

static dWorldID world;
static dSpaceID space;
static dGeomID ground;
static dJointGroupID contactgroup;
static int flag = 0;

static MyObject sphere; //box, capsule, cylinder;


void handleError(int error, const char *description) {
	fprintf(stderr, "handleError: %d\n%s\n", error, description);
}

void handleResize(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
}

/* Here we start building a mesh. At each vertex it will have an XYZ position
and an RGB color. It will also have an NOP unit normal, but in this special
mesh the normals happen to equal the positions, so there's no point in retyping
them. */
#define triNum 8
#define vertNum 6
/* For compatibility across hardware, operating systems, and compilers,
OpenGL defines its own versions of double, int, unsigned int, etc. Use these
OpenGL types whenever passing arrays or pointers to OpenGL (unless OpenGL says
otherwise). Do not assume that these types equal your C types. */
GLdouble positions[vertNum * 3] = {
	1.0, 0.0, 0.0,
	-1.0, 0.0, 0.0,
	0.0, 1.0, 0.0,
	0.0, -1.0, 0.0,
	0.0, 0.0, 1.0,
	0.0, 0.0, -1.0};
GLdouble colors[vertNum * 3] = {
	1.0, 0.0, 0.0,
	1.0, 1.0, 0.0,
	0.0, 1.0, 0.0,
	0.0, 1.0, 1.0,
	0.0, 0.0, 1.0,
	1.0, 0.0, 1.0};
GLuint triangles[triNum * 3] = {
	0, 2, 4,
	2, 1, 4,
	1, 3, 4,
	3, 0, 4,
	2, 0, 5,
	1, 2, 5,
	3, 1, 5,
	0, 3, 5};
/* We'll use this angle to animate a rotation of the mesh. */
GLdouble alpha = 0.0;

//Collision handler
static void nearCallback(void *data, dGeomID o1, dGeomID o2)
{
  const int N = 10;
  dContact contact[N];

  //int isGround = ((ground == o1) || (ground == o2));

  int n =  dCollide(o1,o2,N,&contact[0].geom,sizeof(dContact));

  //if (isGround)  {
	if (n >= 1)
		flag = 1;
    else
		flag = 0;
    for (int i = 0; i < n; i++) {
      contact[i].surface.mode = dContactBounce;
      contact[i].surface.mu   = dInfinity;
      contact[i].surface.bounce     = 0.0; // (0.0~1.0) restitution parameter
      contact[i].surface.bounce_vel = 0.0; // minimum incoming velocity for bounce
      dJointID c = dJointCreateContact(world,contactgroup,&contact[i]);
      dJointAttach (c,dGeomGetBody(contact[i].geom.g1),dGeomGetBody(contact[i].geom.g2));

  }
}


void render(void) {
	/* Clear not just the color buffer, but also the depth buffer. */
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-2.0, 2.0, -2.0, 2.0, -2.0, 2.0);
	/* OpenGL's modelview matrix corresponds to C^-1 M in the notation of our
	software graphics engine. That is, it's everything before projection. */
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	/* OpenGL 1.x lighting calculations are done in eye coordinates. When you
	set a light's position, it is automatically transformed by the current
	modelview matrix. */
	GLfloat light[4] = {0.0, 1.0, -1.0, 0.0};
	glLightfv(GL_LIGHT0, GL_POSITION, light);
	/* This is NOT a good way to animate, because it is not tied to the passage
	of real time. See 500time.c for something better. Anyway, we are rotating
	the mesh in world space by 0.1 degrees (not radians, sadly) per frame,
	about the unit vector that points from the origin toward <1, 1, 1>. We
	accomplish this by multiplying a rotation onto the modelview matrix. */
	alpha += 0.1;
	glRotatef(alpha, 1.0, 1.0, 1.0);
	/* Send the arrays of attribute data to OpenGL. Notice that we're sending
	the same array for positions and normals, because our mesh is special in
	that way. */
	glVertexPointer(3, GL_DOUBLE, 0, positions);
	glNormalPointer(GL_DOUBLE, 0, positions);
	glColorPointer(3, GL_DOUBLE, 0, colors);
	/* Draw the triangles, each one a triple of attribute array indices. */
    glDrawElements(GL_TRIANGLES, triNum * 3, GL_UNSIGNED_INT, triangles);

   //simulate collisions
    const dReal *pos1,*R1;//,*pos2,*R2,*pos3,*R3;
   	flag = 0;
	dSpaceCollide(space, 0, &nearCallback);
	dWorldStep(world, 0.01);
	dJointGroupEmpty(contactgroup);

	//draw sphere

	pos1 = dBodyGetPosition(sphere.body); // get a body position
    R1   = dBodyGetRotation(sphere.body); // get a body rotation matrix
	printf("pos = %f /n", pos1[2] );
	//Draw sphere (pos1, R1)


}

int main(int argc, char *argv[]) {
    //ODE inits
	dInitODE();
	world = dWorldCreate();
	space = dHashSpaceCreate(0);
	contactgroup = dJointGroupCreate(0);
	//Graviga
	dWorldSetGravity(world, 0, 0, -0.3);
	ground = dCreatePlane(space, 0, 0, 1, 0);

    //Make a sphere
	dMass m;
	dMassSetZero (&m);
	dReal radius = 0.5;
	dMassSetSphere(m, DENSITY, radius);
	sphere.body = dBodyCreate (world);

	dMassSetSphere (&m, DENSITY, radius);
	dBodySetMass (sphere.body, &m);
	dBodySetPosition(sphere.body, 0, 1, 1);

	sphere.geom = dCreateSphere(space, radius);
	dGeomSetBody(sphere.geom, sphere.body);

	glfwSetErrorCallback(handleError);
    if (glfwInit() == 0)
        return 1;
    GLFWwindow *window;
    window = glfwCreateWindow(768, 512, "Learning OpenGL 1.4", NULL, NULL);
    if (window == NULL) {
        glfwTerminate();
        return 2;
    }
    glfwSetWindowSizeCallback(window, handleResize);
    glfwMakeContextCurrent(window);
    fprintf(stderr, "main: OpenGL %s, GLSL %s.\n",
		glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));
    /* Enable some OpenGL features. Several lights are available, but we'll use
    only the first light (light 0). It defaults to diffuse and ambient lighting
    (not specular or emissive). */
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    /* In diffuse and ambient lighting calculations, the color of the surface
    will be taken from the interpolated color attributes. */
    glEnable(GL_COLOR_MATERIAL);
    /* Enable certain ways of passing attribute information into OpenGL. Just
    to give you an idea of your options in OpenGL 1.4, I explicitly disable the
    other ways of passing attributes. */
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_FOG_COORD_ARRAY);
    glDisableClientState(GL_SECONDARY_COLOR_ARRAY);
    glDisableClientState(GL_EDGE_FLAG_ARRAY);
    glDisableClientState(GL_INDEX_ARRAY);
    while (glfwWindowShouldClose(window) == 0) {
        render();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
	glfwDestroyWindow(window);
    glfwTerminate();
	dWorldDestroy(world);
	dCloseODE();
    return 0;
}
