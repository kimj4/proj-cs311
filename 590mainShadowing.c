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
static dReal density = 5.0;

#define max_contacts 20


// dContact contact[max_contacts];

// ==== scene globals ====
camCamera cam;
texTexture texH, texV, texTrebuchet;
meshGLODE meshGLODEL;
meshGLMesh meshGLL;
sceneNode nodeL;
shadowProgram sdwProg;

// trebuchet 
meshGLODE wheelFL_GLODE, wheelFR_GLODE, wheelBL_GLODE, wheelBR_GLODE,
		  axelF_GLODE, axelB_GLODE, baseL_GLODE, baseR_GLODE,
		  supportFL_GLODE, supportFR_GLODE, supportBL_GLODE, supportBR_GLODE,
		  cross_GLODE, arm_GLODE, counterweight_GLODE, bucket_GLODE;
meshGLMesh wheelFL_GL, wheelFR_GL, wheelBL_GL, wheelBR_GL,
		  axelF_GL, axelB_GL, baseL_GL, baseR_GL,
		  supportFL_GL, supportFR_GL, supportBL_GL, supportBR_GL,
		  cross_GL, arm_GL, counterweight_GL, bucket_GL;
sceneNode wheelFL_node, wheelFR_node, wheelBL_node, wheelBR_node,
		  axelF_node, axelB_node, baseL_node, baseR_node,
		  supportFL_node, supportFR_node, supportBL_node, supportBR_node,
		  cross_node, arm_node, counterweight_node, bucket_node;

// ground
meshGLODE ground_GLODE;
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
			camAddDistance(&cam, -0.5);
		else if (key == GLFW_KEY_J)
			camAddDistance(&cam, 5.0);
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

	meshMesh mesh;
	GLuint attrDims[3] = {3, 2, 3};
	int vaoNums = 2;


// 	// ======================= start trebuchet construction =======================
// 	// crossbar: the thing that holds the throwing arm
// 	if (meshInitializeBox(&mesh, -1.0, 1.0, -0.25, 0.25, -0.25, 0.25) != 0)
// 		return 999;
// 	meshGLInitialize(&cross_GL, &mesh, 3, attrDims, vaoNums);
// 	meshGLVAOInitialize(&cross_GL, 0, attrLocs);
// 	meshGLVAOInitialize(&cross_GL, 1, sdwProg.attrLocs);
// 	meshGLODEInitialize(&cross_GLODE, &cross_GL, &mesh, space);
// 	meshDestroy(&mesh);

// 	// supports: the four diagonally oriented beams that connect crossbar to the bases
// 	if (meshInitializeBox(&mesh, -5.0, 5.0, -0.25, 0.25, -0.25, 0.25) != 0)
// 		return 999;
// 	meshGLInitialize(&supportFL_GL, &mesh, 3, attrDims, vaoNums);
// 	meshGLVAOInitialize(&supportFL_GL, 0, attrLocs);
// 	meshGLVAOInitialize(&supportFL_GL, 1, sdwProg.attrLocs);
// 	meshGLODEInitialize(&supportFL_GLODE, &cross_GL, &mesh, space);
// 	meshDestroy(&mesh);
// 	if (meshInitializeBox(&mesh, -5.0, 5.0, -0.25, 0.25, -0.25, 0.25) != 0)
// 		return 999;
// 	meshGLInitialize(&supportFR_GL, &mesh, 3, attrDims, vaoNums);
// 	meshGLVAOInitialize(&supportFR_GL, 0, attrLocs);
// 	meshGLVAOInitialize(&supportFR_GL, 1, sdwProg.attrLocs);
// 	meshGLODEInitialize(&supportFR_GLODE, &cross_GL, &mesh, space);
// 	meshDestroy(&mesh);
// 	if (meshInitializeBox(&mesh, -5.0, 5.0, -0.25, 0.25, -0.25, 0.25) != 0)
// 		return 999;
// 	meshGLInitialize(&supportBL_GL, &mesh, 3, attrDims, vaoNums);
// 	meshGLVAOInitialize(&supportBL_GL, 0, attrLocs);
// 	meshGLVAOInitialize(&supportBL_GL, 1, sdwProg.attrLocs);
// 	meshGLODEInitialize(&supportBL_GLODE, &cross_GL, &mesh, space);
// 	meshDestroy(&mesh);
// 	if (meshInitializeBox(&mesh, -5.0, 5.0, -0.25, 0.25, -0.25, 0.25) != 0)
// 		return 999;
// 	meshGLInitialize(&supportBR_GL, &mesh, 3, attrDims, vaoNums);
// 	meshGLVAOInitialize(&supportBR_GL, 0, attrLocs);
// 	meshGLVAOInitialize(&supportBR_GL, 1, sdwProg.attrLocs);
// 	meshGLODEInitialize(&supportBR_GLODE, &cross_GL, &mesh, space);
// 	meshDestroy(&mesh);

