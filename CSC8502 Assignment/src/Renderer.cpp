#include "Renderer.h"
#include "../nclgl/Light.h"
#include "../nclgl/HeightMap.h"
#include "../nclgl/Shader.h"
#include "../nclgl/Camera.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/MeshAnimation.h"
#include "../nclgl/MeshMaterial.h"

#define SHADOWSIZE 2048

const int POST_PASSES = 5;
const int LIGHT_NUM = 25;

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	InitAssets();
	InitShaders();
	InitSceneNode();

	Vector3 heightmapSize = heightMap->GetHeightmapSize();

	pointLights = new Light[LIGHT_NUM];
	for (int i = 0; i < LIGHT_NUM; ++i) {
		Light& l = pointLights[i];
		l.SetPosition(Vector3(rand() % (int)heightmapSize.x, 1000.0f, rand() % (int)heightmapSize.z));
		l.SetColour(Vector4(0.5f + (float)(rand() / (float)RAND_MAX), 0.5f + (float)(rand() / (float)RAND_MAX), 0.5f + (float)(rand() / (float)RAND_MAX), 1));
		l.SetRadius((250.0f + (rand() % 250)) * heightmapSize.x * heightmapSize.x);
	}

	glGenTextures(1, &shadowTex);
	glBindTexture(GL_TEXTURE_2D, shadowTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOWSIZE, SHADOWSIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &shadowFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex, 0);

	glGenTextures(1, &postbufferDepthTex);
	glBindTexture(GL_TEXTURE_2D, bufferDepthTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

	for (int i = 0; i < 2; ++i) {
		glGenTextures(1, &postbufferColourTex[i]);
		glBindTexture(GL_TEXTURE_2D, postbufferColourTex[i]);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}

	glGenFramebuffers(1, &postbufferFBO);
	glGenFramebuffers(1, &processFBO);

	glBindFramebuffer(GL_FRAMEBUFFER, postbufferFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, postbufferDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, postbufferDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, postbufferColourTex[0], 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE || !postbufferDepthTex || !postbufferColourTex[0]) {
		return;
	}

	glGenFramebuffers(1, &bufferFBO);
	glGenFramebuffers(1, &pointLightFBO);

	GLenum buffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };

	GenerateScreenTexture(bufferDepthTex, true);
	GenerateScreenTexture(bufferColourTex);
	GenerateScreenTexture(bufferNormalTex);
	GenerateScreenTexture(lightDiffuseTex);
	GenerateScreenTexture(lightSpecularTex);

	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, bufferNormalTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, bufferDepthTex, 0);
	glDrawBuffers(2, buffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, pointLightFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lightDiffuseTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, lightSpecularTex, 0);
	glDrawBuffers(2, buffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		return;
	}

	glDrawBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	MainCamera = new Camera(80.0f, 0.0f, heightmapSize * Vector3(0.4f, 8.0f, 0.5f));
	MapCamera = new Camera(-90.0f, 0.0f, heightmapSize * Vector3(0.5f, 30.0f, 0.5f));
	light = new Light(heightmapSize * Vector3(1.4f, 30.0f, 1.4f), Vector4(1, 1, 1, 1), heightmapSize.x * 10);

	projMatrix = Matrix4::Perspective(1.0f, 30000.0f, (float)width / (float)height, 45.0f);
	waterRotate = 0.0f;
	waterCycle = 0.0f;
	currentFrame = 0;
	frameTime = 0.0f;
	init = true;
}

Renderer::~Renderer(void) {
	delete quad;
	delete rockMesh;
	delete treeMesh;
	delete heightMap;
	delete characterMesh;
	delete anim;
	delete material;
	glDeleteTextures(1, &waterTex);
	glDeleteTextures(1, &grassTex);
	glDeleteTextures(1, &grassBump);
	glDeleteTextures(1, &stoneTex);
	glDeleteTextures(1, &stoneBump);
	glDeleteTextures(1, &sandTex);
	glDeleteTextures(1, &sandBump);
	glDeleteTextures(1, &rockTex);
	glDeleteTextures(1, &rockBump);
	glDeleteTextures(1, &treeTex);
	glDeleteTextures(1, &treeBump);
	glDeleteTextures(6, &cubeMap);
	delete reflectShader;
	delete skyboxShader;
	delete lightShader;
	delete shadowShader;
	delete animationShader;
	delete mapShader;
	delete processShader;
	delete root;
	delete MainCamera;
	delete MapCamera;
	delete light;
	glDeleteTextures(1, &shadowTex);
	glDeleteFramebuffers(1, &shadowFBO);
	glDeleteTextures(1, &bufferDepthTex);
	glDeleteTextures(1, &postbufferDepthTex);
	glDeleteTextures(2, postbufferColourTex);
	glDeleteFramebuffers(1, &bufferFBO);
	glDeleteFramebuffers(1, &processFBO);
}

