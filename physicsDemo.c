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
static dJointGroupID trebuchetJointGroup;
static dGeomID ground;
static dReal radius = 0.25;
static dReal length = 1.0;
static dReal density = 5.0;
static dReal stepsize = 0.1;
#define max_contacts 4



// ==== scene globals ====
camCamera cam;
texTexture texGrass, texSun, texBox, texA, texB, texC;
shadowProgram sdwProg;

// ground
meshGLMesh ground_GL, sun_GL;
sceneNode ground_node, sun_node;

#define NUM_BOUNCIES 50
meshGLMesh bouncyGLs[NUM_BOUNCIES];
sceneNode bouncies[NUM_BOUNCIES];


#define BOX_STACK_LENGTH 4
#define NUM_BOXES BOX_STACK_LENGTH * BOX_STACK_LENGTH * BOX_STACK_LENGTH
meshGLMesh boxGLs[NUM_BOXES];
sceneNode boxes[NUM_BOXES];

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
	// int shiftIsDown = mods & GLFW_MOD_SHIFT;
	// int controlIsDown = mods & GLFW_MOD_CONTROL;
	// int altOptionIsDown = mods & GLFW_MOD_ALT;
	// int superCommandIsDown = mods & GLFW_MOD_SUPER;
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
			camAddDistance(&cam, -50.0);
		else if (key == GLFW_KEY_J)
			camAddDistance(&cam, 50.0);
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
		} else if (key == GLFW_KEY_1) {
			dReal curGravity[3];
			dWorldGetGravity(world, curGravity);
			dWorldSetGravity(world, curGravity[0], curGravity[1], -9.81);
		} else if (key == GLFW_KEY_2) {
			dReal curGravity[3];
			dWorldGetGravity(world, curGravity);
			dWorldSetGravity(world, curGravity[0], curGravity[1], 9.81);
		} else if (key == GLFW_KEY_3) {
			dReal curGravity[3];
			dWorldGetGravity(world, curGravity);
			dWorldSetGravity(world, -5, curGravity[1], curGravity[2]);
		} else if (key == GLFW_KEY_4) {
			dReal curGravity[3];
			dWorldGetGravity(world, curGravity);
			dWorldSetGravity(world, 5, curGravity[1], curGravity[2]);
		} else if (key == GLFW_KEY_5) {
			dReal curGravity[3];
			dWorldGetGravity(world, curGravity);
			dWorldSetGravity(world, curGravity[0], -5, curGravity[2]);
		} else if (key == GLFW_KEY_6) {
			dReal curGravity[3];
			dWorldGetGravity(world, curGravity);
			dWorldSetGravity(world, curGravity[0], 5, curGravity[2]);
		}
	}
}




