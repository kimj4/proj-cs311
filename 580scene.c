/*** Creation and destruction ***/
/* Feel free to read from this struct's members, but don't write to them except
through the accessor functions. */
typedef struct sceneNode sceneNode;
struct sceneNode {
	GLdouble rotation[3][3];
	GLdouble translation[3];
	GLuint unifDim;
	GLdouble *unif;
	meshGLMesh *meshGL;
	sceneNode *firstChild, *nextSibling;
  GLint texNum;
  texTexture **tex;
};

/* Initializes a sceneNode struct. The translation and rotation are initialized to trivial values. The user must remember to call sceneDestroy or
sceneDestroyRecursively when finished. Returns 0 if no error occurred. */
int sceneInitialize(sceneNode *node, GLuint unifDim, GLuint texNum,
      meshGLMesh *mesh, sceneNode *firstChild, sceneNode *nextSibling) {
  node->unif = (GLdouble *)malloc(unifDim * sizeof(GLdouble) +
      texNum * sizeof(texTexture *));
  if (node->unif == NULL)
      return 1;
  node->tex = (texTexture **)&(node->unif[unifDim]);
  node->texNum = texNum;
  mat33Identity(node->rotation);
	vecSet(3, node->translation, 0.0, 0.0, 0.0);
	node->unifDim = unifDim;
	node->meshGL = mesh;
	node->firstChild = firstChild;
	node->nextSibling = nextSibling;
	return 0;
}

/* Deallocates the resources backing this scene node. Does not destroy the
resources backing the mesh, etc. */
void sceneDestroy(sceneNode *node) {
	if (node->unif != NULL)
		free(node->unif);
	node->unif = NULL;
}



/*** Accessors ***/

/* Copies the unifDim-dimensional vector from unif into the node. */
void sceneSetUniform(sceneNode *node, double unif[]) {
	vecCopy(node->unifDim, unif, node->unif);
}

/* Sets one uniform in the node, based on its index in the unif array. */
void sceneSetOneUniform(sceneNode *node, int index, double unif) {
	node->unif[index] = unif;
}

/* Calls sceneDestroy recursively on the node's descendants and younger
siblings, and then on the node itself. */
void sceneDestroyRecursively(sceneNode *node) {
	if (node->firstChild != NULL)
		sceneDestroyRecursively(node->firstChild);
	if (node->nextSibling != NULL)
		sceneDestroyRecursively(node->nextSibling);
	sceneDestroy(node);
}

/* Sets the node's rotation. */
void sceneSetRotation(sceneNode *node, GLdouble rot[3][3]) {
	vecCopy(9, (GLdouble *)rot, (GLdouble *)(node->rotation));
}

/* Sets the node's translation. */
void sceneSetTranslation(sceneNode *node, GLdouble transl[3]) {
	vecCopy(3, transl, node->translation);
}

/* Sets the scene's mesh. */
void sceneSetMesh(sceneNode *node, meshGLMesh *mesh) {
	node->meshGL = mesh;
}

/* Sets the node's first child. */
void sceneSetFirstChild(sceneNode *node, sceneNode *child) {
	node->firstChild = child;
}

/* Sets the node's next sibling. */
void sceneSetNextSibling(sceneNode *node, sceneNode *sibling) {
	node->nextSibling = sibling;
}

/* Adds a sibling to the given node. The sibling shows up as the youngest of
its siblings. */
void sceneAddSibling(sceneNode *node, sceneNode *sibling) {
	if (node->nextSibling == NULL)
		node->nextSibling = sibling;
	else
		sceneAddSibling(node->nextSibling, sibling);
}

/* Adds a child to the given node. The child shows up as the youngest of its
siblings. */
void sceneAddChild(sceneNode *node, sceneNode *child) {
	if (node->firstChild == NULL)
		node->firstChild = child;
	else
		sceneAddSibling(node->firstChild, child);
}

/* Removes a sibling from the given node. Equality of nodes is assessed as
equality of pointers. If the sibling is not present, then has no effect (fails
silently). */
void sceneRemoveSibling(sceneNode *node, sceneNode *sibling) {
	if (node->nextSibling == NULL)
		return;
	else if (node->nextSibling == sibling)
		node->nextSibling = sibling->nextSibling;
	else
		sceneRemoveSibling(node->nextSibling, sibling);
}

/* Removes a child from the given node. Equality of nodes is assessed as
equality of pointers. If the sibling is not present, then has no effect (fails
silently). */
void sceneRemoveChild(sceneNode *node, sceneNode *child) {
	if (node->firstChild == NULL)
		return;
	else if (node->firstChild == child)
		node->firstChild = child->nextSibling;
	else
		sceneRemoveSibling(node->firstChild, child);
}

/* Sets the list of textures to the given one*/
void sceneSetTexture(sceneNode *node, texTexture *tex[]) {
		// vecCopy(node->texNum, tex, node->tex);
		int i;
		for (i = 0; i < node->texNum; i++) {
			printf("here: %p\n", tex[i]);
			node->tex[i] = tex[i];
		}
}