// 	// bases: the beams that each connect the front and back supports for each side
// 	if (meshInitializeBox(&mesh, -7.1, 7.1, -0.25, 0.25, -0.25, 0.25) != 0)
// 		return 999;
// 	meshGLInitialize(&baseL_GL, &mesh, 3, attrDims, vaoNums);
// 	meshGLVAOInitialize(&baseL_GL, 0, attrLocs);
// 	meshGLVAOInitialize(&baseL_GL, 1, sdwProg.attrLocs);
// 	meshGLODEInitialize(&baseL_GLODE, &cross_GL, &mesh, space);
// 	meshDestroy(&mesh);
// 	if (meshInitializeBox(&mesh, -7.1, 7.1, -0.25, 0.25, -0.25, 0.25) != 0)
// 		return 999;
// 	meshGLInitialize(&baseR_GL, &mesh, 3, attrDims, vaoNums);
// 	meshGLVAOInitialize(&baseR_GL, 0, attrLocs);
// 	meshGLVAOInitialize(&baseR_GL, 1, sdwProg.attrLocs);
// 	meshGLODEInitialize(&baseR_GLODE, &cross_GL, &mesh, space);
// 	meshDestroy(&mesh);

// 	// arm: the beam that moves to throw stuff // length 20 for now
// 	if (meshInitializeBox(&mesh, -10, 10, -0.25, 0.25, -0.25, 0.25) != 0)
// 		return 999;
// 	meshGLInitialize(&arm_GL, &mesh, 3, attrDims, vaoNums);
// 	meshGLVAOInitialize(&arm_GL, 0, attrLocs);
// 	meshGLVAOInitialize(&arm_GL, 1, sdwProg.attrLocs);
// 	meshGLODEInitialize(&arm_GLODE, &cross_GL, &mesh, space);
// 	meshDestroy(&mesh);	

// 	// counterweight: either fixed or swinging from the arm
// 	if (meshInitializeBox(&mesh, -10, 10, -0.25, 0.25, -0.25, 0.25) != 0)
// 		return 999;
// 	meshGLInitialize(&counterweight_GL, &mesh, 3, attrDims, vaoNums);
// 	meshGLVAOInitialize(&counterweight_GL, 0, attrLocs);
// 	meshGLVAOInitialize(&counterweight_GL, 1, sdwProg.attrLocs);
// 	meshGLODEInitialize(&counterweight_GLODE, &cross_GL, &mesh, space);
// 	meshDestroy(&mesh);		
// 	// ======================= end trebuchet construction =========================
	
	// Ground
	if (meshInitializeBox(&mesh, -100.0, 100.0, -100.0, 100.0, -1.0, 1.0) != 0) {
		return 1;
	}
	meshGLInitialize(&ground_GL, &mesh, 3, attrDims, vaoNums);
	meshGLVAOInitialize(&ground_GL, 0, attrLocs);
	meshGLVAOInitialize(&ground_GL, 1, sdwProg.attrLocs);
	meshGLODEInitialize(&ground_GLODE, &ground_GL, &mesh, space);
	meshDestroy(&mesh);

	// test object
	if (meshInitializeBox(&mesh, -5, 5, -5, 5, -5, 5) != 0)
		return 11;
	meshGLInitialize(&meshGLL, &mesh, 3, attrDims, vaoNums);
	meshGLVAOInitialize(&meshGLL, 0, attrLocs);
	meshGLVAOInitialize(&meshGLL, 1, sdwProg.attrLocs);
	meshGLODEInitialize(&meshGLODEL, &meshGLL, &mesh, space);
	meshDestroy(&mesh);

	if (sceneInitialize(&nodeL, 3, 1, &meshGLODEL, NULL, NULL, world, 0) != 0)
		return 1;

	if (sceneInitialize(&ground_node, 3, 1, &ground_GLODE, NULL, &nodeL, world, 1) != 0)
		return 2;
	

	dReal x,y,z;
	// GLdouble trans[3] = {0.0, 0.0, 10.0};
	// sceneSetTranslation(&ground_node, trans); // tree trunk translation
	x = 0.0;
	y = 0.0;
	z = 10.0;
	dBodySetPosition(ground_node.body, x, y, z);