void Renderer::GenerateScreenTexture(GLuint& into, bool depth) {
	glGenTextures(1, &into);
	glBindTexture(GL_TEXTURE_2D, into);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	GLuint format = depth ? GL_DEPTH_COMPONENT24 : GL_RGBA8;
	GLuint type = depth ? GL_DEPTH_COMPONENT : GL_RGBA;

	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, type, GL_UNSIGNED_BYTE, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::InitAssets() {
	quad = Mesh::GenerateQuad();
	sphere = Mesh::LoadFromMeshFile("Sphere.msh");
	rockMesh = Mesh::LoadFromMeshFile("Stone_01_Base.msh");
	treeMesh = Mesh::LoadFromMeshFile("palm_tree.msh");
	heightMap = new HeightMap(TEXTUREDIR"Island2.png");

	characterMesh = Mesh::LoadFromMeshFile("Role_T.msh");
	anim = new MeshAnimation("Role_T.anm");
	material = new MeshMaterial("Role_T.mat");

	for (int i = 0; i < characterMesh->GetSubMeshCount(); ++i) {
		const MeshMaterialEntry* matEntry = material->GetMaterialForLayer(i);

		const string* filename = nullptr;
		matEntry->GetEntry("Diffuse", &filename);
		string path = TEXTUREDIR + *filename;
		GLuint texID = SOIL_load_OGL_texture(path.c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y);
		matTextures.emplace_back(texID);
	}

	waterTex = SOIL_load_OGL_texture(TEXTUREDIR"water.TGA", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	grassTex = SOIL_load_OGL_texture(TEXTUREDIR"Grass3.jfif", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	grassBump = SOIL_load_OGL_texture(TEXTUREDIR"Grass3Bump.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	stoneTex = SOIL_load_OGL_texture(TEXTUREDIR"Stone2.jfif", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	stoneBump = SOIL_load_OGL_texture(TEXTUREDIR"Stone2Bump.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	sandTex = SOIL_load_OGL_texture(TEXTUREDIR"sand.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	sandBump = SOIL_load_OGL_texture(TEXTUREDIR"sandBump.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	rockTex = SOIL_load_OGL_texture(TEXTUREDIR"stone_01_albedo.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	rockBump = SOIL_load_OGL_texture(TEXTUREDIR"stone_01_BumpNormal.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	treeTex = SOIL_load_OGL_texture(TEXTUREDIR"treetex.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	treeBump = SOIL_load_OGL_texture(TEXTUREDIR"treenormal.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	glassTex = SOIL_load_OGL_texture(TEXTUREDIR"stainedglass.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	cubeMap = SOIL_load_OGL_cubemap(
		TEXTUREDIR"rusted_west.jpg", TEXTUREDIR"rusted_east.jpg",
		TEXTUREDIR"rusted_up.jpg", TEXTUREDIR"rusted_down.jpg",
		TEXTUREDIR"rusted_south.jpg", TEXTUREDIR"rusted_north.jpg",
		SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	if (!quad || !sphere || !rockMesh || !treeMesh || !heightMap || !waterTex || !grassTex || !grassBump || !stoneTex || !stoneBump || !sandTex || !sandBump || !rockTex || !rockBump || !treeTex || !treeBump || !cubeMap) {
		return;
	}

	SetTextureRepeating(grassTex, true);
	SetTextureRepeating(grassBump, true);
	SetTextureRepeating(stoneTex, true);
	SetTextureRepeating(stoneBump, true);
	SetTextureRepeating(sandTex, true);
	SetTextureRepeating(sandBump, true);
	SetTextureRepeating(waterTex, true);
}

void Renderer::InitShaders() {
	reflectShader = new Shader("reflectVertex.glsl", "reflectFragment.glsl");
	skyboxShader = new Shader("skyboxVertex.glsl", "skyboxFragment.glsl");
	lightShader = new Shader("PerPixelVertex.glsl", "PerPixelFragment.glsl");
	shadowShader = new Shader("shadowVert.glsl", "shadowFrag.glsl");
	mapShader = new Shader("TexturedVertex.glsl", "TexturedFragment.glsl");
	processShader = new Shader("TexturedVertex.glsl", "processfrag.glsl");
	sceneShader = new Shader("PerPixelVertex.glsl", "bufferFragment.glsl");
	pointlightShader = new Shader("pointlightvert.glsl", "pointlightfrag.glsl");
	combineShader = new Shader("combinevert.glsl", "combinefrag.glsl");
	animationShader = new Shader("SkinningVertex.glsl", "TexturedFragment.glsl");

	if (!reflectShader->LoadSuccess() || !skyboxShader->LoadSuccess() || !lightShader->LoadSuccess() || !shadowShader->LoadSuccess() || !sceneShader->LoadSuccess() || !pointlightShader->LoadSuccess()/*|| !mapShader->LoadSuccess() || !processShader->LoadSuccess()*/) {
		return;
	}
}

void Renderer::InitSceneNode() {
	root = new SceneNode();

	for (int i = 0; i < 5; i++) {
		SceneNode* s = new SceneNode();
		s->SetColour(Vector4(1.0f, 1.0f, 1.0f, 0.5f));
		s->SetTransform(Matrix4::Translation(Vector3(15000, 1600.0f + 200 * i, 4000.0f + 100.0f + 1000 * i)));
		s->SetModelScale(Vector3(500.0f, 500.0f, 500.0f));
		s->SetBoundingRadius(10000.0f);
		s->SetMesh(quad);
		s->SetTexture(glassTex);
		root->AddChild(s);
	}

	SceneNode* rock = new SceneNode();
	rock->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	rock->SetTransform(Matrix4::Translation(Vector3(10000, 1100.0f, 10000)) * Matrix4::Rotation(90, Vector3(0, 1, 0)));
	rock->SetModelScale(Vector3(10, 10, 10));
	rock->SetBoundingRadius(10000.0f);
	rock->SetMesh(rockMesh);
	rock->SetTexture(rockTex);
	rock->SetBump(rockBump);
	root->AddChild(rock);

	SceneNode* rock2 = new SceneNode();
	rock2->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	rock2->SetTransform(Matrix4::Translation(Vector3(10200, 1100.0f, 9800)));
	rock2->SetModelScale(Vector3(10, 10, 10));
	rock2->SetBoundingRadius(10000.0f);
	rock2->SetMesh(rockMesh);
	rock2->SetTexture(rockTex);
	rock2->SetBump(rockBump);
	root->AddChild(rock2);

	SceneNode* rock3 = new SceneNode();
	rock3->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	rock3->SetTransform(Matrix4::Translation(Vector3(9800, 1100.0f, 9800)));
	rock3->SetModelScale(Vector3(10, 10, 10));
	rock3->SetBoundingRadius(10000.0f);
	rock3->SetMesh(rockMesh);
	rock3->SetTexture(rockTex);
	rock3->SetBump(rockBump);
	root->AddChild(rock3);

	SceneNode* rock4 = new SceneNode();
	rock4->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	rock4->SetTransform(Matrix4::Translation(Vector3(10000, 1100.0f, 9600)) * Matrix4::Rotation(-90, Vector3(0, 1, 0)));
	rock4->SetModelScale(Vector3(10, 10, 10));
	rock4->SetBoundingRadius(10000.0f);
	rock4->SetMesh(rockMesh);
	rock4->SetTexture(rockTex);
	rock4->SetBump(rockBump);
	root->AddChild(rock4);

	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 8; j++) {
			SceneNode* tree = new SceneNode();
			tree->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
			tree->SetTransform(Matrix4::Translation(Vector3(7000 + i * 250, 1000.0f, 8500 + j * 150)));
			tree->SetModelScale(Vector3(1, 1, 1));
			tree->SetBoundingRadius(10000.0f);
			tree->SetMesh(treeMesh);
			tree->SetTexture(treeTex);
			tree->SetBump(treeBump);
			root->AddChild(tree);
		}
	}
}

void Renderer::UpdateScene(float dt) {
	if (Window::GetKeyboard()->KeyDown(KEYBOARD_M)) {
		toggleMap();
	}
	if (Window::GetKeyboard()->KeyDown(KEYBOARD_N)) {
		toggleMapMode();
	}
	if (Window::GetKeyboard()->KeyDown(KEYBOARD_B)) {
		toggleMode();
	}
	MainCamera->UpdateCamera(dt);
	MapCamera->SetPosition(Vector3(MainCamera->GetPosition().x, heightMap->GetHeightmapSize().y * 30.0f, MainCamera->GetPosition().z));
	viewMatrix = MainCamera->BuildViewMatrix();
	frameFrustum.FromMatrix(projMatrix * viewMatrix);
	waterRotate += dt * 0.5f;
	waterCycle += dt * 0.2f;
	frameTime -= dt;
	while (frameTime < 0.0f) {
		currentFrame = (currentFrame + 1) % anim->GetFrameCount();
		frameTime += 1.0f / anim->GetFrameRate();
	}
	root->Update(dt);
}

void Renderer::RenderScene() {
	BuildNodeLists(root);
	SortNodeLists();
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	DrawSkybox();
	DrawWater();
	if (!otherMode) {
		DrawShadowScene();
		DrawMainScene();
		
	}
	else {
		FillBuffers();
		DrawPointLights();
		CombineBuffers();
	}
	
	DrawCharacter();
	
	
	if (showMiniMap) {
		if (!otherMap) {
			DrawMiniMap();
		}
		else {
			DrawMap();
			DrawPostProcess();
			DisplayMap();
		}
	}
	ClearNodeLists();
}

void Renderer::FillBuffers() {
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	BindShader(sceneShader);
	
	glUniform3fv(glGetUniformLocation(sceneShader->GetProgram(), "cameraPos"), 1, (float*)&MainCamera->GetPosition());
	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "doHeightMap"), 1);

	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, grassTex);

	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "bumpTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, grassBump);

	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex2"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, stoneTex);

	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "bumpTex2"), 3);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, stoneBump);

	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex3"), 4);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, sandTex);

	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "bumpTex3"), 5);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, sandBump);

	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "shadowTex"), 6);
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, shadowTex);

	viewMatrix = MainCamera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(500.0f, 30000.0f, (float)width / (float)height, 45.0f);
	modelMatrix = Matrix4::Scale(Vector3(1, 15, 1));

	UpdateShaderMatrices();

	heightMap->Draw();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawPointLights() {
	glBindFramebuffer(GL_FRAMEBUFFER, pointLightFBO);
	BindShader(pointlightShader);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glBlendFunc(GL_ONE, GL_ONE);
	glCullFace(GL_FRONT);
	glDepthFunc(GL_ALWAYS);
	glDepthMask(GL_FALSE);

	glUniform1i(glGetUniformLocation(pointlightShader->GetProgram(), "depthTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferDepthTex);

	glUniform1i(glGetUniformLocation(pointlightShader->GetProgram(), "normTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bufferNormalTex);

	glUniform3fv(glGetUniformLocation(pointlightShader->GetProgram(), "cameraPos"), 1, (float*)&MainCamera->GetPosition());

	glUniform2f(glGetUniformLocation(pointlightShader->GetProgram(), "pixelSize"), 1.0f / width, 1.0f / height);

	Matrix4 invViewProj = (projMatrix * viewMatrix).Inverse();
	glUniformMatrix4fv(glGetUniformLocation(pointlightShader->GetProgram(), "inverseProjView"), 1, false, invViewProj.values);

	UpdateShaderMatrices();

	for (int i = 0; i < LIGHT_NUM; ++i) {
		Light& l = pointLights[i];
		SetShaderLight(l);
		sphere->Draw();
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glCullFace(GL_BACK);
	glDepthFunc(GL_LEQUAL);

	glDepthMask(GL_TRUE);

	glClearColor(0.2f, 0.2f, 0.2f, 1);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::CombineBuffers() {
	BindShader(combineShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();

	glUniform1i(glGetUniformLocation(combineShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex);

	glUniform1i(glGetUniformLocation(combineShader->GetProgram(), "diffuseLight"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, lightDiffuseTex);

	glUniform1i(glGetUniformLocation(combineShader->GetProgram(), "specularLight"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, lightSpecularTex);

	quad->Draw();
}

void Renderer::DrawSkybox() {
	glDepthMask(GL_FALSE);

	BindShader(skyboxShader);

	projMatrix = Matrix4::Perspective(1.0f, 30000.0f, (float)width / (float)height, 45.0f);
	viewMatrix = MainCamera->BuildViewMatrix();
	UpdateShaderMatrices();

	quad->Draw();

	glDepthMask(GL_TRUE);
}

void Renderer::DrawHeightmap() {
	if (drawShadow == false) {
		glUniform3fv(glGetUniformLocation(lightShader->GetProgram(), "cameraPos"), 1, (float*)&MainCamera->GetPosition());
		glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "doHeightMap"), 1);

		glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "diffuseTex"), 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, grassTex);

		glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "bumpTex"), 1);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, grassBump);

		glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "diffuseTex2"), 2);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, stoneTex);

		glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "bumpTex2"), 3);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, stoneBump);

		glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "diffuseTex3"), 4);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, sandTex);

		glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "bumpTex3"), 5);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, sandBump);

		glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "shadowTex"), 6);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, shadowTex);

		textureMatrix.ToIdentity();
	}

	modelMatrix = Matrix4::Scale(Vector3(1, 15, 1));
	UpdateShaderMatrices();

	heightMap->Draw();
}

