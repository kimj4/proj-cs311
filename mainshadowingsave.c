
/* On macOS, compile with...
    clang 590mainShadowing.c /usr/local/gl3w/src/gl3w.o -lglfw -framework OpenGL -framework CoreFoundation

		On ubuntu, compile with
		clang mainshadowingsave.c /usr/local/gl3w/src/gl3w.o -lglfw3 -lGL -lm -lXrandr -lXi -lX11 -lXxf86vm -lpthread -ldl -lXinerama -lXcursor -lrt

*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <sys/time.h>
#include <ode/ode.h>

double getTime(void) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (double)tv.tv_sec + (double)tv.tv_usec * 0.000001;
}

#include "500shader.c"
#include "530vector.c"
#include "580mesh.c"
#include "590matrix.c"
#include "520camera.c"
#include "590texture.c"
#include "580scene.c"
#include "560light.c"
#include "590shadow.c"


typedef struct MyObject MyObject;
	struct MyObject {
    	dBodyID body;
		dGeomID geom;
	};

static dWorldID world;
static dSpaceID space;
static dGeomID ground;
static dJointGroupID contactgroup;
static int flag = 0;

MyObject sphere;

camCamera cam;
texTexture texH, texBall;
meshGLMesh floorGL, sphereGL;
sceneNode nodeFloor, nodeSphere;
/* We need just one shadow program, because all of our meshes have the same
attribute structure. */
shadowProgram sdwProg;
shadowProgram sdwProg2;
/* We need one shadow map per shadow-casting light. */
lightLight light;
shadowMap sdwMap;

lightLight light2;
shadowMap sdwMap2;
/* The main shader program has extra hooks for shadowing. */
GLuint program;
GLint viewingLoc, modelingLoc;
GLint unifLocs[1], textureLocs[1];
GLint attrLocs[3];
GLint lightPosLoc, lightColLoc, lightAttLoc, lightDirLoc, lightCosLoc;
GLint camPosLoc;
GLint viewingSdwLoc, textureSdwLoc;
//TODO: do i need two cameras?
GLint lightPosLoc2, lightColLoc2, lightAttLoc2, lightDirLoc2, lightCosLoc2;
GLint viewingSdwLoc2, textureSdwLoc2;

void handleError(int error, const char *description) {
	fprintf(stderr, "handleError: %d\n%s\n", error, description);
}

void handleResize(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
	camSetWidthHeight(&cam, width, height);
}

void handleKey(GLFWwindow *window, int key, int scancode, int action, int mods) {
	int shiftIsDown = mods & GLFW_MOD_SHIFT;
	int controlIsDown = mods & GLFW_MOD_CONTROL;
	int altOptionIsDown = mods & GLFW_MOD_ALT;
	int superCommandIsDown = mods & GLFW_MOD_SUPER;
	if (action == GLFW_PRESS && key == GLFW_KEY_L) {
		camSwitchProjectionType(&cam);
	} else if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		if (key == GLFW_KEY_O)
			camAddTheta(&cam, -0.1);
		else if (key == GLFW_KEY_P)
			camAddTheta(&cam, 0.1);
		else if (key == GLFW_KEY_I)
			camAddPhi(&cam, -0.1);
		else if (key == GLFW_KEY_K) {
			camAddPhi(&cam, 0.1);
			// printf("%f\n", cam.phi);
		}
		else if (key == GLFW_KEY_U)
			camAddDistance(&cam, -0.5);
		else if (key == GLFW_KEY_J)
			camAddDistance(&cam, 3.0);
		else if (key == GLFW_KEY_Y) {
			GLdouble vec[3];
			vecCopy(3, light.translation, vec);
			vec[1] += 1.0;
			lightSetTranslation(&light, vec);
		} else if (key == GLFW_KEY_H) {
			GLdouble vec[3];
			vecCopy(3, light.translation, vec);
			vec[1] -= 1.0;
			lightSetTranslation(&light, vec);
		}
		else if (key == GLFW_KEY_T) {
			GLdouble vec[3];
			vecCopy(3, light.translation, vec);
			vec[0] += 1.0;
			lightSetTranslation(&light, vec);
		} else if (key == GLFW_KEY_G) {
			GLdouble vec[3];
			vecCopy(3, light.translation, vec);
			vec[0] -= 1.0;
			lightSetTranslation(&light, vec);
		}
	}
}

