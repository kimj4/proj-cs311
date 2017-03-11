/* On macOS, compile with...
    clang 590mainShadowing.c /usr/local/gl3w/src/gl3w.o -lglfw -framework OpenGL -framework CoreFoundation
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

// === ODE globals ====
static dWorldID world;
static dSpaceID space;
static dJointGroupID contactgroup;
static dGeomID ground;
static dReal radius = 0.25;
static dReal length = 1.0;

#define DENSITY (5.0)
#define max_contacts 10


dContactGeom contact_array[100];
int skip = sizeof(dContactGeom);

// ==== scene globals ====
camCamera cam;
texTexture texH, texV, texW, texT, texL, texTrebuchet;
meshGLODE meshGLODEH, meshGLODEV, meshGLODEW, meshGLODET, meshGLODEL, meshGLODETrebuchet;
meshGLMesh meshGLH, meshGLV, meshGLW, meshGLT, meshGLL, meshGLTrebuchet;
sceneNode nodeH, nodeV, nodeW, nodeT, nodeL, nodeTrebuchet;
shadowProgram sdwProg;

lightLight light;
shadowMap sdwMap;

GLuint program;
GLint viewingLoc, modelingLoc;
GLint unifLocs[1], textureLocs[1];
GLint attrLocs[3];
GLint lightPosLoc, lightColLoc, lightAttLoc, lightDirLoc, lightCosLoc;
GLint camPosLoc;
GLint viewingSdwLoc, textureSdwLoc;


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
		else if (key == GLFW_KEY_K)
			camAddPhi(&cam, 0.1);
		else if (key == GLFW_KEY_U)
			camAddDistance(&cam, -0.5);
		else if (key == GLFW_KEY_J)
			camAddDistance(&cam, 0.5);
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
	// note: meshInitializeBox requires negative to positive values.
	if (texInitializeFile(&texH, "grass.jpg", GL_LINEAR, GL_LINEAR,
    		GL_REPEAT, GL_REPEAT) != 0)
    	return 1;
	if (texInitializeFile(&texV, "granite.jpg", GL_LINEAR, GL_LINEAR,
			GL_REPEAT, GL_REPEAT) != 0)
		return 2;

	if (texInitializeFile(&texW, "water.jpg", GL_LINEAR, GL_LINEAR,
			GL_REPEAT, GL_REPEAT) != 0)
		return 3;
	if (texInitializeFile(&texT, "trunk.jpg", GL_LINEAR, GL_LINEAR,
			GL_REPEAT, GL_REPEAT) != 0)
		return 4;
	if (texInitializeFile(&texL, "tree.jpg", GL_LINEAR, GL_LINEAR,
			GL_REPEAT, GL_REPEAT) != 0)
	return 5;
	GLuint attrDims[3] = {3, 2, 3};
    double zs[12][12] = {
		{5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 20.0},
		{5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 20.0, 25.0},
		{5.0, 5.0, 10.0, 12.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 20.0, 25.0},
		{5.0, 5.0, 10.0, 10.0, 5.0, 5.0, 5.0, 5.0, 5.0, 20.0, 25.0, 27.0},
		{0.0, 0.0, 5.0, 5.0, 5.0, 0.0, 0.0, 0.0, 0.0, 20.0, 20.0, 25.0},
		{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 20.0, 25.0},
		{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
		{0.0, 0.0, 0.0, 0.0, 0.0, 5.0, 5.0, 0.0, 0.0, 0.0, 0.0, 0.0},
		{0.0, 0.0, 0.0, 0.0, 0.0, 5.0, 7.0, 0.0, 0.0, 0.0, 0.0, 0.0},
		{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 20.0, 20.0},
		{5.0, 5.0, 5.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 20.0, 20.0, 20.0},
		{10.0, 10.0, 5.0, 5.0, 0.0, 0.0, 0.0, 5.0, 10.0, 15.0, 20.0, 25.0}};
	double ws[12][12] = {
		{1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0},
		{1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0},
		{1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0},
		{1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0},
		{1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0},
		{1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0},
		{1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0},
		{1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0},
		{1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0},
		{1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0},
		{1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0},
		{1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0}};
	meshMesh mesh, meshLand;
	if (meshInitializeLandscape(&meshLand, 12, 12, 5.0, (double *)zs) != 0)
		return 6;
	if (meshInitializeDissectedLandscape(&mesh, &meshLand, M_PI / 3.0, 1) != 0)
		return 7;
	/* There are now two VAOs per mesh. */


	// ======================= meshGLODE usage here ===============================	
	int vaoNums = 2;

	meshGLInitialize(&meshGLH, &mesh, 3, attrDims, vaoNums);
	meshGLVAOInitialize(&meshGLH, 0, attrLocs);
	meshGLVAOInitialize(&meshGLH, 1, sdwProg.attrLocs);
	meshGLODEInitialize(&meshGLODEH, &meshGLH, &mesh, space);
	meshDestroy(&mesh);

	// === dissected landscape ===
	if (meshInitializeDissectedLandscape(&mesh, &meshLand, M_PI / 3.0, 0) != 0)
		return 8;
	meshDestroy(&meshLand);
	double *vert, normal[2];
	for (int i = 0; i < mesh.vertNum; i += 1) {
		vert = meshGetVertexPointer(&mesh, i);
		normal[0] = -vert[6];
		normal[1] = vert[5];
		vert[3] = (vert[0] * normal[0] + vert[1] * normal[1]) / 20.0;
		vert[4] = vert[2] / 20.0;
	}
	meshGLInitialize(&meshGLV, &mesh, 3, attrDims, vaoNums);
	meshGLVAOInitialize(&meshGLV, 0, attrLocs);
	meshGLVAOInitialize(&meshGLV, 1, sdwProg.attrLocs);
	meshGLODEInitialize(&meshGLODEV, &meshGLV, &mesh, space);
	meshDestroy(&mesh);


	// === landscape ===
	if (meshInitializeLandscape(&mesh, 12, 12, 5.0, (double *)ws) != 0)
		return 9;
	meshGLInitialize(&meshGLW, &mesh, 3, attrDims, vaoNums);
	
	meshGLVAOInitialize(&meshGLW, 0, attrLocs);
	meshGLVAOInitialize(&meshGLW, 1, sdwProg.attrLocs);
	meshGLODEInitialize(&meshGLODEW, &meshGLW, &mesh, space);
	meshDestroy(&mesh);


	// === tree trunk ===
	if (meshInitializeCapsule(&mesh, 1.0, 10.0, 1, 8) != 0)
		return 10;
	meshGLInitialize(&meshGLT, &mesh, 3, attrDims, vaoNums);
	meshGLVAOInitialize(&meshGLT, 0, attrLocs);
	meshGLVAOInitialize(&meshGLT, 1, sdwProg.attrLocs);
	meshGLODEInitialize(&meshGLODET, &meshGLT, &mesh, space);
	meshDestroy(&mesh);

	// === tree leaves === 
	if (meshInitializeBox(&mesh, -5, 5, -5, 5, -5, 5) != 0)
		return 11;
	meshGLInitialize(&meshGLL, &mesh, 3, attrDims, vaoNums);
	meshGLVAOInitialize(&meshGLL, 0, attrLocs);
	meshGLVAOInitialize(&meshGLL, 1, sdwProg.attrLocs);
	meshGLODEInitialize(&meshGLODEL, &meshGLL, &mesh, space);
	meshDestroy(&mesh);