/* Sets the index-th texture in the texture list to the given new texture*/
void sceneSetOneTexture(sceneNode *node, int index, texTexture *tex) {
    node->tex[index] = tex;
}
/*** OpenGL ***/



/* Renders the node, its younger siblings, and their descendants. parent is the
modeling matrix at the parent of the node. If the node has no parent, then this
matrix is the 4x4 identity matrix. Loads the modeling transformation into
modelingLoc. The attribute information exists to be passed to meshGLRender. The
uniform information is analogous, but sceneRender loads it, not meshGLRender. */
void sceneRender(sceneNode *node, GLdouble parent[4][4], GLint modelingLoc,
		GLuint unifNum, GLuint unifDims[], GLint unifLocs[], GLuint VAOindex, GLint *textureLocs) {
	int i;

	// render the textures (Josh says to assume max of 8 concurrent textures)
	for (i = 0; i < node->texNum; i++) {
		switch(i) {
			case(0): {
				texRender(node->tex[i], GL_TEXTURE0, i, textureLocs[i]);
				break;
			}
			case(1): {
				texRender(node->tex[i], GL_TEXTURE1, i, textureLocs[i]);
				break;
			}
			case(2): {
				texRender(node->tex[i], GL_TEXTURE2, i, textureLocs[i]);
				break;
			}
			case(3): {
				texRender(node->tex[i], GL_TEXTURE3, i, textureLocs[i]);
				break;
			}
			case(4): {
				texRender(node->tex[i], GL_TEXTURE4, i, textureLocs[i]);
				break;
			}
			case(5): {
				texRender(node->tex[i], GL_TEXTURE5, i, textureLocs[i]);
				break;
			}
			case(6): {
				texRender(node->tex[i], GL_TEXTURE6, i, textureLocs[i]);
				break;
			}
			case(7): {
				texRender(node->tex[i], GL_TEXTURE7, i, textureLocs[i]);
				break;
			}
			default: {
				printf("Scene Error: there are more than 8 textures.\n");
				break;
			}
		}
	}

	/* Set the uniform modeling matrix. */
	GLdouble selfIsom[4][4], parentMultiplied[4][4];
	GLfloat pmGL[4][4];
	mat44Isometry(node->rotation, node->translation, selfIsom);
	mat444Multiply(parent, selfIsom, parentMultiplied);
	mat44OpenGL(parentMultiplied, pmGL);
	glUniformMatrix4fv(modelingLoc, 1, GL_FALSE, (GLfloat *)pmGL);

	/* Set the other uniforms. */
	int curUnifIdx = 0;
	for (i = 0; i < unifNum; i++) {
		switch(unifDims[i]) {
			case(1): {
				GLfloat unif[1];
				vecOpenGL(1, &node->unif[curUnifIdx], unif);
				glUniform1fv(unifLocs[i], 1, unif);
				curUnifIdx += 1;
				break;
			}
			case(2): {
				GLfloat unif[2];
				vecOpenGL(2, &node->unif[curUnifIdx], unif);
				glUniform2fv(unifLocs[i], 1, unif);
				curUnifIdx += 2;
				break;
			}
			case(3): {
				GLfloat unif[3];
				vecOpenGL(3, &node->unif[curUnifIdx], unif);
				glUniform3fv(unifLocs[i], 1, unif);
				curUnifIdx += 3;
				break;
			}
			case(4): {
				GLfloat unif[4];
				vecOpenGL(4, &node->unif[curUnifIdx], unif);
				glUniform4fv(unifLocs[i], 1, unif);
				curUnifIdx += 4;
				break;
			}
			default: {
				printf("Scene Error: A uniform dimension exceeds 4\n");
				break;
			}
		}
	}

	/* Render the mesh */
	meshGLRender(node->meshGL, VAOindex);

	/* unrender all the textures that were previously rendered. (again up to 8)*/
	for (i = 0; i < node->texNum; i++) {
		switch(i) {
			case(0):{
				texUnrender(node->tex[i], GL_TEXTURE0);
				break;
			}
			case(1):{
				texUnrender(node->tex[i], GL_TEXTURE1);
				break;
			}
			case(2):{
				texUnrender(node->tex[i], GL_TEXTURE2);
				break;
			}
			case(3):{
				texUnrender(node->tex[i], GL_TEXTURE3);
				break;
			}
			case(4):{
				texUnrender(node->tex[i], GL_TEXTURE4);
				break;
			}
			case(5):{
				texUnrender(node->tex[i], GL_TEXTURE5);
				break;
			}
			case(6):{
				texUnrender(node->tex[i], GL_TEXTURE6);
				break;
			}
			case(7):{
				texUnrender(node->tex[i], GL_TEXTURE7);
				break;
			}
		}
	}

	// /* render child and sibling */
	if (node->firstChild != NULL) {
		sceneRender(node->firstChild, parentMultiplied, modelingLoc,
								unifNum, unifDims, unifLocs, VAOindex, textureLocs);
	}
	if (node->nextSibling != NULL) {
		sceneRender(node->nextSibling, parent, modelingLoc,
								unifNum, unifDims, unifLocs, VAOindex, textureLocs);
	}
}