void Renderer::DrawWater() {
	BindShader(reflectShader);
	glDisable(GL_CULL_FACE);
	glUniform3fv(glGetUniformLocation(reflectShader->GetProgram(), "cameraPos"), 1, (float*)& MainCamera->GetPosition());
	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, waterTex);
	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "cubeTex"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

	Vector3 hSize = heightMap->GetHeightmapSize();
	modelMatrix = Matrix4::Translation(hSize * 0.5f * Vector3(1, 7, 1)) * Matrix4::Scale(hSize * 0.5f * Vector3(1, 1, 1)) * Matrix4::Rotation(90, Vector3(1, 0, 0));
	textureMatrix = Matrix4::Translation(Vector3(waterCycle, 0.0f, waterCycle)) * Matrix4::Scale(Vector3(10, 10, 10)) * Matrix4::Rotation(waterRotate, Vector3(0, 0, 1));
	projMatrix = Matrix4::Perspective(1.0f, 30000.0f, (float)width / (float)height, 45.0f);
	UpdateShaderMatrices();

	quad->Draw();
	glEnable(GL_CULL_FACE);
}

void Renderer::DrawShadowScene() {
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);

	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, SHADOWSIZE, SHADOWSIZE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	BindShader(shadowShader);

	viewMatrix = Matrix4::BuildViewMatrix(light->GetPosition(), Vector3(0, 0, 0));
	projMatrix = Matrix4::Perspective(1.0f, 50000.0f, 1, 45.0f);
	shadowMatrix = projMatrix * viewMatrix;

	drawShadow = true;
	DrawHeightmap();
	DrawNodes();

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0, 0, width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawMainScene() {
	BindShader(lightShader);
	viewMatrix = MainCamera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 30000.0f, (float)width / (float)height, 45.0f);
	SetShaderLight(*light);
	
	drawShadow = false;
	DrawHeightmap();
	DrawNodes();
}