//pass meshGLODE to a sceneNode
	if (sceneInitialize(&nodeW, 3, 1, &meshGLODEW, NULL, NULL, world) != 0)
		return 14;
	if (sceneInitialize(&nodeL, 3, 1, &meshGLODEL, NULL, NULL, world) != 0)
		return 16;
	if (sceneInitialize(&nodeT, 3, 1, &meshGLODET, &nodeL, &nodeW, world) != 0)
		return 15;
	if (sceneInitialize(&nodeV, 3, 1, &meshGLODEV, NULL, &nodeT, world) != 0)
		return 13;
	if (sceneInitialize(&nodeH, 3, 1, &meshGLODEH, &nodeV, NULL, world) != 0)
		return 12;
//meshGLODE end usage=============
	dReal x,y,z;


	GLdouble trans[3] = {40.0, 28.0, 5.0};
	sceneSetTranslation(&nodeT, trans); // tree trunk translation
	x = trans[0];
	y = trans[1];
	z = trans[2];
	dBodySetPosition(nodeT.body, x, y, z);


	vecSet(3, trans, 0.0, 0.0, 100.0);
	sceneSetTranslation(&nodeL, trans); // tree leaves translation
	x = trans[0];
	y = trans[1];
	z = trans[2];
	dBodySetPosition (nodeL.body, x, y, z);


	GLdouble unif[3] = {0.0, 0.0, 0.0};
	sceneSetUniform(&nodeH, unif);
	sceneSetUniform(&nodeV, unif);
	sceneSetUniform(&nodeT, unif);
	sceneSetUniform(&nodeL, unif);
	vecSet(3, unif, 1.0, 1.0, 1.0);
	sceneSetUniform(&nodeW, unif);
	texTexture *tex;
	tex = &texH;
	sceneSetTexture(&nodeH, &tex);
	tex = &texV;
	sceneSetTexture(&nodeV, &tex);
	tex = &texW;
	sceneSetTexture(&nodeW, &tex);
	tex = &texT;
	sceneSetTexture(&nodeT, &tex);
	tex = &texL;
	sceneSetTexture(&nodeL, &tex);
	return 0;
}