/* Returns 0 on success, non-zero on failure. Warning: If initialization fails
midway through, then does not properly deallocate all resources. But that's
okay, because the program terminates almost immediately after this function
returns. */
int initializeScene(void) {


	// note: meshInitializeBox requires negative to positive values.
	if (texInitializeFile(&texGrass, "grass.jpg", GL_LINEAR, GL_LINEAR,
    		GL_REPEAT, GL_REPEAT) != 0)
    	return 1;
    if (texInitializeFile(&texSun, "sun.jpg", GL_LINEAR, GL_LINEAR,
    		GL_REPEAT, GL_REPEAT) != 0)
    	return 1;
    if (texInitializeFile(&texBox, "box.jpg", GL_LINEAR, GL_LINEAR,
    		GL_REPEAT, GL_REPEAT) != 0)
    	return 1;
    if (texInitializeFile(&texA, "a.jpg", GL_LINEAR, GL_LINEAR,
    		GL_REPEAT, GL_REPEAT) != 0)
    	return 1;
    if (texInitializeFile(&texB, "b.jpg", GL_LINEAR, GL_LINEAR,
    		GL_REPEAT, GL_REPEAT) != 0)
    	return 1;
    if (texInitializeFile(&texC, "c.jpg", GL_LINEAR, GL_LINEAR,
    		GL_REPEAT, GL_REPEAT) != 0)
    	return 1;


	meshMesh mesh;
	GLuint attrDims[3] = {3, 2, 3};
	int vaoNums = 2;



	int i;

	// ==== initialize meshGLMeshes for boxes
	int boxXL = 40;
	int boxYL = 20;
	int boxZL = 20;
	int boxDensity = 100;
	for (i = 0; i < NUM_BOXES; i ++) {
		if (meshInitializeBox(&mesh, -boxXL / 2, boxXL / 2, -boxYL / 2, boxYL / 2, -boxZL / 2, boxZL / 2, world, space, boxDensity) != 0) {
			return 1;
		}
		meshGLInitialize(&boxGLs[i], &mesh, 3, attrDims, vaoNums);
		meshGLVAOInitialize(&boxGLs[i], 0, attrLocs);
		meshGLVAOInitialize(&boxGLs[i], 1, sdwProg.attrLocs);
		meshDestroy(&mesh);
	}

	// ==== initialize mesGLMeshes for bouncies
	int objectDensity = 20;
	for (i = 0; i < NUM_BOUNCIES; i ++) {
		switch(i % 3) {
			case(0): {
				if (meshInitializeBox(&mesh, -10.0, 10.0, -10.0, 10.0, -10.0, 10.0, world, space, objectDensity) != 0) {
					return 1;
				}
				meshGLInitialize(&bouncyGLs[i], &mesh, 3, attrDims, vaoNums);
				meshGLVAOInitialize(&bouncyGLs[i], 0, attrLocs);
				meshGLVAOInitialize(&bouncyGLs[i], 1, sdwProg.attrLocs);
				meshDestroy(&mesh);
				break;
			} case (1): {
				if (meshInitializeSphere(&mesh, 10.0, 10, 10, world, space, objectDensity) != 0) {
					return 1;
				}
				meshGLInitialize(&bouncyGLs[i], &mesh, 3, attrDims, vaoNums);
				meshGLVAOInitialize(&bouncyGLs[i], 0, attrLocs);
				meshGLVAOInitialize(&bouncyGLs[i], 1, sdwProg.attrLocs);
				meshDestroy(&mesh);
				break;
			} case (2): {
				if (meshInitializeCapsule(&mesh, 10.0, 30.0, 10, 10, world, space, objectDensity) != 0) {
					return 1;
				}
				// if (meshInitializeSphere(&mesh, 20.0, 20, 20, world, space, objectDensity) != 0) {
				// 	return 1;
				// }
				meshGLInitialize(&bouncyGLs[i], &mesh, 3, attrDims, vaoNums);
				meshGLVAOInitialize(&bouncyGLs[i], 0, attrLocs);
				meshGLVAOInitialize(&bouncyGLs[i], 1, sdwProg.attrLocs);
				meshDestroy(&mesh);
				break;
			} default: {
				printf("something went wrong\n");
				break;
			}
		}	
	}
	dReal x, y, z;
	// ==== initialize scenes for boxes
	int boxIdx = 0;
	x = 0;
	y = 0;
	z = 0;
	for (i = NUM_BOXES - 1; i >= 0; i --) {
		if (i != NUM_BOXES - 1) {
			if (sceneInitialize(&boxes[i], 3, 1, &boxGLs[i], NULL, &boxes[i + 1], world) != 0) {
				return 2;
			}
		} else {
			if (sceneInitialize(&boxes[i], 3, 1, &boxGLs[i], NULL, NULL, world) != 0) {
				return 2;
			}
		}
		dBodySetPosition(boxes[i].meshGL->body, x, y, z);
		x = x + boxXL;
		if (i % BOX_STACK_LENGTH == 0) {
			x = 0;
			y = y + boxYL;
		}
		if (i % (BOX_STACK_LENGTH * BOX_STACK_LENGTH) == 0) {
			y = 0;
			z = z + boxZL;
		}

	}
	// int l,m,n;
	// for (l = 0; l < BOX_STACK_LENGTH; l++) {
	// 	for (m = 0; m < BOX_STACK_LENGTH; m++) {
	// 		for (n = 0; n < BOX_STACK_LENGTH; n++) {
				
	// 			dBodySetPosition(boxes[i].meshGL->body, x, y, z);

	// 			x = x + 20;
	// 		}
	// 		y = y + 10;
	// 	}
	// 	z = z + 10;
	// }


	// ==== initialize scenes for bouncies
	for (i = NUM_BOUNCIES - 1; i >= 0 ; i --) { // build bouncies in reverse order (because of children issues)
		if (i != NUM_BOUNCIES - 1) {
			if (sceneInitialize(&bouncies[i], 3, 1, &bouncyGLs[i], NULL, &bouncies[i + 1], world) != 0) {
				return 2;
			}
		} else { // last thing in node list is 
			if (sceneInitialize(&bouncies[i], 3, 1, &bouncyGLs[i], NULL, &boxes[0], world) != 0) {
				return 2;
			}
		}
		x = (float)rand()/(float)(RAND_MAX/100) * 2 - 100;
		y = (float)rand()/(float)(RAND_MAX/100) * 2 - 100;
		// z = (float)rand()/(float)(RAND_MAX/100);
		z = 500;
		dBodySetPosition(bouncies[i].meshGL->body, x, y, z);



		dMatrix3 R;

	    dRFromAxisAndAngle(R, dRandReal() * 2.0 - 1.0,
	                          dRandReal() * 2.0 - 1.0,
	                          dRandReal() * 2.0 - 1.0,
	                          dRandReal() * 10.0 - 5.0);
	    dBodySetRotation(bouncies[i].meshGL->body, R);

	}
	
	// sun
	int sunDensity = 10000.0;
	if (meshInitializeSphere(&mesh, 45, 20, 20, world, space, sunDensity) != 0) {
		return 1;
	}
	meshGLInitialize(&sun_GL, &mesh, 3, attrDims, vaoNums);
	meshGLVAOInitialize(&sun_GL, 0, attrLocs);
	meshGLVAOInitialize(&sun_GL, 1, sdwProg.attrLocs);
	meshDestroy(&mesh);


	// ground
	int groundDensity = 200.0;
	if (meshInitializeBox(&mesh, -1000.0, 1000.0, -1000.0, 1000.0, -1.0, 1.0, world, space, groundDensity) != 0) {
		return 1;
	}
	meshGLInitialize(&ground_GL, &mesh, 3, attrDims, vaoNums);
	meshGLVAOInitialize(&ground_GL, 0, attrLocs);
	meshGLVAOInitialize(&ground_GL, 1, sdwProg.attrLocs);
	meshDestroy(&mesh);

	if (sceneInitialize(&sun_node, 3, 1, &sun_GL, NULL, &bouncies[0], world) != 0)
		return 2;
	if (sceneInitialize(&ground_node, 3, 1, &ground_GL, NULL, &sun_node, world) != 0)
		return 2;


    dBodySetKinematic(ground_node.meshGL->body);
    dBodySetKinematic(sun_node.meshGL->body);
	x = 0.0;
	y = 0.0;
	z = 0.0;
	dBodySetPosition(ground_node.meshGL->body, x, y, z);
	z = 495.0;
	dBodySetPosition(sun_node.meshGL->body, x, y, z);

	texTexture *tex;
	tex = &texGrass;
	sceneSetTexture(&ground_node, &tex);
	tex = &texSun;
	sceneSetTexture(&sun_node, &tex);


	tex = &texBox;
	for (i = 0; i < NUM_BOXES; i++) {
		sceneSetTexture(&boxes[i], &tex);
	}

	for (i = 0; i < NUM_BOUNCIES; i ++) {
		switch(i % 3) {
			case(0): {
				tex = &texA;
				sceneSetTexture(&bouncies[i], &tex);
				break;
			} case(1): {
				tex = &texB;
				sceneSetTexture(&bouncies[i], &tex);
				break;
			} case(2): {
				tex = &texC;
				sceneSetTexture(&bouncies[i], &tex);
				break;
			} default: {
				printf("something went wrong\n");
				break;
			}
		}
		
	}
	return 0;
}

