#pragma once
#include "../nclgl/OGLRenderer.h"
#include "../nclgl/Frustum.h"
#include <fstream>

class Camera;
class Mesh;
class MeshAnimation;
class MeshMaterial;
class HeightMap;
class SceneNode;
class Environment;

class Renderer : public OGLRenderer {
public:
	Renderer(Window& parent);
	~Renderer();

	void InitAssets();
	void InitShaders();
	void InitSceneNode();

	void RenderScene() override;
	void UpdateScene(float dt) override;

protected:
	void DrawSkybox();
	void DrawHeightmap();
	void DrawWater();
	void DrawShadowScene();
	void DrawMainScene();
	void DrawMap();
	void DrawPostProcess();
	void DisplayMap();
	void DrawMiniMap();
	void DrawCharacter();

	void FillBuffers();
	void DrawPointLights();
	void CombineBuffers();

	void GenerateScreenTexture(GLuint& into, bool depth = false);
	
	void BuildNodeLists(SceneNode* from);
	void SortNodeLists();
	void ClearNodeLists();
	void DrawNodes();
	void DrawNode(SceneNode* n);

	void toggleMap();
	void toggleMapMode();
	void toggleMode();

	Light* light;
	Light* pointLights;
	Camera* MainCamera;
	Camera* MapCamera;

	//Shaders
	Shader* lightShader;
	Shader* reflectShader;
	Shader* skyboxShader;
	Shader* shadowShader;
	Shader* mapShader;
	Shader* processShader;
	Shader* sceneShader;
	Shader* pointlightShader;
	Shader* combineShader;
	Shader* animationShader;

	//SceneNode
	SceneNode* root;
	Frustum frameFrustum;
	vector<SceneNode*> transparentNodeList;
	vector<SceneNode*> nodeList;

	//Mesh
	HeightMap* heightMap;
	Mesh* quad;
	Mesh* sphere;
	Mesh* rockMesh;
	Mesh* treeMesh;
	Mesh* characterMesh;
	MeshAnimation* anim;
	MeshMaterial* material;
	vector<GLuint> matTextures;

	//Deferred Shading
	GLuint bufferFBO;
	GLuint bufferColourTex;
	GLuint bufferNormalTex;
	GLuint bufferDepthTex;
	GLuint pointLightFBO;
	GLuint lightDiffuseTex;
	GLuint lightSpecularTex;

	//PostProcess
	GLuint postbufferFBO;
	GLuint processFBO;
	GLuint postbufferColourTex[2];
	GLuint postbufferDepthTex;

	//Shadow
	GLuint shadowTex;
	GLuint shadowFBO;
	bool drawShadow;

	//Texture & Bump
	GLuint cubeMap;
	GLuint waterTex;
	GLuint grassTex;
	GLuint grassBump;
	GLuint stoneTex;
	GLuint stoneBump;
	GLuint sandTex;
	GLuint sandBump;
	GLuint rockTex;
	GLuint rockBump;
	GLuint treeTex;
	GLuint treeBump;
	GLuint glassTex;

	//Temp Texture for SceneNode
	GLuint texture;
	GLuint bump;

	//Animation
	float waterRotate;
	float waterCycle;

	int currentFrame;
	float frameTime;

	bool showMiniMap = false;
	bool otherMap = false;
	bool otherMode = false;
};