void destroyScene(void) {
 	texDestroy(&texH);
 	texDestroy(&texV);
 	texDestroy(&texW);
 	texDestroy(&texT);
 	texDestroy(&texL);
   //implement meshGLODE destruction
 	meshGLDestroy(&meshGLH);
 	meshGLDestroy(&meshGLV);
 	meshGLDestroy(&meshGLW);
 	meshGLDestroy(&meshGLT);
 	meshGLDestroy(&meshGLL);
 	sceneDestroyRecursively(&nodeH);
}

/* Returns 0 on success, non-zero on failure. Warning: If initialization fails
midway through, then does not properly deallocate all resources. But that's
okay, because the program terminates almost immediately after this function
returns. */
int initializeCameraLight(void) {
  	GLdouble vec[3] = {30.0, 30.0, 5.0};
	camSetControls(&cam, camPERSPECTIVE, M_PI / 6.0, 10.0, 768.0, 768.0, 100.0,
								 M_PI / 4.0, M_PI / 4.0, vec);
	lightSetType(&light, lightSPOT);
	vecSet(3, vec, 45.0, 30.0, 20.0);
	lightShineFrom(&light, vec, M_PI * 3.0 / 4.0, M_PI * 3.0 / 4.0);
	vecSet(3, vec, 1.0, 1.0, 1.0);
	lightSetColor(&light, vec);
	vecSet(3, vec, 1.0, 0.0, 0.0);
	lightSetAttenuation(&light, vec);
	lightSetSpotAngle(&light, M_PI / 3.0);
	/* Configure shadow mapping. */
	if (shadowProgramInitialize(&sdwProg, 3) != 0)
		return 1;
	if (shadowMapInitialize(&sdwMap, 1024, 1024) != 0)
		return 2;
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
			vec3 diffRefl = max(0.2, diffInt) * lightCol * diffuse;\
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


//Collision Detection from the manual
void nearCallback (void *data, dGeomID o1, dGeomID o2) {
    if (dGeomIsSpace (o1) || dGeomIsSpace (o2)) { 

        // colliding a space with something :
        dSpaceCollide2 (o1, o2, data,&nearCallback); 
   
        // collide all geoms internal to the space(s)
        if (dGeomIsSpace (o1))
            dSpaceCollide ((dSpaceID)o1, data, &nearCallback);
        if (dGeomIsSpace (o2))
            dSpaceCollide ((dSpaceID)o2, data, &nearCallback);

    } else {
        // colliding two non-space geoms, so generate contact points between o1 and o2
        int num_contact = dCollide (o1, o2, max_contacts, contact_array, skip);


        // const dReal *a = dGeomGetPosition(contact_array[num_contact].g1);
        // const dReal *a = dGeomGetPosition(o1);
        // dGeomSetPosition(o1, a[0] + 10, a[1], a[2]);
		
        // printf("%i\n", num_contact);
        // add these contact points to the simulation ...
    }
}

// static void nearCallback(void *data, dGeomID o1, dGeomID o2){
// 	dContact contact[max_Contact];

// 	if (dGeomIsSpace(o1) || dGeomIsSpace(o2)) {
// 		printf("nearCallback: if\n");
// 		dSpaceCollide2(o1, o2, data, &nearCallback);
	
// 		if (dGeomIsSpace(o1))
// 			dSpaceCollide((dSpaceID)o1, data, &nearCallback);
// 		if (dGeomIsSpace(o2))
// 			dSpaceCollide((dSpaceID)o2, data, &nearCallback);
//   	} else {
//   		// printf("nearCallback: else\n");
//   		// printf("o1: %p\n", o1);
// 	  	//number of contacts
// 	  	int n = dCollide(o1, o2, max_Contact, &contact[0].geom, sizeof(contact[0].geom));
// 		int i;
// 		for (int i = 0; i < n; i++) {
// 			contact[i].surface.mode = dContactBounce;
// 			contact[i].surface.mu   = dInfinity;
// 			contact[i].surface.bounce     = 0.0; // (0.0~1.0) restitution parameter
// 			contact[i].surface.bounce_vel = 0.0; // minimum incoming velocity for bounce
// 			dJointID c = dJointCreateContact(world,contactgroup,&contact[i]);
// 			dJointAttach(c,dGeomGetBody(contact[i].geom.g1),dGeomGetBody(contact[i].geom.g2));
// 		}

//   	}	
// }

void render(void) {

	GLdouble identity[4][4];
	mat44Identity(identity);

	/* Save the viewport transformation. */
	GLint viewport[4];

	glGetIntegerv(GL_VIEWPORT, viewport);
	GLint sdwTextureLocs[1] = {-1};
	shadowMapRender(&sdwMap, &sdwProg, &light, -100.0, -1.0);
	sceneRender(&nodeH, identity, sdwProg.modelingLoc, 0, NULL, NULL, 1,
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
	GLuint unifDims[1] = {3};
	sceneRender(&nodeH, identity, modelingLoc, 1, unifDims, unifLocs, 0,
		textureLocs);
	// printf("Render: sceneRender on scene finishes\n");
	/* For each shadow-casting light, turn it off when finished rendering. */
	shadowUnrender(GL_TEXTURE7);

	dSpaceCollide(space, 0, &nearCallback);
	dWorldStep(world, 0.01);
	dJointGroupEmpty(contactgroup);

	const dReal *pos1 = dBodyGetPosition(nodeL.body);
	const dReal *rot1 = dBodyGetRotation(nodeL.body);
	nodeL.translation[2] = pos1[2];

	// const dReal *a = dBodyGetPosition(nodeL.body); 
	// const dReal *b = dGeomGetPosition(nodeL.meshGLODE->geom);

	// printf("%f\n", a[2]);
	// printf("%f\n", b[2]);


}

int main(void) {
	
	dInitODE();
	world = dWorldCreate();
	space = dHashSpaceCreate(0);
	contactgroup = dJointGroupCreate(0);
	dWorldSetGravity(world, 0, 0, -9.81);
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
	dWorldDestroy(world);
	dCloseODE();
	return 0;
}