/* Returns 0 on success, non-zero on failure. Warning: If initialization fails
midway through, then does not properly deallocate all resources. But that's
okay, because the program terminates almost immediately after this function
returns. */
int initializeScene(void) {
	if (texInitializeFile(&texH, "gray.jpeg", GL_LINEAR, GL_LINEAR,
    		GL_REPEAT, GL_REPEAT) != 0)
		return 1;
	if (texInitializeFile(&texBall, "granite.jpg", GL_LINEAR, GL_LINEAR,
		GL_REPEAT, GL_REPEAT) != 0)
    return 1;

	GLuint attrDims[3] = {3, 2, 3};

	// NEW ADDITIONS FOR PHYSICS ENGINE ===========
	meshMesh floor, sphere;
	if (meshInitializeBox(&floor, -100.0, 100.0, -100.0, 100.0, -1.0, 1.0) != 0) {
		return 20;
	}

	if (meshInitializeSphere(&sphere, 5, 20, 20)) {
		return 21;
	}

	double *vert, normal[2];
	for (int i = 0; i < floor.vertNum; i += 1) {
		vert = meshGetVertexPointer(&floor, i);
		normal[0] = -vert[6];
		normal[1] = vert[5];
		vert[3] = (vert[0] * normal[0] + vert[1] * normal[1]) / 20.0;
		vert[4] = vert[2] / 20.0;
	}

	meshGLInitialize(&floorGL, &floor, 3, attrDims, 3);
	meshGLVAOInitialize(&floorGL, 0, attrLocs);
	meshGLVAOInitialize(&floorGL, 1, sdwProg.attrLocs);
	meshGLVAOInitialize(&floorGL, 2, sdwProg2.attrLocs);
	meshDestroy(&floor);

	meshGLInitialize(&sphereGL, &sphere, 3, attrDims, 3);
	meshGLVAOInitialize(&sphereGL, 0, attrLocs);
	meshGLVAOInitialize(&sphereGL, 1, sdwProg.attrLocs);
	meshGLVAOInitialize(&sphereGL, 2, sdwProg2.attrLocs);
	meshDestroy(&sphere);



	if (sceneInitialize(&nodeSphere, 3, 1, &sphereGL, NULL, NULL) != 0) {
		return 22;
	}

	if (sceneInitialize(&nodeFloor, 3, 1, &floorGL, &nodeSphere, NULL) != 0) {
		return 21;
	}

	texTexture *tex;

	GLdouble trans[3] = {0.0, 0.0, 0.0};
	sceneSetTranslation(&nodeFloor, trans);
	GLdouble unif[3] = {1.0, 1.0, 1.0};
	sceneSetUniform(&nodeFloor, unif);

	vecSet(3, trans, -90.0, 0.0, 70.0);
	sceneSetTranslation(&nodeSphere, trans);
	sceneSetUniform(&nodeSphere, unif);
	tex = &texH;
	sceneSetTexture(&nodeFloor, &tex);
	tex = &texBall;
	sceneSetTexture(&nodeSphere, &tex);
	return 0;
}

void destroyScene(void) {
	texDestroy(&texH);
	meshGLDestroy(&floorGL);
	sceneDestroyRecursively(&nodeFloor);
}

/* Returns 0 on success, non-zero on failure. Warning: If initialization fails
midway through, then does not properly deallocate all resources. But that's
okay, because the program terminates almost immediately after this function
returns. */
int initializeCameraLight(void) {
  	GLdouble vec[3] = {30.0, 30.0, 5.0};
	camSetControls(&cam, camPERSPECTIVE, M_PI / 6.0, 10.0, 768.0, 768.0, 500.0,
								 M_PI / 3.0, M_PI / 4.0, vec);
	lightSetType(&light, lightSPOT);
	vecSet(3, vec, 0.0, 0.0, 100.0);
	lightShineFrom(&light, vec, M_PI, M_PI / 2);
	vecSet(3, vec, 1.0, 1.0, 1.0);
	lightSetColor(&light, vec);
	vecSet(3, vec, 1.0, 0.0, 0.0);
	lightSetAttenuation(&light, vec);
	lightSetSpotAngle(&light, M_PI / 4.0);
	/* Configure shadow mapping. */
	if (shadowProgramInitialize(&sdwProg, 3) != 0)
		return 1;
	if (shadowMapInitialize(&sdwMap, 1024, 1024) != 0)
		return 2;

	// vecSet(3, vec, 40.0, 40.0, 5.0);
	// camSetControls(&cam, camPERSPECTIVE, M_PI / 6.0, 10.0, 768.0, 768.0, 100.0,
	// 							 M_PI / 4.0, M_PI / 4.0, vec);
	lightSetType(&light2, lightSPOT);
	vecSet(3, vec, 50.0, 40.0, 25.0);
	lightShineFrom(&light2, vec, M_PI * 3.0 / 4.0, M_PI * 3.0 / 4.0);
	vecSet(3, vec, 1.0, 0.0, 0.0);
	lightSetColor(&light2, vec);
	vecSet(3, vec, 1.0, 0.0, 0.0);
	lightSetAttenuation(&light2, vec);
	lightSetSpotAngle(&light2, M_PI / 2.0);
	if (shadowProgramInitialize(&sdwProg2, 3) != 0)
		return 3;
	if (shadowMapInitialize(&sdwMap2, 1024, 1024) != 0)
		return 4;
	return 0;
}

