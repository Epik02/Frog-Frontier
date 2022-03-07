#include <Logging.h>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <filesystem>
#include <json.hpp>
#include <fstream>
#include <sstream>
#include <typeindex>
#include <optional>
#include <string>

// GLM math library
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <GLM/gtx/common.hpp> // for fmod (floating modulus)

// Graphics
#include "Graphics/IndexBuffer.h"
#include "Graphics/VertexBuffer.h"
#include "Graphics/VertexArrayObject.h"
#include "Graphics/Shader.h"
#include "Graphics/Texture2D.h"
#include "Graphics/VertexTypes.h"

// Utilities
#include "Utils/MeshBuilder.h"
#include "Utils/MeshFactory.h"
#include "Utils/ObjLoader.h"
#include "Utils/ImGuiHelper.h"
#include "Utils/ResourceManager/ResourceManager.h"
#include "Utils/FileHelpers.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/StringUtils.h"
#include "Utils/GlmDefines.h"

// Gameplay
#include "Gameplay/Material.h"
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"

// Components
#include "Gameplay/Components/IComponent.h"
#include "Gameplay/Components/Camera.h"
#include "Gameplay/Components/RotatingBehaviour.h"
#include "Gameplay/Components/JumpBehaviour.h"
#include "Gameplay/Components/RenderComponent.h"
#include "Gameplay/Components/MaterialSwapBehaviour.h"

// Physics
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Physics/Colliders/BoxCollider.h"
#include "Gameplay/Physics/Colliders/PlaneCollider.h"
#include "Gameplay/Physics/Colliders/SphereCollider.h"
#include "Gameplay/Physics/Colliders/ConvexMeshCollider.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Graphics/DebugDraw.h"
#include "Gameplay/Physics/CollisionRect.h"

#include "fmod.hpp"

//#define LOG_GL_NOTIFICATIONS

/*
	Handles debug messages from OpenGL
	https://www.khronos.org/opengl/wiki/Debug_Output#Message_Components
	@param source    Which part of OpenGL dispatched the message
	@param type      The type of message (ex: error, performance issues, deprecated behavior)
	@param id        The ID of the error or message (to distinguish between different types of errors, like nullref or index out of range)
	@param severity  The severity of the message (from High to Notification)
	@param length    The length of the message
	@param message   The human readable message from OpenGL
	@param userParam The pointer we set with glDebugMessageCallback (should be the game pointer)
*/
void GlDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
	std::string sourceTxt;
	switch (source) {
		case GL_DEBUG_SOURCE_API: sourceTxt = "DEBUG"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM: sourceTxt = "WINDOW"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: sourceTxt = "SHADER"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY: sourceTxt = "THIRD PARTY"; break;
		case GL_DEBUG_SOURCE_APPLICATION: sourceTxt = "APP"; break;
		case GL_DEBUG_SOURCE_OTHER: default: sourceTxt = "OTHER"; break;
	}
	switch (severity) {
		case GL_DEBUG_SEVERITY_LOW:          LOG_INFO("[{}] {}", sourceTxt, message); break;
		case GL_DEBUG_SEVERITY_MEDIUM:       LOG_WARN("[{}] {}", sourceTxt, message); break;
		case GL_DEBUG_SEVERITY_HIGH:         LOG_ERROR("[{}] {}", sourceTxt, message); break;
			#ifdef LOG_GL_NOTIFICATIONS
		case GL_DEBUG_SEVERITY_NOTIFICATION: LOG_INFO("[{}] {}", sourceTxt, message); break;
			#endif
		default: break;
	}
}

// Stores our GLFW window in a global variable for now
GLFWwindow* window;
// The current size of our window in pixels
glm::ivec2 windowSize = glm::ivec2(800, 800);
// The title of our GLFW window
std::string windowTitle = "Frog Frontier";

std::vector<CollisionRect> collisions;
CollisionRect playerCollision;

// using namespace should generally be avoided, and if used, make sure it's ONLY in cpp files
using namespace Gameplay;
using namespace Gameplay::Physics;

// The scene that we will be rendering
Scene::Sptr scene = nullptr;

int SceneLoad(Scene::Sptr& scene, std::string& path)
{
	// Since it's a reference to a ptr, this will
		// overwrite the existing scene!
	scene = nullptr;
	scene = Scene::Load(path);
	std::cout << scene << std::endl << path << std::endl;



	return true;
}

void GlfwWindowResizedCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	windowSize = glm::ivec2(width, height);
	if (windowSize.x * windowSize.y > 0) {
		scene->MainCamera->ResizeWindow(width, height);
	}
}

/// <summary>
/// Handles intializing GLFW, should be called before initGLAD, but after Logger::Init()
/// Also handles creating the GLFW window
/// </summary>
/// <returns>True if GLFW was initialized, false if otherwise</returns>
bool initGLFW() {
	// Initialize GLFW
	if (glfwInit() == GLFW_FALSE) {
		LOG_ERROR("Failed to initialize GLFW");
		return false;
	}

	//Create a new GLFW window and make it current
	window = glfwCreateWindow(windowSize.x, windowSize.y, windowTitle.c_str(), nullptr, nullptr);
	glfwMakeContextCurrent(window);
	
	// Set our window resized callback
	glfwSetWindowSizeCallback(window, GlfwWindowResizedCallback);

	return true;
}

/// <summary>
/// Handles initializing GLAD and preparing our GLFW window for OpenGL calls
/// </summary>
/// <returns>True if GLAD is loaded, false if there was an error</returns>
bool initGLAD() {
	if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0) {
		LOG_ERROR("Failed to initialize Glad");
		return false;
	}
	return true;
}

int index = 1;
bool paused = false;
bool performedtask = false;
bool enterclick = false;
bool playerLose = false;
bool playerWin = false;

/// <summary>
/// Draws a widget for saving or loading our scene
/// </summary>
/// <param name="scene">Reference to scene pointer</param>
/// <param name="path">Reference to path string storage</param>
/// <returns>True if a new scene has been loaded</returns>
bool DrawSaveLoadImGui(Scene::Sptr& scene, std::string& path) {
	// Since we can change the internal capacity of an std::string,
	// we can do cool things like this!
	ImGui::InputText("Path", path.data(), path.capacity());

	// Draw a save button, and save when pressed
	if (ImGui::Button("Save")) {
		scene->Save(path);
	}
	ImGui::SameLine();
	// Load scene from file button
	if (ImGui::Button("Load")) {
		// Since it's a reference to a ptr, this will
		// overwrite the existing scene!
		SceneLoad(scene,path);

		return true;
	}

	

	if (glfwGetKey(window, GLFW_KEY_ENTER) ==GLFW_RELEASE)
	{
		enterclick = false;
	}

	if (glfwGetKey(window, GLFW_KEY_ENTER) && scene->FindObjectByName("player") == NULL && scene->FindObjectByName("Filter") != NULL && enterclick == false) {

		switch (index) {
		case 1:
			path = "Level.json";
			SceneLoad(scene, path);
			break;
		case 2:
			path = "CS.json";
			SceneLoad(scene, path);
			break;
		case 3:
			return 0;
			break;
		}

		enterclick = true;
		return true;
	}
	else if (glfwGetKey(window, GLFW_KEY_ENTER) && scene->FindObjectByName("player") == NULL && scene->FindObjectByName("Filter") == NULL && enterclick == false)
	{
		path = "menu.json";
		SceneLoad(scene, path);
		enterclick = true;
		return true;
	}

	if (glfwGetKey(window, GLFW_KEY_ENTER) && scene->FindObjectByName("player") != NULL && (paused == true || playerLose == true || playerWin == true)&& index == 2 && enterclick == false)
	{
		path = "menu.json";
		SceneLoad(scene, path);
		enterclick == true;
		return true;
	}

	return false;
}



/// <summary>
/// Draws some ImGui controls for the given light
/// </summary>
/// <param name="title">The title for the light's header</param>
/// <param name="light">The light to modify</param>
/// <returns>True if the parameters have changed, false if otherwise</returns>
bool DrawLightImGui(const Scene::Sptr& scene, const char* title, int ix) {
	bool isEdited = false;
	bool result = false;
	Light& light = scene->Lights[ix];
	ImGui::PushID(&light); // We can also use pointers as numbers for unique IDs
	if (ImGui::CollapsingHeader(title)) {
		isEdited |= ImGui::DragFloat3("Pos", &light.Position.x, 0.01f);
		isEdited |= ImGui::ColorEdit3("Col", &light.Color.r);
		isEdited |= ImGui::DragFloat("Range", &light.Range, 0.1f);

		result = ImGui::Button("Delete");
	}
	if (isEdited) {
		scene->SetShaderLight(ix);
	}

	ImGui::PopID();
	return result;
}

GameObject::Sptr createBackgroundAsset(std::string num, glm::vec3 position, float scale, glm::vec3 rotation, MeshResource::Sptr Mesh, Material::Sptr& Material1) {
	GameObject::Sptr nextObstacle = scene->CreateGameObject("BackgroundAsset" + num);
	{
		// Set and rotation position in the scene
		nextObstacle->SetPostion(glm::vec3(position.x, position.y, position.z)); //was -0.73 before
		nextObstacle->SetRotation(glm::vec3(rotation.x, rotation.y, rotation.z));
		nextObstacle->SetScale(glm::vec3(scale));

		// Add a render component
		RenderComponent::Sptr renderer = nextObstacle->Add<RenderComponent>();
		renderer->SetMesh(Mesh);
		renderer->SetMaterial(Material1);

		//collisions.push_back(CollisionRect(nextObstacle->GetPosition(), 1.0f, 2.0f, std::stoi(num))); //should change make it so the collisions are handled by invisible objects

		return nextObstacle;
	}
}

GameObject::Sptr createGroundObstacle(std::string num, glm::vec3 position, glm::vec3 scale, glm::vec3 rotation, MeshResource::Sptr Mesh, Material::Sptr& Material1) {

	MeshResource::Sptr cubeMesh = ResourceManager::CreateAsset<MeshResource>("Mushroom.obj");
	Texture2D::Sptr    boxTexture = ResourceManager::CreateAsset<Texture2D>("textures/MushroomUV.png");

	GameObject::Sptr nextObstacle = scene->CreateGameObject("Obstacle" + num);
	{
		// Set and rotation position in the scene
		nextObstacle->SetPostion(glm::vec3(position.x, position.y, position.z)); //was -0.73 before
		nextObstacle->SetRotation(glm::vec3(rotation.x, rotation.y, rotation.z));
		nextObstacle->SetScale(glm::vec3(scale.x, scale.y, scale.z));

		// Add a render component
		RenderComponent::Sptr renderer = nextObstacle->Add<RenderComponent>();
		renderer->SetMesh(Mesh);
		renderer->SetMaterial(Material1);

		//collisions.push_back(CollisionRect(nextObstacle->GetPosition(), 1.0f, 2.0f, std::stoi(num))); //should change make it so the collisions are handled by invisible objects

		return nextObstacle;
	}
}

GameObject::Sptr createCollision(std::string num, float x, float z, float xscale, float yscale) {

	MeshResource::Sptr cubeMesh = ResourceManager::CreateAsset<MeshResource>("cube.obj");
	Texture2D::Sptr    BlankTex = ResourceManager::CreateAsset<Texture2D>("textures/blank.png");

	Material::Sptr BlankMaterial = ResourceManager::CreateAsset<Material>();
	{
		BlankMaterial->Name = "Blank";
		BlankMaterial->MatShader = scene->BaseShader;
		BlankMaterial->Texture = BlankTex;
		BlankMaterial->Shininess = 2.0f;
	}
	GameObject::Sptr nextObstacle = scene->CreateGameObject("Collision" + num);
	{
		// Set and rotation position in the scene
		nextObstacle->SetPostion(glm::vec3(x, 0.0f, z)); //was -0.73 before
		nextObstacle->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));
		nextObstacle->SetScale(glm::vec3(xscale * 0.5f, yscale * 0.5f, 1.f));

		// Add a render component
		RenderComponent::Sptr renderer = nextObstacle->Add<RenderComponent>();
		renderer->SetMesh(cubeMesh);
		renderer->SetMaterial(BlankMaterial);

		collisions.push_back(CollisionRect(nextObstacle->GetPosition(), xscale, yscale, std::stoi(num)));

		return nextObstacle;
	}
}

bool isUpPressed = false;
bool isJumpPressed = false;
bool playerFlying = false;
bool playerMove = false;
int clickCount = 0;
bool playerJumping = false;
bool soundprompt = false;

//secoundary keyboard function for controls when on the menu or level select screen, possibly when paused?
// currently used for testing
void SceneChanger()
{
	if (glfwGetKey(window, GLFW_KEY_UP) && performedtask == false) {
		if (index - 1 < 1)
		{
			index = 3;
			std::cout << index << std::endl;
			performedtask = true;
		}
		else
		{
			index -= 1;
			std::cout << index << std::endl;
			performedtask = true;
		}
	}
	else if (glfwGetKey(window, GLFW_KEY_DOWN) && performedtask == false) {
		if (index + 1 > 3)
		{
			index = 1;
			std::cout << index << std::endl;
			performedtask = true;
		}
		else
		{
			index += 1;
			std::cout << index << std::endl;
			performedtask = true;
		}
	}
	
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_UP) == GLFW_RELEASE)
	{
		performedtask = false;
	}

	if (scene->FindObjectByName("Filter") != NULL)
	{
		if (index == 1)
		{
			scene->FindObjectByName("Filter")->SetPostion(glm::vec3(1.f, 0.5f, 3.01f));
		}
		else if (index == 2)
		{
			scene->FindObjectByName("Filter")->SetPostion(glm::vec3(1.f, 0.15f, 3.01f));
		}
		else if (index == 3)
		{
			scene->FindObjectByName("Filter")->SetPostion(glm::vec3(1.f, -0.2f, 3.01f));
		}
	}

}

void keyboard() 
{

	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && performedtask == false)
	{
		if (paused == true)
		{
			paused = false;
			
		}
		else if (paused == false)
		{
			paused = true;
		}

		performedtask = true;
	}

	if (paused == true || playerLose == true || playerWin == true)
	{
		if ((glfwGetKey(window, GLFW_KEY_UP) || glfwGetKey(window, GLFW_KEY_DOWN)) && performedtask == false) {
			if (index == 1)
			{
				index = 2;
				std::cout << index << std::endl;
				performedtask = true;
			}
			else if (index == 2)
			{
				index = 1;
				std::cout << index << std::endl;
				performedtask = true;
			}
			
		}

		if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS && index == 1)
		{
			if (paused == true)
			{
				paused = false;
			}

			if (playerLose == true)
			{
				playerLose = false;
				playerMove = true;
			}

			if (playerWin == true)
			{
				playerWin = false;
				playerMove = true;
			}
		}
	}

	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_UP) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_RELEASE)
	{
		performedtask = false;
	}
	
	if (paused == false)
	{
		//sliding
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
			scene->FindObjectByName("player")->SetScale(glm::vec3(0.5f, 0.25f, 0.5f));

		}
		else {
			scene->FindObjectByName("player")->SetScale(glm::vec3(0.5f, 0.5f, 0.5f));
		}

		if (clickCount % 2 == 0) {
			playerFlying = false;
		}
		//std::cout << clickCount;

		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
			playerMove = true;
		}

		if (playerMove == true) {
			scene->FindObjectByName("player")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 0.2f, scene->FindObjectByName("player")->GetPosition().y, scene->FindObjectByName("player")->GetPosition().z)); // makes the player move
		}

		if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
			if (!isUpPressed) {
				if (playerJumping == false) {
					//ballxpos = xpos;
				}
				playerJumping = true;
			}
			isJumpPressed = true;
		}
		else {
			isJumpPressed = false;
			playerJumping = false;
		}

		if (playerJumping == true) {
			if (scene->FindObjectByName("player")->GetPosition().z < 5.1) {
				//scene->FindObjectByName("player")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x, scene->FindObjectByName("player")->GetPosition().y, (scene->FindObjectByName("player")->GetPosition().z) + 1.0));
				scene->FindObjectByName("player")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x, scene->FindObjectByName("player")->GetPosition().y, scene->FindObjectByName("player")->GetPosition().z + 0.04));
				clickCount = clickCount + 1;
			}
			else {
				playerJumping = false;
				scene->FindObjectByName("player")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x, scene->FindObjectByName("player")->GetPosition().y, scene->FindObjectByName("player")->GetPosition().z - 0.04));
			}
			//else if (scene->FindObjectByName("player")->GetPosition().z >= 5) {
			//	scene->FindObjectByName("player")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x, scene->FindObjectByName("player")->GetPosition().y, 5.1));
			//}
		}
	}

	if (scene->FindObjectByName("player")->GetPosition().x < -400.f) {
		playerMove = false;
	}
}

glm::vec3 keypoints[4];
glm::vec3 currentPos;
int limit = 0;
float m_segmentTimer = 0.f;
float m_segmentTravelTime = 1.0f;
int m_segmentIndex = 0.f;
float timePassed;
float duration = 3.f;

glm::vec3 Lerp(glm::vec3 p0, glm::vec3 p1, float t) //may have messed up was templated function before
{
	return (1.0f - t) * p0 + t * p1;
}