void Renderer::DrawMap() {
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	BindShader(mapShader);
	viewMatrix = MapCamera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 500000.0f, 1, 45.0f);
	modelMatrix = Matrix4::Scale(Vector3(1, 15, 1));
	UpdateShaderMatrices();
	
	glUniform3fv(glGetUniformLocation(mapShader->GetProgram(), "cameraPos"), 1, (float*)& MainCamera->GetPosition());

	glUniform1i(glGetUniformLocation(mapShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, grassTex);

	glUniform1i(glGetUniformLocation(mapShader->GetProgram(), "diffuseTex2"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, stoneTex);

	glUniform1i(glGetUniformLocation(mapShader->GetProgram(), "diffuseTex3"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, sandTex);

	heightMap->Draw();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawPostProcess() {
	glBindFramebuffer(GL_FRAMEBUFFER, processFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, postbufferColourTex[1], 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	BindShader(processShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();

	glDisable(GL_DEPTH_TEST);

	glUniform1i(glGetUniformLocation(processShader->GetProgram(), "sceneTex"), 0);
	glActiveTexture(GL_TEXTURE0);

	for (int i = 0; i < POST_PASSES; ++i) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, postbufferColourTex[1], 0);
		glUniform1i(glGetUniformLocation(processShader->GetProgram(), "isVertical"), 0);

		glBindTexture(GL_TEXTURE_2D, postbufferColourTex[0]);
		quad->Draw();

		glUniform1i(glGetUniformLocation(processShader->GetProgram(), "isVertical"), 1);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, postbufferColourTex[0], 0);
		glBindTexture(GL_TEXTURE_2D, postbufferColourTex[1]);
		quad->Draw();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);
}

void Renderer::DisplayMap() {
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glViewport(width - 250, height - 250, 250, 250);
	BindShader(mapShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, postbufferColourTex[0]);
	glUniform1i(glGetUniformLocation(mapShader->GetProgram(), "diffuseTex"), 0);
	quad->Draw();
	glViewport(0, 0, width, height);
	glEnable(GL_DEPTH_TEST);
}

void Renderer::DrawMiniMap() {
	glDisable(GL_DEPTH_TEST);
	glViewport(width - 250, height - 250, 250, 250);
	BindShader(lightShader);
	projMatrix = Matrix4::Perspective(1000.0f, 10000.0f, 1, 45.0f);
	SetShaderLight(*light);
	viewMatrix = MapCamera->BuildViewMatrix();
	DrawHeightmap();
	glViewport(0, 0, width, height);
	glEnable(GL_DEPTH_TEST);
}

void Renderer::DrawCharacter() {
	BindShader(animationShader);
	glUniform1i(glGetUniformLocation(animationShader->GetProgram(), "diffuseTex"), 0);
	modelMatrix = Matrix4::Translation(Vector3(10000, 1150.0f, 9800)) * Matrix4::Scale(Vector3(200, 200, 200));
	UpdateShaderMatrices();

	vector<Matrix4> frameMatrices;

	const Matrix4* invBindPose = characterMesh->GetInverseBindPose();
	const Matrix4* frameData = anim->GetJointData(currentFrame);

	for (unsigned int i = 0; i < characterMesh->GetJointCount(); ++i) {
		frameMatrices.emplace_back(frameData[i] * invBindPose[i]);
	}

	int j = glGetUniformLocation(animationShader->GetProgram(), "joints");
	glUniformMatrix4fv(j, frameMatrices.size(), false, (float*)frameMatrices.data());

	for (int i = 0; i < characterMesh->GetSubMeshCount(); ++i) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, matTextures[i]);
		characterMesh->DrawSubMesh(i);
	}
}