/* Returns 0 on success, non-zero on failure. */
int initializeShaderProgram(void) {

	GLchar vertexCode[] = "\
		#version 140\n\
		uniform mat4 viewing;\
		uniform mat4 modeling;\
		uniform mat4 viewingSdw;\
		in vec3 position;\
		in vec2 texCoords;\
		in vec3 normal;\
		out vec3 fragPos;\
		out vec3 normalDir;\
		out vec2 st;\
		out vec4 fragSdw;\
		void main(void) {\
			mat4 scaleBias = mat4(\
				0.5, 0.0, 0.0, 0.0, \
				0.0, 0.5, 0.0, 0.0, \
				0.0, 0.0, 0.5, 0.0, \
				0.5, 0.5, 0.5, 1.0);\
			vec4 worldPos = modeling * vec4(position, 1.0);\
			gl_Position = viewing * worldPos;\
			fragSdw = scaleBias * viewingSdw * worldPos;\
			fragPos = vec3(worldPos);\
			normalDir = vec3(modeling * vec4(normal, 0.0));\
			st = texCoords;\
		}";
	GLchar fragmentCode[] = "\
		#version 140\n\
		uniform sampler2D texture0;\
		uniform vec3 specular;\
		uniform vec3 camPos;\
		uniform vec3 lightPos;\
		uniform vec3 lightCol;\
		uniform vec3 lightAtt;\
		uniform vec3 lightAim;\
		uniform float lightCos;\
		uniform sampler2DShadow textureSdw;\
		in vec3 fragPos;\
		in vec3 normalDir;\
		in vec2 st;\
		in vec4 fragSdw;\
		out vec4 fragColor;\
		void main(void) {\
			vec3 diffuse = vec3(texture(texture0, st));\
			vec3 litDir = normalize(lightPos - fragPos);\
			float diffInt, specInt = 0.0;\
			if (dot(lightAim, -litDir) < lightCos)\
				diffInt = 0.0;\
			else\
				diffInt = 1.0;\
			float sdw = textureProj(textureSdw, fragSdw);\
			diffInt *= sdw;\
			specInt *= sdw;\
			vec3 diffRefl = max(0.4, diffInt) * lightCol * diffuse;\
			vec3 specRefl = specInt * lightCol * specular;\
			fragColor = vec4(diffRefl + specRefl, 1.0);\
		}";
	program = makeProgram(vertexCode, fragmentCode);
	if (program != 0) {
		glUseProgram(program);
		attrLocs[0] = glGetAttribLocation(program, "position");
		attrLocs[1] = glGetAttribLocation(program, "texCoords");
		attrLocs[2] = glGetAttribLocation(program, "normal");
		viewingLoc = glGetUniformLocation(program, "viewing");
		modelingLoc = glGetUniformLocation(program, "modeling");
		unifLocs[0] = glGetUniformLocation(program, "specular");
		textureLocs[0] = glGetUniformLocation(program, "texture0");
		camPosLoc = glGetUniformLocation(program, "camPos");
		lightPosLoc = glGetUniformLocation(program, "lightPos");
		lightColLoc = glGetUniformLocation(program, "lightCol");
		lightAttLoc = glGetUniformLocation(program, "lightAtt");
		lightDirLoc = glGetUniformLocation(program, "lightAim");
		lightCosLoc = glGetUniformLocation(program, "lightCos");
		viewingSdwLoc = glGetUniformLocation(program, "viewingSdw");
		textureSdwLoc = glGetUniformLocation(program, "textureSdw");
	}

	return (program == 0);
}

double velocity = 0;
int direction = -1;