void useLERP(float deltaTime, glm::vec3& LERPPos)
{
	timePassed = 0;
	float distanceCompleted = timePassed / duration;

	if (limit < 1)
	{
		//keypoint 1
		glm::vec3 go = glm::vec3(8.f, 0.f, 8.f);
		keypoints[0] = go;

		//keypoint 2
		glm::vec3 go2 = glm::vec3(2.f, 2.f, 2.f);
		keypoints[1] = go2;

		//keypoint 3
		glm::vec3 go3 = glm::vec3(6.f, 0.f, 6.f);
		keypoints[2] = go3;

		//keypoint 4
		glm::vec3 go4 = glm::vec3(4.f, 2.f, 4.f);
		keypoints[3] = go4;

		limit = limit + 1;
	}

	m_segmentTimer += deltaTime;

	while (m_segmentTimer > m_segmentTravelTime)
	{
		m_segmentTimer -= m_segmentTravelTime;
		m_segmentIndex += 1;

		if (m_segmentIndex >= keypoints->length())
		{
			m_segmentIndex = 0;
		}
	}
	float t = m_segmentTimer / m_segmentTravelTime;

	//LERP
	glm::vec3 p0, p1;
	int p1_index, p0_index;
	p1_index = m_segmentIndex;
	p0_index = (p1_index == 0) ? keypoints->length() - 1 : p1_index - 1;

	p0 = keypoints[p0_index];
	p1 = keypoints[p1_index];

	LERPPos = Lerp(p0, p1, t);
	//std::cout << keypoints->length() << "\n";

}


glm::vec3 useSEEK(glm::vec3 gameObject, glm::vec3 seekObject)//make pass by reference if messes up
{
	if (gameObject != seekObject) {
		if (gameObject.x < seekObject.x) {
			gameObject.x = gameObject.x + 0.01f;
		}
		else if (gameObject.x > seekObject.x) {
			gameObject.x = gameObject.x - 0.01f;
		}
		else if (gameObject.x == seekObject.x) {
			gameObject.x = gameObject.x;
		}

		if (gameObject.z < seekObject.z) {
			gameObject.z = gameObject.z + 0.01f;
		}
		else if (gameObject.z > seekObject.z) {
			gameObject.z = gameObject.z - 0.01f;
		}
		else if (gameObject.z == seekObject.z) {
			gameObject.z = gameObject.z;
		}
		return gameObject;
	}
}




