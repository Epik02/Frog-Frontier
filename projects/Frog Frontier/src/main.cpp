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
std::string windowTitle = "INFR-1350U";

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

	

	if (glfwGetKey(window, GLFW_KEY_ENTER) && scene->FindObjectByName("player") == NULL && scene->FindObjectByName("Filter") != NULL) {

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

		return true;
	}
	else if (glfwGetKey(window, GLFW_KEY_ENTER) && scene->FindObjectByName("player") == NULL && scene->FindObjectByName("Filter") == NULL)
	{
		path = "menu.json";
		SceneLoad(scene, path);

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

GameObject::Sptr createGroundObstacle(std::string num, glm::vec3 position, float scale, glm::vec3 rotation, MeshResource::Sptr Mesh, Material::Sptr& Material1) {

	MeshResource::Sptr cubeMesh = ResourceManager::CreateAsset<MeshResource>("Mushroom.obj");
	Texture2D::Sptr    boxTexture = ResourceManager::CreateAsset<Texture2D>("textures/MushroomUV.png");

	Material::Sptr rockMaterial = ResourceManager::CreateAsset<Material>();
	{
		rockMaterial->Name = "Box";
		rockMaterial->MatShader = scene->BaseShader;
		rockMaterial->Texture = boxTexture;
		rockMaterial->Shininess = 2.0f;
	}
	GameObject::Sptr nextObstacle = scene->CreateGameObject("Obstacle" + num);
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

GameObject::Sptr createCollision(std::string num, float x, float z, float xscale, float yscale) {

	MeshResource::Sptr cubeMesh = ResourceManager::CreateAsset<MeshResource>("cube.obj");
	Texture2D::Sptr    boxTexture = ResourceManager::CreateAsset<Texture2D>("textures/box.png");

	Material::Sptr rockMaterial = ResourceManager::CreateAsset<Material>();
	{
		rockMaterial->Name = "Box";
		rockMaterial->MatShader = scene->BaseShader;
		rockMaterial->Texture = boxTexture;
		rockMaterial->Shininess = 2.0f;
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
		renderer->SetMaterial(rockMaterial);

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
bool performedtask = false;

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

void keyboard() {

	

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
		scene->FindObjectByName("player")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 0.2f, scene->FindObjectByName("player")->GetPosition().y, scene->FindObjectByName("player")->GetPosition().z)); // makes the camera follow the player
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

	////displays win screen
	//if (scene->FindObjectByName("player")->GetPosition().x < -47.f) {
	//	scene->FindObjectByName("plane5")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 4, 3.2f, 6.3f));
	//	scene->FindObjectByName("plane5")->SetRotation(glm::vec3(45.f, 0.0f, -180.f));
	//	scene->FindObjectByName("plane5")->SetScale(glm::vec3(36.f, 29.4f, 50.f));
	//}
	//std::cout << clickCount;
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

			Material::Sptr rockMaterial = ResourceManager::CreateAsset<Material>();
			{
				rockMaterial->Name = "lbo";
				rockMaterial->MatShader = scene->BaseShader;
				rockMaterial->Texture = rockTexture;
				rockMaterial->Shininess = 2.0f;
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

			// Set up all our sample objects
			GameObject::Sptr plane = scene->CreateGameObject("Plane");
			{
				// Scale up the plane
				plane->SetPostion(glm::vec3(0.060f, 3.670f, -0.430f));
				plane->SetScale(glm::vec3(47.880f, 23.7f, 48.38f));

				// Create and attach a RenderComponent to the object to draw our mesh
				RenderComponent::Sptr renderer = plane->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(grassMaterial);

				// Attach a plane collider that extends infinitely along the X/Y axis
				RigidBody::Sptr physics = plane->Add<RigidBody>(/*static by default*/);
				physics->AddCollider(PlaneCollider::Create());
			}
			GameObject::Sptr plane2 = scene->CreateGameObject("Plane2");
			{
				// Scale up the plane
				plane2->SetPostion(glm::vec3(-47.820f, 3.670f, -0.430f));
				plane2->SetScale(glm::vec3(47.880f, 23.7f, 48.38f));

				// Create and attach a RenderComponent to the object to draw our mesh
				RenderComponent::Sptr renderer = plane2->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(grassMaterial);

				// Attach a plane collider that extends infinitely along the X/Y axis
				RigidBody::Sptr physics = plane2->Add<RigidBody>(/*static by default*/);
				physics->AddCollider(PlaneCollider::Create());
			}
			GameObject::Sptr plane3 = scene->CreateGameObject("Plane3");
			{
				// Scale up the plane
				plane3->SetPostion(glm::vec3(-7.880f, -7.540f, 8.220f));
				plane3->SetRotation(glm::vec3(87.0F, 0.f, -180.0f));
				plane3->SetScale(glm::vec3(63.390F, 17.610f, 52.54f));

				// Create and attach a RenderComponent to the object to draw our mesh
				RenderComponent::Sptr renderer = plane3->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(bgMaterial);

				// Attach a plane collider that extends infinitely along the X/Y axis
				RigidBody::Sptr physics = plane3->Add<RigidBody>(/*static by default*/);
				physics->AddCollider(PlaneCollider::Create());
			}
			GameObject::Sptr plane4 = scene->CreateGameObject("Plane4");
			{
				// Scale up the plane
				plane4->SetPostion(glm::vec3(-71.260f, -7.540f, 8.220f));
				plane4->SetRotation(glm::vec3(87.0F, 0.f, -180.0f));
				plane4->SetScale(glm::vec3(63.390F, 17.610f, 52.54f));

				// Create and attach a RenderComponent to the object to draw our mesh
				RenderComponent::Sptr renderer = plane4->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(bgMaterial);

				// Attach a plane collider that extends infinitely along the X/Y axis
				RigidBody::Sptr physics = plane4->Add<RigidBody>(/*static by default*/);
				physics->AddCollider(PlaneCollider::Create());
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
			GameObject::Sptr plane6 = scene->CreateGameObject("Plane6");
			{
				// Scale up the plane
				plane6->SetPostion(glm::vec3(-95.680f, 3.670f, -0.430f));
				plane6->SetScale(glm::vec3(47.880f, 23.7f, 48.38f));

				// Create and attach a RenderComponent to the object to draw our mesh
				RenderComponent::Sptr renderer = plane6->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(grassMaterial);

				// Attach a plane collider that extends infinitely along the X/Y axis
				RigidBody::Sptr physics = plane6->Add<RigidBody>(/*static by default*/);
				physics->AddCollider(PlaneCollider::Create());
			}

			GameObject::Sptr plane7 = scene->CreateGameObject("Plane7");
			{
				// Scale up the plane
				plane7->SetPostion(glm::vec3(-53.280f, -7.540f, 32.610f));
				plane7->SetRotation(glm::vec3(87.0F, 0.f, -180.0f));
				plane7->SetScale(glm::vec3(200.F, 33.050f, 54.550f));

				// Create and attach a RenderComponent to the object to draw our mesh
				RenderComponent::Sptr renderer = plane7->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(greenMaterial);

				// Attach a plane collider that extends infinitely along the X/Y axis
				RigidBody::Sptr physics = plane7->Add<RigidBody>(/*static by default*/);
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
				renderer->SetMaterial(rockMaterial);

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
			createGroundObstacle("2", glm::vec3(-15.f, 0.0f, -0.660), 0.5, glm::vec3(90.f, 0.0f, 0.0f), mushroomMesh, mushroomMaterial);
			createGroundObstacle("3", glm::vec3(-37.930f, 0.0f, -0.750), 1.2f, glm::vec3(90.f, 0.0f, 73.f), vinesMesh, vinesMaterial);
			createGroundObstacle("4", glm::vec3(-55.970f, 0.0f, 1.730f), 0.2f, glm::vec3(0.0f, 0.0f, -45.f), cobwebMesh, cobwebMaterial);

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

								/// Testing to make a second scene /// 
										/// it works poggers ///

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

		Material::Sptr rockMaterial = ResourceManager::CreateAsset<Material>();
		{
			rockMaterial->Name = "lbo";
			rockMaterial->MatShader = scene->BaseShader;
			rockMaterial->Texture = rockTexture;
			rockMaterial->Shininess = 2.0f;
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
		scene->Lights.resize(18);
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

		scene->Lights[5].Position = glm::vec3(-80.0f, 1.0f, 40.0f);
		scene->Lights[5].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[5].Range = 1000.0f;

		scene->Lights[6].Position = glm::vec3(-120.0f, 1.0f, 40.0f);
		scene->Lights[6].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[6].Range = 1000.0f;

		scene->Lights[7].Position = glm::vec3(-160.0f, 1.0f, 40.0f);
		scene->Lights[7].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[7].Range = 1000.0f;

		scene->Lights[8].Position = glm::vec3(-200.0f, 1.0f, 40.0f);
		scene->Lights[8].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[8].Range = 1000.0f;

		scene->Lights[9].Position = glm::vec3(-240.0f, 1.0f, 40.0f);
		scene->Lights[9].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[9].Range = 1000.0f;

		scene->Lights[10].Position = glm::vec3(-280.0f, 1.0f, 40.0f);
		scene->Lights[10].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[10].Range = 1000.0f;

		scene->Lights[11].Position = glm::vec3(-320.0f, 1.0f, 40.0f);
		scene->Lights[11].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[11].Range = 1000.0f;

		scene->Lights[12].Position = glm::vec3(-360.0f, 1.0f, 40.0f);
		scene->Lights[12].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[12].Range = 1000.0f;

		scene->Lights[13].Position = glm::vec3(-400.0f, 1.0f, 40.0f);
		scene->Lights[13].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[13].Range = 1000.0f;

		scene->Lights[14].Position = glm::vec3(-200.0f, 1.0f, 40.0f);
		scene->Lights[14].Color = glm::vec3(0.2f, 0.8f, 0.1f);
		scene->Lights[14].Range = 1000.0f;

		scene->Lights[15].Position = glm::vec3(-201.0f, 0.0f, 3.0f);
		scene->Lights[15].Color = glm::vec3(0.2f, 0.8f, 0.1f);

		scene->Lights[16].Position = glm::vec3(-400.0f, 1.0f, 40.0f);
		scene->Lights[16].Color = glm::vec3(0.2f, 0.8f, 0.1f);
		scene->Lights[16].Range = 1000.0f;

		scene->Lights[17].Position = glm::vec3(-401.0f, 0.0f, 3.0f);
		scene->Lights[17].Color = glm::vec3(0.2f, 0.8f, 0.1f);

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

		// Set up all our sample objects
		GameObject::Sptr ground = scene->CreateGameObject("Ground1");
		{
			//groud 1
			// Scale up the plane
			ground->SetPostion(glm::vec3(0.060f, 3.670f, -0.430f));
			ground->SetScale(glm::vec3(47.880f, 23.7f, 48.38f));

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = ground->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(grassMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = ground->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(PlaneCollider::Create());
		}
		GameObject::Sptr ground2 = scene->CreateGameObject("Ground2");
		{
			//ground 2
			// Scale up the plane
			ground2->SetPostion(glm::vec3(-47.820f, 3.670f, -0.430f));
			ground2->SetScale(glm::vec3(47.880f, 23.7f, 48.38f));

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = ground2->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(grassMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = ground2->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(PlaneCollider::Create());
		}

		GameObject::Sptr ground3 = scene->CreateGameObject("Ground3");
		{
			//ground 3
			// Scale up the plane
			ground3->SetPostion(glm::vec3(-95.680f, 3.670f, -0.430f));
			ground3->SetScale(glm::vec3(47.880f, 23.7f, 48.38f));

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = ground3->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(grassMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = ground3->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(PlaneCollider::Create());
		}

		GameObject::Sptr ground4 = scene->CreateGameObject("Ground4");
		{
			//ground 4
			// Scale up the plane
			ground4->SetPostion(glm::vec3(-143.54f, 3.670f, -0.430f));
			ground4->SetScale(glm::vec3(47.880f, 23.7f, 48.38f));

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = ground4->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(grassMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = ground4->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(PlaneCollider::Create());
		}

		GameObject::Sptr ground5 = scene->CreateGameObject("Ground5");
		{
			//ground 5
			// Scale up the plane
			ground5->SetPostion(glm::vec3(-191.4f, 3.670f, -0.430f));
			ground5->SetScale(glm::vec3(47.880f, 23.7f, 48.38f));

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = ground5->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(grassMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = ground5->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(PlaneCollider::Create());
		}

		GameObject::Sptr ground6 = scene->CreateGameObject("Ground6");
		{
			//ground 6
			// Scale up the plane
			ground6->SetPostion(glm::vec3(-239.26f, 3.670f, -0.430f));
			ground6->SetScale(glm::vec3(47.880f, 23.7f, 48.38f));

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = ground6->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(grassMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = ground6->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(PlaneCollider::Create());
		}

		GameObject::Sptr ground7 = scene->CreateGameObject("Ground7");
		{
			//ground 7
			// Scale up the plane
			ground7->SetPostion(glm::vec3(-287.12f, 3.670f, -0.430f));
			ground7->SetScale(glm::vec3(47.880f, 23.7f, 48.38f));

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = ground7->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(grassMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = ground7->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(PlaneCollider::Create());
		}

		GameObject::Sptr ground8 = scene->CreateGameObject("Ground8");
		{
			//ground 8
			// Scale up the plane
			ground8->SetPostion(glm::vec3(-334.98f, 3.670f, -0.430f));
			ground8->SetScale(glm::vec3(47.880f, 23.7f, 48.38f));

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = ground8->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(grassMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = ground8->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(PlaneCollider::Create());
		}

		GameObject::Sptr ground9 = scene->CreateGameObject("Ground9");
		{
			//ground 9
			// Scale up the plane
			ground9->SetPostion(glm::vec3(-382.84f, 3.670f, -0.430f));
			ground9->SetScale(glm::vec3(47.880f, 23.7f, 48.38f));

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = ground9->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(grassMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = ground9->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(PlaneCollider::Create());
		}

		GameObject::Sptr ground10 = scene->CreateGameObject("Ground10");
		{
			//ground 10
			// Scale up the plane
			ground10->SetPostion(glm::vec3(-430.7f, 3.670f, -0.430f));
			ground10->SetScale(glm::vec3(47.880f, 23.7f, 48.38f));

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = ground10->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(grassMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = ground10->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(PlaneCollider::Create());
		}

		GameObject::Sptr ground0 = scene->CreateGameObject("Ground0");
		{
			//ground 0
			// Scale up the plane
			ground0->SetPostion(glm::vec3(47.92f, 3.670f, -0.430f));
			ground0->SetScale(glm::vec3(47.880f, 23.7f, 48.38f));

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = ground0->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(grassMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = ground0->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(PlaneCollider::Create());
		}

		GameObject::Sptr wall0 = scene->CreateGameObject("Wall0");
		{
			//wall 1
			// Scale up the plane
			wall0->SetPostion(glm::vec3(55.900f, -7.540f, 11.0f));
			wall0->SetRotation(glm::vec3(87.0F, 0.f, -180.0f));
			wall0->SetScale(glm::vec3(63.390F, 24.f, 52.54f));

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = wall0->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(bgMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = wall0->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(PlaneCollider::Create());
		}

		GameObject::Sptr wall1 = scene->CreateGameObject("Wall1");
		{
			//wall 1
			// Scale up the plane
			wall1->SetPostion(glm::vec3(-7.500f, -7.540f, 11.0f));
			wall1->SetRotation(glm::vec3(87.0F, 0.f, -180.0f));
			wall1->SetScale(glm::vec3(63.390F, 24.f, 52.54f));

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = wall1->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(bgMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = wall1->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(PlaneCollider::Create());
		}
		GameObject::Sptr wall2 = scene->CreateGameObject("Wall2");
		{
			//wall 2
			// Scale up the plane
			wall2->SetPostion(glm::vec3(-70.9f, -7.540f, 11.f));
			wall2->SetRotation(glm::vec3(87.0F, 0.f, -180.0f));
			wall2->SetScale(glm::vec3(63.390F, 24.f, 52.54f));

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = wall2->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(bgMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = wall2->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(PlaneCollider::Create());
		}

		GameObject::Sptr wall3 = scene->CreateGameObject("Wall3");
		{
			//wall 1
			// Scale up the plane
			wall3->SetPostion(glm::vec3(-134.3f, -7.540f, 11.0f));
			wall3->SetRotation(glm::vec3(87.0F, 0.f, -180.0f));
			wall3->SetScale(glm::vec3(63.390F, 24.f, 52.54f));

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = wall3->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(bgMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = wall3->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(PlaneCollider::Create());
		}

		GameObject::Sptr wall4 = scene->CreateGameObject("Wall4");
		{
			//wall 1
			// Scale up the plane
			wall4->SetPostion(glm::vec3(-197.7f, -7.540f, 11.0f));
			wall4->SetRotation(glm::vec3(87.0F, 0.f, -180.0f));
			wall4->SetScale(glm::vec3(63.390F, 24.f, 52.54f));

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = wall4->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(bgMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = wall4->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(PlaneCollider::Create());
		}

		GameObject::Sptr wall5 = scene->CreateGameObject("Wall5");
		{
			//wall 5
			// Scale up the plane
			wall5->SetPostion(glm::vec3(-261.1f, -7.540f, 11.0f));
			wall5->SetRotation(glm::vec3(87.0F, 0.f, -180.0f));
			wall5->SetScale(glm::vec3(63.390F, 24.f, 52.54f));

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = wall5->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(bgMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = wall5->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(PlaneCollider::Create());
		}

		GameObject::Sptr wall6 = scene->CreateGameObject("Wall6");
		{
			//wall 6
			// Scale up the plane
			wall6->SetPostion(glm::vec3(-324.5f, -7.540f, 11.0f));
			wall6->SetRotation(glm::vec3(87.0F, 0.f, -180.0f));
			wall6->SetScale(glm::vec3(63.390F, 24.f, 52.54f));

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = wall6->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(bgMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = wall6->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(PlaneCollider::Create());
		}

		GameObject::Sptr wall7 = scene->CreateGameObject("Wall7");
		{
			//wall 7
			// Scale up the plane
			wall7->SetPostion(glm::vec3(-387.9f, -7.540f, 11.0f));
			wall7->SetRotation(glm::vec3(87.0F, 0.f, -180.0f));
			wall7->SetScale(glm::vec3(63.390F, 24.f, 52.54f));

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = wall7->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(bgMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = wall7->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(PlaneCollider::Create());
		}

		GameObject::Sptr wall8 = scene->CreateGameObject("Wall8");
		{
			//wall 8
			// Scale up the plane
			wall8->SetPostion(glm::vec3(-451.3f, -7.540f, 11.0f));
			wall8->SetRotation(glm::vec3(87.0F, 0.f, -180.0f));
			wall8->SetScale(glm::vec3(63.390F, 24.f, 52.54f));

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = wall8->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(bgMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = wall8->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(PlaneCollider::Create());
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
			renderer->SetMaterial(rockMaterial);

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
		createGroundObstacle("2", glm::vec3(-20.f, 0.0f, -0.660), 0.5, glm::vec3(90.f, 0.0f, 0.0f), mushroomMesh, mushroomMaterial); //mushroom 1 (small jump)
		createGroundObstacle("3", glm::vec3(-60.f, 0.0f, 3.0), 1.0, glm::vec3(90.f, 0.0f, 73.f), vinesMesh, vinesMaterial); // vine 1 (jump blocking)
		createGroundObstacle("4", glm::vec3(-110.f, 0.0f, 3.3f), 0.25f, glm::vec3(0.0f, 0.0f, -75.f), cobwebMesh, cobwebMaterial); //cobweb 1 (tall jump)
		createGroundObstacle("5", glm::vec3(-45.f, 0.0f, -0.660), 0.5, glm::vec3(90.f, 0.0f, 0.0f), mushroomMesh, mushroomMaterial); //mushroom 2
		createGroundObstacle("6", glm::vec3(-150.f, 5.530f, 0.250f), 1.5, glm::vec3(90.f, 0.0f, -25.f), vinesMesh, vinesMaterial); // vine 2 (squish blocking)
		createGroundObstacle("7", glm::vec3(-150.240f, 0.f, 7.88f), 0.25f, glm::vec3(0.0f, 0.0f, 84.f), cobwebMesh, cobwebMaterial); //cobweb 2 (squish Blocking 2)

		createGroundObstacle("8", glm::vec3(-170.f, 0.0f, -0.660), 0.5, glm::vec3(90.f, 0.0f, 0.0f), mushroomMesh, mushroomMaterial); //mushroom 3 (small jump)
		createGroundObstacle("9", glm::vec3(-200.f, 0.0f, 3.0), 1.0, glm::vec3(90.f, 0.0f, 73.f), vinesMesh, vinesMaterial); // vine 3 (jump blocking)
		createGroundObstacle("10", glm::vec3(-220.f, 0.0f, 3.3f), 0.25f, glm::vec3(0.0f, 0.0f, -75.f), cobwebMesh, cobwebMaterial); //cobweb 3 (tall jump)
		createGroundObstacle("11", glm::vec3(-230.f, 0.0f, -0.660), 0.5, glm::vec3(90.f, 0.0f, 0.0f), mushroomMesh, mushroomMaterial); //mushroom 4 (small jump)
		createGroundObstacle("12", glm::vec3(-250.f, 0.0f, 3.0), 1.0, glm::vec3(90.f, 0.0f, 73.f), vinesMesh, vinesMaterial); // vine 4 (jump blocking)
		createGroundObstacle("13", glm::vec3(-275.f, 0.0f, -0.660), 0.5, glm::vec3(90.f, 0.0f, 0.0f), mushroomMesh, mushroomMaterial); //mushroom 5 (small jump)
		createGroundObstacle("14", glm::vec3(-300.f, 5.530f, 0.250f), 1.5, glm::vec3(90.f, 0.0f, -25.f), vinesMesh, vinesMaterial); // vine 5 (squish blocking)
		createGroundObstacle("15", glm::vec3(-300.240f, 0.f, 7.88f), 0.25f, glm::vec3(0.0f, 0.0f, 84.f), cobwebMesh, cobwebMaterial); //cobweb 4 (squish Blocking 2)

		createGroundObstacle("16", glm::vec3(-310.f, 0.0f, -0.660), 0.5, glm::vec3(90.f, 0.0f, 0.0f), mushroomMesh, mushroomMaterial); //mushroom 6 (small jump)
		createGroundObstacle("17", glm::vec3(-315.f, 0.0f, -0.660), 0.5, glm::vec3(90.f, 0.0f, 0.0f), mushroomMesh, mushroomMaterial); //mushroom 7 (small jump)
		createGroundObstacle("18", glm::vec3(-320.f, 0.0f, -0.660), 0.5, glm::vec3(90.f, 0.0f, 0.0f), mushroomMesh, mushroomMaterial); //mushroom 8 (small jump)
		createGroundObstacle("19", glm::vec3(-325.f, 0.0f, 3.3f), 0.25f, glm::vec3(0.0f, 0.0f, -75.f), cobwebMesh, cobwebMaterial); //cobweb 5 (tall jump)
		createGroundObstacle("20", glm::vec3(-340.f, 0.0f, 3.0), 1.0, glm::vec3(90.f, 0.0f, 73.f), vinesMesh, vinesMaterial); // vine 6 (jump blocking)
		createGroundObstacle("21", glm::vec3(-345.f, 5.530f, 0.250f), 1.5, glm::vec3(90.f, 0.0f, -25.f), vinesMesh, vinesMaterial); // vine 7 (squish blocking)
		createGroundObstacle("22", glm::vec3(-345.240f, 0.f, 7.88f), 0.25f, glm::vec3(0.0f, 0.0f, 84.f), cobwebMesh, cobwebMaterial); //cobweb 6 (squish Blocking 2)
		createGroundObstacle("23", glm::vec3(-360.f, 0.0f, 3.3f), 0.25f, glm::vec3(0.0f, 0.0f, -75.f), cobwebMesh, cobwebMaterial); //cobweb 7 (tall jump)
		createGroundObstacle("24", glm::vec3(-380.f, 5.530f, 0.250f), 1.5, glm::vec3(90.f, 0.0f, -25.f), vinesMesh, vinesMaterial); // vine 8 (squish blocking)
		createGroundObstacle("25", glm::vec3(-380.240f, 0.f, 7.88f), 0.25f, glm::vec3(0.0f, 0.0f, 84.f), cobwebMesh, cobwebMaterial); //cobweb 8 (squish Blocking 2)
		createGroundObstacle("26", glm::vec3(-395.f, 0.0f, 3.3f), 0.25f, glm::vec3(0.0f, 0.0f, -75.f), cobwebMesh, cobwebMaterial); //cobweb 9 (tall jump)



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


	bool paused = false;
	bool isEscapePressed = false;
	std::string goTo = "menu.json";

	///// Game loop /////
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		ImGuiHelper::StartFrame();

		/// check for scene change validity

		


		/// with this change to the check, switching between scenes using scenePath no longer causes the game to crash since if the scene doesn't have a player it wont prompt commands
		if (scene->FindObjectByName("player") != NULL)
		{

			scene->FindObjectByName("Main Camera")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 11.480, 6.290)); // makes the camera follow the player
			scene->FindObjectByName("Main Camera")->SetRotation(glm::vec3(84, 0, -180)); //angled view
			scene->FindObjectByName("player")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x, 0.f, scene->FindObjectByName("player")->GetPosition().z)); // makes the camera follow the player

			//Stops the player from rotating
			scene->FindObjectByName("player")->SetRotation(glm::vec3(90.f, scene->FindObjectByName("player")->GetRotation().y, 0.f));

			keyboard();

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
			}
			//JumpBehaviour test;
			//test.Update();

			playerCollision.update(scene->FindObjectByName("player")->GetPosition()); // to update
			scene->FindObjectByName("player")->Get<JumpBehaviour>()->getPlayerCoords(scene->FindObjectByName("player")->GetPosition()); //send the players coordinates to JumpBehavior so we know when the player is on the ground

		}
		else
		{
			SceneChanger();
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