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
static dReal stepsize = 0.01;
#define max_contacts 20



// ==== scene globals ====
camCamera cam;
texTexture texH, texTrebuchet;
shadowProgram sdwProg;

// trebuchet 
meshGLMesh wheelFL_GL, wheelFR_GL, wheelBL_GL, wheelBR_GL,
		  axelF_GL, axelB_GL, baseL_GL, baseR_GL,
		  supportFL_GL, supportFR_GL, supportBL_GL, supportBR_GL,
		  cross_GL, arm_GL, counterweight_GL, bucket_GL;
sceneNode wheelFL_node, wheelFR_node, wheelBL_node, wheelBR_node,
		  axelF_node, axelB_node, baseL_node, baseR_node,
		  supportFL_node, supportFR_node, supportBL_node, supportBR_node,
		  cross_node, arm_node, counterweight_node, bucket_node;

// ground
meshGLMesh ground_GL;
sceneNode ground_node;

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
			camAddDistance(&cam, -10.5);
		else if (key == GLFW_KEY_J)
			camAddDistance(&cam, 10.0);
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
	if (texInitializeFile(&texH, "grass.jpg", GL_LINEAR, GL_LINEAR,
    		GL_REPEAT, GL_REPEAT) != 0)
    	return 1;
	if (texInitializeFile(&texTrebuchet, "lumber.jpg", GL_LINEAR, GL_LINEAR,
			GL_REPEAT, GL_REPEAT) != 0)
		return 2;

	meshMesh mesh;
	GLuint attrDims[3] = {3, 2, 3};
	int vaoNums = 2;


	// ======================= start trebuchet construction =======================
	// crossbar: the thing that holds the throwing arm
	GLdouble tu = 5; // trebuchet unit
	int trebuchetBodyDensity = 20;
	if (meshInitializeBox(&mesh, -1.0 * tu, 1.0 * tu, -0.25 * tu, 0.25 * tu, -0.25 * tu, 0.25 * tu, world, space, trebuchetBodyDensity) != 0)
		return 999;
	meshGLInitialize(&cross_GL, &mesh, 3, attrDims, vaoNums);
	meshGLVAOInitialize(&cross_GL, 0, attrLocs);
	meshGLVAOInitialize(&cross_GL, 1, sdwProg.attrLocs);
	meshDestroy(&mesh);

	// supports: the four diagonally oriented beams that connect crossbar to the bases
	if (meshInitializeBox(&mesh, -5.0 * tu, 5.0 * tu, -0.25 * tu, 0.25 * tu, -0.25 * tu, 0.25 * tu, world, space, trebuchetBodyDensity) != 0)
		return 999;
	meshGLInitialize(&supportFL_GL, &mesh, 3, attrDims, vaoNums);
	meshGLVAOInitialize(&supportFL_GL, 0, attrLocs);
	meshGLVAOInitialize(&supportFL_GL, 1, sdwProg.attrLocs);
	meshDestroy(&mesh);
	if (meshInitializeBox(&mesh, -5.0 * tu, 5.0 * tu, -0.25 * tu, 0.25 * tu, -0.25 * tu, 0.25 * tu, world, space, trebuchetBodyDensity) != 0)
		return 999;
	meshGLInitialize(&supportFR_GL, &mesh, 3, attrDims, vaoNums);
	meshGLVAOInitialize(&supportFR_GL, 0, attrLocs);
	meshGLVAOInitialize(&supportFR_GL, 1, sdwProg.attrLocs);
	meshDestroy(&mesh);
	if (meshInitializeBox(&mesh, -5.0 * tu, 5.0 * tu, -0.25 * tu, 0.25 * tu, -0.25 * tu, 0.25 * tu, world, space, trebuchetBodyDensity) != 0)
		return 999;
	meshGLInitialize(&supportBL_GL, &mesh, 3, attrDims, vaoNums);
	meshGLVAOInitialize(&supportBL_GL, 0, attrLocs);
	meshGLVAOInitialize(&supportBL_GL, 1, sdwProg.attrLocs);
	meshDestroy(&mesh);
	if (meshInitializeBox(&mesh, -5.0 * tu, 5.0 * tu, -0.25 * tu, 0.25 * tu, -0.25 * tu, 0.25 * tu, world, space, trebuchetBodyDensity) != 0)
		return 999;
	meshGLInitialize(&supportBR_GL, &mesh, 3, attrDims, vaoNums);
	meshGLVAOInitialize(&supportBR_GL, 0, attrLocs);
	meshGLVAOInitialize(&supportBR_GL, 1, sdwProg.attrLocs);
	meshDestroy(&mesh);

	// bases: the beams that each connect the front and back supports for each side
	if (meshInitializeBox(&mesh, -7.1 * tu, 7.1 * tu, -0.25 * tu, 0.25 * tu, -0.25 * tu, 0.25 * tu, world, space, trebuchetBodyDensity) != 0)
		return 999;
	meshGLInitialize(&baseL_GL, &mesh, 3, attrDims, vaoNums);
	meshGLVAOInitialize(&baseL_GL, 0, attrLocs);
	meshGLVAOInitialize(&baseL_GL, 1, sdwProg.attrLocs);
	meshDestroy(&mesh);
	if (meshInitializeBox(&mesh, -7.1 * tu, 7.1 * tu, -0.25 * tu, 0.25 * tu, -0.25 * tu, 0.25 * tu, world, space, trebuchetBodyDensity) != 0)
		return 999;
	meshGLInitialize(&baseR_GL, &mesh, 3, attrDims, vaoNums);
	meshGLVAOInitialize(&baseR_GL, 0, attrLocs);
	meshGLVAOInitialize(&baseR_GL, 1, sdwProg.attrLocs);
	meshDestroy(&mesh);

	// arm: the beam that moves to throw stuff // length 20 for now
	if (meshInitializeBox(&mesh, -10 * tu, 10 * tu, -0.25 * tu, 0.25 * tu, -0.25 * tu, 0.25 * tu, world, space, trebuchetBodyDensity) != 0)
		return 999;
	meshGLInitialize(&arm_GL, &mesh, 3, attrDims, vaoNums);
	meshGLVAOInitialize(&arm_GL, 0, attrLocs);
	meshGLVAOInitialize(&arm_GL, 1, sdwProg.attrLocs);
	meshDestroy(&mesh);	

	// counterweight: either fixed or swinging from the arm
	if (meshInitializeBox(&mesh, -10 * tu, 10 * tu, -0.25 * tu, 0.25 * tu, -0.25 * tu, 0.25 * tu, world, space, trebuchetBodyDensity) != 0)
		return 999;
	meshGLInitialize(&counterweight_GL, &mesh, 3, attrDims, vaoNums);
	meshGLVAOInitialize(&counterweight_GL, 0, attrLocs);
	meshGLVAOInitialize(&counterweight_GL, 1, sdwProg.attrLocs);
	meshDestroy(&mesh);		
	// ======================= end trebuchet construction =========================
	
	// Ground
	int groundDensity = 200.0;
	if (meshInitializeBox(&mesh, -100.0, 100.0, -100.0, 100.0, -1.0, 1.0, world, space, groundDensity) != 0) {
		return 1;
	}
	meshGLInitialize(&ground_GL, &mesh, 3, attrDims, vaoNums);
	meshGLVAOInitialize(&ground_GL, 0, attrLocs);
	meshGLVAOInitialize(&ground_GL, 1, sdwProg.attrLocs);
	meshDestroy(&mesh);

	if (sceneInitialize(&counterweight_node, 3, 1, &counterweight_GL, NULL, NULL, world) != 0)
		return 1;	

	if (sceneInitialize(&arm_node, 3, 1, &arm_GL, NULL, &counterweight_node, world) != 0)
		return 1;	

	if (sceneInitialize(&baseR_node, 3, 1, &baseR_GL, NULL, &arm_node, world) != 0)
		return 1;

	if (sceneInitialize(&baseL_node, 3, 1, &baseL_GL, NULL, &baseR_node, world) != 0)
		return 1;	

	if (sceneInitialize(&supportBR_node, 3, 1, &supportBR_GL, NULL, &baseL_node, world) != 0)
		return 1;

	if (sceneInitialize(&supportBL_node, 3, 1, &supportBL_GL, NULL, &supportBR_node, world) != 0)
		return 1;

	if (sceneInitialize(&supportFR_node, 3, 1, &supportFR_GL, NULL, &supportBL_node, world) != 0)
		return 1;	

	if (sceneInitialize(&supportFL_node, 3, 1, &supportFL_GL, NULL, &supportFR_node, world) != 0)
		return 1;

	if (sceneInitialize(&cross_node, 3, 1, &cross_GL, NULL, &supportFL_node, world) != 0)
		return 1;

	if (sceneInitialize(&ground_node, 3, 1, &ground_GL, NULL, &cross_node, world) != 0)
		return 2;
	

    dBodySetKinematic(ground_node.meshGL->body);



	// GLdouble axis[3] = {0.0, 0.0, 0.0}; // rotate a thing about the z axis
	// GLdouble rot[3][3];
	// mat33AngleAxisRotation(M_PI / 2, axis, rot);
	// const dReal rotODE[9] = {rot[0][0], rot[0][1], rot[0][2],
	// 						 rot[1][0], rot[1][1], rot[1][2],
	// 						 rot[2][0], rot[2][1], rot[2][2]};




	dReal x,y,z;
	x = 0.0;
	y = 0.0;
	z = 0.0;
	dBodySetPosition(ground_node.meshGL->body, x, y, z);
	x = 0.0;
	y = 0.0;
	z = 10.0;
	dBodySetPosition(cross_node.meshGL->body, x, y, z);
	x = x + 25;
	y = y + 8;
	z = z + 0;
	dBodySetPosition(supportBL_node.meshGL->body, x, y, z);



	dBodySetRotation(cross_node.meshGL->body, rotODE);

	y = -200;
	y = y + 5;
	z = z + 5;
	dBodySetPosition(supportBR_node.meshGL->body, x, y, z);
	y = y + 5;
	z = z + 5;
	dBodySetPosition(supportFL_node.meshGL->body, x, y, z);
	y = y + 5;
	z = z + 5;
	dBodySetPosition(supportFR_node.meshGL->body, x, y, z);
	y = y + 5;
	z = z + 5;
	dBodySetPosition(baseL_node.meshGL->body, x, y, z);
	y = y + 5;
	z = z + 5;
	dBodySetPosition(baseR_node.meshGL->body, x, y, z);
	y = y + 5;
	z = z + 5;
	dBodySetPosition(arm_node.meshGL->body, x, y, z);
	y = y + 5;
	z = z + 5;
	dBodySetPosition(counterweight_node.meshGL->body, x, y, z);


	// dContact contact[max_contacts];
	// dContact contact2[max_contacts];
	// dJointID testjoint = dJointCreateFixed(world, trebuchetJointGroup);
	// dJointAttach(testjoint, supportBL_node.meshGL->body, cross_node.meshGL->body);
	// if (int numc = dCollide(cross_node.meshGL->geom, supportBL_node.meshGL->geom, max_contacts, &contact[0].geom, sizeof(dContact))) {
	// 	printf(" ====== %i\n", numc);
	// 	int i;
	// 	for (i = 0; i < numc; i ++) {

	// 		dJointID c = dJointCreateContact(world, trebuchetJointGroup, contact + i);
	// 		dJointSetFixed(c);
	//     	dJointAttach(c, cross_node.meshGL->body, supportBL_node.meshGL->body);
	    	
	//     }
	// }

	// if (int numc = dCollide(cross_node.meshGL->geom, supportFL_node.meshGL->geom, max_contacts, &contact[0].geom, sizeof(dContact))) {
	// 	int i;
	// 	printf("%i\n", numc);
	// 	for (i = 0; i < numc; i ++) {
	// 		dJointID c = dJointCreateContact(world, trebuchetJointGroup, contact2 + i);
	//     	dJointAttach(c, supportFL_node.meshGL->body, cross_node.meshGL->body);
	//     }
	// }

	texTexture *tex;
	tex = &texTrebuchet;
	sceneSetTexture(&cross_node, &tex);
	sceneSetTexture(&supportFL_node, &tex);
	sceneSetTexture(&supportFR_node, &tex);
	sceneSetTexture(&supportBL_node, &tex);
	sceneSetTexture(&supportBR_node, &tex);
	sceneSetTexture(&baseL_node, &tex);
	sceneSetTexture(&baseR_node, &tex);
	sceneSetTexture(&arm_node, &tex);
	sceneSetTexture(&counterweight_node, &tex);

	tex = &texH;
	sceneSetTexture(&ground_node, &tex);


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
	camSetControls(&cam, camPERSPECTIVE, M_PI / 6.0, 10.0, 768.0, 768.0, 300.0,
								 M_PI / 4.0, M_PI / 4.0, vec);
	lightSetType(&light, lightSPOT);
	vecSet(3, vec, 30.0, 30.0, 20.0);
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

        contact[i].surface.bounce = 0.3;

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
	shadowMapRender(&sdwMap, &sdwProg, &light, -100.0, -1.0);
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
	nodeUpdateTransRot(&cross_node);
	nodeUpdateTransRot(&supportFL_node);
	nodeUpdateTransRot(&supportFR_node);
	nodeUpdateTransRot(&supportBL_node);
	nodeUpdateTransRot(&supportBR_node);
	nodeUpdateTransRot(&baseL_node);
	nodeUpdateTransRot(&baseR_node);
	nodeUpdateTransRot(&arm_node);
	nodeUpdateTransRot(&counterweight_node);
	// nodeUpdateTransRot(&);
	// nodeUpdateTransRot(&);
	// nodeUpdateTransRot(&);
	// nodeUpdateTransRot(&);




}
void startODE(void){
	dInitODE2(0);
	world = dWorldCreate();
	// space = dHashSpaceCreate(0); // come back to this later maybe
	space = dSimpleSpaceCreate(0);
	contactgroup = dJointGroupCreate(0);
	trebuchetJointGroup = dJointGroupCreate(100);
	dWorldSetGravity(world, 0.0, 0.0, -9.81);
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
	
		// dWorldStep(world, 0.5);
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