int main() {
	Logger::Init(); // We'll borrow the logger from the toolkit, but we need to initialize it

	//Initialize GLFW
	if (!initGLFW())
		return 1;

	//Initialize GLAD
	if (!initGLAD())
		return 1;

	// Let OpenGL know that we want debug output, and route it to our handler function
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(GlDebugMessage, nullptr);

	// Initialize our ImGui helper
	ImGuiHelper::Init(window);

	// Initialize our resource manager
	ResourceManager::Init();

	// Register all our resource types so we can load them from manifest files
	ResourceManager::RegisterType<Texture2D>();
	ResourceManager::RegisterType<Material>();
	ResourceManager::RegisterType<MeshResource>();
	ResourceManager::RegisterType<Shader>();

	// Register all of our component types so we can load them from files
	ComponentManager::RegisterType<Camera>();
	ComponentManager::RegisterType<RenderComponent>();
	ComponentManager::RegisterType<RigidBody>();
	ComponentManager::RegisterType<TriggerVolume>();
	ComponentManager::RegisterType<RotatingBehaviour>();
	ComponentManager::RegisterType<JumpBehaviour>();
	ComponentManager::RegisterType<MaterialSwapBehaviour>();

	// GL states, we'll enable depth testing and backface fulling
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glCullFace(GL_BACK);
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

	FMOD::System* system;
	FMOD::Sound* sound1, * sound2, * sound3, *sound4;
	FMOD::Channel* channel = 0;
	FMOD_RESULT       result;
	void* extradriverdata = 0;
	
	result = FMOD::System_Create(&system);

	result = system->init(32, FMOD_INIT_NORMAL, extradriverdata);

	result = system->createSound("media/Click.wav", FMOD_DEFAULT, 0, &sound1);

	result = system->createSound("media/Pop1.wav", FMOD_DEFAULT, 0, &sound2);

	result = system->createSound("media/Pop2.wav", FMOD_DEFAULT, 0, &sound3);

	result = system->createSound("media/bensound-funnysong.wav", FMOD_LOOP_NORMAL, 0, &sound4);

	bool loadScene = false;
	// For now we can use a toggle to generate our scene vs load from file
	if (loadScene) {
		ResourceManager::LoadManifest("manifest.json");
		scene = Scene::Load("scene.json");
	} 
	else {
	
									/// Makes scene 1 ///
		{
// Create our OpenGL resources
			Shader::Sptr uboShader = ResourceManager::CreateAsset<Shader>(std::unordered_map<ShaderPartType, std::string>{
				{ ShaderPartType::Vertex, "shaders/vertex_shader.glsl" },
				{ ShaderPartType::Fragment, "shaders/frag_blinn_phong_textured.glsl" }
			});

			MeshResource::Sptr monkeyMesh = ResourceManager::CreateAsset<MeshResource>("Monkey.obj");
			Texture2D::Sptr    boxTexture = ResourceManager::CreateAsset<Texture2D>("textures/box-diffuse.png");
			Texture2D::Sptr    mushroomTexture = ResourceManager::CreateAsset<Texture2D>("textures/MushroomUV.png");
			Texture2D::Sptr    vinesTexture = ResourceManager::CreateAsset<Texture2D>("textures/VinesUV.png");
			Texture2D::Sptr    cobwebTexture = ResourceManager::CreateAsset<Texture2D>("textures/CobwebUV.png");
			Texture2D::Sptr    rockTexture = ResourceManager::CreateAsset<Texture2D>("textures/grey.png");
			Texture2D::Sptr    grassTexture = ResourceManager::CreateAsset<Texture2D>("textures/ground.png");
			Texture2D::Sptr    winTexture = ResourceManager::CreateAsset<Texture2D>("textures/win.png");
			Texture2D::Sptr    ladybugTexture = ResourceManager::CreateAsset<Texture2D>("textures/lbuv.png");
			Texture2D::Sptr    bgTexture = ResourceManager::CreateAsset<Texture2D>("textures/bg.png");
			Texture2D::Sptr    monkeyTex = ResourceManager::CreateAsset<Texture2D>("textures/monkey-uvMap.png");
			Texture2D::Sptr    greenTex = ResourceManager::CreateAsset<Texture2D>("textures/green.png");

			// Create an empty scene
			scene = std::make_shared<Scene>();

			// I hate this
			scene->BaseShader = uboShader;

			// Create our materials
			Material::Sptr boxMaterial = ResourceManager::CreateAsset<Material>();
			{
				boxMaterial->Name = "Box";
				boxMaterial->MatShader = scene->BaseShader;
				boxMaterial->Texture = boxTexture;
				boxMaterial->Shininess = 2.0f;
			}

			Material::Sptr greenMaterial = ResourceManager::CreateAsset<Material>();
			{
				greenMaterial->Name = "green";
				greenMaterial->MatShader = scene->BaseShader;
				greenMaterial->Texture = greenTex;
				greenMaterial->Shininess = 2.0f;
			}

			Material::Sptr bgMaterial = ResourceManager::CreateAsset<Material>();
			{
				bgMaterial->Name = "bg";
				bgMaterial->MatShader = scene->BaseShader;
				bgMaterial->Texture = bgTexture;
				bgMaterial->Shininess = 2.0f;
			}

			Material::Sptr grassMaterial = ResourceManager::CreateAsset<Material>();
			{
				grassMaterial->Name = "Grass";
				grassMaterial->MatShader = scene->BaseShader;
				grassMaterial->Texture = grassTexture;
				grassMaterial->Shininess = 2.0f;
			}

			Material::Sptr winMaterial = ResourceManager::CreateAsset<Material>();
			{
				winMaterial->Name = "win";
				winMaterial->MatShader = scene->BaseShader;
				winMaterial->Texture = winTexture;
				winMaterial->Shininess = 2.0f;
			}

			Material::Sptr ladybugMaterial = ResourceManager::CreateAsset<Material>();
			{
				ladybugMaterial->Name = "lbo";
				ladybugMaterial->MatShader = scene->BaseShader;
				ladybugMaterial->Texture = ladybugTexture;
				ladybugMaterial->Shininess = 2.0f;
			}

			Material::Sptr monkeyMaterial = ResourceManager::CreateAsset<Material>();
			{
				monkeyMaterial->Name = "Monkey";
				monkeyMaterial->MatShader = scene->BaseShader;
				monkeyMaterial->Texture = monkeyTex;
				monkeyMaterial->Shininess = 256.0f;

			}

			Material::Sptr mushroomMaterial = ResourceManager::CreateAsset<Material>();
			{
				mushroomMaterial->Name = "Mushroom";
				mushroomMaterial->MatShader = scene->BaseShader;
				mushroomMaterial->Texture = mushroomTexture;
				mushroomMaterial->Shininess = 256.0f;

			}

			Material::Sptr vinesMaterial = ResourceManager::CreateAsset<Material>();
			{
				vinesMaterial->Name = "vines";
				vinesMaterial->MatShader = scene->BaseShader;
				vinesMaterial->Texture = vinesTexture;
				vinesMaterial->Shininess = 256.0f;

			}

			Material::Sptr cobwebMaterial = ResourceManager::CreateAsset<Material>();
			{
				cobwebMaterial->Name = "cobweb";
				cobwebMaterial->MatShader = scene->BaseShader;
				cobwebMaterial->Texture = cobwebTexture;
				cobwebMaterial->Shininess = 256.0f;

			}

			// Create some lights for our scene
			scene->Lights.resize(5);
			scene->Lights[0].Position = glm::vec3(0.0f, 1.0f, 40.0f);
			scene->Lights[0].Color = glm::vec3(0.2f, 0.8f, 0.1f);
			scene->Lights[0].Range = 1000.0f;

			scene->Lights[1].Position = glm::vec3(1.0f, 0.0f, 3.0f);
			scene->Lights[1].Color = glm::vec3(0.2f, 0.8f, 0.1f);

			scene->Lights[2].Position = glm::vec3(0.0f, 1.0f, 3.0f);
			scene->Lights[2].Color = glm::vec3(1.0f, 0.2f, 0.1f);

			scene->Lights[3].Position = glm::vec3(-40.0f, 1.0f, 40.0f);
			scene->Lights[3].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[3].Range = 1000.0f;

			scene->Lights[4].Position = glm::vec3(-20.0f, 1.0f, 40.0f);
			scene->Lights[4].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[4].Range = 1000.0f;

			// We'll create a mesh that is a simple plane that we can resize later
			MeshResource::Sptr planeMesh = ResourceManager::CreateAsset<MeshResource>();
			MeshResource::Sptr cubeMesh = ResourceManager::CreateAsset<MeshResource>("cube.obj");
			MeshResource::Sptr mushroomMesh = ResourceManager::CreateAsset<MeshResource>("Mushroom.obj");
			MeshResource::Sptr vinesMesh = ResourceManager::CreateAsset<MeshResource>("Vines.obj");
			MeshResource::Sptr cobwebMesh = ResourceManager::CreateAsset<MeshResource>("Cobweb.obj");
			MeshResource::Sptr ladybugMesh = ResourceManager::CreateAsset<MeshResource>("lbo.obj"); //change to lbo.obj
			planeMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(1.0f)));
			planeMesh->GenerateMesh();

			// Set up the scene's camera
			GameObject::Sptr camera = scene->CreateGameObject("Main Camera");
			{
				camera->SetPostion(glm::vec3(0, 6.8, 2));
				camera->SetRotation(glm::vec3(90, 0, -180));
				camera->SetScale(glm::vec3(0.8f, 0.8f, 0.8f));
				camera->LookAt(glm::vec3(0.0f));

				Camera::Sptr cam = camera->Add<Camera>();

				// Make sure that the camera is set as the scene's main camera!
				scene->MainCamera = cam;
			}

			GameObject::Sptr plane5 = scene->CreateGameObject("plane5");
			{
				// Scale up the plane
				plane5->SetPostion(glm::vec3(-48.f, 0.f, -7.f));
				plane5->SetScale(glm::vec3(50.0F));

				// Create and attach a RenderComponent to the object to draw our mesh
				RenderComponent::Sptr renderer = plane5->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(winMaterial);

				// Attach a plane collider that extends infinitely along the X/Y axis
				RigidBody::Sptr physics = plane5->Add<RigidBody>(/*static by default*/);
				physics->AddCollider(PlaneCollider::Create());
			}

			GameObject::Sptr player = scene->CreateGameObject("player");
			{
				// Set position in the scene
				player->SetPostion(glm::vec3(6.f, 0.0f, 1.0f));
				player->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
				player->SetScale(glm::vec3(0.5f, 0.5f, 0.5f));

				// Add some behaviour that relies on the physics body
				player->Add<JumpBehaviour>(player->GetPosition());
				//player->Get<JumpBehaviour>(player->GetPosition());
				// Create and attach a renderer for the monkey
				RenderComponent::Sptr renderer = player->Add<RenderComponent>();
				renderer->SetMesh(ladybugMesh);
				renderer->SetMaterial(ladybugMaterial);

				collisions.push_back(CollisionRect(player->GetPosition(), 1.0f, 1.0f, 0));

				// Add a dynamic rigid body to this monkey
				RigidBody::Sptr physics = player->Add<RigidBody>(RigidBodyType::Dynamic);
				physics->AddCollider(ConvexMeshCollider::Create());


				// We'll add a behaviour that will interact with our trigger volumes
				MaterialSwapBehaviour::Sptr triggerInteraction = player->Add<MaterialSwapBehaviour>();
				triggerInteraction->EnterMaterial = boxMaterial;
				triggerInteraction->ExitMaterial = monkeyMaterial;
			}
			GameObject::Sptr jumpingObstacle = scene->CreateGameObject("Trigger2");
			{
				// Set and rotation position in the scene
				jumpingObstacle->SetPostion(glm::vec3(40.f, 0.0f, 1.0f));
				jumpingObstacle->SetRotation(glm::vec3(0.0f, 0.0f, 0.0f));
				jumpingObstacle->SetScale(glm::vec3(0.5f, 0.5f, 0.5f));

				// Add a render component
				RenderComponent::Sptr renderer = jumpingObstacle->Add<RenderComponent>();
				renderer->SetMesh(cubeMesh);
				renderer->SetMaterial(boxMaterial);

				collisions.push_back(CollisionRect(jumpingObstacle->GetPosition(), 1.0f, 1.0f, 1));

				//// This is an example of attaching a component and setting some parameters
				//RotatingBehaviour::Sptr behaviour = jumpingObstacle->Add<RotatingBehaviour>();
				//behaviour->RotationSpeed = glm::vec3(0.0f, 0.0f, -90.0f);
			}

			// Kinematic rigid bodies are those controlled by some outside controller
			// and ONLY collide with dynamic objects
			RigidBody::Sptr physics = jumpingObstacle->Add<RigidBody>(RigidBodyType::Kinematic);
			physics->AddCollider(ConvexMeshCollider::Create());

			//Obstacles
			createGroundObstacle("2", glm::vec3(-15.f, 0.0f, -0.660), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(90.f, 0.0f, 0.0f), mushroomMesh, mushroomMaterial);
			createGroundObstacle("3", glm::vec3(-37.930f, 0.0f, -0.750), glm::vec3(1.2f, 1.2f, 1.2f), glm::vec3(90.f, 0.0f, 73.f), vinesMesh, vinesMaterial);
			createGroundObstacle("4", glm::vec3(-55.970f, 0.0f, 1.730f), glm::vec3(0.2f, 0.2f, 0.2f), glm::vec3(0.0f, 0.0f, -45.f), cobwebMesh, cobwebMaterial);

			//Collisions
			//createCollision("2", -14.660f, 1.560f, 1.f, 1.f);
			//createCollision("3", -15.410f, 1.560f, 1.f, 1.f);
			//createCollision("4", -14.970f, 1.860f, 1.f, 1.f);
			//createCollision("5", -15.190f, 0.450f, 1.f, 1.f);

			// Save the asset manifest for all the resources we just loaded
			ResourceManager::SaveManifest("manifest.json");
			// Save the scene to a JSON file
			scene->Save("scene.json");
		}

								/// Working Level ///

		{
		// Create our OpenGL resources
		Shader::Sptr uboShader = ResourceManager::CreateAsset<Shader>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shader.glsl" },
			{ ShaderPartType::Fragment, "shaders/frag_blinn_phong_textured.glsl" }
		});

		MeshResource::Sptr monkeyMesh = ResourceManager::CreateAsset<MeshResource>("Monkey.obj");
		Texture2D::Sptr    boxTexture = ResourceManager::CreateAsset<Texture2D>("textures/box-diffuse.png");
		Texture2D::Sptr    mushroomTexture = ResourceManager::CreateAsset<Texture2D>("textures/MushroomUV.png");
		Texture2D::Sptr    vinesTexture = ResourceManager::CreateAsset<Texture2D>("textures/VinesUV.png");
		Texture2D::Sptr    cobwebTexture = ResourceManager::CreateAsset<Texture2D>("textures/CobwebUV.png");
		Texture2D::Sptr    rockTexture = ResourceManager::CreateAsset<Texture2D>("textures/grey.png");
		Texture2D::Sptr    grassTexture = ResourceManager::CreateAsset<Texture2D>("textures/ground.png");
		Texture2D::Sptr    winTexture = ResourceManager::CreateAsset<Texture2D>("textures/win.png");
		Texture2D::Sptr    ladybugTexture = ResourceManager::CreateAsset<Texture2D>("textures/lbuv.png");
		Texture2D::Sptr    bgTexture = ResourceManager::CreateAsset<Texture2D>("textures/bg.png");
		Texture2D::Sptr    monkeyTex = ResourceManager::CreateAsset<Texture2D>("textures/monkey-uvMap.png");
		Texture2D::Sptr    greenTex = ResourceManager::CreateAsset<Texture2D>("textures/green.png");
		Texture2D::Sptr    PanelTex = ResourceManager::CreateAsset<Texture2D>("textures/Panel.png");
		Texture2D::Sptr    ResumeTex = ResourceManager::CreateAsset<Texture2D>("textures/ResumeText.png");
		Texture2D::Sptr    MainMenuTex = ResourceManager::CreateAsset<Texture2D>("textures/MainMenuText.png");
		Texture2D::Sptr    ButtonTex = ResourceManager::CreateAsset<Texture2D>("textures/Button Background.png");
		Texture2D::Sptr    PauseTex = ResourceManager::CreateAsset<Texture2D>("textures/Pause.png");
		Texture2D::Sptr    FilterTex = ResourceManager::CreateAsset<Texture2D>("textures/Button Filter.png");
		Texture2D::Sptr    WinnerTex = ResourceManager::CreateAsset<Texture2D>("textures/Winner.png");
		Texture2D::Sptr    LoserTex = ResourceManager::CreateAsset<Texture2D>("textures/YouLose.png");
		Texture2D::Sptr    ReplayTex = ResourceManager::CreateAsset<Texture2D>("textures/ReplayText.png");
		Texture2D::Sptr    BranchTex = ResourceManager::CreateAsset<Texture2D>("textures/BranchUV.png");
		Texture2D::Sptr    LogTex = ResourceManager::CreateAsset<Texture2D>("textures/logUV.png");
		Texture2D::Sptr    PlantTex = ResourceManager::CreateAsset<Texture2D>("textures/Plantuv.png");
		Texture2D::Sptr    SunflowerTex = ResourceManager::CreateAsset<Texture2D>("textures/SunflowerUV.png");
		Texture2D::Sptr    twigTex = ResourceManager::CreateAsset<Texture2D>("textures/TwigUV.png");
		Texture2D::Sptr    ToadTex = ResourceManager::CreateAsset<Texture2D>("textures/Toad.png");
		Texture2D::Sptr    BlankTex = ResourceManager::CreateAsset<Texture2D>("textures/blank.png");
		Texture2D::Sptr    ForegroundTex = ResourceManager::CreateAsset<Texture2D>("textures/fg1.png");
		Texture2D::Sptr    RockTex = ResourceManager::CreateAsset<Texture2D>("textures/rockTexture.png");
		Texture2D::Sptr    frogTex = ResourceManager::CreateAsset<Texture2D>("textures/froguv.png");
		Texture2D::Sptr    tmTex = ResourceManager::CreateAsset<Texture2D>("textures/tmuv.png");
		Texture2D::Sptr    bmTex = ResourceManager::CreateAsset<Texture2D>("textures/bmuv.png");
		Texture2D::Sptr    PBTex = ResourceManager::CreateAsset<Texture2D>("textures/PauseButton.png");
		Texture2D::Sptr    BGTex = ResourceManager::CreateAsset<Texture2D>("textures/BackgroundUV.png");
		Texture2D::Sptr    BGGrassTex = ResourceManager::CreateAsset<Texture2D>("textures/grassuv.png");
		Texture2D::Sptr    ProgressTex = ResourceManager::CreateAsset<Texture2D>("textures/progressbar.png");
		Texture2D::Sptr    PbarbugTex = ResourceManager::CreateAsset<Texture2D>("textures/progressmeter.png");
		Texture2D::Sptr    Grass1Tex = ResourceManager::CreateAsset<Texture2D>("textures/Grass1.png");
		Texture2D::Sptr    Grass2Tex = ResourceManager::CreateAsset<Texture2D>("textures/Grass2.png");
		Texture2D::Sptr    Grass3Tex = ResourceManager::CreateAsset<Texture2D>("textures/Grass3.png");
		Texture2D::Sptr    Grass4Tex = ResourceManager::CreateAsset<Texture2D>("textures/Grass4.png");
		Texture2D::Sptr    Grass5Tex = ResourceManager::CreateAsset<Texture2D>("textures/Grass5.png");
		Texture2D::Sptr    ExitTreeTex = ResourceManager::CreateAsset<Texture2D>("textures/ExitTreeUV.png");

		// Create an empty scene
		scene = std::make_shared<Scene>();

		// I hate this
		scene->BaseShader = uboShader;
		
		// Create our materials
		Material::Sptr boxMaterial = ResourceManager::CreateAsset<Material>();
		{
			boxMaterial->Name = "Box";
			boxMaterial->MatShader = scene->BaseShader;
			boxMaterial->Texture = boxTexture;
			boxMaterial->Shininess = 2.0f;
		}

		Material::Sptr PbarbugMaterial = ResourceManager::CreateAsset<Material>();
		{
			PbarbugMaterial->Name = "minibug";
			PbarbugMaterial->MatShader = scene->BaseShader;
			PbarbugMaterial->Texture = PbarbugTex;
			PbarbugMaterial->Shininess = 2.0f;
		}

		Material::Sptr greenMaterial = ResourceManager::CreateAsset<Material>();
		{
			greenMaterial->Name = "green";
			greenMaterial->MatShader = scene->BaseShader;
			greenMaterial->Texture = greenTex;
			greenMaterial->Shininess = 2.0f;
		}

		Material::Sptr bgMaterial = ResourceManager::CreateAsset<Material>();
		{
			bgMaterial->Name = "bg";
			bgMaterial->MatShader = scene->BaseShader;
			bgMaterial->Texture = bgTexture;
			bgMaterial->Shininess = 2.0f;
		}

		Material::Sptr grassMaterial = ResourceManager::CreateAsset<Material>();
		{
			grassMaterial->Name = "Grass";
			grassMaterial->MatShader = scene->BaseShader;
			grassMaterial->Texture = grassTexture;
			grassMaterial->Shininess = 2.0f;
		}

		Material::Sptr winMaterial = ResourceManager::CreateAsset<Material>();
		{
			winMaterial->Name = "win";
			winMaterial->MatShader = scene->BaseShader;
			winMaterial->Texture = winTexture;
			winMaterial->Shininess = 2.0f;
		}

		Material::Sptr ladybugMaterial = ResourceManager::CreateAsset<Material>();
		{
			ladybugMaterial->Name = "lbo";
			ladybugMaterial->MatShader = scene->BaseShader;
			ladybugMaterial->Texture = ladybugTexture;
			ladybugMaterial->Shininess = 2.0f;
		}

		Material::Sptr monkeyMaterial = ResourceManager::CreateAsset<Material>();
		{
			monkeyMaterial->Name = "Monkey";
			monkeyMaterial->MatShader = scene->BaseShader;
			monkeyMaterial->Texture = monkeyTex;
			monkeyMaterial->Shininess = 256.0f;

		}

		Material::Sptr mushroomMaterial = ResourceManager::CreateAsset<Material>();
		{
			mushroomMaterial->Name = "Mushroom";
			mushroomMaterial->MatShader = scene->BaseShader;
			mushroomMaterial->Texture = mushroomTexture;
			mushroomMaterial->Shininess = 256.0f;

		}

		Material::Sptr vinesMaterial = ResourceManager::CreateAsset<Material>();
		{
			vinesMaterial->Name = "vines";
			vinesMaterial->MatShader = scene->BaseShader;
			vinesMaterial->Texture = vinesTexture;
			vinesMaterial->Shininess = 256.0f;

		}

		Material::Sptr cobwebMaterial = ResourceManager::CreateAsset<Material>();
		{
			cobwebMaterial->Name = "cobweb";
			cobwebMaterial->MatShader = scene->BaseShader;
			cobwebMaterial->Texture = cobwebTexture;
			cobwebMaterial->Shininess = 256.0f;

		}

		Material::Sptr PanelMaterial = ResourceManager::CreateAsset<Material>();
		{
			PanelMaterial->Name = "Panel";
			PanelMaterial->MatShader = scene->BaseShader;
			PanelMaterial->Texture = PanelTex;
			PanelMaterial->Shininess = 2.0f;
		}

		Material::Sptr ResumeMaterial = ResourceManager::CreateAsset<Material>();
		{
			ResumeMaterial->Name = "Resume";
			ResumeMaterial->MatShader = scene->BaseShader;
			ResumeMaterial->Texture = ResumeTex;
			ResumeMaterial->Shininess = 2.0f;
		}

		Material::Sptr MainMenuMaterial = ResourceManager::CreateAsset<Material>();
		{
			MainMenuMaterial->Name = "Main Menu";
			MainMenuMaterial->MatShader = scene->BaseShader;
			MainMenuMaterial->Texture = MainMenuTex;
			MainMenuMaterial->Shininess = 2.0f;
		}

		Material::Sptr PauseMaterial = ResourceManager::CreateAsset<Material>();
		{
			PauseMaterial->Name = "Pause";
			PauseMaterial->MatShader = scene->BaseShader;
			PauseMaterial->Texture = PauseTex;
			PauseMaterial->Shininess = 2.0f;
		}

		Material::Sptr ButtonMaterial = ResourceManager::CreateAsset<Material>();
		{
			ButtonMaterial->Name = "Button";
			ButtonMaterial->MatShader = scene->BaseShader;
			ButtonMaterial->Texture = ButtonTex;
			ButtonMaterial->Shininess = 2.0f;
		}

		Material::Sptr FilterMaterial = ResourceManager::CreateAsset<Material>();
		{
			FilterMaterial->Name = "Button Filter";
			FilterMaterial->MatShader = scene->BaseShader;
			FilterMaterial->Texture = FilterTex;
			FilterMaterial->Shininess = 2.0f;
		}

		Material::Sptr WinnerMaterial = ResourceManager::CreateAsset<Material>();
		{
			WinnerMaterial->Name = "WinnerLogo";
			WinnerMaterial->MatShader = scene->BaseShader;
			WinnerMaterial->Texture = WinnerTex;
			WinnerMaterial->Shininess = 2.0f;
		}

		Material::Sptr LoserMaterial = ResourceManager::CreateAsset<Material>();
		{
			LoserMaterial->Name = "LoserLogo";
			LoserMaterial->MatShader = scene->BaseShader;
			LoserMaterial->Texture = LoserTex;
			LoserMaterial->Shininess = 2.0f;
		}

		Material::Sptr ReplayMaterial = ResourceManager::CreateAsset<Material>();
		{
			ReplayMaterial->Name = "Replay Text";
			ReplayMaterial->MatShader = scene->BaseShader;
			ReplayMaterial->Texture = ReplayTex;
			ReplayMaterial->Shininess = 2.0f;
		}

		Material::Sptr BranchMaterial = ResourceManager::CreateAsset<Material>();
		{
			BranchMaterial->Name = "Branch";
			BranchMaterial->MatShader = scene->BaseShader;
			BranchMaterial->Texture = BranchTex;
			BranchMaterial->Shininess = 2.0f;
		}

		Material::Sptr LogMaterial = ResourceManager::CreateAsset<Material>();
		{
			LogMaterial->Name = "Log";
			LogMaterial->MatShader = scene->BaseShader;
			LogMaterial->Texture = LogTex;
			LogMaterial->Shininess = 2.0f;
		}

		Material::Sptr PlantMaterial = ResourceManager::CreateAsset<Material>();
		{
			PlantMaterial->Name = "Plant";
			PlantMaterial->MatShader = scene->BaseShader;
			PlantMaterial->Texture = PlantTex;
			PlantMaterial->Shininess = 2.0f;
		}

		Material::Sptr SunflowerMaterial = ResourceManager::CreateAsset<Material>();
		{
			SunflowerMaterial->Name = "Sunflower";
			SunflowerMaterial->MatShader = scene->BaseShader;
			SunflowerMaterial->Texture = SunflowerTex;
			SunflowerMaterial->Shininess = 2.0f;
		}

		Material::Sptr ToadMaterial = ResourceManager::CreateAsset<Material>();
		{
			ToadMaterial->Name = "Toad";
			ToadMaterial->MatShader = scene->BaseShader;
			ToadMaterial->Texture = ToadTex;
			ToadMaterial->Shininess = 2.0f;
		}
		Material::Sptr BlankMaterial = ResourceManager::CreateAsset<Material>();
		{
			BlankMaterial->Name = "Blank";
			BlankMaterial->MatShader = scene->BaseShader;
			BlankMaterial->Texture = BlankTex;
			BlankMaterial->Shininess = 2.0f;
		}
		Material::Sptr ForegroundMaterial = ResourceManager::CreateAsset<Material>();
		{
			ForegroundMaterial->Name = "Foreground";
			ForegroundMaterial->MatShader = scene->BaseShader;
			ForegroundMaterial->Texture = ForegroundTex;
			ForegroundMaterial->Shininess = 2.0f;
		}
		Material::Sptr rockMaterial = ResourceManager::CreateAsset<Material>();
		{
			rockMaterial->Name = "Rock";
			rockMaterial->MatShader = scene->BaseShader;
			rockMaterial->Texture = RockTex;
			rockMaterial->Shininess = 2.0f;
		}

		Material::Sptr twigMaterial = ResourceManager::CreateAsset<Material>();
		{
			twigMaterial->Name = "twig";
			twigMaterial->MatShader = scene->BaseShader;
			twigMaterial->Texture = twigTex;
			twigMaterial->Shininess = 2.0f;
		}
		Material::Sptr frogMaterial = ResourceManager::CreateAsset<Material>();
		{
			frogMaterial->Name = "frog";
			frogMaterial->MatShader = scene->BaseShader;
			frogMaterial->Texture = frogTex;
			frogMaterial->Shininess = 2.0f;
		}
		Material::Sptr tmMaterial = ResourceManager::CreateAsset<Material>();
		{
			tmMaterial->Name = "tallmushroom";
			tmMaterial->MatShader = scene->BaseShader;
			tmMaterial->Texture = tmTex;
			tmMaterial->Shininess = 2.0f;
		}
		Material::Sptr bmMaterial = ResourceManager::CreateAsset<Material>();
		{
			bmMaterial->Name = "branchmushroom";
			bmMaterial->MatShader = scene->BaseShader;
			bmMaterial->Texture = bmTex;
			bmMaterial->Shininess = 2.0f;
		}

		Material::Sptr PBMaterial = ResourceManager::CreateAsset<Material>();
		{
			PBMaterial->Name = "PauseButton";
			PBMaterial->MatShader = scene->BaseShader;
			PBMaterial->Texture = PBTex;
			PBMaterial->Shininess = 2.0f;
		}
		Material::Sptr BGMaterial = ResourceManager::CreateAsset<Material>();
		{
			BGMaterial->Name = "BG";
			BGMaterial->MatShader = scene->BaseShader;
			BGMaterial->Texture = BGTex;
			BGMaterial->Shininess = 2.0f;
		}
		Material::Sptr BGGrassMaterial = ResourceManager::CreateAsset<Material>();
		{
			BGGrassMaterial->Name = "BGGrass";
			BGGrassMaterial->MatShader = scene->BaseShader;
			BGGrassMaterial->Texture = BGGrassTex;
			BGGrassMaterial->Shininess = 2.0f;
		}
		Material::Sptr ProgressBarMaterial = ResourceManager::CreateAsset<Material>();
		{
			ProgressBarMaterial->Name = "ProgressBar";
			ProgressBarMaterial->MatShader = scene->BaseShader;
			ProgressBarMaterial->Texture = ProgressTex;
			ProgressBarMaterial->Shininess = 2.0f;
		}
		Material::Sptr grass1Material = ResourceManager::CreateAsset<Material>();
		{
			grass1Material->Name = "grass1";
			grass1Material->MatShader = scene->BaseShader;
			grass1Material->Texture = Grass1Tex;
			grass1Material->Shininess = 2.0f;
		}
		Material::Sptr grass2Material = ResourceManager::CreateAsset<Material>();
		{
			grass2Material->Name = "grass2";
			grass2Material->MatShader = scene->BaseShader;
			grass2Material->Texture = Grass2Tex;
			grass2Material->Shininess = 2.0f;
		}
		Material::Sptr grass3Material = ResourceManager::CreateAsset<Material>();
		{
			grass3Material->Name = "grass3";
			grass3Material->MatShader = scene->BaseShader;
			grass3Material->Texture = Grass3Tex;
			grass3Material->Shininess = 2.0f;
		}
		Material::Sptr grass4Material = ResourceManager::CreateAsset<Material>();
		{
			grass4Material->Name = "grass4";
			grass4Material->MatShader = scene->BaseShader;
			grass4Material->Texture = Grass4Tex;
			grass4Material->Shininess = 2.0f;
		}
		Material::Sptr grass5Material = ResourceManager::CreateAsset<Material>();
		{
			grass5Material->Name = "grass5";
			grass5Material->MatShader = scene->BaseShader;
			grass5Material->Texture = Grass5Tex;
			grass5Material->Shininess = 2.0f;
		}
		Material::Sptr ExitTreeMaterial = ResourceManager::CreateAsset<Material>();
		{
			ExitTreeMaterial->Name = "ExitTree";
			ExitTreeMaterial->MatShader = scene->BaseShader;
			ExitTreeMaterial->Texture = ExitTreeTex;
			ExitTreeMaterial->Shininess = 2.0f;
		}


		// Create some lights for our scene
		scene->Lights.resize(18);
		scene->Lights[0].Position = glm::vec3(0.0f, 1.0f, 40.0f);
		scene->Lights[0].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[0].Range = 1000.0f;

		scene->Lights[1].Position = glm::vec3(-50.f, 0.0f, 3.0f);
		scene->Lights[1].Color = glm::vec3(1.f, 1.f, 1.f);

		scene->Lights[2].Position = glm::vec3(-100.f, 1.0f, 3.0f);
		scene->Lights[2].Color = glm::vec3(1.f, 1.f, 1.f);

		scene->Lights[3].Position = glm::vec3(-150.0f, 1.0f, 40.0f);
		scene->Lights[3].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[3].Range = 1000.0f;

		//scene->Lights[4].Position = glm::vec3(-200.0f, 1.0f, 40.0f);
		//scene->Lights[4].Color = glm::vec3(1.f, 1.f, 1.f);
		//scene->Lights[4].Range = 1000.0f;

		//scene->Lights[5].Position = glm::vec3(-250.0f, 1.0f, 40.0f);
		//scene->Lights[5].Color = glm::vec3(1.f, 1.f, 1.f);
		//scene->Lights[5].Range = 1000.0f;

		//scene->Lights[6].Position = glm::vec3(-300.0f, 1.0f, 40.0f);
		//scene->Lights[6].Color = glm::vec3(1.f, 1.f, 1.f);
		//scene->Lights[6].Range = 1000.0f;

		//scene->Lights[7].Position = glm::vec3(-350.0f, 1.0f, 40.0f);
		//scene->Lights[7].Color = glm::vec3(1.f, 1.f, 1.f);
		//scene->Lights[7].Range = 1000.0f;

		//scene->Lights[8].Position = glm::vec3(-200.0f, 1.0f, 40.0f);
		//scene->Lights[8].Color = glm::vec3(1.f, 1.f, 1.f);
		//scene->Lights[8].Range = 1000.0f;

		scene->Lights[4].Position = glm::vec3(-240.0f, 1.0f, 40.0f);
		scene->Lights[4].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[4].Range = 1000.0f;

		scene->Lights[5].Position = glm::vec3(-280.0f, 1.0f, 40.0f);
		scene->Lights[5].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[5].Range = 1000.0f;

		scene->Lights[6].Position = glm::vec3(-320.0f, 1.0f, 40.0f);
		scene->Lights[6].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[6].Range = 1000.0f;

		scene->Lights[7].Position = glm::vec3(-360.0f, 1.0f, 40.0f);
		scene->Lights[7].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[7].Range = 1000.0f;

		scene->Lights[8].Position = glm::vec3(-400.0f, 1.0f, 40.0f);
		scene->Lights[8].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[8].Range = 1000.0f;

		scene->Lights[9].Position = glm::vec3(-200.0f, 1.0f, 40.0f);
		scene->Lights[9].Color = glm::vec3(0.2f, 0.8f, 0.1f);
		scene->Lights[9].Range = 1000.0f;

		scene->Lights[10].Position = glm::vec3(-201.0f, 0.0f, 3.0f);
		scene->Lights[10].Color = glm::vec3(0.2f, 0.8f, 0.1f);

		scene->Lights[11].Position = glm::vec3(-400.0f, 1.0f, 40.0f);
		scene->Lights[11].Color = glm::vec3(0.2f, 0.8f, 0.1f);
		scene->Lights[11].Range = 1000.0f;

		scene->Lights[12].Position = glm::vec3(-401.0f, 0.0f, 3.0f);
		scene->Lights[12].Color = glm::vec3(0.2f, 0.8f, 0.1f);

		// We'll create a mesh that is a simple plane that we can resize later
		MeshResource::Sptr planeMesh = ResourceManager::CreateAsset<MeshResource>();
		MeshResource::Sptr cubeMesh = ResourceManager::CreateAsset<MeshResource>("cube.obj");
		MeshResource::Sptr mushroomMesh = ResourceManager::CreateAsset<MeshResource>("Mushroom.obj");
		MeshResource::Sptr vinesMesh = ResourceManager::CreateAsset<MeshResource>("Vines.obj");
		MeshResource::Sptr cobwebMesh = ResourceManager::CreateAsset<MeshResource>("Cobweb.obj");
		MeshResource::Sptr ladybugMesh = ResourceManager::CreateAsset<MeshResource>("lbo.obj");

		MeshResource::Sptr BranchMesh = ResourceManager::CreateAsset<MeshResource>("Branch.obj");
		MeshResource::Sptr LogMesh = ResourceManager::CreateAsset<MeshResource>("Log.obj");
		MeshResource::Sptr Plant1Mesh = ResourceManager::CreateAsset<MeshResource>("PLANT.obj");
		MeshResource::Sptr Plant2Mesh = ResourceManager::CreateAsset<MeshResource>("Plant2.obj");
		MeshResource::Sptr Plant3Mesh = ResourceManager::CreateAsset<MeshResource>("Plant3.obj");

		MeshResource::Sptr SunflowerMesh = ResourceManager::CreateAsset<MeshResource>("Sunflower.obj");
		MeshResource::Sptr ToadMesh = ResourceManager::CreateAsset<MeshResource>("ToadStool.obj");
		MeshResource::Sptr Rock1Mesh = ResourceManager::CreateAsset<MeshResource>("Rock1.obj");
		MeshResource::Sptr Rock2Mesh = ResourceManager::CreateAsset<MeshResource>("Rock2.obj");
		MeshResource::Sptr Rock3Mesh = ResourceManager::CreateAsset<MeshResource>("Rock3.obj");
		MeshResource::Sptr twigMesh = ResourceManager::CreateAsset<MeshResource>("Twig.obj");
		MeshResource::Sptr frogMesh = ResourceManager::CreateAsset<MeshResource>("frog.obj");

		MeshResource::Sptr tmMesh = ResourceManager::CreateAsset<MeshResource>("tm.obj");
		MeshResource::Sptr bmMesh = ResourceManager::CreateAsset<MeshResource>("bm.obj");

		MeshResource::Sptr PBMesh = ResourceManager::CreateAsset<MeshResource>("PB.obj");
		MeshResource::Sptr BGMesh = ResourceManager::CreateAsset<MeshResource>("Background.obj");
		MeshResource::Sptr ExitTreeMesh = ResourceManager::CreateAsset<MeshResource>("ExitTree.obj");

		planeMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(1.0f)));
		planeMesh->GenerateMesh();

		GameObject::Sptr PauseLogo = scene->CreateGameObject("PauseLogo");
		{
			// Set position in the scene
			PauseLogo->SetPostion(glm::vec3(1.f, 5.75f, 8.f));
			// Scale down the plane
			PauseLogo->SetScale(glm::vec3(3.927f, 1.96f, 0.5f));
			//Rotate Logo
			PauseLogo->SetRotation(glm::vec3(80.f, 0.f, 180.f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = PauseLogo->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(PauseMaterial);
		}

		GameObject::Sptr WinnerLogo = scene->CreateGameObject("WinnerLogo");
		{
			// Set position in the scene
			WinnerLogo->SetPostion(glm::vec3(1.f, 5.75f, 8.f));
			// Scale down the plane
			WinnerLogo->SetScale(glm::vec3(3.927f, 1.96f, 0.5f));
			//Rotate Logo
			WinnerLogo->SetRotation(glm::vec3(80.f, 0.f, 180.f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = WinnerLogo->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(WinnerMaterial);
		}

		GameObject::Sptr LoserLogo = scene->CreateGameObject("LoserLogo");
		{
			// Set position in the scene
			LoserLogo->SetPostion(glm::vec3(1.f, 5.75f, 8.f));
			// Scale down the plane
			LoserLogo->SetScale(glm::vec3(3.927f, 1.96f, 0.5f));
			//Rotate Logo
			LoserLogo->SetRotation(glm::vec3(80.f, 0.f, 180.f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = LoserLogo->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(LoserMaterial);
		}

		//Background Assets
		createBackgroundAsset("1", glm::vec3(-1.f, 5.0f, -0.f), 0.5, glm::vec3(83.f, 5.0f, 0.0f), BranchMesh, BranchMaterial);
		createBackgroundAsset("2", glm::vec3(-200.f, 5.0f, -0.f), 0.5, glm::vec3(83.f, 5.0f, 0.0f), BranchMesh, BranchMaterial);

		createBackgroundAsset("3", glm::vec3(-186.f, 4.0f, -0.660), 3, glm::vec3(83.f, 5.0f, 0.0f), LogMesh, LogMaterial);
		createBackgroundAsset("4", glm::vec3(-20.f, 0.0f, -0.660), 0.5, glm::vec3(400.f, 5.0f, 0.0f), LogMesh, LogMaterial);

		createBackgroundAsset("5", glm::vec3(-54.5, 5.470f, -0.330), 6.0, glm::vec3(83.f, 5.0f, 0.0f), Plant1Mesh, PlantMaterial);
		createBackgroundAsset("6", glm::vec3(-140.5, 5.470f, -0.330), 6.0, glm::vec3(83.f, 5.0f, 0.0f), Plant2Mesh, PlantMaterial);
		createBackgroundAsset("7", glm::vec3(-240.5, 5.470f, -0.330), 6.0, glm::vec3(83.f, 5.0f, 0.0f), Plant3Mesh, PlantMaterial);

		createBackgroundAsset("8", glm::vec3(-20.f, 0.0f, -0.660), 6.0, glm::vec3(83.f, 5.0f, 0.0f), SunflowerMesh, SunflowerMaterial);

		createBackgroundAsset("9", glm::vec3(-13.f, 5.0f, -0.430), 0.5, glm::vec3(83.f, 5.0f, 90.0f), ToadMesh, ToadMaterial);
		createBackgroundAsset("10", glm::vec3(-90.f, 5.0f, -0.430), 0.5, glm::vec3(83.f, 5.0f, 90.0f), ToadMesh, ToadMaterial);
		createBackgroundAsset("11", glm::vec3(-330.f, 5.0f, -0.430), 0.5, glm::vec3(83.f, 5.0f, 90.0f), ToadMesh, ToadMaterial);

		createBackgroundAsset("12", glm::vec3(-20.f, 0.0f, -0.660), 0.5, glm::vec3(83.f, 5.0f, 90.0f), Rock1Mesh, boxMaterial);
		createBackgroundAsset("13", glm::vec3(-20.f, 0.0f, -0.660), 0.5, glm::vec3(83.f, 5.0f, 90.0f), Rock2Mesh, rockMaterial);
		createBackgroundAsset("14", glm::vec3(-20.f, 0.0f, -0.660), 0.5, glm::vec3(83.f, 5.0f, 90.0f), Rock3Mesh, rockMaterial);

		createBackgroundAsset("15", glm::vec3(-20.f, 6.0f, -0.660), 0.5, glm::vec3(83.f, 5.0f, 90.0f), twigMesh, twigMaterial);
		createBackgroundAsset("16", glm::vec3(13.060f, 1.530f, 0.550), 0.05, glm::vec3(83.f, -7.0f, 90.0f), frogMesh, frogMaterial);

		createBackgroundAsset("17", glm::vec3(7.5f, 4.6f, -1.8), 0.5, glm::vec3(83.f, -7.0f, 90.0f), bmMesh, bmMaterial);
		createBackgroundAsset("18", glm::vec3(-6.f, -12.660f, 0.400), 0.05, glm::vec3(83.f, -7.0f, 90.0f), tmMesh, tmMaterial);

		//Obstacles
		createGroundObstacle("2", glm::vec3(-20.f, 0.0f, -0.660), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(90.f, 0.0f, 0.0f), mushroomMesh, mushroomMaterial); //mushroom 1 (small jump)
		createGroundObstacle("3", glm::vec3(-60.f, 0.0f, 3.0), glm::vec3(1.f, 1.f, 1.f), glm::vec3(90.f, 0.0f, 73.f), vinesMesh, vinesMaterial); // vine 1 (jump blocking)
		createGroundObstacle("4", glm::vec3(-110.f, 0.0f, 3.3f), glm::vec3(0.25f, 0.25f, 0.25f), glm::vec3(0.0f, 0.0f, -75.f), cobwebMesh, cobwebMaterial); //cobweb 1 (tall jump)
		createGroundObstacle("5", glm::vec3(-45.f, 0.0f, -0.660), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(90.f, 0.0f, 0.0f), mushroomMesh, mushroomMaterial); //mushroom 2
		createGroundObstacle("6", glm::vec3(-150.f, 5.530f, 0.250f), glm::vec3(1.5f, 1.5f, 1.5f), glm::vec3(90.f, 0.0f, -25.f), vinesMesh, vinesMaterial); // vine 2 (squish blocking)
		createGroundObstacle("7", glm::vec3(-150.240f, 0.f, 7.88f), glm::vec3(0.25f, 0.25f, 0.25f), glm::vec3(0.0f, 0.0f, 84.f), cobwebMesh, cobwebMaterial); //cobweb 2 (squish Blocking 2)

		createGroundObstacle("8", glm::vec3(-170.f, 0.0f, -0.660), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(90.f, 0.0f, 0.0f), mushroomMesh, mushroomMaterial); //mushroom 3 (small jump)
		createGroundObstacle("9", glm::vec3(-200.f, 0.0f, 3.0), glm::vec3(1.f, 1.f, 1.f), glm::vec3(90.f, 0.0f, 73.f), vinesMesh, vinesMaterial); // vine 3 (jump blocking)
		createGroundObstacle("10", glm::vec3(-220.f, 0.0f, 3.3f), glm::vec3(0.25f, 0.25f, 0.25f), glm::vec3(0.0f, 0.0f, -75.f), cobwebMesh, cobwebMaterial); //cobweb 3 (tall jump)
		createGroundObstacle("11", glm::vec3(-230.f, 0.0f, -0.660), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(90.f, 0.0f, 0.0f), mushroomMesh, mushroomMaterial); //mushroom 4 (small jump)
		createGroundObstacle("12", glm::vec3(-250.f, 0.0f, 3.0), glm::vec3(1.f, 1.f, 1.f), glm::vec3(90.f, 0.0f, 73.f), vinesMesh, vinesMaterial); // vine 4 (jump blocking)
		createGroundObstacle("13", glm::vec3(-275.f, 0.0f, -0.660), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(90.f, 0.0f, 0.0f), mushroomMesh, mushroomMaterial); //mushroom 5 (small jump)
		createGroundObstacle("14", glm::vec3(-300.f, 5.530f, 0.250f), glm::vec3(1.5f, 1.5f, 1.5f), glm::vec3(90.f, 0.0f, -25.f), vinesMesh, vinesMaterial); // vine 5 (squish blocking)
		createGroundObstacle("15", glm::vec3(-300.240f, 0.f, 7.88f), glm::vec3(0.25f, 0.25f, 0.25f), glm::vec3(0.0f, 0.0f, 84.f), cobwebMesh, cobwebMaterial); //cobweb 4 (squish Blocking 2)

		createGroundObstacle("16", glm::vec3(-310.f, 0.0f, -0.660), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(90.f, 0.0f, 0.0f), mushroomMesh, mushroomMaterial); //mushroom 6 (small jump)
		createGroundObstacle("17", glm::vec3(-315.f, 0.0f, -0.660), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(90.f, 0.0f, 0.0f), mushroomMesh, mushroomMaterial); //mushroom 7 (small jump)
		createGroundObstacle("18", glm::vec3(-320.f, 0.0f, -0.660), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(90.f, 0.0f, 0.0f), mushroomMesh, mushroomMaterial); //mushroom 8 (small jump)
		createGroundObstacle("19", glm::vec3(-325.f, 0.0f, 3.3f), glm::vec3(0.25f, 0.25f, 0.25f), glm::vec3(0.0f, 0.0f, -75.f), cobwebMesh, cobwebMaterial); //cobweb 5 (tall jump)
		createGroundObstacle("20", glm::vec3(-340.f, 0.0f, 3.0), glm::vec3(1.f, 1.f, 1.f), glm::vec3(90.f, 0.0f, 73.f), vinesMesh, vinesMaterial); // vine 6 (jump blocking)
		createGroundObstacle("21", glm::vec3(-345.f, 5.530f, 0.250f), glm::vec3(1.5f, 1.5f, 1.5f), glm::vec3(90.f, 0.0f, -25.f), vinesMesh, vinesMaterial); // vine 7 (squish blocking)
		createGroundObstacle("22", glm::vec3(-345.240f, 0.f, 7.88f), glm::vec3(0.25f, 0.25f, 0.25f), glm::vec3(0.0f, 0.0f, 84.f), cobwebMesh, cobwebMaterial); //cobweb 6 (squish Blocking 2)
		createGroundObstacle("23", glm::vec3(-360.f, 0.0f, 3.3f), glm::vec3(0.25f, 0.25f, 0.25f), glm::vec3(0.0f, 0.0f, -75.f), cobwebMesh, cobwebMaterial); //cobweb 7 (tall jump)
		createGroundObstacle("24", glm::vec3(-380.f, 5.530f, 0.250f), glm::vec3(1.5f, 1.5f, 1.5f), glm::vec3(90.f, 0.0f, -25.f), vinesMesh, vinesMaterial); // vine 8 (squish blocking)
		createGroundObstacle("25", glm::vec3(-380.240f, 0.f, 7.88f), glm::vec3(0.25f, 0.25f, 0.25f), glm::vec3(0.0f, 0.0f, 84.f), cobwebMesh, cobwebMaterial); //cobweb 8 (squish Blocking 2)
		createGroundObstacle("26", glm::vec3(-395.f, 0.0f, 3.3f), glm::vec3(0.25f, 0.25f, 0.25f), glm::vec3(0.0f, 0.0f, -75.f), cobwebMesh, cobwebMaterial); //cobweb 9 (tall jump)

		//3D Backgrounds
		createGroundObstacle("27", glm::vec3(107.7f, -55.830f, -1.7f), glm::vec3(6.f, 6.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGMesh, BGMaterial);
		createGroundObstacle("28", glm::vec3(0.f, -55.830f, -1.7f), glm::vec3(6.f, 6.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGMesh, BGMaterial);
		createGroundObstacle("29", glm::vec3(-107.7f, -55.830f, -1.7f), glm::vec3(6.f, 6.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGMesh, BGMaterial);

		createGroundObstacle("30", glm::vec3(-215.4f, -55.830f, -1.7f), glm::vec3(6.f, 6.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGMesh, BGMaterial);
		createGroundObstacle("31", glm::vec3(-323.1f, -55.830f, -1.7f), glm::vec3(6.f, 6.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGMesh, BGMaterial);
		createGroundObstacle("32", glm::vec3(-430.8f, -55.830f, -1.7f), glm::vec3(6.f, 6.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGMesh, BGMaterial);

		//2DBackGrounds
		createGroundObstacle("33", glm::vec3(350.f, -130.0f, 64.130f), glm::vec3(375.0f, 125.0f, 250.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, bgMaterial);
		createGroundObstacle("34", glm::vec3(-50.f, -130.0f, 64.130f), glm::vec3(375.0f, 125.0f, 250.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, bgMaterial);
		createGroundObstacle("35", glm::vec3(-425.f, -130.0f, 64.130f), glm::vec3(375.0f, 125.0f, 250.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, bgMaterial);
		createGroundObstacle("36", glm::vec3(-1225.f, -130.0f, 64.130f), glm::vec3(375.0f, 125.0f, 250.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, bgMaterial);
		createGroundObstacle("37", glm::vec3(-1650.f, -130.0f, 64.130f), glm::vec3(375.0f, 125.0f, 250.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, bgMaterial);

		//Grass
		createGroundObstacle("38", glm::vec3(-18.170f, -38.230f, 5.250f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass1Material);
		createGroundObstacle("39", glm::vec3(29.580f, 46.920f, 5.080f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass2Material);
		createGroundObstacle("40", glm::vec3(2.090f, -47.360f, 5.090f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass3Material);
		createGroundObstacle("41", glm::vec3(35.700f, -37.550f, 5.310f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass4Material);
		createGroundObstacle("42", glm::vec3(-47.340f, -37.120f, 5.560f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass5Material);

		createGroundObstacle("43", glm::vec3(-18.170f - 103.5f, -38.230f, 5.250f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass1Material);
		createGroundObstacle("44", glm::vec3(29.580f - 103.5f, 46.920f, 5.080f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass2Material);
		createGroundObstacle("45", glm::vec3(2.090f - 103.5f, -47.360f, 5.090f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass3Material);
		createGroundObstacle("46", glm::vec3(35.700f - 103.5f, -37.550f, 5.310f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass4Material);
		createGroundObstacle("47", glm::vec3(-47.340f - 103.5f, -37.120f, 5.560f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass5Material);

		createGroundObstacle("48", glm::vec3(-18.170f - 103.5f - 103.5f, -38.230f, 5.250f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass1Material);
		createGroundObstacle("49", glm::vec3(29.580f - 103.5f - 103.5f, 46.920f, 5.080f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass2Material);
		createGroundObstacle("50", glm::vec3(2.090f - 103.5f - 103.5f, -47.360f, 5.090f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass3Material);
		createGroundObstacle("51", glm::vec3(35.700f - 103.5f - 103.5f, -37.550f, 5.310f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass4Material);
		createGroundObstacle("52", glm::vec3(-47.340f - 103.5f - 103.5f, -37.120f, 5.560f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass5Material);

		createGroundObstacle("53", glm::vec3(-18.170f - 103.5f - 103.5f - 103.5f, -38.230f, 5.250f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass1Material);
		createGroundObstacle("54", glm::vec3(29.580f - 103.5f - 103.5f - 103.5f, 46.920f, 5.080f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass2Material);
		createGroundObstacle("55", glm::vec3(2.090f - 103.5f - 103.5f - 103.5f, -47.360f, 5.090f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass3Material);
		createGroundObstacle("56", glm::vec3(35.700f - 103.5f - 103.5f - 103.5f, -37.550f, 5.310f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass4Material);
		createGroundObstacle("57", glm::vec3(-47.340f - 103.5f - 103.5f - 103.5f, -37.120f, 5.560f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass5Material);

		//Exit Tree
		createGroundObstacle("58", glm::vec3(-409.f, -3.380f, -0.340f), glm::vec3(3.0f, 3.0f, 3.0f), glm::vec3(90.0f, 0.0f, 140.f), ExitTreeMesh, ExitTreeMaterial);

		//Foreground Grass
		createGroundObstacle("59", glm::vec3(40.f, -14.390f, 5.390f), glm::vec3(40.0f, 11.770f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, ForegroundMaterial);
		createGroundObstacle("60", glm::vec3(0.f, -14.390f, 5.390f), glm::vec3(40.0f, 11.770f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, ForegroundMaterial);
		createGroundObstacle("61", glm::vec3(-40.f, -14.390f, 5.390f), glm::vec3(40.0f, 11.770f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, ForegroundMaterial);
		createGroundObstacle("62", glm::vec3(-80.f, -14.390f, 5.390f), glm::vec3(40.0f, 11.770f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, ForegroundMaterial);
		createGroundObstacle("63", glm::vec3(-120.f, -14.390f, 5.390f), glm::vec3(40.0f, 11.770f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, ForegroundMaterial);
		createGroundObstacle("64", glm::vec3(-160.f, -14.390f, 5.390f), glm::vec3(40.0f, 11.770f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, ForegroundMaterial);

		createGroundObstacle("65", glm::vec3(-200.f, -14.390f, 5.390f), glm::vec3(40.0f, 11.770f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, ForegroundMaterial);
		createGroundObstacle("66", glm::vec3(-240.f, -14.390f, 5.390f), glm::vec3(40.0f, 11.770f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, ForegroundMaterial);
		createGroundObstacle("67", glm::vec3(-280.f, -14.390f, 5.390f), glm::vec3(40.0f, 11.770f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, ForegroundMaterial);
		createGroundObstacle("68", glm::vec3(-320.f, -14.390f, 5.390f), glm::vec3(40.0f, 11.770f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, ForegroundMaterial);
		createGroundObstacle("69", glm::vec3(-360.f, -14.390f, 5.390f), glm::vec3(40.0f, 11.770f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, ForegroundMaterial);
		createGroundObstacle("70", glm::vec3(-400.f, -14.390f, 5.390f), glm::vec3(40.0f, 11.770f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, ForegroundMaterial);


		//Collisions

		//mushroom 1 collision
		createCollision("2", -19.660f, 1.560f, 1.f, 1.f);
		createCollision("3", -20.410f, 1.560f, 1.f, 1.f);
		createCollision("4", -19.970f, 1.860f, 1.f, 1.f);
		createCollision("5", -20.190f, 0.450f, 1.f, 1.f);

		//mushroom 2 collision
		createCollision("6", -44.660f, 1.560f, 1.f, 1.f);
		createCollision("7", -45.410f, 1.560f, 1.f, 1.f);
		createCollision("8", -44.970f, 1.860f, 1.f, 1.f);
		createCollision("9", -45.190f, 0.450f, 1.f, 1.f);

		// vine 1 collisions
		createCollision("10", -53.30f, 11.f, 0.5f, 5.5f);
		createCollision("11", -60.30f, 11.f, 0.5f, 5.5f);
		createCollision("12", -56.8f, 6.f, 4.0f, 0.5f);
		createCollision("13", -56.8f, 17.f, 4.0f, 0.5f);

		//cobweb collisions
		createCollision("14", -110.f, 1.63f, 1.f, 7.f);

		// squish collision
		createCollision("15", -149.780f, 10.4f, 0.420f, 10.f);

		//mushroom 3 collision
		createCollision("16", -169.660f, 1.560f, 1.f, 1.f);
		createCollision("17", -170.410f, 1.560f, 1.f, 1.f);
		createCollision("18", -169.970f, 1.860f, 1.f, 1.f);
		createCollision("19", -170.190f, 0.450f, 1.f, 1.f);

		// vine 3 collisions
		createCollision("20", -193.30f, 11.f, 0.5f, 5.5f);
		createCollision("21", -200.30f, 11.f, 0.5f, 5.5f);
		createCollision("22", -196.8f, 6.f, 4.0f, 0.5f);
		createCollision("23", -196.8f, 17.f, 4.0f, 0.5f);

		//cobweb collisions
		createCollision("24", -220.f, 1.63f, 1.f, 7.f);

		//mushroom 4 collision
		createCollision("25", -229.660f, 1.560f, 1.f, 1.f);
		createCollision("26", -230.410f, 1.560f, 1.f, 1.f);
		createCollision("27", -229.970f, 1.860f, 1.f, 1.f);
		createCollision("28", -230.190f, 0.450f, 1.f, 1.f);

		// vine 4 collisions
		createCollision("29", -243.30f, 11.f, 0.5f, 5.5f);
		createCollision("30", -250.30f, 11.f, 0.5f, 5.5f);
		createCollision("31", -246.8f, 6.f, 4.0f, 0.5f);
		createCollision("32", -246.8f, 17.f, 4.0f, 0.5f);

		//mushroom 5 collision
		createCollision("33", -274.660f, 1.560f, 1.f, 1.f);
		createCollision("34", -275.410f, 1.560f, 1.f, 1.f);
		createCollision("35", -274.970f, 1.860f, 1.f, 1.f);
		createCollision("36", -275.190f, 0.450f, 1.f, 1.f);

		// squish collision
		createCollision("37", -299.780f, 10.4f, 0.420f, 10.f);

		//mushroom 6 collision
		createCollision("38", -309.660f, 1.560f, 1.f, 1.f);
		createCollision("39", -310.410f, 1.560f, 1.f, 1.f);
		createCollision("40", -309.970f, 1.860f, 1.f, 1.f);
		createCollision("41", -310.190f, 0.450f, 1.f, 1.f);

		//mushroom 7 collision
		createCollision("42", -314.660f, 1.560f, 1.f, 1.f);
		createCollision("43", -315.410f, 1.560f, 1.f, 1.f);
		createCollision("44", -314.970f, 1.860f, 1.f, 1.f);
		createCollision("45", -315.190f, 0.450f, 1.f, 1.f);

		//mushroom 8 collision
		createCollision("46", -319.660f, 1.560f, 1.f, 1.f);
		createCollision("47", -320.410f, 1.560f, 1.f, 1.f);
		createCollision("48", -319.970f, 1.860f, 1.f, 1.f);
		createCollision("49", -320.190f, 0.450f, 1.f, 1.f);

		//cobweb collisions
		createCollision("50", -325.f, 1.63f, 1.f, 7.f);

		// vine 6 collisions
		createCollision("51", -333.30f, 11.f, 0.5f, 5.5f);
		createCollision("52", -340.30f, 11.f, 0.5f, 5.5f);
		createCollision("53", -336.8f, 6.f, 4.0f, 0.5f);
		createCollision("54", -336.8f, 17.f, 4.0f, 0.5f);

		// squish collision
		createCollision("55", -344.780f, 10.4f, 0.420f, 10.f);

		//cobweb collisions
		createCollision("56", -360.f, 1.63f, 1.f, 7.f);

		// squish collision
		createCollision("57", -379.780f, 10.4f, 0.420f, 10.f);

		//cobweb collisions
		createCollision("58", -395.f, 1.63f, 1.f, 7.f);

		// Set up the scene's camera
		GameObject::Sptr camera = scene->CreateGameObject("Main Camera");
		{
			camera->SetPostion(glm::vec3(0, 6.8, 2));
			camera->SetRotation(glm::vec3(90, 0, -180));
			camera->SetScale(glm::vec3(0.8f, 0.8f, 0.8f));
			camera->LookAt(glm::vec3(0.0f));

			Camera::Sptr cam = camera->Add<Camera>();

			// Make sure that the camera is set as the scene's main camera!
			scene->MainCamera = cam;
		}

		GameObject::Sptr plane5 = scene->CreateGameObject("plane5");
		{
			//under 1
			// Scale up the plane
			plane5->SetPostion(glm::vec3(-48.f, 0.f, -7.f));
			plane5->SetScale(glm::vec3(50.0F));

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = plane5->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(winMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = plane5->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(PlaneCollider::Create());
		}


		GameObject::Sptr player = scene->CreateGameObject("player");
		{
			// Set position in the scene
			player->SetPostion(glm::vec3(6.f, 0.0f, 1.0f));
			player->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			player->SetScale(glm::vec3(0.5f, 0.5f, 0.5f));

			// Add some behaviour that relies on the physics body
			player->Add<JumpBehaviour>(player->GetPosition());
			//player->Get<JumpBehaviour>(player->GetPosition());
			// Create and attach a renderer for the monkey
			RenderComponent::Sptr renderer = player->Add<RenderComponent>();
			renderer->SetMesh(ladybugMesh);
			renderer->SetMaterial(ladybugMaterial);

			collisions.push_back(CollisionRect(player->GetPosition(), 1.0f, 1.0f, 0));

			// Add a dynamic rigid body to this monkey
			RigidBody::Sptr physics = player->Add<RigidBody>(RigidBodyType::Dynamic);
			physics->AddCollider(ConvexMeshCollider::Create());


			// We'll add a behaviour that will interact with our trigger volumes
			MaterialSwapBehaviour::Sptr triggerInteraction = player->Add<MaterialSwapBehaviour>();
			triggerInteraction->EnterMaterial = boxMaterial;
			triggerInteraction->ExitMaterial = monkeyMaterial;
		}
		GameObject::Sptr jumpingObstacle = scene->CreateGameObject("Trigger2");
		{
			// Set and rotation position in the scene
			jumpingObstacle->SetPostion(glm::vec3(40.f, 0.0f, 1.0f));
			jumpingObstacle->SetRotation(glm::vec3(0.0f, 0.0f, 0.0f));
			jumpingObstacle->SetScale(glm::vec3(0.5f, 0.5f, 0.5f));

			// Add a render component
			RenderComponent::Sptr renderer = jumpingObstacle->Add<RenderComponent>();
			renderer->SetMesh(cubeMesh);
			renderer->SetMaterial(boxMaterial);

			collisions.push_back(CollisionRect(jumpingObstacle->GetPosition(), 1.0f, 1.0f, 1));

			//// This is an example of attaching a component and setting some parameters
			//RotatingBehaviour::Sptr behaviour = jumpingObstacle->Add<RotatingBehaviour>();
			//behaviour->RotationSpeed = glm::vec3(0.0f, 0.0f, -90.0f);
		}

		GameObject::Sptr ButtonBack1 = scene->CreateGameObject("ButtonBack1");
		{
			// Set position in the scene
			ButtonBack1->SetPostion(glm::vec3(1.f, 6.25f, 6.f));
			// Scale down the plane
			ButtonBack1->SetScale(glm::vec3(3.0f, 0.8f, 0.5f));
			//set rotateee
			ButtonBack1->SetRotation(glm::vec3(-80.f, 0.f, 0.f));


			// Create and attach a render component
			RenderComponent::Sptr renderer = ButtonBack1->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(ButtonMaterial);
		}

		GameObject::Sptr ButtonBack2 = scene->CreateGameObject("ButtonBack2");
		{
			// Set position in the scene
			ButtonBack2->SetPostion(glm::vec3(1.f, 6.5f, 5.f));
			// Scale down the plane
			ButtonBack2->SetScale(glm::vec3(3.0f, 0.8f, 0.5f));
			//spin things
			ButtonBack2->SetRotation(glm::vec3(-80.0f, 0.f, 0.f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = ButtonBack2->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(ButtonMaterial);
		}

		GameObject::Sptr ResumeText = scene->CreateGameObject("ResumeText");
		{
			// Set position in the scene
			ResumeText->SetPostion(glm::vec3(1.0f, 8.0f, 6.1f));
			// Scale down the plane
			ResumeText->SetScale(glm::vec3(2.0f, 0.4f, 0.5f));
			ResumeText->SetRotation(glm::vec3(80.0f, 0.f, -180.f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = ResumeText->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(ResumeMaterial);
		}

		GameObject::Sptr ReplayText = scene->CreateGameObject("ReplayText");
		{
			// Set position in the scene
			ReplayText->SetPostion(glm::vec3(1.0f, 8.0f, 6.1f));
			// Scale down the plane
			ReplayText->SetScale(glm::vec3(2.0f, 0.4f, 0.5f));
			ReplayText->SetRotation(glm::vec3(80.0f, 0.f, -180.f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = ReplayText->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(ReplayMaterial);
		}

		GameObject::Sptr MainMenuText = scene->CreateGameObject("MainMenuText");
		{
			// Set position in the scene
			MainMenuText->SetPostion(glm::vec3(1.0f, 8.0f, 5.4f));
			// Scale down the plane
			MainMenuText->SetScale(glm::vec3(2.0f, 0.4f, 0.5f));
			MainMenuText->SetRotation(glm::vec3(80.0f, 0.f, -180.f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = MainMenuText->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(MainMenuMaterial);
		}

		GameObject::Sptr PBar = scene->CreateGameObject("ProgressBarGO");
		{
			// Scale up the plane
			PBar->SetPostion(glm::vec3(0.060f, 3.670f, -0.510f));
			PBar->SetRotation(glm::vec3(-90, -180.0f, 0.0f));
			PBar->SetScale(glm::vec3(15.f, 1.620f, 47.950f));

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = PBar->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(ProgressBarMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = PBar->Add<RigidBody>(/*static by default*/);
		}

		GameObject::Sptr PBug = scene->CreateGameObject("ProgressBarProgress");
		{
			// Scale up the plane
			PBug->SetPostion(glm::vec3(0.060f, 3.670f, -0.510f));
			PBug->SetRotation(glm::vec3(-90, -180.0f, 0.0f));
			PBug->SetScale(glm::vec3(1.5f, 1.5f, 1.5f));

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = PBug->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(PbarbugMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = PBug->Add<RigidBody>(/*static by default*/);
		}

		//Objects with transparency need to be loaded in last otherwise it creates issues
		GameObject::Sptr PanelPause = scene->CreateGameObject("PanelPause");
		{
			// Set position in the scene
			PanelPause->SetPostion(glm::vec3(1.f, -15.f, 6.5f));
			// Scale down the plane
			PanelPause->SetScale(glm::vec3(10.0f, 10.0f, 10.0f));
			// Rotate panel
			PanelPause->SetRotation(glm::vec3(-80.f, 0.f, 0.f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = PanelPause->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(PanelMaterial);
		}

		GameObject::Sptr Filter = scene->CreateGameObject("Filter");
		{
			// Set position in the scene
			Filter->SetPostion(glm::vec3(1.0f, 6.51f, 5.f));
			// Scale down the plane
			Filter->SetScale(glm::vec3(3.0f, 0.8f, 1.0f));
			Filter->SetRotation(glm::vec3(-80.f, 0.0f, 0.0f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = Filter->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(FilterMaterial);

			// This object is a renderable only, it doesn't have any behaviours or
			// physics bodies attached!
		}

		// Creates Ground Collisions
		GameObject::Sptr plane = scene->CreateGameObject("Plane");
		{
			// Scale up the plane
			plane->SetPostion(glm::vec3(0.060f, 3.670f, -0.510f));
			plane->SetScale(glm::vec3(47.880f, 23.7f, 48.38f));

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = plane->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(BlankMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = plane->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(PlaneCollider::Create());
		}

		// Kinematic rigid bodies are those controlled by some outside controller
		// and ONLY collide with dynamic objects
		RigidBody::Sptr physics = jumpingObstacle->Add<RigidBody>(RigidBodyType::Kinematic);
		physics->AddCollider(ConvexMeshCollider::Create());

		// Save the asset manifest for all the resources we just loaded
		ResourceManager::SaveManifest("manifest.json");
		// Save the scene to a JSON file
		scene->Save("Level.json");

		}

																	//// Making a 'Menu' Scene ////

		{
		// Create our OpenGL resources
		Shader::Sptr uboShader = ResourceManager::CreateAsset<Shader>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shader.glsl" },
			{ ShaderPartType::Fragment, "shaders/frag_blinn_phong_textured.glsl" }
		});

		MeshResource::Sptr monkeyMesh = ResourceManager::CreateAsset<MeshResource>("Monkey.obj");
		Texture2D::Sptr    boxTexture = ResourceManager::CreateAsset<Texture2D>("textures/box-diffuse.png");
		Texture2D::Sptr    grassTexture = ResourceManager::CreateAsset<Texture2D>("textures/grass.png");
		Texture2D::Sptr    monkeyTex = ResourceManager::CreateAsset<Texture2D>("textures/monkey-uvMap.png");
		Texture2D::Sptr    MenuTex = ResourceManager::CreateAsset<Texture2D>("textures/Game Poster 3.png");
	
		
		// Textures for UI

		Texture2D::Sptr    ButtonBackTex = ResourceManager::CreateAsset<Texture2D>("textures/Button Background.png");
		Texture2D::Sptr    ButtonStartTex = ResourceManager::CreateAsset<Texture2D>("textures/Start Text.png");
		Texture2D::Sptr    ButtonExitTex = ResourceManager::CreateAsset<Texture2D>("textures/Exit Text.png");
		Texture2D::Sptr    FFLogoTex = ResourceManager::CreateAsset<Texture2D>("textures/Frog Frontier Logo.png");
		Texture2D::Sptr    BackTextTex = ResourceManager::CreateAsset<Texture2D>("textures/Exit Text.png");
		Texture2D::Sptr    StartTextTex = ResourceManager::CreateAsset<Texture2D>("textures/Start Text.png");
		Texture2D::Sptr    FilterTex = ResourceManager::CreateAsset<Texture2D>("textures/Button Filter.png");
		Texture2D::Sptr    ControlsTex = ResourceManager::CreateAsset<Texture2D>("textures/ControlsText.png");

		// Create an empty scene
		scene = std::make_shared<Scene>();

		// I hate this
		scene->BaseShader = uboShader;

		// Create our materials
		Material::Sptr boxMaterial = ResourceManager::CreateAsset<Material>();
		{
			boxMaterial->Name = "Box";
			boxMaterial->MatShader = scene->BaseShader;
			boxMaterial->Texture = boxTexture;
			boxMaterial->Shininess = 2.0f;
		}

		Material::Sptr grassMaterial = ResourceManager::CreateAsset<Material>();
		{
			grassMaterial->Name = "Grass";
			grassMaterial->MatShader = scene->BaseShader;
			grassMaterial->Texture = grassTexture;
			grassMaterial->Shininess = 2.0f;
		}

		Material::Sptr monkeyMaterial = ResourceManager::CreateAsset<Material>();
		{
			monkeyMaterial->Name = "Monkey";
			monkeyMaterial->MatShader = scene->BaseShader;
			monkeyMaterial->Texture = monkeyTex;
			monkeyMaterial->Shininess = 256.0f;

		}

		Material::Sptr MenuMaterial = ResourceManager::CreateAsset<Material>();
		{
			MenuMaterial->Name = "Menu";
			MenuMaterial->MatShader = scene->BaseShader;
			MenuMaterial->Texture = MenuTex;
			MenuMaterial->Shininess = 2.0f;
		}

		Material::Sptr ButtonBackMaterial = ResourceManager::CreateAsset<Material>();
		{
			ButtonBackMaterial->Name = "ButtonBackground";
			ButtonBackMaterial->MatShader = scene->BaseShader;
			ButtonBackMaterial->Texture = ButtonBackTex;
			ButtonBackMaterial->Shininess = 2.0f;
		}

		Material::Sptr ButtonStartMaterial = ResourceManager::CreateAsset<Material>();
		{
			ButtonStartMaterial->Name = "ButtonStart";
			ButtonStartMaterial->MatShader = scene->BaseShader;
			ButtonStartMaterial->Texture = ButtonStartTex;
			ButtonStartMaterial->Shininess = 2.0f;
		}

		Material::Sptr ButtonExitMaterial = ResourceManager::CreateAsset<Material>();
		{
			ButtonExitMaterial->Name = "ButtonExit";
			ButtonExitMaterial->MatShader = scene->BaseShader;
			ButtonExitMaterial->Texture = ButtonExitTex;
			ButtonExitMaterial->Shininess = 2.0f;
		}

		Material::Sptr FFLogoMaterial = ResourceManager::CreateAsset<Material>();
		{
			FFLogoMaterial->Name = "Frog Frontier Logo";
			FFLogoMaterial->MatShader = scene->BaseShader;
			FFLogoMaterial->Texture = FFLogoTex;
			FFLogoMaterial->Shininess = 2.0f;
		}

		Material::Sptr BackTextMaterial = ResourceManager::CreateAsset<Material>();
		{
			BackTextMaterial->Name = "Back Button Text";
			BackTextMaterial->MatShader = scene->BaseShader;
			BackTextMaterial->Texture = BackTextTex;
			BackTextMaterial->Shininess = 2.0f;
		}

		Material::Sptr StartTextMaterial = ResourceManager::CreateAsset<Material>();
		{
			StartTextMaterial->Name = "Start Button Text";
			StartTextMaterial->MatShader = scene->BaseShader;
			StartTextMaterial->Texture = StartTextTex;
			StartTextMaterial->Shininess = 2.0f;
		}

		Material::Sptr FilterMaterial = ResourceManager::CreateAsset<Material>();
		{
			FilterMaterial->Name = "Button Filter";
			FilterMaterial->MatShader = scene->BaseShader;
			FilterMaterial->Texture = FilterTex;
			FilterMaterial->Shininess = 2.0f;
		}

		Material::Sptr ControlsMaterial = ResourceManager::CreateAsset<Material>();
		{
			ControlsMaterial->Name = "Controls Text";
			ControlsMaterial->MatShader = scene->BaseShader;
			ControlsMaterial->Texture = ControlsTex;
			ControlsMaterial->Shininess = 2.0f;
		}

		// Create some lights for our scene
		scene->Lights.resize(4);
		scene->Lights[0].Position = glm::vec3(3.0f, 3.0f, 3.0f);
		scene->Lights[0].Color = glm::vec3(0.892f, 1.0f, 0.882f);
		scene->Lights[0].Range = 10.0f;

		scene->Lights[1].Position = glm::vec3(3.0f, -3.0f, 3.0f);
		scene->Lights[1].Color = glm::vec3(0.892f, 1.0f, 0.882f);
		scene->Lights[1].Range = 10.0f;

		scene->Lights[2].Position = glm::vec3(-3.0f, 3.0f, 3.0f);
		scene->Lights[2].Color = glm::vec3(0.892f, 1.0f, 0.882f);
		scene->Lights[2].Range = 10.0f;

		scene->Lights[3].Position = glm::vec3(-3.0f, -3.0f, 3.0f);
		scene->Lights[3].Color = glm::vec3(0.892f, 1.0f, 0.882f);
		scene->Lights[3].Range = 10.0f;

		// We'll create a mesh that is a simple plane that we can resize later
		MeshResource::Sptr planeMesh = ResourceManager::CreateAsset<MeshResource>();
		MeshResource::Sptr cubeMesh = ResourceManager::CreateAsset<MeshResource>("cube.obj");
		planeMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(1.0f)));
		planeMesh->GenerateMesh();

		// Set up the scene's camera
		GameObject::Sptr camera = scene->CreateGameObject("Main Camera");
		{
			camera->SetPostion(glm::vec3(0, 0, 5));
			camera->SetRotation(glm::vec3(0, -0, 0));

			Camera::Sptr cam = camera->Add<Camera>();

			// Make sure that the camera is set as the scene's main camera!
			scene->MainCamera = cam;
		}

		// Set up all our sample objects
		GameObject::Sptr plane = scene->CreateGameObject("Plane");
		{
			// Scale up the plane
			plane->SetScale(glm::vec3(10.0F));

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = plane->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(MenuMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = plane->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(PlaneCollider::Create());
		}

		GameObject::Sptr GameLogo = scene->CreateGameObject("Game Menu Logo");
		{
			// Set position in the scene
			GameLogo->SetPostion(glm::vec3(1.0f, 1.25f, 3.0f));
			// Scale down the plane
			GameLogo->SetScale(glm::vec3(1.5f, 1.0f, 1.0f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = GameLogo->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(FFLogoMaterial);

			// This object is a renderable only, it doesn't have any behaviours or
			// physics bodies attached!
		}

		GameObject::Sptr ButtonBackground1 = scene->CreateGameObject("ButtonBackground1");
		{
			// Set position in the scene
			ButtonBackground1->SetPostion(glm::vec3(1.0f, 0.5f, 3.0f));
			// Scale down the plane
			ButtonBackground1->SetScale(glm::vec3(1.25f, 0.250f, 1.0f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = ButtonBackground1->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(ButtonBackMaterial);

			// This object is a renderable only, it doesn't have any behaviours or
			// physics bodies attached!
		}


		GameObject::Sptr ButtonBackground2 = scene->CreateGameObject("ButtonBackground2");
		{
			// Set position in the scene
			ButtonBackground2->SetPostion(glm::vec3(1.0f, 0.15f, 3.0f));
			// Scale down the plane
			ButtonBackground2->SetScale(glm::vec3(1.25f, 0.250f, 1.0f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = ButtonBackground2->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(ButtonBackMaterial);

			// This object is a renderable only, it doesn't have any behaviours or
			// physics bodies attached!
		}

		GameObject::Sptr ButtonBackground3 = scene->CreateGameObject("ButtonBackground3");
		{
			// Set position in the scene
			ButtonBackground3->SetPostion(glm::vec3(1.0f, -0.2f, 3.0f));
			// Scale down the plane
			ButtonBackground3->SetScale(glm::vec3(1.25f, 0.250f, 1.0f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = ButtonBackground3->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(ButtonBackMaterial);

			// This object is a renderable only, it doesn't have any behaviours or
			// physics bodies attached!
		}

		GameObject::Sptr StartButton = scene->CreateGameObject("StartButton");
		{
			// Set position in the scene
			StartButton->SetPostion(glm::vec3(0.75f, 0.375f, 3.5f));
			// Scale down the plane
			StartButton->SetScale(glm::vec3(0.5f, 0.125f, 1.0f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = StartButton->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(StartTextMaterial);

			// This object is a renderable only, it doesn't have any behaviours or
			// physics bodies attached!
		}

		GameObject::Sptr BackButton = scene->CreateGameObject("BackButton");
		{
			// Set position in the scene
			BackButton->SetPostion(glm::vec3(0.750f, -0.15f, 3.5f));
			// Scale down the plane
			BackButton->SetScale(glm::vec3(0.5f, 0.125f, 1.0f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = BackButton->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(BackTextMaterial);

			// This object is a renderable only, it doesn't have any behaviours or
			// physics bodies attached!
		}

		GameObject::Sptr ControlsButton = scene->CreateGameObject("ControlsButton");
		{
			// Set position in the scene
			ControlsButton->SetPostion(glm::vec3(0.750f, 0.115f, 3.5f));
			// Scale down the plane
			ControlsButton->SetScale(glm::vec3(0.8f, 0.25f, 1.0f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = ControlsButton->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(ControlsMaterial);

			// This object is a renderable only, it doesn't have any behaviours or
			// physics bodies attached!
		}

		GameObject::Sptr Filter = scene->CreateGameObject("Filter");
		{
			// Set position in the scene
			Filter->SetPostion(glm::vec3(1.0f, 0.5f, 3.010f));
			// Scale down the plane
			Filter->SetScale(glm::vec3(1.25f, 0.25f, 1.0f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = Filter->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(FilterMaterial);

			// This object is a renderable only, it doesn't have any behaviours or
			// physics bodies attached!
		}

		// Save the asset manifest for all the resources we just loaded
		ResourceManager::SaveManifest("manifest.json");
		// Save the scene to a JSON file
		scene->Save("menu.json");

		}

										//// Making a 'Control' Scene ////

		{
		// Create our OpenGL resources
		Shader::Sptr uboShader = ResourceManager::CreateAsset<Shader>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shader.glsl" },
			{ ShaderPartType::Fragment, "shaders/frag_blinn_phong_textured.glsl" }
		});

		Texture2D::Sptr    boxTexture = ResourceManager::CreateAsset<Texture2D>("textures/box-diffuse.png");
		Texture2D::Sptr    grassTexture = ResourceManager::CreateAsset<Texture2D>("textures/grass.png");
		Texture2D::Sptr    monkeyTex = ResourceManager::CreateAsset<Texture2D>("textures/monkey-uvMap.png");
		Texture2D::Sptr    MenuTex = ResourceManager::CreateAsset<Texture2D>("textures/ControlsMenu.png");

		// Textures for UI

		Texture2D::Sptr    ButtonBackTex = ResourceManager::CreateAsset<Texture2D>("textures/Button Background.png");
		Texture2D::Sptr    BackTextTex = ResourceManager::CreateAsset<Texture2D>("textures/Back Text.png");

		// Create an empty scene
		scene = std::make_shared<Scene>();

		// I hate this
		scene->BaseShader = uboShader;

		// Create our materials
		Material::Sptr boxMaterial = ResourceManager::CreateAsset<Material>();
		{
			boxMaterial->Name = "Box";
			boxMaterial->MatShader = scene->BaseShader;
			boxMaterial->Texture = boxTexture;
			boxMaterial->Shininess = 2.0f;
		}

		Material::Sptr grassMaterial = ResourceManager::CreateAsset<Material>();
		{
			grassMaterial->Name = "Grass";
			grassMaterial->MatShader = scene->BaseShader;
			grassMaterial->Texture = grassTexture;
			grassMaterial->Shininess = 2.0f;
		}


		Material::Sptr MenuMaterial = ResourceManager::CreateAsset<Material>();
		{
			MenuMaterial->Name = "Menu";
			MenuMaterial->MatShader = scene->BaseShader;
			MenuMaterial->Texture = MenuTex;
			MenuMaterial->Shininess = 2.0f;
		}

		Material::Sptr ButtonBackMaterial = ResourceManager::CreateAsset<Material>();
		{
			ButtonBackMaterial->Name = "ButtonBackground";
			ButtonBackMaterial->MatShader = scene->BaseShader;
			ButtonBackMaterial->Texture = ButtonBackTex;
			ButtonBackMaterial->Shininess = 2.0f;
		}

		Material::Sptr BackTextMaterial = ResourceManager::CreateAsset<Material>();
		{
			BackTextMaterial->Name = "Back Button Text";
			BackTextMaterial->MatShader = scene->BaseShader;
			BackTextMaterial->Texture = BackTextTex;
			BackTextMaterial->Shininess = 2.0f;
		}

		// Create some lights for our scene
		scene->Lights.resize(4);
		scene->Lights[0].Position = glm::vec3(3.0f, 3.0f, 3.0f);
		scene->Lights[0].Color = glm::vec3(0.892f, 1.0f, 0.882f);
		scene->Lights[0].Range = 10.0f;

		scene->Lights[1].Position = glm::vec3(3.0f, -3.0f, 3.0f);
		scene->Lights[1].Color = glm::vec3(0.892f, 1.0f, 0.882f);
		scene->Lights[1].Range = 10.0f;

		scene->Lights[2].Position = glm::vec3(-3.0f, 3.0f, 3.0f);
		scene->Lights[2].Color = glm::vec3(0.892f, 1.0f, 0.882f);
		scene->Lights[2].Range = 10.0f;

		scene->Lights[3].Position = glm::vec3(-3.0f, -3.0f, 3.0f);
		scene->Lights[3].Color = glm::vec3(0.892f, 1.0f, 0.882f);
		scene->Lights[3].Range = 10.0f;

		// We'll create a mesh that is a simple plane that we can resize later
		MeshResource::Sptr planeMesh = ResourceManager::CreateAsset<MeshResource>();
		MeshResource::Sptr cubeMesh = ResourceManager::CreateAsset<MeshResource>("cube.obj");
		planeMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(1.0f)));
		planeMesh->GenerateMesh();

		// Set up the scene's camera
		GameObject::Sptr camera = scene->CreateGameObject("Main Camera");
		{
			camera->SetPostion(glm::vec3(0, 0, 5));
			camera->SetRotation(glm::vec3(0, -0, 0));

			Camera::Sptr cam = camera->Add<Camera>();

			// Make sure that the camera is set as the scene's main camera!
			scene->MainCamera = cam;
		}

		// Set up all our sample objects
		GameObject::Sptr plane = scene->CreateGameObject("Plane");
		{
			// Scale up the plane
			plane->SetScale(glm::vec3(10.0F));

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = plane->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(MenuMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = plane->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(PlaneCollider::Create());
		}

		GameObject::Sptr ButtonBackground2 = scene->CreateGameObject("ButtonBackground2");
		{
			// Set position in the scene
			ButtonBackground2->SetPostion(glm::vec3(0.0f, -1.5f, 3.0f));
			// Scale down the plane
			ButtonBackground2->SetScale(glm::vec3(1.25f, 0.250f, 1.0f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = ButtonBackground2->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(ButtonBackMaterial);

			// This object is a renderable only, it doesn't have any behaviours or
			// physics bodies attached!
		}


		GameObject::Sptr BackButton = scene->CreateGameObject("BackButton");
		{
			// Set position in the scene
			BackButton->SetPostion(glm::vec3(0.f, -1.12f, 3.5f));
			// Scale down the plane
			BackButton->SetScale(glm::vec3(0.5f, 0.125f, 1.0f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = BackButton->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(BackTextMaterial);

			// This object is a renderable only, it doesn't have any behaviours or
			// physics bodies attached!
		}

		// Save the asset manifest for all the resources we just loaded
		ResourceManager::SaveManifest("manifest.json");
		// Save the scene to a JSON file
		scene->Save("CS.json");

		}



												//// Level Select Scene ////

		{
		// Create our OpenGL resources
		Shader::Sptr uboShader = ResourceManager::CreateAsset<Shader>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shader.glsl" },
			{ ShaderPartType::Fragment, "shaders/frag_blinn_phong_textured.glsl" }
		});

		MeshResource::Sptr monkeyMesh = ResourceManager::CreateAsset<MeshResource>("Monkey.obj");
		Texture2D::Sptr    boxTexture = ResourceManager::CreateAsset<Texture2D>("textures/box-diffuse.png");
		Texture2D::Sptr    grassTexture = ResourceManager::CreateAsset<Texture2D>("textures/grass.png");
		Texture2D::Sptr    monkeyTex = ResourceManager::CreateAsset<Texture2D>("textures/monkey-uvMap.png");
		Texture2D::Sptr    MenuTex = ResourceManager::CreateAsset<Texture2D>("textures/Game Poster 2 Extended.png");
		
		Texture2D::Sptr    LSButtonTex = ResourceManager::CreateAsset<Texture2D>("textures/Level Button Background 1.png");
		Texture2D::Sptr    LSLogoTex = ResourceManager::CreateAsset<Texture2D>("textures/Frog Frontier Logo Side Scroller.png");
		Texture2D::Sptr    ButtonBackTex = ResourceManager::CreateAsset<Texture2D>("textures/Button Background.png");
		Texture2D::Sptr    BackTex = ResourceManager::CreateAsset<Texture2D>("textures/Back Text.png");

		Texture2D::Sptr    Tex1 = ResourceManager::CreateAsset<Texture2D>("textures/1.png");
		Texture2D::Sptr    Tex2 = ResourceManager::CreateAsset<Texture2D>("textures/2.png");
		Texture2D::Sptr    Tex3 = ResourceManager::CreateAsset<Texture2D>("textures/3.png");
		Texture2D::Sptr    Tex4 = ResourceManager::CreateAsset<Texture2D>("textures/4.png");
		Texture2D::Sptr    Tex5 = ResourceManager::CreateAsset<Texture2D>("textures/5.png");
		Texture2D::Sptr    Tex6 = ResourceManager::CreateAsset<Texture2D>("textures/6.png");
		Texture2D::Sptr    Tex7 = ResourceManager::CreateAsset<Texture2D>("textures/7.png");
		Texture2D::Sptr    Tex8 = ResourceManager::CreateAsset<Texture2D>("textures/8.png");
		Texture2D::Sptr    Tex9 = ResourceManager::CreateAsset<Texture2D>("textures/9.png");
		Texture2D::Sptr    Tex10 = ResourceManager::CreateAsset<Texture2D>("textures/10.png");

		// Create an empty scene
		scene = std::make_shared<Scene>();

		// I hate this
		scene->BaseShader = uboShader;

		// Create our materials
		Material::Sptr boxMaterial = ResourceManager::CreateAsset<Material>();
		{
			boxMaterial->Name = "Box";
			boxMaterial->MatShader = scene->BaseShader;
			boxMaterial->Texture = boxTexture;
			boxMaterial->Shininess = 2.0f;
		}

		Material::Sptr grassMaterial = ResourceManager::CreateAsset<Material>();
		{
			grassMaterial->Name = "Grass";
			grassMaterial->MatShader = scene->BaseShader;
			grassMaterial->Texture = grassTexture;
			grassMaterial->Shininess = 2.0f;
		}

		Material::Sptr monkeyMaterial = ResourceManager::CreateAsset<Material>();
		{
			monkeyMaterial->Name = "Monkey";
			monkeyMaterial->MatShader = scene->BaseShader;
			monkeyMaterial->Texture = monkeyTex;
			monkeyMaterial->Shininess = 256.0f;

		}

		Material::Sptr MenuMaterial = ResourceManager::CreateAsset<Material>();
		{
			MenuMaterial->Name = "Menu";
			MenuMaterial->MatShader = scene->BaseShader;
			MenuMaterial->Texture = MenuTex;
			MenuMaterial->Shininess = 2.0f;
		}

		Material::Sptr LSButtonMaterial = ResourceManager::CreateAsset<Material>();
		{
			LSButtonMaterial->Name = "LSButton";
			LSButtonMaterial->MatShader = scene->BaseShader;
			LSButtonMaterial->Texture = LSButtonTex;
			LSButtonMaterial->Shininess = 2.0f;
		}

		Material::Sptr LSLogoMaterial = ResourceManager::CreateAsset<Material>();
		{
			LSLogoMaterial->Name = "LSLogo";
			LSLogoMaterial->MatShader = scene->BaseShader;
			LSLogoMaterial->Texture = LSLogoTex;
			LSLogoMaterial->Shininess = 2.0f;
		}

		Material::Sptr ButtonBackMaterial = ResourceManager::CreateAsset<Material>();
		{
			ButtonBackMaterial->Name = "ButtonBackground";
			ButtonBackMaterial->MatShader = scene->BaseShader;
			ButtonBackMaterial->Texture = ButtonBackTex;
			ButtonBackMaterial->Shininess = 2.0f;
		}

		Material::Sptr BackMaterial = ResourceManager::CreateAsset<Material>();
		{
			BackMaterial->Name = "Back";
			BackMaterial->MatShader = scene->BaseShader;
			BackMaterial->Texture = BackTex;
			BackMaterial->Shininess = 2.0f;
		}

		Material::Sptr Material1 = ResourceManager::CreateAsset<Material>();
		{
			Material1->Name = "1";
			Material1->MatShader = scene->BaseShader;
			Material1->Texture = Tex1;
			Material1->Shininess = 2.0f;
		}

		Material::Sptr Material2 = ResourceManager::CreateAsset<Material>();
		{
			Material2->Name = "2";
			Material2->MatShader = scene->BaseShader;
			Material2->Texture = Tex2;
			Material2->Shininess = 2.0f;
		}

		Material::Sptr Material3 = ResourceManager::CreateAsset<Material>();
		{
			Material3->Name = "3";
			Material3->MatShader = scene->BaseShader;
			Material3->Texture = Tex3;
			Material3->Shininess = 2.0f;
		}

		Material::Sptr Material4 = ResourceManager::CreateAsset<Material>();
		{
			Material4->Name = "4";
			Material4->MatShader = scene->BaseShader;
			Material4->Texture = Tex4;
			Material4->Shininess = 2.0f;
		}

		Material::Sptr Material5 = ResourceManager::CreateAsset<Material>();
		{
			Material5->Name = "5";
			Material5->MatShader = scene->BaseShader;
			Material5->Texture = Tex5;
			Material5->Shininess = 2.0f;
		}

		Material::Sptr Material6 = ResourceManager::CreateAsset<Material>();
		{
			Material6->Name = "6";
			Material6->MatShader = scene->BaseShader;
			Material6->Texture = Tex6;
			Material6->Shininess = 2.0f;
		}

		Material::Sptr Material7 = ResourceManager::CreateAsset<Material>();
		{
			Material7->Name = "7";
			Material7->MatShader = scene->BaseShader;
			Material7->Texture = Tex7;
			Material7->Shininess = 2.0f;
		}

		Material::Sptr Material8 = ResourceManager::CreateAsset<Material>();
		{
			Material8->Name = "8";
			Material8->MatShader = scene->BaseShader;
			Material8->Texture = Tex8;
			Material8->Shininess = 2.0f;
		}
		
		Material::Sptr Material9 = ResourceManager::CreateAsset<Material>();
		{
			Material9->Name = "9";
			Material9->MatShader = scene->BaseShader;
			Material9->Texture = Tex9;
			Material9->Shininess = 2.0f;
		}

		Material::Sptr Material10 = ResourceManager::CreateAsset<Material>();
		{
			Material10->Name = "10";
			Material10->MatShader = scene->BaseShader;
			Material10->Texture = Tex10;
			Material10->Shininess = 2.0f;
		}

		// Create some lights for our scene
		scene->Lights.resize(4);
		scene->Lights[0].Position = glm::vec3(3.0f, 3.0f, 3.0f);
		scene->Lights[0].Color = glm::vec3(0.892f, 1.0f, 0.882f);
		scene->Lights[0].Range = 10.0f;

		scene->Lights[1].Position = glm::vec3(3.0f, -3.0f, 3.0f);
		scene->Lights[1].Color = glm::vec3(0.892f, 1.0f, 0.882f);
		scene->Lights[1].Range = 10.0f;

		scene->Lights[2].Position = glm::vec3(-3.0f, 3.0f, 3.0f);
		scene->Lights[2].Color = glm::vec3(0.892f, 1.0f, 0.882f);
		scene->Lights[2].Range = 10.0f;

		scene->Lights[3].Position = glm::vec3(-3.0f, -3.0f, 3.0f);
		scene->Lights[3].Color = glm::vec3(0.892f, 1.0f, 0.882f);
		scene->Lights[3].Range = 10.0f;

		// We'll create a mesh that is a simple plane that we can resize later
		MeshResource::Sptr planeMesh = ResourceManager::CreateAsset<MeshResource>();
		MeshResource::Sptr cubeMesh = ResourceManager::CreateAsset<MeshResource>("cube.obj");
		planeMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(1.0f)));
		planeMesh->GenerateMesh();

		// Set up the scene's camera
		GameObject::Sptr camera = scene->CreateGameObject("Main Camera");
		{
			camera->SetPostion(glm::vec3(0, 0, 5));
			camera->SetRotation(glm::vec3(0, -0, 0));

			Camera::Sptr cam = camera->Add<Camera>();

			// Make sure that the camera is set as the scene's main camera!
			scene->MainCamera = cam;
		}

		// Set up all our sample objects
		GameObject::Sptr plane = scene->CreateGameObject("Plane");
		{
			// Scale up the plane
			plane->SetScale(glm::vec3(15.0F, 10.0f, 10.0f));

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = plane->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(MenuMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = plane->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(PlaneCollider::Create());
		}

		GameObject::Sptr LsButton1 = scene->CreateGameObject("LSButton1");
		{
			// Set position in the scene
			LsButton1->SetPostion(glm::vec3(0.3f, 0.5f, 3.0f));
			// Scale down the plane
			LsButton1->SetScale(glm::vec3(0.5f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = LsButton1->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(LSButtonMaterial);

			// This object is a renderable only, it doesn't have any behaviours or
			// physics bodies attached!
		}

		GameObject::Sptr LsButton2 = scene->CreateGameObject("LSButton2");
		{
			// Set position in the scene
			LsButton2->SetPostion(glm::vec3(0.9f, 0.5f, 3.0f));
			// Scale down the plane
			LsButton2->SetScale(glm::vec3(0.5f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = LsButton2->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(LSButtonMaterial);

			// This object is a renderable only, it doesn't have any behaviours or
			// physics bodies attached!
		}

		GameObject::Sptr LsButton3 = scene->CreateGameObject("LSButton3");
		{
			// Set position in the scene
			LsButton3->SetPostion(glm::vec3(1.5f, 0.5f, 3.0f));
			// Scale down the plane
			LsButton3->SetScale(glm::vec3(0.5f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = LsButton3->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(LSButtonMaterial);

			// This object is a renderable only, it doesn't have any behaviours or
			// physics bodies attached!
		}

		GameObject::Sptr LsButton4 = scene->CreateGameObject("LSButton4");
		{
			// Set position in the scene
			LsButton4->SetPostion(glm::vec3(2.1f, 0.5f, 3.0f));
			// Scale down the plane
			LsButton4->SetScale(glm::vec3(0.5f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = LsButton4->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(LSButtonMaterial);

			// This object is a renderable only, it doesn't have any behaviours or
			// physics bodies attached!
		}

		GameObject::Sptr LsButton5 = scene->CreateGameObject("LSButton5");
		{
			// Set position in the scene
			LsButton5->SetPostion(glm::vec3(2.7f, 0.5f, 3.0f));
			// Scale down the plane
			LsButton5->SetScale(glm::vec3(0.5f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = LsButton5->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(LSButtonMaterial);

			// This object is a renderable only, it doesn't have any behaviours or
			// physics bodies attached!
		}

		GameObject::Sptr LsButton6 = scene->CreateGameObject("LSButton6");
		{
			// Set position in the scene
			LsButton6->SetPostion(glm::vec3(0.3f,-0.1f, 3.0f));
			// Scale down the plane
			LsButton6->SetScale(glm::vec3(0.5f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = LsButton6->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(LSButtonMaterial);

			// This object is a renderable only, it doesn't have any behaviours or
			// physics bodies attached!
		}

		GameObject::Sptr LsButton7 = scene->CreateGameObject("LSButton7");
		{
			// Set position in the scene
			LsButton7->SetPostion(glm::vec3(0.9f, -0.1f, 3.0f));
			// Scale down the plane
			LsButton7->SetScale(glm::vec3(0.5f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = LsButton7->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(LSButtonMaterial);

			// This object is a renderable only, it doesn't have any behaviours or
			// physics bodies attached!
		}

		GameObject::Sptr LsButton8 = scene->CreateGameObject("LSButton8");
		{
			// Set position in the scene
			LsButton8->SetPostion(glm::vec3(1.5f, -0.1f, 3.0f));
			// Scale down the plane
			LsButton8->SetScale(glm::vec3(0.5f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = LsButton8->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(LSButtonMaterial);

			// This object is a renderable only, it doesn't have any behaviours or
			// physics bodies attached!
		}

		GameObject::Sptr LsButton9 = scene->CreateGameObject("LSButton9");
		{
			// Set position in the scene
			LsButton9->SetPostion(glm::vec3(2.1f, -0.1f, 3.0f));
			// Scale down the plane
			LsButton9->SetScale(glm::vec3(0.5f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = LsButton9->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(LSButtonMaterial);

			// This object is a renderable only, it doesn't have any behaviours or
			// physics bodies attached!
		}

		GameObject::Sptr LsButton10 = scene->CreateGameObject("LSButton10");
		{
			// Set position in the scene
			LsButton10->SetPostion(glm::vec3(2.7f, -0.1f, 3.0f));
			// Scale down the plane
			LsButton10->SetScale(glm::vec3(0.5f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = LsButton10->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(LSButtonMaterial);

			// This object is a renderable only, it doesn't have any behaviours or
			// physics bodies attached!
		}

		GameObject::Sptr BackButtonBack = scene->CreateGameObject("BackButtonBack");
		{
			// Set position in the scene
			BackButtonBack->SetPostion(glm::vec3(1.75f, -1.0f, 3.0f));
			// Scale down the plane
			BackButtonBack->SetScale(glm::vec3(2.0f, 0.4f, 0.5f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = BackButtonBack->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(ButtonBackMaterial);

			// This object is a renderable only, it doesn't have any behaviours or
			// physics bodies attached!
		}

		GameObject::Sptr LsLogo = scene->CreateGameObject("LSLogo");
		{
			// Set position in the scene
			LsLogo->SetPostion(glm::vec3(1.5f, 1.25f, 3.0f));
			// Scale down the plane
			LsLogo->SetScale(glm::vec3(2.75f, 1.0f, 0.5f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = LsLogo->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(LSLogoMaterial);

			// This object is a renderable only, it doesn't have any behaviours or
			// physics bodies attached!
		}
		
		GameObject::Sptr Back = scene->CreateGameObject("Back");
		{
			Back->SetPostion(glm::vec3(1.3f, -0.75f, 3.5f));
			Back->SetScale(glm::vec3(0.5f, 0.125f, 0.5f));

			RenderComponent::Sptr renderer = Back->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(BackMaterial);
		}

		GameObject::Sptr Num1 = scene->CreateGameObject("Num1");
		{
			Num1->SetPostion(glm::vec3(0.225f, 0.375f, 3.5f));
			Num1->SetScale(glm::vec3(0.25f));

			RenderComponent::Sptr renderer = Num1->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(Material1);
		}

		GameObject::Sptr Num2 = scene->CreateGameObject("Num2");
		{
			Num2->SetPostion(glm::vec3(0.675f, 0.375f, 3.5f));
			Num2->SetScale(glm::vec3(0.25f));

			RenderComponent::Sptr renderer = Num2->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(Material2);
		}

		GameObject::Sptr Num3 = scene->CreateGameObject("Num3");
		{
			Num3->SetPostion(glm::vec3(1.125f, 0.375f, 3.5f));
			Num3->SetScale(glm::vec3(0.25f));

			RenderComponent::Sptr renderer = Num3->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(Material3);
		}

		GameObject::Sptr Num4 = scene->CreateGameObject("Num4");
		{
			Num4->SetPostion(glm::vec3(1.575f, 0.375f, 3.5f));
			Num4->SetScale(glm::vec3(0.25f));

			RenderComponent::Sptr renderer = Num4->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(Material4);
		}

		GameObject::Sptr Num5 = scene->CreateGameObject("Num5");
		{
			Num5->SetPostion(glm::vec3(2.025f, 0.375f, 3.5f));
			Num5->SetScale(glm::vec3(0.25f));

			RenderComponent::Sptr renderer = Num5->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(Material5);
		}

		GameObject::Sptr Num6 = scene->CreateGameObject("Num6");
		{
			Num6->SetPostion(glm::vec3(0.225f, -0.075f, 3.5f));
			Num6->SetScale(glm::vec3(0.25f));

			RenderComponent::Sptr renderer = Num6->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(Material6);
		}

		GameObject::Sptr Num7 = scene->CreateGameObject("Num7");
		{
			Num7->SetPostion(glm::vec3(0.675f, -0.075f, 3.5f));
			Num7->SetScale(glm::vec3(0.25f));

			RenderComponent::Sptr renderer = Num7->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(Material7);
		}

		GameObject::Sptr Num8 = scene->CreateGameObject("Num8");
		{
			Num8->SetPostion(glm::vec3(1.125f, -0.075f, 3.5f));
			Num8->SetScale(glm::vec3(0.25f));

			RenderComponent::Sptr renderer = Num8->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(Material8);
		}

		GameObject::Sptr Num9 = scene->CreateGameObject("Num9");
		{
			Num9->SetPostion(glm::vec3(1.575f, -0.075f, 3.5f));
			Num9->SetScale(glm::vec3(0.25f));

			RenderComponent::Sptr renderer = Num9->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(Material9);
		}

		GameObject::Sptr Num10 = scene->CreateGameObject("Num10");
		{
			Num10->SetPostion(glm::vec3(2.025f, -0.075f, 3.5f));
			Num10->SetScale(glm::vec3(0.25f));

			RenderComponent::Sptr renderer = Num10->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(Material10);
		}

		// Save the asset manifest for all the resources we just loaded
		ResourceManager::SaveManifest("manifest.json");
		// Save the scene to a JSON file
		scene->Save("LS.json");

		}


		scene = Scene::Load("menu.json");
	}

	// Call scene awake to start up all of our components
	scene->Window = window;
	scene->Awake();

	// We'll use this to allow editing the save/load path
	// via ImGui, note the reserve to allocate extra space
	// for input!
	std::string scenePath = "scene.json"; 
	scenePath.reserve(256); 

	bool isRotating = true;
	float rotateSpeed = 90.0f;

	// Our high-precision timer
	double lastFrame = glfwGetTime();


	BulletDebugMode physicsDebugMode = BulletDebugMode::None;
	float playbackSpeed = 2.0f;

	nlohmann::json editorSceneState;


	result = system->playSound(sound4, 0, false, &channel);
	bool isEscapePressed = false;

	float ProgressBarTime = 0; //will calculate the time from the beginning to the end of the level
	float ProgressBarTemp = 0; //temp value so we can calculate time elapsed from beginning to end of level
	float ProgressBarTempPaused = 0;
	float ProgressBarTempPaused2 = 0;
	float ProgressBarPaused = 0;

	///// Game loop /////
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		ImGuiHelper::StartFrame();

		//SDL_GL_Set

		/// test FMOD
		if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
			{
			result = system->playSound(sound1, 0, false, &channel);
			std::cout << "K pressed" << std::endl;
			}
		if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
		{
			result = system->playSound(sound2, 0, false, &channel);
			std::cout << "J pressed" << std::endl;
		}
		if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS)
		{
			result = system->playSound(sound3, 0, false, &channel);
			std::cout << "H pressed" << std::endl;
		}


	

		/// with this change to the check, switching between scenes using scenePath no longer causes the game to crash since if the scene doesn't have a player it wont prompt commands
		if (scene->FindObjectByName("player") != NULL)
		{
			if (paused == true)
			{
				scene->FindObjectByName("PanelPause")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 6, 6.5));
				scene->FindObjectByName("ButtonBack1")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 6.25, 6.0));
				scene->FindObjectByName("ButtonBack2")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 6.5, 5.0));
				scene->FindObjectByName("ResumeText")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 8, 6.1));
				scene->FindObjectByName("MainMenuText")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 8, 5.4));
				scene->FindObjectByName("PauseLogo")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 5.75, 8.0));
				ProgressBarTempPaused = ProgressBarTime;


				if (index == 1)
				{
					scene->FindObjectByName("Filter")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 6.26, 6.0));
				}
				else if (index == 2)
				{
					scene->FindObjectByName("Filter")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 6.51, 5.0));
				}

			}
			
			

			if (playerLose == true)
			{
				scene->FindObjectByName("PanelPause")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 6, 6.5));
				scene->FindObjectByName("ButtonBack1")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 6.25, 6.0));
				scene->FindObjectByName("ButtonBack2")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 6.5, 5.0));
				scene->FindObjectByName("ReplayText")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 8, 6.1));
				scene->FindObjectByName("MainMenuText")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 8, 5.4));
				scene->FindObjectByName("LoserLogo")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 5.75, 8.0));
				ProgressBarTemp = glfwGetTime();

				if (index == 1)
				{
					scene->FindObjectByName("Filter")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 6.26, 6.0));
				}
				else if (index == 2)
				{
					scene->FindObjectByName("Filter")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 6.51, 5.0));
				}
			}

			if (playerWin == true)
			{
				scene->FindObjectByName("PanelPause")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 6, 6.5));
				scene->FindObjectByName("ButtonBack1")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 6.25, 6.0));
				scene->FindObjectByName("ButtonBack2")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 6.5, 5.0));
				scene->FindObjectByName("ReplayText")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 8, 6.1));
				scene->FindObjectByName("MainMenuText")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 8, 5.4));
				scene->FindObjectByName("WinnerLogo")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 5.75, 8.0));
				ProgressBarTemp = glfwGetTime();

if (index == 1)
{
	scene->FindObjectByName("Filter")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 6.26, 6.0));
}
else if (index == 2)
{
	scene->FindObjectByName("Filter")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 6.51, 5.0));
}
			}

			if (paused != true && playerLose != true && playerWin != true)
			{
				//originally these were all back at -15 but idk if that makes the game more jank cause of overlap so i tried to spread em out
				scene->FindObjectByName("PanelPause")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, -15, -25));
				scene->FindObjectByName("ButtonBack1")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, -20, -26));
				scene->FindObjectByName("ButtonBack2")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, -22, -27));
				scene->FindObjectByName("ResumeText")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, -23, -28));
				scene->FindObjectByName("MainMenuText")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 24, -15, -29));
				scene->FindObjectByName("PauseLogo")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, -25, -30));
				scene->FindObjectByName("Filter")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, -26, -31));
				scene->FindObjectByName("LoserLogo")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, -27, -32));
				scene->FindObjectByName("ReplayText")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, -28, -33));
				scene->FindObjectByName("WinnerLogo")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, -29, -34));
			}

			if (paused == true || playerLose == true || playerWin == true)
			{
				if (((glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)) && soundprompt == false)
				{
					result = system->playSound(sound1, 0, false, &channel);
					soundprompt = true;
				}

				if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_UP) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
				{
					soundprompt = false;
				}

				if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS && soundprompt == false)
				{
					result = system->playSound(sound2, 0, false, &channel);
					soundprompt = true;
				}
			}
			//std::cout << GLFW_REFRESH_RATE;
			if (paused == false) {
				ProgressBarTime = glfwGetTime() - ProgressBarTemp;
				ProgressBarTime = ProgressBarTime / 2.5;
			}
			std::cout << ProgressBarTime << "\n";

			scene->FindObjectByName("Main Camera")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 11.480, 6.290)); // makes the camera follow the player
			scene->FindObjectByName("Main Camera")->SetRotation(glm::vec3(84, 0, -180)); //angled view (stops camera from rotating)
			scene->FindObjectByName("player")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x, 0.f, scene->FindObjectByName("player")->GetPosition().z)); // makes the camera follow the player

			scene->FindObjectByName("ProgressBarGO")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 1.620, 13)); //makes progress bar follow the player
			scene->FindObjectByName("ProgressBarProgress")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x + 2 - ProgressBarTime, 1.7, 12.75)); //Makes Progress of progress bar follow the player


			//Stops the player from rotating
			scene->FindObjectByName("player")->SetRotation(glm::vec3(90.f, scene->FindObjectByName("player")->GetRotation().y, 0.f));

			keyboard();
			if (soundprompt == false && glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && (paused != true && playerLose != true && playerWin != true))
			{
				result = system->playSound(sound3, 0, false, &channel);
				soundprompt = true;
			}
			else if(glfwGetKey(window, GLFW_KEY_UP) == GLFW_RELEASE && (paused != true && playerLose != true && playerWin != true))
			{
				soundprompt = false;
			}


			//collisions system
			for (std::vector<int>::size_type i = 0; i != collisions.size(); i++) {
				if (collisions[i].id == 0) {
					collisions[i].update(scene->FindObjectByName("player")->GetPosition());
				}
				if (collisions[i].id == 1) {
					collisions[i].update(scene->FindObjectByName("Trigger2")->GetPosition());
				}
			}

			for (std::vector<int>::size_type i = 0; i != collisions.size(); i++) {
				playerCollision.rectOverlap(playerCollision, collisions[i]); //changed ballcollision to playercollision
			}

			if (playerCollision.hitEntered == true) {
				scene->FindObjectByName("player")->SetPostion(glm::vec3(6.f, 0.f, scene->FindObjectByName("player")->GetPosition().z));
				std::cout << "colision detected";
				playerCollision.hitEntered = false;
				playerMove = false;
				playerLose = true;
			}
			//JumpBehaviour test;
			//test.Update();

			playerCollision.update(scene->FindObjectByName("player")->GetPosition()); // to update
			scene->FindObjectByName("player")->Get<JumpBehaviour>()->getPlayerCoords(scene->FindObjectByName("player")->GetPosition()); //send the players coordinates to JumpBehavior so we know when the player is on the ground

			if (scene->FindObjectByName("player")->GetPosition().x < -400)
			{
				scene->FindObjectByName("player")->SetPostion(glm::vec3(6.f, 0.f, scene->FindObjectByName("player")->GetPosition().z));
				playerMove = false;
				playerWin = true;
			}

		}
		else
		{
		SceneChanger();

		if (((glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)) && soundprompt == false)
		{
			result = system->playSound(sound1, 0, false, &channel);
			soundprompt = true;
		}

		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_UP) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_RELEASE)
		{
			soundprompt = false;
		}

		if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS && soundprompt == false)
		{
			result = system->playSound(sound2, 0, false, &channel);
			soundprompt = true;
		}

		}
		
		// Calculate the time since our last frame (dt)
		double thisFrame = glfwGetTime();
		float dt = static_cast<float>(thisFrame - lastFrame);

		// Showcasing how to use the imGui library!
		bool isDebugWindowOpen = ImGui::Begin("Debugging");
		if (isDebugWindowOpen) {
			// Draws a button to control whether or not the game is currently playing
			static char buttonLabel[64];
			sprintf_s(buttonLabel, "%s###playmode", scene->IsPlaying ? "Exit Play Mode" : "Enter Play Mode");
			if (ImGui::Button(buttonLabel)) {
			//if (glfwGetKey(window, GLFW_KEY_UP)){
			// Save scene so it can be restored when exiting play mode
				if (!scene->IsPlaying) {
					editorSceneState = scene->ToJson();
				}

				// Toggle state
				scene->IsPlaying = !scene->IsPlaying;

				// If we've gone from playing to not playing, restore the state from before we started playing
				if (!scene->IsPlaying) {
					scene = nullptr;
					// We reload to scene from our cached state
					scene = Scene::FromJson(editorSceneState);
					// Don't forget to reset the scene's window and wake all the objects!
					scene->Window = window;
					scene->Awake();
				}
			}


			/// If your game gets reaaaally chunky take this code out
			if (glfwGetKey(window, GLFW_KEY_UP) && scene->IsPlaying == false) {
				scene->IsPlaying = true;
			}
			//////

			// Make a new area for the scene saving/loading
			ImGui::Separator();
			if (DrawSaveLoadImGui(scene, scenePath)) {
				// C++ strings keep internal lengths which can get annoying
				// when we edit it's underlying datastore, so recalcualte size
				scenePath.resize(strlen(scenePath.c_str()));

				// We have loaded a new scene, call awake to set
				// up all our components
				scene->Window = window;
				scene->Awake();
			}
			ImGui::Separator();
			// Draw a dropdown to select our physics debug draw mode
			if (BulletDebugDraw::DrawModeGui("Physics Debug Mode:", physicsDebugMode)) {
				scene->SetPhysicsDebugDrawMode(physicsDebugMode);
			}
			LABEL_LEFT(ImGui::SliderFloat, "Playback Speed:    ", &playbackSpeed, 0.0f, 10.0f);
			ImGui::Separator();
		}

		// Clear the color and depth buffers
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Update our application level uniforms every frame

		// Draw some ImGui stuff for the lights
		if (isDebugWindowOpen) {
			for (int ix = 0; ix < scene->Lights.size(); ix++) {
				char buff[256];
				sprintf_s(buff, "Light %d##%d", ix, ix);
				// DrawLightImGui will return true if the light was deleted
				if (DrawLightImGui(scene, buff, ix)) {
					// Remove light from scene, restore all lighting data
					scene->Lights.erase(scene->Lights.begin() + ix);
					scene->SetupShaderAndLights();
					// Move back one element so we don't skip anything!
					ix--;
				}
			}
			// As long as we don't have max lights, draw a button
			// to add another one
			if (scene->Lights.size() < scene->MAX_LIGHTS) {
				if (ImGui::Button("Add Light")) {
					scene->Lights.push_back(Light());
					scene->SetupShaderAndLights();
				}
			}
			// Split lights from the objects in ImGui
			ImGui::Separator();
		}

		dt *= playbackSpeed;

		// Perform updates for all components
		scene->Update(dt);

		// Grab shorthands to the camera and shader from the scene
		Camera::Sptr camera = scene->MainCamera;

		// Cache the camera's viewprojection
		glm::mat4 viewProj = camera->GetViewProjection();
		DebugDrawer::Get().SetViewProjection(viewProj);

		// Update our worlds physics!
		scene->DoPhysics(dt);

		// Draw object GUIs
		if (isDebugWindowOpen) {
			scene->DrawAllGameObjectGUIs();
		}
		
		// The current material that is bound for rendering
		Material::Sptr currentMat = nullptr;
		Shader::Sptr shader = nullptr;

		// Render all our objects
		ComponentManager::Each<RenderComponent>([&](const RenderComponent::Sptr& renderable) {

			// If the material has changed, we need to bind the new shader and set up our material and frame data
			// Note: This is a good reason why we should be sorting the render components in ComponentManager
			if (renderable->GetMaterial() != currentMat) {
				currentMat = renderable->GetMaterial();
				shader = currentMat->MatShader;

				shader->Bind();
				shader->SetUniform("u_CamPos", scene->MainCamera->GetGameObject()->GetPosition());
				currentMat->Apply();
			}

			// Grab the game object so we can do some stuff with it
			GameObject* object = renderable->GetGameObject();

			// Set vertex shader parameters
			shader->SetUniformMatrix("u_ModelViewProjection", viewProj * object->GetTransform());
			shader->SetUniformMatrix("u_Model", object->GetTransform());
			shader->SetUniformMatrix("u_NormalMatrix", glm::mat3(glm::transpose(glm::inverse(object->GetTransform()))));

			// Draw the object
			renderable->GetMesh()->Draw();
		});


		// End our ImGui window
		ImGui::End();

		VertexArrayObject::Unbind();

		lastFrame = thisFrame;
		ImGuiHelper::EndFrame();
		glfwSwapBuffers(window);
	}

	// Clean up the ImGui library
	ImGuiHelper::Cleanup();

	// Clean up the resource manager
	ResourceManager::Cleanup();

	// Clean up the toolkit logger so we don't leak memory
	Logger::Uninitialize();
	return 0;
}