void destroyScene(void) {
	// add destroy stuff 	
}

/* Returns 0 on success, non-zero on failure. Warning: If initialization fails
midway through, then does not properly deallocate all resources. But that's
okay, because the program terminates almost immediately after this function
returns. */
int initializeCameraLight(void) {
  	GLdouble vec[3] = {0.0, 0.0, 0.0};
	// camSetControls(&cam, camPERSPECTIVE, M_PI / 6.0, 1000.0, 768.0, 768.0, 1000.0,
	// 							 M_PI / 4.0, M_PI / 4.0, vec);
	camSetControls(&cam, camPERSPECTIVE, M_PI / 6.0, 6.0, 1000.0, 1000.0, 1500.0,
								 M_PI / 4.0, M_PI / 4.0, vec);
	lightSetType(&light, lightSPOT);
	vecSet(3, vec, 0.0, 0.0, 500.0);
	lightShineFrom(&light, vec, M_PI , M_PI);
	vecSet(3, vec, 1.0, 1.0, 1.0);
	lightSetColor(&light, vec);
	vecSet(3, vec, 1.0, 0.0, 0.0);
	lightSetAttenuation(&light, vec);
	lightSetSpotAngle(&light, M_PI / 2);
	/* Configure shadow mapping. */
	if (shadowProgramInitialize(&sdwProg, 3) != 0)
		return 1;
	if (shadowMapInitialize(&sdwMap, 1024, 1024) != 0)
		return 2;

	// camAddDistance(&cam, 1000.0);
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
			vec3 diffRefl = max(0.6, diffInt) * lightCol * diffuse;\
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


static void nearCallback (void *data, dGeomID o1, dGeomID o2) {

    // Temporary index for each contact
    int i;


   // Get the dynamics body for each geom
    dBodyID b1 = dGeomGetBody(o1);
    dBodyID b2 = dGeomGetBody(o2);
	//Starting joints preserved if == 1
	if (dAreConnected(b1, b2) == 1)
		return;


    // Create an array of dContact objects to hold the contact joints
    dContact contact[max_contacts];



    // Now we set the joint properties of each contact. Going into the full details here would require a tutorial of its
       // own. I'll just say that the members of the dContact structure control the joint behaviour, such as friction,
       // velocity and bounciness. See section 7.3.7 of the ODE manual and have fun experimenting to learn more.  
    for (i = 0; i < max_contacts; i++) {
        contact[i].surface.mode = dContactBounce;
        contact[i].surface.mu = dInfinity;
        // contact[i].surface.mu2 = 0.5;
        contact[i].surface.bounce = 0.9;
        contact[i].surface.bounce_vel = 0.1;
    }



    // Here we do the actual collision test by calling dCollide. It returns the number of actual contact points or zero
       // if there were none. As well as the geom IDs, max number of contacts we also pass the address of a dContactGeom
       // as the fourth parameter. dContactGeom is a substructure of a dContact object so we simply pass the address of
       // the first dContactGeom from our array of dContact objects and then pass the offset to the next dContactGeom
       // as the fifth paramater, which is the size of a dContact structure. That made sense didn't it?  
    if (int numc = dCollide(o1, o2, max_contacts, &contact[0].geom, sizeof(dContact))) {
        // To add each contact point found to our joint group we call dJointCreateContact which is just one of the many
           // different joint types available.  
        for (i = 0; i < numc; i++) {
            // dJointCreateContact needs to know which world and joint group to work with as well as the dContact
               // object itself. It returns a new dJointID which we then use with dJointAttach to finally create the
               // temporary contact joint between the two geom bodies.
            dJointID c = dJointCreateContact(world, contactgroup, contact + i);
            dJointAttach(c, b1, b2);
        }
    }
}
void nodeUpdateTransRot(sceneNode* node) {
	const dReal *pos = dBodyGetPosition(node->meshGL->body);
	const dReal *rot = dBodyGetRotation(node->meshGL->body);
	dReal x,y,z;
	int changed = 0;

		
		
		
	if (pos[0] > 1000 || pos[0] < -1000) {
		x = (float)rand()/(float)(RAND_MAX/100) * 2 - 100;
		y = (float)rand()/(float)(RAND_MAX/100) * 2 - 100;
		z = 500;
		changed = 1;
	}
	if (pos[1] > 1000 || pos[1] < -1000) {
		x = (float)rand()/(float)(RAND_MAX/100) * 2 - 100;
		y = (float)rand()/(float)(RAND_MAX/100) * 2 - 100;
		z = 500;
		changed = 1;
	}
	if (pos[2] > 5000 || pos[2] < -300) {
		// z = (float)rand()/(float)(RAND_MAX/100) + 300;
		printf("here\n");
		x = (float)rand()/(float)(RAND_MAX/100) * 2 - 100;
		y = (float)rand()/(float)(RAND_MAX/100) * 2 - 100;
		z = 500;
		changed = 1;
	}
	if (changed) {
		dBodySetPosition(node->meshGL->body, x, y, z);
	}


	node->translation[0] = pos[0];
   	node->translation[1] = pos[1];
	node->translation[2] = pos[2];


	// ODE rotation vec is in the following convention
	// r00 r01 r02 x
	// r10 r11 r12 y
	// r20 r21 r22 z
	// where x, y, z are undefined
	node->rotation[0][0] = rot[0];
	node->rotation[0][1] = rot[1];
	node->rotation[0][2] = rot[2];
	node->rotation[1][0] = rot[4];
	node->rotation[1][1] = rot[5];
	node->rotation[1][2] = rot[6];
	node->rotation[2][0] = rot[8];
	node->rotation[2][1] = rot[9];
	node->rotation[2][2] = rot[10];
	
}


void render(void) {

	GLdouble identity[4][4];
	mat44Identity(identity);

	/* Save the viewport transformation. */
	GLint viewport[4];

	glGetIntegerv(GL_VIEWPORT, viewport);
	GLint sdwTextureLocs[1] = {-1};
	shadowMapRender(&sdwMap, &sdwProg, &light, -1000.0, -1.0);
	sceneRender(&ground_node, identity, sdwProg.modelingLoc, 0, NULL, NULL, 1,
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
	// sceneRender(&nodeH, identity, modelingLoc, 1, unifDims, unifLocs, 0,
		// textureLocs);
	sceneRender(&ground_node, identity, modelingLoc, 1, unifDims, unifLocs, 0, textureLocs);
	shadowUnrender(GL_TEXTURE7);

	//ODE simulation step
	
	//Seriously, no sceneSetTranslation() usage here??
	// I changed it to handle this operation.
	nodeUpdateTransRot(&ground_node);
	nodeUpdateTransRot(&sun_node);
	int i;
	for (i = 0; i <NUM_BOXES; i++) {
		nodeUpdateTransRot(&boxes[i]);
	}
	for (i = 0; i < NUM_BOUNCIES; i ++) {
		nodeUpdateTransRot(&bouncies[i]);
	}
}
void startODE(void){
	dInitODE2(0);
	world = dWorldCreate();
	// space = dHashSpaceCreate(0); // come back to this later maybe
	space = dSimpleSpaceCreate(0);
	contactgroup = dJointGroupCreate(0);
	trebuchetJointGroup = dJointGroupCreate(100);
	dWorldSetGravity(world, 0.0, 0.0, 3 * -9.81);
	// dWorldSetGravity(world, 0.0, 0.0, -0.0);
	dWorldSetContactSurfaceLayer(world, 0.001);
	//error correction parameters. Sets the world to double-precision
	dWorldSetERP(world, 0.3);
	dWorldSetCFM(world, (dReal) pow(10,-10));
	ground = dCreatePlane(space, 0.0, 0.0, 1.0, 0.0);
	
}


int main(void) {
	//moved ODE setup into funct
	startODE();
   
    
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




		dSpaceCollide(space, 0, &nearCallback);
	
		// dWorldStep(world, stepsize);
		dWorldQuickStep(world, stepsize);

		dJointGroupEmpty(contactgroup);


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
	dSpaceDestroy(space);
	dCloseODE();
	return 0;
}