void Renderer::BuildNodeLists(SceneNode* from) {
	if (frameFrustum.InsideFrustum(*from)) {
		Vector3 dir = from->GetWorldTransform().GetPositionVector() - MainCamera->GetPosition();
		from->SetCameraDistance(Vector3::Dot(dir, dir));

		if (from->GetColour().w < 1.0f) {
			transparentNodeList.push_back(from);
		}
		else {
			nodeList.push_back(from);
		}
	}

	for (vector<SceneNode*>::const_iterator i = from->GetChildIteratorStart(); i != from->GetChildIteratorEnd(); i++) {
		BuildNodeLists(*i);
	}
}

void Renderer::SortNodeLists() {
	std::sort(transparentNodeList.rbegin(), transparentNodeList.rend(), SceneNode::CompareByCameraDistance);
	std::sort(nodeList.begin(), nodeList.end(), SceneNode::CompareByCameraDistance);
}

void Renderer::DrawNodes() {
	for (const auto& i : nodeList) {
		DrawNode(i);
	}
	for (const auto& i : transparentNodeList) {
		glDisable(GL_CULL_FACE);
		DrawNode(i);
		glEnable(GL_CULL_FACE);
	}
}

void Renderer::DrawNode(SceneNode* n) {
	if (n->GetMesh()) {
	modelMatrix = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());

	if (drawShadow == false) {
		glUniform3fv(glGetUniformLocation(lightShader->GetProgram(), "cameraPos"), 1, (float*)&MainCamera->GetPosition());
		glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "doHeightMap"), 0);

		texture = n->GetTexture();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);

		bump = n->GetBump();
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, bump);

		glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "shadowTex"), 2);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, shadowTex);
	}
	UpdateShaderMatrices();
	n->Draw(*this);
	}
}

void Renderer::ClearNodeLists() {
	transparentNodeList.clear();
	nodeList.clear();
}

void Renderer::toggleMap() {
	showMiniMap = !showMiniMap;
}

void Renderer::toggleMapMode() {
	otherMap = !otherMap;
}

void Renderer::toggleMode() {
	otherMode = !otherMode;
}