void render(void) {
	// ============ for physics demo ============
	double friction = .86;
	if (nodeSphere.translation[0] < 100) {
		if (nodeSphere.translation[2] > 6) {
			velocity = velocity - 9.8 / 60;
		} else {
			// reduce energy every time ball hits the ground
			printf("vel before %f\n", velocity);
			velocity = friction * velocity * -1;
			printf("vel after %f\n", velocity);
		}
	} else {
		velocity = velocity - 9.8 / 60;
	}
	

	nodeSphere.translation[2] += velocity;
	nodeSphere.translation[0] += 2;

	if (nodeSphere.translation[0] > 150) {
		nodeSphere.translation[0] = -100;
		nodeSphere.translation[2] = 80;
	} 
	// ============ end physics demo ============

	GLdouble identity[4][4];
	mat44Identity(identity);

	/* Save the viewport transformation. */
	GLint viewport[4];

	glGetIntegerv(GL_VIEWPORT, viewport);
	GLint sdwTextureLocs[1] = {-1};
	shadowMapRender(&sdwMap, &sdwProg, &light, -100.0, -1.0);
	// shadowMapRender(&sdwMap2, &sdwProg2, &light2, -100.0, -1.0);

	sceneRender(&nodeFloor, identity, sdwProg.modelingLoc, 0, NULL, NULL, 1,
		sdwTextureLocs);

	/* Finish preparing the shadow maps, restore the viewport, and begin to
	render the scene. */
	shadowMapUnrender();

	glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(program);
	camRender(&cam, viewingLoc);
	GLfloat vec[3];
	vecOpenGL(3, cam.translation, vec);
	glUniform3fv(camPosLoc, 1, vec);
	/* For each light, we have to connect it to the shader program, as always.
	For each shadow-casting light, we must also connect its shadow map. */
	lightRender(&light, lightPosLoc, lightColLoc, lightAttLoc, lightDirLoc,
		lightCosLoc);
	shadowRender(&sdwMap, viewingSdwLoc, GL_TEXTURE7, 7, textureSdwLoc);
	// lightRender(&light2, lightPosLoc2, lightColLoc2, lightAttLoc2, lightDirLoc2, lightCosLoc2);
	// shadowRender(&sdwMap2, viewingSdwLoc2, GL_TEXTURE6, 6, textureSdwLoc2);
	GLuint unifDims[1] = {3};
	sceneRender(&nodeFloor, identity, modelingLoc, 1, unifDims, unifLocs, 0,
		textureLocs);
	// printf("Render: sceneRender on scene finishes\n");
	/* For each shadow-casting light, turn it off when finished rendering. */
	shadowUnrender(GL_TEXTURE7);
	// shadowUnrender(GL_TEXTURE6);
}

int main(void) {
	//ODE inits
	dInitODE();
	world = dWorldCreate();
	space = dHashSpaceCreate(0);
	contactgroup = dJointGroupCreate(0);
	//Gravity
	dWorldSetGravity(world, 0, 0, -9.8); // SI units
	ground = dCreatePlane(space, 0, 0, 1, 0);


	double oldTime;
	double newTime = getTime();
  	glfwSetErrorCallback(handleError);
  	if (glfwInit() == 0) {
  		fprintf(stderr, "main: glfwInit failed.\n");
    	return 1;
  	}
  	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  	GLFWwindow *window;
  	window = glfwCreateWindow(768, 768, "Shadows", NULL, NULL);
  	if (window == NULL) {
  		fprintf(stderr, "main: glfwCreateWindow failed.\n");
    	glfwTerminate();
    return 2;
  	}
  	glfwSetWindowSizeCallback(window, handleResize);
  	glfwSetKeyCallback(window, handleKey);
  	glfwMakeContextCurrent(window);
  	if (gl3wInit() != 0) {
  		fprintf(stderr, "main: gl3wInit failed.\n");
  		glfwDestroyWindow(window);
  		glfwTerminate();
  		return 3;
  	}

  	fprintf(stderr, "main: OpenGL %s, GLSL %s.\n",
	        glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));
	/* We no longer do glDepthRange(1.0, 0.0). Instead we have changed our
	projection matrices. */
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

  	if (initializeShaderProgram() != 0)
  		return 3;
  	/* Initialize the shadow mapping before the meshes. Why? */
	if (initializeCameraLight() != 0)
		return 4;
  	if (initializeScene() != 0)
  		return 5;

  	while (glfwWindowShouldClose(window) == 0) {
  		oldTime = newTime;
  		newTime = getTime();
  		if (floor(newTime) - floor(oldTime) >= 1.0)
			fprintf(stderr, "main: %f frames/sec\n", 1.0 / (newTime - oldTime));
		render();
    	glfwSwapBuffers(window);
    	glfwPollEvents();
  	}
  	/* Deallocate more resources than ever. */
  	shadowProgramDestroy(&sdwProg);
  	shadowMapDestroy(&sdwMap);
  	glDeleteProgram(program);
  	destroyScene();
	glfwDestroyWindow(window);
  	glfwTerminate();
  	return 0;
}