// 	vecSet(3, trans, 0.0, 0.0, 15.0);
// 	sceneSetTranslation(&nodeL, trans); // tree leaves translation
// 	// x = (dReal)trans[0];
// 	// y = (dReal)trans[1];
// 	// z = (dReal)trans[2];
	x = 0.0;
	y = 0.0;
	z = 100.0;
	dBodySetPosition(nodeL.body, x, y, z);
	// dBodySetTorque(nodeL.body, 100.0, 0.0, 0.0);
	dBodySetForce(nodeL.body, 20.0, 0.0, 0.0);

	texTexture *tex;
	tex = &texV;
	sceneSetTexture(&nodeL, &tex);
	tex = &texH;
	sceneSetTexture(&ground_node, &tex);


	printf("ground pointer: %p\n", ground_node.meshGLODE->geom);
	printf("box pointer: %p\n", nodeL.meshGLODE->geom);


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
  	GLdouble vec[3] = {120.0, 120.0, 120.0};
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




//Collision Detection from the manual
//May need dGeomID node.meshGLODE.geom
static void nearCallback (void *data, dGeomID o1, dGeomID o2) {
	// printf("%p\n", o1);
	// printf("%p\n", o2);
	dBodyID b1 = dGeomGetBody(o1);
	dBodyID b2 = dGeomGetBody(o2);

	dContact contact[max_contacts];

	// colliding two non-space geoms, so generate contact points between o1 and o2
	int num_contact = dCollide(o1, o2, max_contacts, &contact[0].geom, sizeof(dContact));
	// printf("num_contact: %i\n", num_contact);
	int i;

	 for (i = 0; i < max_contacts; i++) {
		contact[i].surface.mode = dContactSoftCFM | dContactSoftERP | dContactBounce;
		// contact[i].surface.mode = dContactSoftERP | dContactBounce;
		// contact[i].surface.mode = dContactBounce;
		contact[i].surface.mu = dInfinity;
		contact[i].surface.soft_cfm = 1e-8;
		contact[i].surface.soft_erp = 1.0;
		contact[i].surface.bounce = 0.25;
		contact[i].surface.bounce_vel = 0.1;
	}

  //   if (num_contact > 0){
		// if ((o1 == ground) || (o2 == ground)) {
		// 	printf("1\n");
		// 	for(i = 0; i < num_contact; i++) {
		// 		// printf("there\n");
		// 		// contact[i].surface.mode = dContactSoftCFM | dContactSoftERP | dContactApprox1;
		// 		contact[i].surface.mode = dContactSoftERP | dContactApprox1;
		// 		contact[i].surface.mu = 0.5;
		// 		// contact[i].surface.soft_cfm = 1e-8;
		// 		contact[i].surface.soft_erp = 1.0;
		// 		contact[i].surface.bounce = 0.9;
		// 		contact[i].surface.bounce_vel = 0.9;
		// 		dJointID con = dJointCreateContact(world, contactgroup, &contact[i]);
		// 		dJointAttach(con, dGeomGetBody(contact[i].geom.g1), dGeomGetBody(contact[i].geom.g2));              
		// 	}
		// }
   //      else {
   //      	printf("here\n");
   //          for (i = 0; i < num_contact; i++) {
			// 	// contact[i].surface.mode = dContactSoftCFM | dContactSoftERP | dContactBounce;
			// 	// contact[i].surface.mode = dContactSoftERP | dContactBounce;
			// 	contact[i].surface.mode = dContactBounce;
			// 	contact[i].surface.mu = dInfinity;
			// 	// contact[i].surface.soft_cfm = 1e-8;
			// 	// contact[i].surface.soft_erp = 1.0;
			// 	contact[i].surface.bounce = 0.25;
			// 	contact[i].surface.bounce_vel = 0.1;
			// }
   //      }
  //       for (i = 0; i < num_contact; i++) {
		// 	dJointID con = dJointCreateContact(world, contactgroup, &contact[i]);
		// 	dJointAttach(con, dGeomGetBody(contact[i].geom.g1), dGeomGetBody(contact[i].geom.g2));
		// }
		for (i = 0; i < num_contact; i++) {
			dJointID con = dJointCreateContact(world, contactgroup, contact + i);
			dJointAttach(con, dGeomGetBody(contact[i].geom.g1), dGeomGetBody(contact[i].geom.g2));
		}
	// }
    return;
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
	


	const dReal *pos1 = dBodyGetPosition(nodeL.body);
	// const dReal *rot1 = dBodyGetRotation(nodeL.body);
   	nodeL.translation[0] = pos1[0];
   	nodeL.translation[1] = pos1[1];
	nodeL.translation[2] = pos1[2];

	const dReal *pos2 = dBodyGetPosition(ground_node.body);	
	ground_node.translation[0] = pos2[0];	
   	ground_node.translation[1] = pos2[1];
	ground_node.translation[2] = pos2[2];
    
//     // GLdouble posGL[3];
//     // vecCopy(3, pos1, posGL);
//     // sceneSetTranslation(&nodeL, posGL);
    
// //    sceneSetRotation(&nodeL, rot1);
//     printf("nodeL translations:\n");
//     printf("x = %f,y = %f, z = %f\n", nodeL.translation[0],nodeL.translation[1], nodeL.translation[2]);

	if (pos1[0] > 300 || pos1[0] < -300 || pos1[1] > 300 || pos1[1] < -300 || pos1[2] > 1000 || pos1[2] < -200) {
		printf("body out of bounds\n");
		exit(999);
	}
    // printf("body translations:\n");
    // printf("x = %f,y = %f, z = %f\n", pos1[0],pos1[1], pos1[2]);

    //getchar();
	//Derefernce rot1?
	// mat33Copy(rot1, nodeL.rotation);
	// nodeL.rotation = rot1;
	// nodeL.rotation[0][0] = rot1[0];
	// nodeL.rotation[0][1] = rot1[1];
	// nodeL.rotation[0][2] = rot1[2];
	// nodeL.rotation[1][0] = rot1[3];
	// nodeL.rotation[1][1] = rot1[4];
	// nodeL.rotation[1][2] = rot1[5];
	// nodeL.rotation[2][0] = rot1[6];
	// nodeL.rotation[2][1] = rot1[7];
	// nodeL.rotation[2][2] = rot1[8];

    // printf("\n");
    // printf("%f   %f   %f\n", rot1[0], rot1[1], rot1[2]);
    // printf("%f   %f   %f\n", rot1[3], rot1[4], rot1[6]);
    // printf("%f   %f   %f\n", rot1[7], rot1[8], rot1[9]);
    // printf("%f   %f   %f\n", rot1[10], rot1[11], rot1[12]);
    // printf("\n");

	// nodeL.rotation[0][0] = rot1[0];
	// nodeL.rotation[1][0] = rot1[1];
	// nodeL.rotation[2][0] = rot1[2];
	// nodeL.rotation[0][1] = rot1[3];
	// nodeL.rotation[1][1] = rot1[4];
	// nodeL.rotation[2][1] = rot1[5];
	// nodeL.rotation[0][2] = rot1[6];
	// nodeL.rotation[1][2] = rot1[7];
	// nodeL.rotation[2][2] = rot1[8];

	// const dReal *a = dBodyGetPosition(nodeL.body);
	//const dReal *b = dGeomGetPosition(nodeL.meshGLODE->geom);

	// printf("%f\n", a[2]);
	// printf("%f\n", b[2]);


}
void startODE(void){
	dInitODE2(0);
	world = dWorldCreate();
	// space = dHashSpaceCreate(0); // come back to this later maybe
	space = dSimpleSpaceCreate(0);
	contactgroup = dJointGroupCreate(0);
	dWorldSetGravity(world, 2.0, 0.0, -9.81);
	ground = dCreatePlane(space, 0.0, 0.0, 1.0, 0.0);
	
}


int main(void) {
	//moved ODE setup into funct
	startODE();
    //Getting a strange "Bad arguments in dBodySetMass" error
//    dMass m;
//    dMassSetZero(&m);
//    dMassSetBox(&m, density, 5, 5, 5);
//    dBodySetMass(nodeL.body, &m);
   
    
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



	// dBodySetKinematic(ground_node.body);






	while (glfwWindowShouldClose(window) == 0) {
		oldTime = newTime;
		newTime = getTime();
		if (floor(newTime) - floor(oldTime) >= 1.0)
		fprintf(stderr, "main: %f frames/sec\n", 1.0 / (newTime - oldTime));




		dSpaceCollide(space, 0, &nearCallback);
	
		// dWorldStep(world, 0.5);
		dWorldQuickStep(world, 0.05);

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
	dCloseODE();
	return 0;
}
