#include <Logging.h>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <filesystem>
#include <json.hpp>
#include <fstream>
#include <sstream>
#include <istream>
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
//#include "Gameplay/Components/JumpBehaviour.h"
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

std::ofstream timeToBeat;
int giveScoreOnce = 0;
int scoreLineCount = 0;

std::string getScores;
std::ifstream text("unsortedScores.txt");
std::vector<std::string> scores;
std::vector<float> floatScores;
bool scoreWritten = false; //So the code only writes the score to the text file once everytime the player wins

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
glm::ivec2 windowSize = glm::ivec2(1422, 800);
// The title of our GLFW window
std::string windowTitle = "Frog Frontier";

std::vector<CollisionRect> collisions;
CollisionRect playerCollision;

// using namespace should generally be avoided, and if used, make sure it's ONLY in cpp files
using namespace Gameplay;
using namespace Gameplay::Physics;

// The scene that we will be rendering
Scene::Sptr scene = nullptr;

MeshResource::Sptr planeMesh;
MeshResource::Sptr cubeMesh;
MeshResource::Sptr mushroomMesh;
MeshResource::Sptr vinesMesh;
MeshResource::Sptr cobwebMesh;
MeshResource::Sptr cobweb2Mesh;
MeshResource::Sptr ladybugMesh;

//Anim test
MeshResource::Sptr fly1Mesh;
MeshResource::Sptr fly2Mesh;

MeshResource::Sptr BranchMesh;
MeshResource::Sptr LogMesh;
MeshResource::Sptr Plant2Mesh;
MeshResource::Sptr Plant3Mesh;

MeshResource::Sptr SunflowerMesh;
MeshResource::Sptr ToadMesh;
MeshResource::Sptr Rock1Mesh;
MeshResource::Sptr Rock2Mesh;
MeshResource::Sptr Rock3Mesh;
MeshResource::Sptr twigMesh;
MeshResource::Sptr frogMesh;

MeshResource::Sptr CampfireMesh;
MeshResource::Sptr PuddleMesh;
MeshResource::Sptr SignPostMesh;
MeshResource::Sptr HangingRockMesh;
MeshResource::Sptr RockPileMesh;
MeshResource::Sptr RockTunnelMesh;
MeshResource::Sptr RockWallMesh1;
MeshResource::Sptr RockWallMesh2;

MeshResource::Sptr BGMineMesh;
MeshResource::Sptr CaveEntranceMesh;
MeshResource::Sptr Crystal1Mesh;
MeshResource::Sptr Crystal2Mesh;
MeshResource::Sptr StalagmiteMesh;
MeshResource::Sptr StalagtiteMesh;
MeshResource::Sptr GoldbarMesh;
MeshResource::Sptr GoldPile1Mesh;
MeshResource::Sptr GoldPile2Mesh;

MeshResource::Sptr tmMesh;
MeshResource::Sptr bmMesh;

MeshResource::Sptr PBMesh;
MeshResource::Sptr BGMesh;
MeshResource::Sptr ExitTreeMesh;
MeshResource::Sptr BGRockMesh;
MeshResource::Sptr ExitRockMesh;

//Running Animation
MeshResource::Sptr runningMesh1;
MeshResource::Sptr runningMesh2;
MeshResource::Sptr runningMesh3;
MeshResource::Sptr runningMesh4;
MeshResource::Sptr runningMesh5;
MeshResource::Sptr runningMesh6;
MeshResource::Sptr runningMesh7;
MeshResource::Sptr runningMesh8;
MeshResource::Sptr runningMesh9;
MeshResource::Sptr runningMesh10;
MeshResource::Sptr runningMesh11;
MeshResource::Sptr runningMesh12;
MeshResource::Sptr runningMesh13;
MeshResource::Sptr runningMesh14;
MeshResource::Sptr runningMesh15;
MeshResource::Sptr runningMesh16;
MeshResource::Sptr runningMesh17;
MeshResource::Sptr runningMesh18;

//Flying Animation
MeshResource::Sptr flyingMesh1;
MeshResource::Sptr flyingMesh2;
MeshResource::Sptr flyingMesh3;
MeshResource::Sptr flyingMesh4;
MeshResource::Sptr flyingMesh5;
MeshResource::Sptr flyingMesh6;
MeshResource::Sptr flyingMesh7;
MeshResource::Sptr flyingMesh8;

//Running Animation
MeshResource::Sptr slidingMesh1;
MeshResource::Sptr slidingMesh2;
MeshResource::Sptr slidingMesh3;
MeshResource::Sptr slidingMesh4;
MeshResource::Sptr slidingMesh5;
MeshResource::Sptr slidingMesh6;
MeshResource::Sptr slidingMesh7;
MeshResource::Sptr slidingMesh8;
MeshResource::Sptr slidingMesh9;
MeshResource::Sptr slidingMesh10;
MeshResource::Sptr slidingMesh11;
MeshResource::Sptr slidingMesh12;
MeshResource::Sptr slidingMesh13;
MeshResource::Sptr slidingMesh14;
MeshResource::Sptr slidingMesh15;
MeshResource::Sptr slidingMesh16;
MeshResource::Sptr slidingMesh17;
MeshResource::Sptr slidingMesh18;
MeshResource::Sptr slidingMesh19;
MeshResource::Sptr slidingMesh20;
MeshResource::Sptr slidingMesh21;
MeshResource::Sptr slidingMesh22;
MeshResource::Sptr slidingMesh23;
MeshResource::Sptr slidingMesh24;
MeshResource::Sptr slidingMesh25;
MeshResource::Sptr slidingMesh26;
MeshResource::Sptr slidingMesh27;
MeshResource::Sptr slidingMesh28;

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
float transitiontimer = 0.0f;
float transitionleft = 0.0f;
bool transitioncomplete = true;
bool DoTransition = false;
bool firstload = false;
//Gonna make a scenevalue to tell the keyboards what to do or some other scene specific update changes
// So 11 is main menu, 12 is controls, 13 is levelselect
// and then the actual levels can just have their value
int scenevalue = 11;

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
		SceneLoad(scene, path);

		return true;
	}



	if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_RELEASE)
	{
		enterclick = false;
	}

	//press enter starts timer
	//when timer completes sets value to true
	//transition scene

	if (glfwGetKey(window, GLFW_KEY_ENTER) && DoTransition == false && scene->FindObjectByName("Main Camera") != NULL && scene->FindObjectByName("Filter") != NULL && enterclick == false && (scenevalue == 11 || scenevalue == 12 || scenevalue == 13)) //menu
	{
		DoTransition = true;
		transitiontimer = glfwGetTime() + 2.0;
		transitioncomplete = false;
		//	scene->FindObjectByName("FrogBody")->SetPostion(glm::vec3(scene->FindObjectByName("Main Camera")->GetPosition().x - 0.2f, scene->FindObjectByName("Main Camera")->GetPosition().y, scene->FindObjectByName("Main Camera")->GetPosition().z - 1.4));

		scene->FindObjectByName("FrogBody")->SetPostion(glm::vec3(scene->FindObjectByName("Main Camera")->GetPosition().x - 3.4, scene->FindObjectByName("Main Camera")->GetPosition().y - 1.05, scene->FindObjectByName("Main Camera")->GetPosition().z - 1.4));
		scene->FindObjectByName("FrogHeadTop")->SetPostion(glm::vec3(scene->FindObjectByName("Main Camera")->GetPosition().x - 3.4, scene->FindObjectByName("Main Camera")->GetPosition().y - 1.05, scene->FindObjectByName("Main Camera")->GetPosition().z - 1.38));
		scene->FindObjectByName("FrogHeadBot")->SetPostion(glm::vec3(scene->FindObjectByName("Main Camera")->GetPosition().x - 3.4, scene->FindObjectByName("Main Camera")->GetPosition().y - 1.05, scene->FindObjectByName("Main Camera")->GetPosition().z - 1.39));
		scene->FindObjectByName("FrogTongue")->SetPostion(glm::vec3(scene->FindObjectByName("Main Camera")->GetPosition().x - 3.4, scene->FindObjectByName("Main Camera")->GetPosition().y - 1.05, scene->FindObjectByName("Main Camera")->GetPosition().z - 1.41));
		scene->FindObjectByName("BushTransition")->SetPostion(glm::vec3(scene->FindObjectByName("Main Camera")->GetPosition().x + 5, scene->FindObjectByName("Main Camera")->GetPosition().y, scene->FindObjectByName("Main Camera")->GetPosition().z - 1.2));
	}
	/*
	if (glfwGetKey(window, GLFW_KEY_ENTER) && DoTransition == false && scene->FindObjectByName("Main Camera") != NULL && scene->FindObjectByName("Filter") != NULL && enterclick == false && (scenevalue == 1 || scenevalue == 2 || scenevalue == 3) && (index == 2|| index == 3) && (paused == true || playerLose == true || playerWin == true)) //menu
	{
		DoTransition = true;
		transitiontimer = glfwGetTime() + 2.0;
		transitioncomplete = false;
		//	scene->FindObjectByName("FrogBody")->SetPostion(glm::vec3(scene->FindObjectByName("Main Camera")->GetPosition().x - 0.2f, scene->FindObjectByName("Main Camera")->GetPosition().y, scene->FindObjectByName("Main Camera")->GetPosition().z - 1.4));

		scene->FindObjectByName("FrogBody")->SetPostion(glm::vec3(scene->FindObjectByName("Main Camera")->GetPosition().x - 3.4, scene->FindObjectByName("Main Camera")->GetPosition().y - 1.05, scene->FindObjectByName("Main Camera")->GetPosition().z - 1.4));
		scene->FindObjectByName("FrogHeadTop")->SetPostion(glm::vec3(scene->FindObjectByName("Main Camera")->GetPosition().x - 3.4, scene->FindObjectByName("Main Camera")->GetPosition().y - 1.05, scene->FindObjectByName("Main Camera")->GetPosition().z - 1.38));
		scene->FindObjectByName("FrogHeadBot")->SetPostion(glm::vec3(scene->FindObjectByName("Main Camera")->GetPosition().x - 3.4, scene->FindObjectByName("Main Camera")->GetPosition().y - 1.05, scene->FindObjectByName("Main Camera")->GetPosition().z - 1.39));
		scene->FindObjectByName("FrogTongue")->SetPostion(glm::vec3(scene->FindObjectByName("Main Camera")->GetPosition().x - 3.4, scene->FindObjectByName("Main Camera")->GetPosition().y - 1.05, scene->FindObjectByName("Main Camera")->GetPosition().z - 1.41));
		scene->FindObjectByName("BushTransition")->SetPostion(glm::vec3(scene->FindObjectByName("Main Camera")->GetPosition().x + 5, scene->FindObjectByName("Main Camera")->GetPosition().y, scene->FindObjectByName("Main Camera")->GetPosition().z - 1.2));
	}
	*/

	if (DoTransition == true && transitioncomplete == false)
	{
		//movement test
		//scene->FindObjectByName("FrogBody")->SetPostion(glm::vec3(scene->FindObjectByName("FrogBody")->GetPosition().x - 0.2f, scene->FindObjectByName("FrogBody")->GetPosition().y, scene->FindObjectByName("FrogBody")->GetPosition().z));
		if (transitiontimer >= glfwGetTime())
		{
			transitionleft = transitiontimer - glfwGetTime();
			if (transitionleft >= 1.68 && transitionleft <= 2.0) //start jump
			{
				scene->FindObjectByName("FrogBody")->SetPostion(glm::vec3(scene->FindObjectByName("FrogBody")->GetPosition().x + 0.04051f, scene->FindObjectByName("FrogBody")->GetPosition().y + 0.015625, scene->FindObjectByName("FrogBody")->GetPosition().z));
				scene->FindObjectByName("FrogHeadTop")->SetPostion(glm::vec3(scene->FindObjectByName("FrogHeadTop")->GetPosition().x + 0.04051f, scene->FindObjectByName("FrogHeadTop")->GetPosition().y + 0.015625, scene->FindObjectByName("FrogHeadTop")->GetPosition().z));
				scene->FindObjectByName("FrogHeadBot")->SetPostion(glm::vec3(scene->FindObjectByName("FrogHeadBot")->GetPosition().x + 0.04051f, scene->FindObjectByName("FrogHeadBot")->GetPosition().y + 0.015625, scene->FindObjectByName("FrogHeadBot")->GetPosition().z));
				scene->FindObjectByName("FrogTongue")->SetPostion(glm::vec3(scene->FindObjectByName("FrogTongue")->GetPosition().x + 0.04051f, scene->FindObjectByName("FrogTongue")->GetPosition().y + 0.015625, scene->FindObjectByName("FrogTongue")->GetPosition().z));

				scene->FindObjectByName("FrogBody")->SetScale(glm::vec3(1.5f, scene->FindObjectByName("FrogBody")->GetScale().y - 0.0052, 1.0f));
				scene->FindObjectByName("FrogHeadTop")->SetScale(glm::vec3(1.5f, scene->FindObjectByName("FrogHeadTop")->GetScale().y - 0.0052, 1.0f));
				scene->FindObjectByName("FrogHeadBot")->SetScale(glm::vec3(1.5f, scene->FindObjectByName("FrogHeadBot")->GetScale().y - 0.0052, 1.0f));
			}
			else if (transitionleft >= 1.44) // start falling
			{
				scene->FindObjectByName("FrogBody")->SetPostion(glm::vec3(scene->FindObjectByName("FrogBody")->GetPosition().x + 0.04051f, scene->FindObjectByName("FrogBody")->GetPosition().y - 0.015625, scene->FindObjectByName("FrogBody")->GetPosition().z));
				scene->FindObjectByName("FrogHeadTop")->SetPostion(glm::vec3(scene->FindObjectByName("FrogHeadTop")->GetPosition().x + 0.04051f, scene->FindObjectByName("FrogHeadTop")->GetPosition().y - 0.015625, scene->FindObjectByName("FrogHeadTop")->GetPosition().z));
				scene->FindObjectByName("FrogHeadBot")->SetPostion(glm::vec3(scene->FindObjectByName("FrogHeadBot")->GetPosition().x + 0.04051f, scene->FindObjectByName("FrogHeadBot")->GetPosition().y - 0.015625, scene->FindObjectByName("FrogHeadBot")->GetPosition().z));
				scene->FindObjectByName("FrogTongue")->SetPostion(glm::vec3(scene->FindObjectByName("FrogTongue")->GetPosition().x + 0.04051f, scene->FindObjectByName("FrogTongue")->GetPosition().y - 0.015625, scene->FindObjectByName("FrogTongue")->GetPosition().z));

				scene->FindObjectByName("FrogBody")->SetScale(glm::vec3(1.5f, scene->FindObjectByName("FrogBody")->GetScale().y - 0.0052, 1.0f));
				scene->FindObjectByName("FrogHeadTop")->SetScale(glm::vec3(1.5f, scene->FindObjectByName("FrogHeadTop")->GetScale().y - 0.0052, 1.0f));
				scene->FindObjectByName("FrogHeadBot")->SetScale(glm::vec3(1.5f, scene->FindObjectByName("FrogHeadBot")->GetScale().y - 0.0052, 1.0f));
			}
			else if (transitionleft >= 1.36) //squish land
			{
				scene->FindObjectByName("FrogBody")->SetPostion(glm::vec3(scene->FindObjectByName("FrogBody")->GetPosition().x, scene->FindObjectByName("FrogBody")->GetPosition().y - 0.015625, scene->FindObjectByName("FrogBody")->GetPosition().z));
				scene->FindObjectByName("FrogHeadTop")->SetPostion(glm::vec3(scene->FindObjectByName("FrogHeadTop")->GetPosition().x, scene->FindObjectByName("FrogHeadTop")->GetPosition().y - 0.015625, scene->FindObjectByName("FrogHeadTop")->GetPosition().z));
				scene->FindObjectByName("FrogHeadBot")->SetPostion(glm::vec3(scene->FindObjectByName("FrogHeadBot")->GetPosition().x, scene->FindObjectByName("FrogHeadBot")->GetPosition().y - 0.015625, scene->FindObjectByName("FrogHeadBot")->GetPosition().z));
				scene->FindObjectByName("FrogTongue")->SetPostion(glm::vec3(scene->FindObjectByName("FrogTongue")->GetPosition().x, scene->FindObjectByName("FrogTongue")->GetPosition().y - 0.015625, scene->FindObjectByName("FrogTongue")->GetPosition().z));

				scene->FindObjectByName("FrogBody")->SetScale(glm::vec3(1.5f, scene->FindObjectByName("FrogBody")->GetScale().y - 0.0052, 1.0f));
				scene->FindObjectByName("FrogHeadTop")->SetScale(glm::vec3(1.5f, scene->FindObjectByName("FrogHeadTop")->GetScale().y - 0.0052, 1.0f));
				scene->FindObjectByName("FrogHeadBot")->SetScale(glm::vec3(1.5f, scene->FindObjectByName("FrogHeadBot")->GetScale().y - 0.0052, 1.0f));

			}
			else if (transitionleft >= 1.28) //standing
			{
				scene->FindObjectByName("FrogBody")->SetPostion(glm::vec3(scene->FindObjectByName("FrogBody")->GetPosition().x, scene->FindObjectByName("FrogBody")->GetPosition().y + 0.015625, scene->FindObjectByName("FrogBody")->GetPosition().z));
				scene->FindObjectByName("FrogHeadTop")->SetPostion(glm::vec3(scene->FindObjectByName("FrogHeadTop")->GetPosition().x, scene->FindObjectByName("FrogHeadTop")->GetPosition().y + 0.015625, scene->FindObjectByName("FrogHeadTop")->GetPosition().z));
				scene->FindObjectByName("FrogHeadBot")->SetPostion(glm::vec3(scene->FindObjectByName("FrogHeadBot")->GetPosition().x, scene->FindObjectByName("FrogHeadBot")->GetPosition().y + 0.015625, scene->FindObjectByName("FrogHeadBot")->GetPosition().z));
				scene->FindObjectByName("FrogTongue")->SetPostion(glm::vec3(scene->FindObjectByName("FrogTongue")->GetPosition().x, scene->FindObjectByName("FrogTongue")->GetPosition().y + 0.015625, scene->FindObjectByName("FrogTongue")->GetPosition().z));
				//scaleup

				scene->FindObjectByName("FrogBody")->SetScale(glm::vec3(1.5f, scene->FindObjectByName("FrogBody")->GetScale().y + 0.046875, 1.0f));
				scene->FindObjectByName("FrogHeadTop")->SetScale(glm::vec3(1.5f, scene->FindObjectByName("FrogHeadTop")->GetScale().y + 0.046875, 1.0f));
				scene->FindObjectByName("FrogHeadBot")->SetScale(glm::vec3(1.5f, scene->FindObjectByName("FrogHeadBot")->GetScale().y + 0.046875, 1.0f));
			}
			else if (transitionleft >= 1.2) //tongue out
			{
				scene->FindObjectByName("FrogHeadBot")->SetRotation(glm::vec3(0.0f, 0.0f, scene->FindObjectByName("FrogHeadBot")->GetRotation().z - 18));
				scene->FindObjectByName("FrogTongue")->SetRotation(glm::vec3(0.0f, 0.0f, 18.0f));
				scene->FindObjectByName("FrogTongue")->SetScale(glm::vec3(scene->FindObjectByName("FrogTongue")->GetScale().x + 4.5, 0.1, 1));
				//mouth open
				//tongue starts going
			}
			else if (transitionleft >= 1.0) //tongue grabbed
			{
				scene->FindObjectByName("FrogTongue")->SetScale(glm::vec3(scene->FindObjectByName("FrogTongue")->GetScale().x + 0.2375, 0.1, 1));
				//body doesnt move
				//tongue reaches other side of screen
			}
			else if (transitionleft >= 0.76) //tongue pulling
			{
				scene->FindObjectByName("BushTransition")->SetPostion(glm::vec3(scene->FindObjectByName("BushTransition")->GetPosition().x - 0.07, 0, 3.8));
				std::cout << "initial move";
				//body doesnt move
				//bush transition starts moving
			}
			else if (transitionleft >= 0.2) //tongue pulled
			{
				scene->FindObjectByName("FrogBody")->SetPostion(glm::vec3(scene->FindObjectByName("FrogBody")->GetPosition().x - 0.05, scene->FindObjectByName("FrogBody")->GetPosition().y, scene->FindObjectByName("FrogBody")->GetPosition().z));
				scene->FindObjectByName("FrogHeadTop")->SetPostion(glm::vec3(scene->FindObjectByName("FrogHeadTop")->GetPosition().x - 0.05, scene->FindObjectByName("FrogHeadTop")->GetPosition().y, scene->FindObjectByName("FrogHeadTop")->GetPosition().z));
				scene->FindObjectByName("FrogHeadBot")->SetPostion(glm::vec3(scene->FindObjectByName("FrogHeadBot")->GetPosition().x - 0.05, scene->FindObjectByName("FrogHeadBot")->GetPosition().y, scene->FindObjectByName("FrogHeadBot")->GetPosition().z));
				scene->FindObjectByName("FrogTongue")->SetPostion(glm::vec3(scene->FindObjectByName("FrogTongue")->GetPosition().x - 0.05, scene->FindObjectByName("FrogTongue")->GetPosition().y, scene->FindObjectByName("FrogTongue")->GetPosition().z));

				scene->FindObjectByName("FrogTongue")->SetScale(glm::vec3(scene->FindObjectByName("FrogTongue")->GetScale().x - 0.36, 0.1, 1));
				scene->FindObjectByName("BushTransition")->SetPostion(glm::vec3(scene->FindObjectByName("BushTransition")->GetPosition().x - 0.12, 0, 3.8));
				//body slides offscreen
				//almost blackout
				std::cout << "tongue pulled";
			}
			else if (transitionleft > 0.0) //finshing
			{
				scene->FindObjectByName("BushTransition")->SetPostion(glm::vec3(scene->FindObjectByName("BushTransition")->GetPosition().x - 0.05, 0, 3.8));
				std::cout << "finish";
				//complete blackout
			}


		}

		if (transitiontimer <= glfwGetTime())
		{
			transitioncomplete = true;
		}
	}

	if (DoTransition == true && transitioncomplete == true && scene->FindObjectByName("player") == NULL && scene->FindObjectByName("Filter") != NULL && enterclick == false && scenevalue == 11) //menu
	{
		switch (index) {
		case 1:
			path = "LS.json";
			SceneLoad(scene, path);
			scenevalue = 13;
			index = 1;
			DoTransition = false;
			break;
		case 2:
			path = "CS.json";
			SceneLoad(scene, path);
			scenevalue = 12;
			index = 1;
			DoTransition = false;
			break;
		case 3:
			glfwDestroyWindow(window);
			glfwTerminate();
			exit(EXIT_SUCCESS);
			break;
		}


		enterclick = true;
		return true;
	}
	else if (glfwGetKey(window, GLFW_KEY_ENTER) && scene->FindObjectByName("player") == NULL && scene->FindObjectByName("Filter") == NULL && enterclick == false && scenevalue == 12) //controls
	{
		path = "menu.json";
		SceneLoad(scene, path);
		scenevalue = 11;
		index = 1;
		enterclick = true;
		DoTransition = false;
		return true;
	}
	else if (glfwGetKey(window, GLFW_KEY_ENTER) && scene->FindObjectByName("player") == NULL && scene->FindObjectByName("Filter") != NULL && enterclick == false && scenevalue == 13) // level select
	{
		switch (index) {
		case 1:
			path = "Level1.json";
			SceneLoad(scene, path);
			scenevalue = 1;
			index = 1;
			enterclick = true;
			DoTransition = false;
			break;
		case 2:
			path = "Level.json";
			SceneLoad(scene, path);
			scenevalue = 2;
			index = 1;
			enterclick = true;
			DoTransition = false;
			break;
		case 3:
			path = "Level3.json";
			SceneLoad(scene, path);
			scenevalue = 3;
			index = 1;
			enterclick = true;
			DoTransition = false;
			break;
		case 4:
			path = "level4.json";
			SceneLoad(scene, path);
			scenevalue = 4;
			index = 1;
			enterclick = true;
			DoTransition = false;
			break;
		case 5:
			path = "Level5.json";
			SceneLoad(scene, path);
			scenevalue = 5;
			index = 1;
			enterclick = true;
			DoTransition = false;
			break;
		case 6:
			path = "Level6.json";
			SceneLoad(scene, path);
			scenevalue = 6;
			index = 1;
			enterclick = true;
			DoTransition = false;
			break;
		case 7:
			path = "menu.json";
			SceneLoad(scene, path);
			scenevalue = 11;
			index = 1;
			enterclick = true;
			DoTransition = false;
			break;
		}
		enterclick = true;
		return true;
	}

	if (glfwGetKey(window, GLFW_KEY_ENTER) && scene->FindObjectByName("player") != NULL && (paused == true || playerLose == true || playerWin == true) && enterclick == false) //pause
	{
		if (index == 2)
		{
			path = "LS.json";
			SceneLoad(scene, path);
			index = 1;
			scenevalue = 13;
			enterclick = true;
			DoTransition = false;
			return true;
		}
		else if (index == 3)
		{
			path = "menu.json";
			SceneLoad(scene, path);
			index = 1;
			scenevalue = 11;
			enterclick = true;
			DoTransition = false;
			return true;
		}

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
bool playerSliding = false;
bool returnToGround = false;
bool playerPlaying = false;
bool playerMove = false;
int clickCount = 0;
bool soundprompt = false;
float flighttime = 0.0f;
float jumptime = 0.0f;
bool verticalinput = false;
float jumpheight = 0.0000;
float x = 0;
float JTime = 0;
float JTemp = 0;
float PTime = 0;
float PTemp = 0;
float PTemp2 = 0;
float runAnimTemp2 = 0;
float AnimTime = 0;
float runLoopNumber = 1; //number of times run animation has looped, we multiply this by 1.05 so we can return runAnimTime to 0 and repeat the Animation
float FPSIncrease = 0.0; //gradually will increase so we can continue to play animations
float runAnimTemp = 0;
float FTime = 0;
float FTemp = 0;
float FResetTime = 0;
float FResetTemp = 0;
float RemainingFTime = 0;
bool playerJumping = false;
bool runningAnim = true;
bool loadMeshOnce = true;
float animIntervals = 0;
int animFrame = 0;

bool running = true;
bool sliding = false;
bool flying = false;




//secoundary keyboard function for controls when on the menu or level select screen, possibly when paused?
// currently used for testing
void SceneChanger()
{
	if (scenevalue == 11)
	{
		PTime = 0;
		PTemp = 0;
		PTemp2 = 0;
		playerPlaying = false;
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
	}
	else if (scenevalue == 13)
	{
		PTime = 0;
		PTemp = 0;
		PTemp2 = 0;
		playerPlaying = false;
		if (glfwGetKey(window, GLFW_KEY_UP) && performedtask == false) {
			if (index == 1 || index == 3 || index == 5)
			{
				index = 7;
				std::cout << index << std::endl;
				performedtask = true;
			}
			else if (index == 7)
			{
				index = 4;
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
			if (index == 2 || index == 4 || index == 6)
			{
				index = 7;
				std::cout << index << std::endl;
				performedtask = true;
			}
			else if (index == 7)
			{
				index = 3;
				performedtask = true;
			}
			else
			{
				index += 1;
				std::cout << index << std::endl;
				performedtask = true;
			}
		}
		else if (glfwGetKey(window, GLFW_KEY_LEFT) && performedtask == false)
		{
			if (index - 2 == 0)
			{
				index = 6;
				std::cout << index << std::endl;
				performedtask = true;
			}
			else if (index - 2 == -1)
			{
				index = 5;
				std::cout << index << std::endl;
				performedtask = true;
			}
			else if (index == 7)
			{
				index = 2;
				std::cout << index << std::endl;
				performedtask = true;
			}
			else
			{
				index -= 2;
				std::cout << index << std::endl;
				performedtask = true;
			}
		}
		else if (glfwGetKey(window, GLFW_KEY_RIGHT) && performedtask == false)
		{
			if (index + 2 == 7)
			{
				index = 1;
				std::cout << index << std::endl;
				performedtask = true;
			}
			else if (index + 2 == 8)
			{
				index = 2;
				std::cout << index << std::endl;
				performedtask = true;
			}
			else if (index == 7)
			{
				index = 6;
				std::cout << index << std::endl;
				performedtask = true;
			}
			else
			{
				index += 2;
				std::cout << index << std::endl;
				performedtask = true;
			}
		}
	}


	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_UP) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_RELEASE)
	{
		performedtask = false;
	}

	if (scene->FindObjectByName("Filter") != NULL)
	{
		if (scenevalue == 11)
		{
			if (index == 1)
			{
				scene->FindObjectByName("Filter")->SetPostion(glm::vec3(1.75f, 0.1f, 3.01f));
			}
			else if (index == 2)
			{
				scene->FindObjectByName("Filter")->SetPostion(glm::vec3(1.75f, -0.35f, 3.01f));
			}
			else if (index == 3)
			{
				scene->FindObjectByName("Filter")->SetPostion(glm::vec3(1.75f, -0.8f, 3.01f));
			}
		}

		if (scenevalue == 13)
		{

			if (index == 1)
			{
				scene->FindObjectByName("Filter")->SetPostion(glm::vec3(0.7f, 0.4f, 3.01f));
				scene->FindObjectByName("Filter")->SetScale(glm::vec3(0.7f));
			}
			else if (index == 2)
			{
				scene->FindObjectByName("Filter")->SetPostion(glm::vec3(0.7f, -0.4f, 3.01f));
				scene->FindObjectByName("Filter")->SetScale(glm::vec3(0.7f));
			}
			else if (index == 3)
			{
				scene->FindObjectByName("Filter")->SetPostion(glm::vec3(1.6f, 0.4f, 3.01f));
				scene->FindObjectByName("Filter")->SetScale(glm::vec3(0.7f));
			}
			else if (index == 4)
			{
				scene->FindObjectByName("Filter")->SetPostion(glm::vec3(1.6f, -0.4f, 3.01f));
				scene->FindObjectByName("Filter")->SetScale(glm::vec3(0.7f));
			}
			else if (index == 5)
			{
				scene->FindObjectByName("Filter")->SetPostion(glm::vec3(2.5f, 0.4f, 3.01f));
				scene->FindObjectByName("Filter")->SetScale(glm::vec3(0.7f));
			}
			else if (index == 6)
			{
				scene->FindObjectByName("Filter")->SetPostion(glm::vec3(2.5f, -0.4f, 3.01f));
				scene->FindObjectByName("Filter")->SetScale(glm::vec3(0.7f));
			}
			else if (index == 7)
			{
				scene->FindObjectByName("Filter")->SetPostion(glm::vec3(1.75f, -1.f, 3.01f));
				scene->FindObjectByName("Filter")->SetScale(glm::vec3(2.f, 0.4, 0.5));
			}
		}
	}
}

void readScores() {
	while (getline(text, getScores)) {
		scores.push_back(getScores);
		floatScores.push_back(std::stof(getScores)); //converts strings to floats
		scoreLineCount = scoreLineCount + 1; //gets the number of lines in the text file
	}
	text.close();
}

int partition(std::vector<float>& arrayToSort, int low, int high, float pivot) {

	int index1 = low;
	int index2 = low;
	float temp;
	while (index1 <= high) {
		if (arrayToSort[index1] > pivot) {
			index1 = index1 + 1;
		}
		else {
			temp = arrayToSort[index1]; //swaps
			arrayToSort[index1] = arrayToSort[index2];
			arrayToSort[index2] = temp;
			index2 = index2 + 1;
			index1 = index1 + 1;
		}
	}
	return index2 - 1;
}

void quickSort(std::vector<float>& arrayToSort, int low, int high) { //low == first index, high = last index
	if (low < high) {
		float pivot = arrayToSort[high];
		int position = partition(arrayToSort, low, high, pivot);

		quickSort(arrayToSort, low, position - 1);
		quickSort(arrayToSort, position + 1, high);
	}
}

void keyboard()
{
	//Loads Keyframes for animations
	//if (loadMeshOnce) {
	//}
	//loadMeshOnce

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
		if ((glfwGetKey(window, GLFW_KEY_UP) && performedtask == false)) {

			playerPlaying == false;

			if (index == 1)
			{
				index = 3;
				std::cout << index << std::endl;
				performedtask = true;
			}
			else if (index == 2)
			{
				index = 1;
				std::cout << index << std::endl;
				performedtask = true;
			}
			else if (index == 3)
			{
				index = 2;
				std::cout << index << std::endl;
				performedtask = true;
			}
		}
		else if (glfwGetKey(window, GLFW_KEY_DOWN) && performedtask == false) {
			if (index == 1)
			{
				index = 2;
				std::cout << index << std::endl;
				performedtask = true;
			}
			else if (index == 2)
			{
				index = 3;
				std::cout << index << std::endl;
				performedtask = true;
			}
			else if (index == 3)
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
	else if (paused == false) {
		playerPlaying = true;
	}

	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_UP) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_RELEASE)
	{
		performedtask = false;
	}

	if (paused == false)
	{
		//runningAnim = true;

		//to time the time the player took to beat the level (while ingame)
		if (playerPlaying == true) {
			PTime = glfwGetTime() - PTemp;
			PTime = PTime / 2.5;
			PTime = PTime + PTemp2;
		}
		else {
			PTemp2 = PTime;
			PTemp = glfwGetTime();
		}
		std::cout << PTime << "\n";

		//All Slide Code
		{
			if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
				playerSliding = true;
				running = false;
				flying = false;
				sliding = true;
				scene->FindObjectByName("player")->SetScale(glm::vec3(0.5f, 0.25f, 0.5f));
			}
			else {
				playerSliding = false;
				scene->FindObjectByName("player")->SetScale(glm::vec3(0.5f, 0.5f, 0.5f));
			}
		}

		if (playerMove == true) {
			scene->FindObjectByName("player")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 0.2f, scene->FindObjectByName("player")->GetPosition().y, scene->FindObjectByName("player")->GetPosition().z)); // makes the player move
		}

		//Fly Code
		{
			if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && FTime < 5) {
				playerFlying = true;
				playerJumping = false;
			}
			else {
				if (FTime < 5) {
					RemainingFTime = FTime;
				}
				playerFlying = false;
			}

			//Timer
			if (playerFlying == true) {
				running = false;
				sliding = false;
				flying = true;

				FTime = glfwGetTime() - FTemp + RemainingFTime;
				FTime = (FTime / 2.5) * 8;
			}
			else {
				FTemp = glfwGetTime();
			}

			//Timer so player cant continually reset fly
			if (FTime > 5) {
				FResetTime = glfwGetTime() - FResetTemp;
				FResetTime = (FResetTime / 2.5) * 8;
			}
			else {
				FResetTemp = glfwGetTime();
			}

			if (scene->FindObjectByName("player")->GetPosition().z < 10.1 && playerFlying == true) {
				scene->FindObjectByName("player")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x, scene->FindObjectByName("player")->GetPosition().y, scene->FindObjectByName("player")->GetPosition().z + 0.6));
			}
			else if (returnToGround == true && playerFlying == false && scene->FindObjectByName("player")->GetPosition().z > 0.3) {
				scene->FindObjectByName("player")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x, scene->FindObjectByName("player")->GetPosition().y, scene->FindObjectByName("player")->GetPosition().z - 0.12));
			}

			if (FTime > 5) {
				RemainingFTime = 0;
				returnToGround = true;
				playerFlying = false;
			}
			else {
				returnToGround = false;
			}

			if (FResetTime > 3) {
				FTime = 0;
			}
			//std::cout << FTime << "\n";
		}

		//All Jump Code
		{
			if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
				playerJumping = true;
			}

			if (playerJumping == true) {
				running = false;
				sliding = false;
				flying = true;

				//scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(flyingMesh1);
				JTime = glfwGetTime() - JTemp;
				JTime = JTime / 2.5;

				scene->FindObjectByName("player")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x, scene->FindObjectByName("player")->GetPosition().y, jumpheight));
			}
			else {
				JTemp = glfwGetTime();
				//animIntervals = 0;
				//scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(ladybugMesh); //sets obj to default
			}

			x = JTime * 12; //Multiply to increase speed of jump
			//std::cout << JTime << "\n";

			//parabola function so the jump will slow as it reaches the max height
			jumpheight = (-4 * pow(x - 1.5, 2) + 9) + 0.2; //0.2 is currently the ladybugs starting point on z

			if (jumpheight < 0) { //so the ladybug doesnt go through the ground
				playerJumping = false;
			}

		}
		if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) { //shows all scores from text file
			for (int i = 0; i < scoreLineCount; i++) {
				std::cout << floatScores[i] << "\n";
			}
		}
	}

	//Run Animations (still working on lerping them) ***SWITCHING BETWEEN TOO MANY KEYFRAMES IN TOO SHORT A TIME WILL CAUSE THE GAME TO CRASH***
	if (runningAnim == true) {
		AnimTime = glfwGetTime() - runAnimTemp;
		AnimTime = AnimTime / 2.5;
		AnimTime = AnimTime + runAnimTemp2;
	}
	else {
		runAnimTemp2 = AnimTime;
		runAnimTemp = glfwGetTime();
	}
	//std::cout << AnimTime << "\n" << FPSIncrease << "\n";

	if (AnimTime >= (0.02 + FPSIncrease) && AnimTime < (0.04 + FPSIncrease)) {
		animFrame = animFrame + 1;
		FPSIncrease = FPSIncrease + 0.02;
	}
	else if (AnimTime < 0.02 || playerJumping == false) {
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(runningMesh1); //sets obj to default
	}
	if (scene->FindObjectByName("player")->GetPosition().z <= 0.3) {
		runningAnim = true;
	}

	if (playerJumping == false && playerFlying == false && playerSliding == false) {
		running = true;
		flying = false;
	}

	if (running == true) {
		if (animFrame >= 18) {
			animFrame = 0;
		}
	}
	else if (flying == true) {
		if (animFrame == 26) {
			animFrame = 19;
		}
		if (animFrame <= 18) {
			animFrame = 19;
		}
	}
	else if (sliding == true) {
		if (animFrame <= 26) {
			animFrame = 27;
		}
		if (animFrame == 52) {
			animFrame = 27;
		}
	}

	if (playerPlaying) {
		runningAnim = true;
	}
	else {
		runningAnim = false;
	}

	//std::cout << animFrame << "\n" << AnimTime << "\n";

	switch (animFrame) {
	case 1:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(runningMesh1); //running animation start
		break;
	case 2:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(runningMesh2);
		break;
	case 3:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(runningMesh3);
		break;
	case 4:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(runningMesh4);
		break;
	case 5:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(runningMesh5);
		break;
	case 6:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(runningMesh6);
		break;
	case 7:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(runningMesh7);
		break;
	case 8:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(runningMesh8);
		break;
	case 9:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(runningMesh9);
		break;
	case 10:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(runningMesh10);
		break;
	case 11:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(runningMesh11);
		break;
	case 12:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(runningMesh12);
		break;
	case 13:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(runningMesh13);
		break;
	case 14:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(runningMesh14);
		break;
	case 15:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(runningMesh15);
		break;
	case 16:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(runningMesh16);
		break;
	case 17:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(runningMesh17);
		break;
	case 18:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(runningMesh18); //running animation end
		break;
	case 19:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(flyingMesh1); //flying animation start
		break;
	case 20:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(flyingMesh2);
		break;
	case 21:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(flyingMesh3);
		break;
	case 22:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(flyingMesh4);
		break;
	case 23:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(flyingMesh5);
		break;
	case 24:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(flyingMesh6);
		break;
	case 25:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(flyingMesh7);
		break;
	case 26:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(flyingMesh8); //flying animation end
		break;
	case 27:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(slidingMesh1); //sliding animation start
		break;
	case 28:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(slidingMesh2);
		break;
	case 29:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(slidingMesh3);
		break;
	case 30:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(slidingMesh4);
		break;
	case 31:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(slidingMesh5);
		break;
	case 32:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(slidingMesh6);
		break;
	case 33:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(slidingMesh7);
		break;
	case 34:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(slidingMesh8); 
		break;
	case 35:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(slidingMesh9);
		break;
	case 36:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(slidingMesh10);
		break;
	case 37:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(slidingMesh11);
		break;
	case 38:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(slidingMesh12);
		break;
	case 39:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(slidingMesh13);
		break;
	case 40:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(slidingMesh14);
		break;
	case 41:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(slidingMesh15);
		break;
	case 42:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(slidingMesh16);
		break;
	case 43:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(slidingMesh17);
		break;
	case 44:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(slidingMesh18);
		break;
	case 45:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(slidingMesh19);
		break;
	case 46:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(slidingMesh20);
		break;
	case 47:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(slidingMesh21);
		break;
	case 48:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(slidingMesh22);
		break;
	case 49:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(slidingMesh23);
		break;
	case 50:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(slidingMesh24);
		break;
	case 51:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(slidingMesh25);
		break;
	case 52:
		scene->FindObjectByName("player")->Get<RenderComponent>()->SetMesh(slidingMesh26); //Sliding Animation end
		break;
	}


	if (playerFlying == false) {
		if (scene->FindObjectByName("player")->GetPosition().z > 0.3) {
			scene->FindObjectByName("player")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x, scene->FindObjectByName("player")->GetPosition().y, scene->FindObjectByName("player")->GetPosition().z - 0.4));
		}
		else if (scene->FindObjectByName("player")->GetPosition().z <= 0.3)
		{
			isJumpPressed = false;
		}


		if (scenevalue == 1)
		{
			if (scene->FindObjectByName("player")->GetPosition().x < -800.f) {
				playerMove = false;
			}
		}
		else if (scenevalue == 2)
		{
			if (scene->FindObjectByName("player")->GetPosition().x < -400.f) {
				playerMove = false;
			}
		}
		else if (scenevalue == 3)
		{
			if (scene->FindObjectByName("player")->GetPosition().x < -1200.f) {
				playerMove = false;
			}
		}
		else if (scenevalue == 4)
		{
			if (scene->FindObjectByName("player")->GetPosition().x < -1600.f)
			{
				playerMove = false;
			}
		}
		else if (scenevalue == 5)
		{
			if (scene->FindObjectByName("player")->GetPosition().x < -2000.f) {
				playerMove = false;
			}
		}
		else if (scenevalue == 6)
		{
			if (scene->FindObjectByName("player")->GetPosition().x < -2400.f) {
				playerMove = false;
			}
		}

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
	//ComponentManager::RegisterType<JumpBehaviour>();
	ComponentManager::RegisterType<MaterialSwapBehaviour>();

	// GL states, we'll enable depth testing and backface fulling
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glCullFace(GL_BACK);
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

	FMOD::System* system;
	FMOD::Sound* sound1, * sound2, * sound3, * sound4, * sound5, *sound6, *sound7, *sound8, *sound9, *sound10, *sound11;
	FMOD::Channel* channel = 0;
	FMOD_RESULT       result;
	void* extradriverdata = 0;

	result = FMOD::System_Create(&system);

	result = system->init(32, FMOD_INIT_NORMAL, extradriverdata);

	result = system->createSound("media/Click.wav", FMOD_DEFAULT, 0, &sound1);

	result = system->createSound("media/Pop1.wav", FMOD_DEFAULT, 0, &sound2);

	result = system->createSound("media/Pop2.wav", FMOD_DEFAULT, 0, &sound3);

	result = system->createSound("media/bensound-funnysong.wav", FMOD_LOOP_NORMAL, 0, &sound4);

	result = system->createSound("media/Pitched-Pop.wav", FMOD_LOOP_NORMAL, 0, &sound5);

	result = system->createSound("media/Buzz.wav", FMOD_DEFAULT, 0, &sound6);

	result = system->createSound("media/Frog Tongue.wav", FMOD_DEFAULT, 0, &sound7);

	result = system->createSound("media/Grass.wav", FMOD_DEFAULT, 0, &sound8);

	result = system->createSound("media/Main song beat.wav", FMOD_LOOP_NORMAL, 0, &sound9);

	result = system->createSound("media/Rock crushing orange.wav", FMOD_DEFAULT, 0, &sound10);

	result = system->createSound("media/Victory sound effect.wav", FMOD_DEFAULT, 0, &sound11);

	bool loadScene = false;
	// For now we can use a toggle to generate our scene vs load from file
	if (loadScene) {
		ResourceManager::LoadManifest("manifest.json");
		scene = Scene::Load("menu.json");
	}
	else {

		//readScores(); //reads all of the scores before loading the scene
		//quickSort(floatScores, 0, floatScores.size() - 1); //sorts our array from lowest to greatest

		//load all our objects once for all scenes

		//questionable things to keep?
		MeshResource::Sptr monkeyMesh = ResourceManager::CreateAsset<MeshResource>("Monkey.obj");
		Texture2D::Sptr    monkeyTex = ResourceManager::CreateAsset<Texture2D>("textures/monkey-uvMap.png");
		Texture2D::Sptr    boxTexture = ResourceManager::CreateAsset<Texture2D>("textures/box-diffuse.png");

		//png sorted by alphabetical
		Texture2D::Sptr    BGTex = ResourceManager::CreateAsset<Texture2D>("textures/BackgroundUV.png");
		Texture2D::Sptr    bgTexture = ResourceManager::CreateAsset<Texture2D>("textures/bg.png");
		Texture2D::Sptr    BlankTex = ResourceManager::CreateAsset<Texture2D>("textures/blank.png");
		Texture2D::Sptr    bmTex = ResourceManager::CreateAsset<Texture2D>("textures/bmuv.png");
		Texture2D::Sptr    BranchTex = ResourceManager::CreateAsset<Texture2D>("textures/BranchUV.png");
		Texture2D::Sptr    CaveEntranceTex = ResourceManager::CreateAsset<Texture2D>("texures/ExitCaveUV.png");
		Texture2D::Sptr    CampfireTex = ResourceManager::CreateAsset<Texture2D>("textures/CampfireUVFrame.png");
		Texture2D::Sptr    cobwebTexture = ResourceManager::CreateAsset<Texture2D>("textures/CobwebUV.png");
		Texture2D::Sptr    cobweb2Texture = ResourceManager::CreateAsset<Texture2D>("textures/CobwebUVFrame.png");
		Texture2D::Sptr    Crystal1BlueTex = ResourceManager::CreateAsset<Texture2D>("textures/Crystal1UVBlue.png");
		Texture2D::Sptr    Crystal1GreenTex = ResourceManager::CreateAsset<Texture2D>("textures/Crystal1UVGreen.png");
		Texture2D::Sptr    Crystal1PurpleTex = ResourceManager::CreateAsset<Texture2D>("textures/Crystal1UVPurple.png");
		Texture2D::Sptr    Crystal1RedTex = ResourceManager::CreateAsset<Texture2D>("textures/Crystal1UVRed.png");
		Texture2D::Sptr    Crystal2BlueTex = ResourceManager::CreateAsset<Texture2D>("textures/Crystal2UVBlue.png");
		Texture2D::Sptr    Crystal2GreenTex = ResourceManager::CreateAsset<Texture2D>("textures/Crystal2UVGreen.png");
		Texture2D::Sptr    Crystal2YellowTex = ResourceManager::CreateAsset<Texture2D>("textures/Crystal2UVYellow.png");
		Texture2D::Sptr    ExitRockTex = ResourceManager::CreateAsset<Texture2D>("textures/ExitRockUV.png");
		Texture2D::Sptr    ExitTreeTex = ResourceManager::CreateAsset<Texture2D>("textures/ExitTreeUV.png");
		Texture2D::Sptr    ForegroundTex = ResourceManager::CreateAsset<Texture2D>("textures/fg1.png");
		Texture2D::Sptr    frogTex = ResourceManager::CreateAsset<Texture2D>("textures/froguv.png");
		Texture2D::Sptr    GoldBarTex = ResourceManager::CreateAsset<Texture2D>("textures/GoldBarUV.png");
		Texture2D::Sptr    GoldPile1Tex = ResourceManager::CreateAsset<Texture2D>("textures/GoldPile1UV.png");
		Texture2D::Sptr    GoldPile2Tex = ResourceManager::CreateAsset<Texture2D>("textures/GoldPile2UV.png");
		Texture2D::Sptr    BGGrassTex = ResourceManager::CreateAsset<Texture2D>("textures/grassuv.png");
		Texture2D::Sptr    Grass1Tex = ResourceManager::CreateAsset<Texture2D>("textures/Grass1.png");
		Texture2D::Sptr    Grass2Tex = ResourceManager::CreateAsset<Texture2D>("textures/Grass2.png");
		Texture2D::Sptr    Grass3Tex = ResourceManager::CreateAsset<Texture2D>("textures/Grass3.png");
		Texture2D::Sptr    Grass4Tex = ResourceManager::CreateAsset<Texture2D>("textures/Grass4.png");
		Texture2D::Sptr    Grass5Tex = ResourceManager::CreateAsset<Texture2D>("textures/Grass5.png");
		Texture2D::Sptr    greenTex = ResourceManager::CreateAsset<Texture2D>("textures/green.png");
		Texture2D::Sptr    rockTexture = ResourceManager::CreateAsset<Texture2D>("textures/grey.png");
		Texture2D::Sptr    grassTexture = ResourceManager::CreateAsset<Texture2D>("textures/ground.png");
		Texture2D::Sptr    HangingRockTex = ResourceManager::CreateAsset<Texture2D>("textures/HangingRockUV.png");
		Texture2D::Sptr    ladybugTexture = ResourceManager::CreateAsset<Texture2D>("textures/LadybugUV.png");
		Texture2D::Sptr    LogTex = ResourceManager::CreateAsset<Texture2D>("textures/logUV.png");
		Texture2D::Sptr    MineForegroundTex = ResourceManager::CreateAsset<Texture2D>("textures/MineForeground.png");
		Texture2D::Sptr    MineBackgroundTex = ResourceManager::CreateAsset<Texture2D>("textures/MineBackdrop.png");
		Texture2D::Sptr    MineUVTex = ResourceManager::CreateAsset<Texture2D>("textures/MineUV.png");
		Texture2D::Sptr    MBGTex = ResourceManager::CreateAsset<Texture2D>("textures/MountainBackgroundUV.png");
		Texture2D::Sptr    MountainBackdropTex = ResourceManager::CreateAsset<Texture2D>("textures/MountainBackdrop.png");
		Texture2D::Sptr    MountainForegroundTex = ResourceManager::CreateAsset<Texture2D>("textures/MountainForeground.png");
		Texture2D::Sptr    PuddleTex = ResourceManager::CreateAsset<Texture2D>("textures/PuddleUV.png");
		Texture2D::Sptr    mushroomTexture = ResourceManager::CreateAsset<Texture2D>("textures/MushroomUV.png");
		Texture2D::Sptr    PlantTex = ResourceManager::CreateAsset<Texture2D>("textures/Plantuv.png");
		Texture2D::Sptr    RockTex = ResourceManager::CreateAsset<Texture2D>("textures/rockTexture.png");
		Texture2D::Sptr	   RockPileTex = ResourceManager::CreateAsset<Texture2D>("textures/RockPileUV.png");
		Texture2D::Sptr    RockTunnelTex = ResourceManager::CreateAsset<Texture2D>("textures/RockTunnelUV.png");
		Texture2D::Sptr    RockWallTex = ResourceManager::CreateAsset<Texture2D>("textures/RockWallUV.png");
		Texture2D::Sptr    RootTexture = ResourceManager::CreateAsset<Texture2D>("textures/RootUV.png");
		Texture2D::Sptr    SignPostTex = ResourceManager::CreateAsset<Texture2D>("textures/SignPostUV.png");
		Texture2D::Sptr    StalagmiteTex = ResourceManager::CreateAsset<Texture2D>("textures/StalagmiteUV.png");
		Texture2D::Sptr    StalagtiteTex = ResourceManager::CreateAsset<Texture2D>("textures/Stalagtite.png");
		Texture2D::Sptr    SunflowerTex = ResourceManager::CreateAsset<Texture2D>("textures/SunflowerUV.png");
		Texture2D::Sptr    twigTex = ResourceManager::CreateAsset<Texture2D>("textures/TwigUV.png");
		Texture2D::Sptr    ToadTex = ResourceManager::CreateAsset<Texture2D>("textures/Toad.png");
		Texture2D::Sptr    tmTex = ResourceManager::CreateAsset<Texture2D>("textures/tmuv.png");
		Texture2D::Sptr    vinesTexture = ResourceManager::CreateAsset<Texture2D>("textures/VinesUV.png");


		//Menu background
		Texture2D::Sptr    LSMenuTex = ResourceManager::CreateAsset<Texture2D>("textures/Game Poster 2 Extended.png");
		Texture2D::Sptr    MenuTex = ResourceManager::CreateAsset<Texture2D>("textures/Game Poster 3.png");
		Texture2D::Sptr    ControlMenuTex = ResourceManager::CreateAsset<Texture2D>("textures/ControlsMenu.png");

		//Controls screen
		Texture2D::Sptr    ControlTex = ResourceManager::CreateAsset<Texture2D>("textures/Buttons.png");

		//Button UI
		Texture2D::Sptr    ButtonTex = ResourceManager::CreateAsset<Texture2D>("textures/Button Background.png");
		Texture2D::Sptr    ButtonBackTex = ResourceManager::CreateAsset<Texture2D>("textures/Button Background.png");
		Texture2D::Sptr    LSButtonTex = ResourceManager::CreateAsset<Texture2D>("textures/Level Button Background 1.png");
		Texture2D::Sptr    FilterTex = ResourceManager::CreateAsset<Texture2D>("textures/Button Filter.png");
		Texture2D::Sptr    ForestButtonTex = ResourceManager::CreateAsset<Texture2D>("textures/Forest Level.png");
		Texture2D::Sptr    MountainButtonTex = ResourceManager::CreateAsset<Texture2D>("textures/Mountain Level.png");
		Texture2D::Sptr    MineButtonTex = ResourceManager::CreateAsset<Texture2D>("textures/Mine Level.png");

		//UI textures
		Texture2D::Sptr    PauseTex = ResourceManager::CreateAsset<Texture2D>("textures/Pause.png");
		Texture2D::Sptr    PanelTex = ResourceManager::CreateAsset<Texture2D>("textures/Panel.png");
		Texture2D::Sptr    ProgressTex = ResourceManager::CreateAsset<Texture2D>("textures/progressbar.png");
		Texture2D::Sptr    Progress2Tex = ResourceManager::CreateAsset<Texture2D>("textures/progressBar2.png");
		Texture2D::Sptr    Progress3Tex = ResourceManager::CreateAsset<Texture2D>("textures/progressBar3.png");
		Texture2D::Sptr    PbarbugTex = ResourceManager::CreateAsset<Texture2D>("textures/progressmeter.png");
		Texture2D::Sptr    FFLogoTex = ResourceManager::CreateAsset<Texture2D>("textures/Frog Frontier Logo.png");
		Texture2D::Sptr    LSLogoTex = ResourceManager::CreateAsset<Texture2D>("textures/Frog Frontier Logo Side Scroller.png");
		Texture2D::Sptr    PBTex = ResourceManager::CreateAsset<Texture2D>("textures/PauseButton.png");

		//numbered button text
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

		Texture2D::Sptr    Tex1R = ResourceManager::CreateAsset<Texture2D>("textures/1R.png");
		Texture2D::Sptr    Tex2R = ResourceManager::CreateAsset<Texture2D>("textures/2R.png");
		Texture2D::Sptr    Tex3R = ResourceManager::CreateAsset<Texture2D>("textures/3R.png");
		Texture2D::Sptr    Tex4R = ResourceManager::CreateAsset<Texture2D>("textures/4R.png");
		Texture2D::Sptr    Tex5R = ResourceManager::CreateAsset<Texture2D>("textures/5R.png");
		Texture2D::Sptr    Tex6R = ResourceManager::CreateAsset<Texture2D>("textures/6R.png");

		Texture2D::Sptr    Tex1B = ResourceManager::CreateAsset<Texture2D>("textures/1B.png");
		Texture2D::Sptr    Tex2B = ResourceManager::CreateAsset<Texture2D>("textures/2B.png");
		Texture2D::Sptr    Tex3B = ResourceManager::CreateAsset<Texture2D>("textures/3B.png");
		Texture2D::Sptr    Tex4B = ResourceManager::CreateAsset<Texture2D>("textures/4B.png");
		Texture2D::Sptr    Tex5B = ResourceManager::CreateAsset<Texture2D>("textures/5B.png");
		Texture2D::Sptr    Tex6B = ResourceManager::CreateAsset<Texture2D>("textures/6B.png");

		//written text
		Texture2D::Sptr    ResumeTex = ResourceManager::CreateAsset<Texture2D>("textures/ResumeText.png");
		Texture2D::Sptr    MainMenuTex = ResourceManager::CreateAsset<Texture2D>("textures/MainMenuText.png");
		Texture2D::Sptr    ButtonStartTex = ResourceManager::CreateAsset<Texture2D>("textures/Start Text.png");
		Texture2D::Sptr    ButtonExitTex = ResourceManager::CreateAsset<Texture2D>("textures/Exit Text.png");
		Texture2D::Sptr    BackTextTex = ResourceManager::CreateAsset<Texture2D>("textures/Exit Text.png");
		Texture2D::Sptr    StartTextTex = ResourceManager::CreateAsset<Texture2D>("textures/Start Text.png");
		Texture2D::Sptr    ControlsTex = ResourceManager::CreateAsset<Texture2D>("textures/ControlsText.png");
		Texture2D::Sptr    BackTex = ResourceManager::CreateAsset<Texture2D>("textures/Back Text.png");
		Texture2D::Sptr    LStextTex = ResourceManager::CreateAsset<Texture2D>("textures/LSText.png");
		Texture2D::Sptr    ReplayTex = ResourceManager::CreateAsset<Texture2D>("textures/ReplayText.png");

		//text
		Texture2D::Sptr    winTexture = ResourceManager::CreateAsset<Texture2D>("textures/win.png");
		Texture2D::Sptr    WinnerTex = ResourceManager::CreateAsset<Texture2D>("textures/Winner.png");
		Texture2D::Sptr    LoserTex = ResourceManager::CreateAsset<Texture2D>("textures/YouLose.png");

		//menu animation stuff
		Texture2D::Sptr    FrogBodyTex = ResourceManager::CreateAsset<Texture2D>("textures/Frog Body copy.png");
		Texture2D::Sptr    FrogHeadTopTex = ResourceManager::CreateAsset<Texture2D>("textures/Frog Head Top.png");
		Texture2D::Sptr    FrogHeadBotTex = ResourceManager::CreateAsset<Texture2D>("textures/Frog Head Bottom.png");
		Texture2D::Sptr    FrogTongueTex = ResourceManager::CreateAsset<Texture2D>("textures/Tongue.png");
		Texture2D::Sptr    BushTransitionTex = ResourceManager::CreateAsset<Texture2D>("textures/Bush Transition.png");

		// Create our OpenGL resources
		Shader::Sptr uboShader = ResourceManager::CreateAsset<Shader>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shader.glsl" },
			{ ShaderPartType::Fragment, "shaders/frag_blinn_phong_textured.glsl" }
		});



		/// Working Level ///									//////		Level 2		///////////
		{

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
				cobwebMaterial->Texture = cobweb2Texture;
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
			Material::Sptr LStextMaterial = ResourceManager::CreateAsset<Material>();
			{
				LStextMaterial->Name = "LStext";
				LStextMaterial->MatShader = scene->BaseShader;
				LStextMaterial->Texture = LStextTex;
				LStextMaterial->Shininess = 2.0f;
			}

			Material::Sptr FrogBodyMaterial = ResourceManager::CreateAsset<Material>();
			{
				FrogBodyMaterial->Name = "FrogBody";
				FrogBodyMaterial->MatShader = scene->BaseShader;
				FrogBodyMaterial->Texture = FrogBodyTex;
				FrogBodyMaterial->Shininess = 2.0f;
			}

			Material::Sptr FrogHeadTopMaterial = ResourceManager::CreateAsset<Material>();
			{
				FrogHeadTopMaterial->Name = "FrogHeadTop";
				FrogHeadTopMaterial->MatShader = scene->BaseShader;
				FrogHeadTopMaterial->Texture = FrogHeadTopTex;
				FrogHeadTopMaterial->Shininess = 2.0f;
			}

			Material::Sptr FrogHeadBotMaterial = ResourceManager::CreateAsset<Material>();
			{
				FrogHeadBotMaterial->Name = "FrogHeadBot";
				FrogHeadBotMaterial->MatShader = scene->BaseShader;
				FrogHeadBotMaterial->Texture = FrogHeadBotTex;
				FrogHeadBotMaterial->Shininess = 2.0f;
			}

			Material::Sptr FrogTongueMaterial = ResourceManager::CreateAsset<Material>();
			{
				FrogTongueMaterial->Name = "FrogTongue";
				FrogTongueMaterial->MatShader = scene->BaseShader;
				FrogTongueMaterial->Texture = FrogTongueTex;
				FrogTongueMaterial->Shininess = 2.0f;
			}

			Material::Sptr BushTransitionMaterial = ResourceManager::CreateAsset<Material>();
			{
				BushTransitionMaterial->Name = "BushTransition";
				BushTransitionMaterial->MatShader = scene->BaseShader;
				BushTransitionMaterial->Texture = BushTransitionTex;
				BushTransitionMaterial->Shininess = 2.0f;
			}

			// Create some lights for our scene
			// Create some lights for our scene
			// Create some lights for our scene
			scene->Lights.resize(31);
			scene->Lights[0].Position = glm::vec3(0.0f, 1.0f, 40.0f);
			scene->Lights[0].Color = glm::vec3(0.8619f, 1.f, 0.819f);
			scene->Lights[0].Range = 200.0f;

			scene->Lights[1].Position = glm::vec3(-50.f, 0.0f, 40.0f);
			scene->Lights[1].Color = glm::vec3(0.8619f, 1.f, 0.819f);
			scene->Lights[1].Range = 200.0f;

			scene->Lights[2].Position = glm::vec3(-100.f, 1.0f, 40.0f);
			scene->Lights[2].Color = glm::vec3(0.8619f, 1.f, 0.819f);
			scene->Lights[2].Range = 200.0f;

			scene->Lights[3].Position = glm::vec3(-150.0f, 1.0f, 40.0f);
			scene->Lights[3].Color = glm::vec3(0.8619f, 1.f, 0.819f);
			scene->Lights[3].Range = 200.0f;

			scene->Lights[4].Position = glm::vec3(-100.0f, 1.0f, 40.0f);
			scene->Lights[4].Color = glm::vec3(0.8619f, 1.f, 0.819f);
			scene->Lights[4].Range = 200.0f;

			scene->Lights[5].Position = glm::vec3(-150.0f, 1.0f, 40.0f);
			scene->Lights[5].Color = glm::vec3(0.8619f, 1.f, 0.819f);
			scene->Lights[5].Range = 200.0f;

			scene->Lights[6].Position = glm::vec3(-200.0f, 1.0f, 40.0f);
			scene->Lights[6].Color = glm::vec3(0.8619f, 1.f, 0.819f);
			scene->Lights[6].Range = 200.0f;

			scene->Lights[7].Position = glm::vec3(-250.0f, 1.0f, 40.0f);
			scene->Lights[7].Color = glm::vec3(0.8619f, 1.f, 0.819f);
			scene->Lights[7].Range = 200.0f;

			scene->Lights[8].Position = glm::vec3(-300.0f, 1.0f, 40.0f);
			scene->Lights[8].Color = glm::vec3(0.8619f, 1.f, 0.819f);
			scene->Lights[8].Range = 200.0f;

			scene->Lights[9].Position = glm::vec3(-350.0f, 1.0f, 40.0f);
			scene->Lights[9].Color = glm::vec3(0.8619f, 1.f, 0.819f);
			scene->Lights[9].Range = 200.0f;

			scene->Lights[10].Position = glm::vec3(-400.0f, 1.0f, 40.0f);
			scene->Lights[10].Color = glm::vec3(0.8619f, 1.f, 0.819f);
			scene->Lights[10].Range = 200.0f;

			scene->Lights[11].Position = glm::vec3(0.0f, -90.0f, 100.0f);
			scene->Lights[11].Color = glm::vec3(0.45f, 0.678f, 0.1872f);
			scene->Lights[11].Range = 1000.0f;

			scene->Lights[12].Position = glm::vec3(-50.f, -90.0f, 100.0f);
			scene->Lights[12].Color = glm::vec3(0.45f, 0.678f, 0.1872f);
			scene->Lights[12].Range = 1000.0f;

			scene->Lights[13].Position = glm::vec3(-100.f, -90.0f, 100.0f);
			scene->Lights[13].Color = glm::vec3(0.45f, 0.678f, 0.1872f);
			scene->Lights[13].Range = 1000.0f;

			scene->Lights[14].Position = glm::vec3(-150.0f, -90.0f, 100.0f);
			scene->Lights[14].Color = glm::vec3(0.45f, 0.678f, 0.1872f);
			scene->Lights[14].Range = 1000.0f;

			scene->Lights[15].Position = glm::vec3(-100.0f, -90.0f, 100.0f);
			scene->Lights[15].Color = glm::vec3(0.45f, 0.678f, 0.1872f);
			scene->Lights[15].Range = 1000.0f;

			scene->Lights[16].Position = glm::vec3(-150.0f, -90.0f, 100.0f);
			scene->Lights[16].Color = glm::vec3(0.45f, 0.678f, 0.1872f);
			scene->Lights[16].Range = 1000.0f;

			scene->Lights[17].Position = glm::vec3(-200.0f, -90.0f, 100.0f);
			scene->Lights[17].Color = glm::vec3(0.45f, 0.678f, 0.1872f);
			scene->Lights[17].Range = 1000.0f;

			scene->Lights[18].Position = glm::vec3(-250.0f, -90.0f, 100.0f);
			scene->Lights[18].Color = glm::vec3(0.45f, 0.678f, 0.1872f);
			scene->Lights[18].Range = 1000.0f;

			scene->Lights[19].Position = glm::vec3(-300.0f, -90.0f, 100.0f);
			scene->Lights[19].Color = glm::vec3(0.45f, 0.678f, 0.1872f);
			scene->Lights[19].Range = 1000.0f;

			scene->Lights[20].Position = glm::vec3(-350.0f, -90.0f, 100.0f);
			scene->Lights[20].Color = glm::vec3(0.45f, 0.678f, 0.1872f);
			scene->Lights[20].Range = 1000.0f;

			scene->Lights[21].Position = glm::vec3(-400.0f, -90.0f, 100.0f);
			scene->Lights[21].Color = glm::vec3(0.45f, 0.678f, 0.1872f);
			scene->Lights[21].Range = 1000.0f;

			scene->Lights[22].Position = glm::vec3(-000.0f, 20.0f, 5.0f);
			scene->Lights[22].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[22].Range = 150.0f;

			scene->Lights[23].Position = glm::vec3(-50.0f, 20.0f, 5.0f);
			scene->Lights[23].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[23].Range = 150.0f;

			scene->Lights[24].Position = glm::vec3(-100.0f, 20.0f, 5.0f);
			scene->Lights[24].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[24].Range = 150.0f;

			scene->Lights[25].Position = glm::vec3(-150.0f, 20.0f, 5.0f);
			scene->Lights[25].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[25].Range = 150.0f;

			scene->Lights[26].Position = glm::vec3(-200.0f, 20.0f, 5.0f);
			scene->Lights[26].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[26].Range = 150.0f;

			scene->Lights[27].Position = glm::vec3(-250.0f, 20.0f, 5.0f);
			scene->Lights[27].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[27].Range = 150.0f;

			scene->Lights[28].Position = glm::vec3(-300.0f, 20.0f, 5.0f);
			scene->Lights[28].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[28].Range = 150.0f;

			scene->Lights[29].Position = glm::vec3(-350.0f, 20.0f, 5.0f);
			scene->Lights[29].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[29].Range = 150.0f;

			scene->Lights[30].Position = glm::vec3(-400.0f, 20.0f, 5.0f);
			scene->Lights[30].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[30].Range = 150.0f;

			// We'll create a mesh that is a simple plane that we can resize later
			planeMesh = ResourceManager::CreateAsset<MeshResource>();
			cubeMesh = ResourceManager::CreateAsset<MeshResource>("cube.obj");
			mushroomMesh = ResourceManager::CreateAsset<MeshResource>("Mushroom.obj");
			vinesMesh = ResourceManager::CreateAsset<MeshResource>("Vines.obj");
			cobwebMesh = ResourceManager::CreateAsset<MeshResource>("Cobweb.obj");
			cobweb2Mesh = ResourceManager::CreateAsset<MeshResource>("Spiderweb2.obj");

			//Anim test
			fly1Mesh = ResourceManager::CreateAsset<MeshResource>("fly1.obj");
			fly2Mesh = ResourceManager::CreateAsset<MeshResource>("fly2.obj");
			ladybugMesh = ResourceManager::CreateAsset<MeshResource>("lbo2.obj");

			BranchMesh = ResourceManager::CreateAsset<MeshResource>("Branch.obj");
			LogMesh = ResourceManager::CreateAsset<MeshResource>("Log.obj");
			Plant2Mesh = ResourceManager::CreateAsset<MeshResource>("Plant2.obj");
			Plant3Mesh = ResourceManager::CreateAsset<MeshResource>("Plant3.obj");

			SunflowerMesh = ResourceManager::CreateAsset<MeshResource>("Sunflower.obj");
			ToadMesh = ResourceManager::CreateAsset<MeshResource>("ToadStool.obj");
			Rock1Mesh = ResourceManager::CreateAsset<MeshResource>("Rock1.obj");
			Rock2Mesh = ResourceManager::CreateAsset<MeshResource>("Rock2.obj");
			Rock3Mesh = ResourceManager::CreateAsset<MeshResource>("Rock3.obj");
			twigMesh = ResourceManager::CreateAsset<MeshResource>("Twig.obj");
			frogMesh = ResourceManager::CreateAsset<MeshResource>("frog.obj");

			tmMesh = ResourceManager::CreateAsset<MeshResource>("tm.obj");
			bmMesh = ResourceManager::CreateAsset<MeshResource>("bm.obj");

			PBMesh = ResourceManager::CreateAsset<MeshResource>("PB.obj");
			BGMesh = ResourceManager::CreateAsset<MeshResource>("Background.obj");
			ExitTreeMesh = ResourceManager::CreateAsset<MeshResource>("ExitTree.obj");

			planeMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(1.0f)));
			planeMesh->GenerateMesh();



			//Background Assets
			createBackgroundAsset("1", glm::vec3(-1.f, 5.0f, -0.f), 0.5, glm::vec3(83.f, 5.0f, 0.0f), BranchMesh, BranchMaterial);
			createBackgroundAsset("2", glm::vec3(-200.f, 5.0f, -0.f), 0.5, glm::vec3(83.f, 5.0f, 0.0f), BranchMesh, BranchMaterial);

			createBackgroundAsset("3", glm::vec3(-186.f, 4.0f, -0.660), 3, glm::vec3(83.f, 5.0f, 0.0f), LogMesh, LogMaterial);
			createBackgroundAsset("4", glm::vec3(-20.f, 0.0f, -0.660), 0.5, glm::vec3(400.f, 5.0f, 0.0f), LogMesh, LogMaterial);

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
			createGroundObstacle("4", glm::vec3(-110.f, 0.0f, -3.5f), glm::vec3(0.25f, 0.25f, 0.25f), glm::vec3(90.0f, 0.0f, 0.f), cobweb2Mesh, cobwebMaterial); //cobweb 1 (tall jump)
			createGroundObstacle("5", glm::vec3(-45.f, 0.0f, -0.660), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(90.f, 0.0f, 0.0f), mushroomMesh, mushroomMaterial); //mushroom 2
			createGroundObstacle("6", glm::vec3(-150.f, 5.530f, 0.250f), glm::vec3(1.5f, 1.5f, 1.5f), glm::vec3(90.f, 0.0f, -25.f), vinesMesh, vinesMaterial); // vine 2 (squish blocking)
			createGroundObstacle("7", glm::vec3(-150.240f, 0.f, 0.0f), glm::vec3(0.25f, 0.25f, 0.25f), glm::vec3(90.0f, 0.0f, 0.f), cobweb2Mesh, cobwebMaterial); //cobweb 2 (squish Blocking 2)

			createGroundObstacle("8", glm::vec3(-170.f, 0.0f, -0.660), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(90.f, 0.0f, 0.0f), mushroomMesh, mushroomMaterial); //mushroom 3 (small jump)
			createGroundObstacle("9", glm::vec3(-200.f, 0.0f, 3.0), glm::vec3(1.f, 1.f, 1.f), glm::vec3(90.f, 0.0f, 73.f), vinesMesh, vinesMaterial); // vine 3 (jump blocking)
			createGroundObstacle("10", glm::vec3(-220.f, 0.0f, -3.5f), glm::vec3(0.25f, 0.25f, 0.25f), glm::vec3(90.0f, 0.0f, 0.f), cobweb2Mesh, cobwebMaterial); //cobweb 3 (tall jump)
			createGroundObstacle("11", glm::vec3(-230.f, 0.0f, -0.660), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(90.f, 0.0f, 0.0f), mushroomMesh, mushroomMaterial); //mushroom 4 (small jump)
			createGroundObstacle("12", glm::vec3(-250.f, 0.0f, 3.0), glm::vec3(1.f, 1.f, 1.f), glm::vec3(90.f, 0.0f, 73.f), vinesMesh, vinesMaterial); // vine 4 (jump blocking)
			createGroundObstacle("13", glm::vec3(-275.f, 0.0f, -0.660), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(90.f, 0.0f, 0.0f), mushroomMesh, mushroomMaterial); //mushroom 5 (small jump)
			createGroundObstacle("14", glm::vec3(-300.f, 5.530f, 0.250f), glm::vec3(1.5f, 1.5f, 1.5f), glm::vec3(90.f, 0.0f, -25.f), vinesMesh, vinesMaterial); // vine 5 (squish blocking)
			createGroundObstacle("15", glm::vec3(-300.240f, 0.f, 0.0f), glm::vec3(0.25f, 0.25f, 0.25f), glm::vec3(90.0f, 0.0f, 0.f), cobweb2Mesh, cobwebMaterial); //cobweb 4 (squish Blocking 2)

			createGroundObstacle("16", glm::vec3(-310.f, 0.0f, -0.660), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(90.f, 0.0f, 0.0f), mushroomMesh, mushroomMaterial); //mushroom 6 (small jump)
			createGroundObstacle("17", glm::vec3(-315.f, 0.0f, -0.660), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(90.f, 0.0f, 0.0f), mushroomMesh, mushroomMaterial); //mushroom 7 (small jump)
			createGroundObstacle("18", glm::vec3(-320.f, 0.0f, -0.660), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(90.f, 0.0f, 0.0f), mushroomMesh, mushroomMaterial); //mushroom 8 (small jump)
			createGroundObstacle("19", glm::vec3(-325.f, 0.0f, -3.5f), glm::vec3(0.25f, 0.25f, 0.25f), glm::vec3(90.0f, 0.0f, 0.f), cobweb2Mesh, cobwebMaterial); //cobweb 5 (tall jump)
			createGroundObstacle("20", glm::vec3(-340.f, 0.0f, 3.0), glm::vec3(1.f, 1.f, 1.f), glm::vec3(90.f, 0.0f, 73.f), vinesMesh, vinesMaterial); // vine 6 (jump blocking)
			createGroundObstacle("21", glm::vec3(-345.f, 5.530f, 0.250f), glm::vec3(1.5f, 1.5f, 1.5f), glm::vec3(90.f, 0.0f, -25.f), vinesMesh, vinesMaterial); // vine 7 (squish blocking)
			createGroundObstacle("22", glm::vec3(-345.240f, 0.f, 0.0f), glm::vec3(0.25f, 0.25f, 0.25f), glm::vec3(90.0f, 0.0f, 0.f), cobweb2Mesh, cobwebMaterial); //cobweb 6 (squish Blocking 2)
			createGroundObstacle("23", glm::vec3(-360.f, 0.0f, -3.5f), glm::vec3(0.25f, 0.25f, 0.25f), glm::vec3(90.0f, 0.0f, 0.f), cobweb2Mesh, cobwebMaterial); //cobweb 7 (tall jump)
			createGroundObstacle("24", glm::vec3(-380.f, 5.530f, 0.250f), glm::vec3(1.5f, 1.5f, 1.5f), glm::vec3(90.f, 0.0f, -25.f), vinesMesh, vinesMaterial); // vine 8 (squish blocking)
			createGroundObstacle("25", glm::vec3(-380.240f, 0.f, 0.0f), glm::vec3(0.25f, 0.25f, 0.25f), glm::vec3(90.0f, 0.0f, 0.f), cobweb2Mesh, cobwebMaterial); //cobweb 8 (squish Blocking 2)
			createGroundObstacle("26", glm::vec3(-395.f, 0.0f, -3.5f), glm::vec3(0.25f, 0.25f, 0.25f), glm::vec3(90.0f, 0.0f, 0.f), cobweb2Mesh, cobwebMaterial); //cobweb 9 (tall jump)

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


			GameObject::Sptr player = scene->CreateGameObject("player"); //pyr
			{
				// Set position in the scene
				player->SetPostion(glm::vec3(6.f, 0.0f, 1.0f));
				player->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
				player->SetScale(glm::vec3(0.5f, 0.5f, 0.5f));

				// Add some behaviour that relies on the physics body
				//player->Add<JumpBehaviour>(player->GetPosition());
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

			GameObject::Sptr ButtonBack3 = scene->CreateGameObject("ButtonBack3");
			{
				// Set position in the scene
				ButtonBack3->SetPostion(glm::vec3(1.f, 6.5f, 5.f));
				// Scale down the plane
				ButtonBack3->SetScale(glm::vec3(3.0f, 0.8f, 0.5f));
				//spin things
				ButtonBack3->SetRotation(glm::vec3(-80.0f, 0.f, 0.f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = ButtonBack3->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(ButtonMaterial);
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

			GameObject::Sptr LSText = scene->CreateGameObject("LSText");
			{
				// Set position in the scene
				LSText->SetPostion(glm::vec3(1.0f, 8.0f, 6.1f));
				// Scale down the plane
				LSText->SetScale(glm::vec3(2.0f, 0.4f, 0.5f));
				LSText->SetRotation(glm::vec3(80.0f, 0.f, -180.f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = LSText->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(LStextMaterial);
			}

			GameObject::Sptr FrogTongue = scene->CreateGameObject("FrogTongue");
			{
				// Set position in the scene
				FrogTongue->SetPostion(glm::vec3(-3.4f, -1.05f, 3.59f));
				// Scale down the plane
				FrogTongue->SetScale(glm::vec3(1.0f, 0.1f, 1.0f));
				FrogTongue->SetRotation(glm::vec3(0.0f, 0.0f, 45.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = FrogTongue->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(FrogTongueMaterial);
			}

			GameObject::Sptr FrogBody = scene->CreateGameObject("FrogBody");
			{
				// Set position in the scene
				FrogBody->SetPostion(glm::vec3(-3.4f, -1.05f, 3.6f));
				// Scale down the plane
				FrogBody->SetScale(glm::vec3(1.5f, 1.5f, 1.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = FrogBody->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(FrogBodyMaterial);
			}

			GameObject::Sptr FrogHeadBot = scene->CreateGameObject("FrogHeadBot");
			{
				// Set position in the scene
				FrogHeadBot->SetPostion(glm::vec3(-3.4f, -1.05f, 3.61f));
				// Scale down the plane
				FrogHeadBot->SetScale(glm::vec3(1.5f, 1.5f, 1.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = FrogHeadBot->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(FrogHeadBotMaterial);
			}

			GameObject::Sptr FrogHeadTop = scene->CreateGameObject("FrogHeadTop");
			{
				// Set position in the scene
				FrogHeadTop->SetPostion(glm::vec3(-3.4f, -1.05f, 3.62f));
				// Scale down the plane
				FrogHeadTop->SetScale(glm::vec3(1.5f, 1.5f, 1.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = FrogHeadTop->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(FrogHeadTopMaterial);
			}

			GameObject::Sptr BushTransition = scene->CreateGameObject("BushTransition");
			{
				// Set position in the scene
				BushTransition->SetPostion(glm::vec3(5.f, 0.0f, 3.8f));
				// Scale down the plane
				BushTransition->SetScale(glm::vec3(5.8f, 2.5f, 1.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = BushTransition->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(BushTransitionMaterial);
			}

			scene->SetAmbientLight(glm::vec3(0.2f));

			// Kinematic rigid bodies are those controlled by some outside controller
			// and ONLY collide with dynamic objects
			RigidBody::Sptr physics = jumpingObstacle->Add<RigidBody>(RigidBodyType::Kinematic);
			physics->AddCollider(ConvexMeshCollider::Create());

			// Save the asset manifest for all the resources we just loaded
			ResourceManager::SaveManifest("manifest.json");
			// Save the scene to a JSON file
			scene->Save("Level.json");

		}

		/// Working Level ///									//////		Level 1		////////// scenevalue == 1

		{
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
			Material::Sptr LStextMaterial = ResourceManager::CreateAsset<Material>();
			{
				LStextMaterial->Name = "LStext";
				LStextMaterial->MatShader = scene->BaseShader;
				LStextMaterial->Texture = LStextTex;
				LStextMaterial->Shininess = 2.0f;
			}


			Material::Sptr FrogBodyMaterial = ResourceManager::CreateAsset<Material>();
			{
				FrogBodyMaterial->Name = "FrogBody";
				FrogBodyMaterial->MatShader = scene->BaseShader;
				FrogBodyMaterial->Texture = FrogBodyTex;
				FrogBodyMaterial->Shininess = 2.0f;
			}

			Material::Sptr FrogHeadTopMaterial = ResourceManager::CreateAsset<Material>();
			{
				FrogHeadTopMaterial->Name = "FrogHeadTop";
				FrogHeadTopMaterial->MatShader = scene->BaseShader;
				FrogHeadTopMaterial->Texture = FrogHeadTopTex;
				FrogHeadTopMaterial->Shininess = 2.0f;
			}

			Material::Sptr FrogHeadBotMaterial = ResourceManager::CreateAsset<Material>();
			{
				FrogHeadBotMaterial->Name = "FrogHeadBot";
				FrogHeadBotMaterial->MatShader = scene->BaseShader;
				FrogHeadBotMaterial->Texture = FrogHeadBotTex;
				FrogHeadBotMaterial->Shininess = 2.0f;
			}

			Material::Sptr FrogTongueMaterial = ResourceManager::CreateAsset<Material>();
			{
				FrogTongueMaterial->Name = "FrogTongue";
				FrogTongueMaterial->MatShader = scene->BaseShader;
				FrogTongueMaterial->Texture = FrogTongueTex;
				FrogTongueMaterial->Shininess = 2.0f;
			}

			Material::Sptr BushTransitionMaterial = ResourceManager::CreateAsset<Material>();
			{
				BushTransitionMaterial->Name = "BushTransition";
				BushTransitionMaterial->MatShader = scene->BaseShader;
				BushTransitionMaterial->Texture = BushTransitionTex;
				BushTransitionMaterial->Shininess = 2.0f;
			}


			// Create some lights for our scene
			// Create some lights for our scene
			scene->Lights.resize(31);
			scene->Lights[0].Position = glm::vec3(-400.0f, 1.0f, 40.0f);
			scene->Lights[0].Color = glm::vec3(0.8619f, 1.f, 0.819f);
			scene->Lights[0].Range = 200.0f;

			scene->Lights[1].Position = glm::vec3(-450.f, 0.0f, 40.0f);
			scene->Lights[1].Color = glm::vec3(0.8619f, 1.f, 0.819f);
			scene->Lights[1].Range = 200.0f;

			scene->Lights[2].Position = glm::vec3(-500.f, 1.0f, 40.0f);
			scene->Lights[2].Color = glm::vec3(0.8619f, 1.f, 0.819f);
			scene->Lights[2].Range = 200.0f;

			scene->Lights[3].Position = glm::vec3(-550.0f, 1.0f, 40.0f);
			scene->Lights[3].Color = glm::vec3(0.8619f, 1.f, 0.819f);
			scene->Lights[3].Range = 200.0f;

			scene->Lights[4].Position = glm::vec3(-500.0f, 1.0f, 40.0f);
			scene->Lights[4].Color = glm::vec3(0.8619f, 1.f, 0.819f);
			scene->Lights[4].Range = 200.0f;

			scene->Lights[5].Position = glm::vec3(-550.0f, 1.0f, 40.0f);
			scene->Lights[5].Color = glm::vec3(0.8619f, 1.f, 0.819f);
			scene->Lights[5].Range = 200.0f;

			scene->Lights[6].Position = glm::vec3(-600.0f, 1.0f, 40.0f);
			scene->Lights[6].Color = glm::vec3(0.8619f, 1.f, 0.819f);
			scene->Lights[6].Range = 200.0f;

			scene->Lights[7].Position = glm::vec3(-650.0f, 1.0f, 40.0f);
			scene->Lights[7].Color = glm::vec3(0.8619f, 1.f, 0.819f);
			scene->Lights[7].Range = 200.0f;

			scene->Lights[8].Position = glm::vec3(-700.0f, 1.0f, 40.0f);
			scene->Lights[8].Color = glm::vec3(0.8619f, 1.f, 0.819f);
			scene->Lights[8].Range = 200.0f;

			scene->Lights[9].Position = glm::vec3(-750.0f, 1.0f, 40.0f);
			scene->Lights[9].Color = glm::vec3(0.8619f, 1.f, 0.819f);
			scene->Lights[9].Range = 200.0f;

			scene->Lights[10].Position = glm::vec3(-800.0f, 1.0f, 40.0f);
			scene->Lights[10].Color = glm::vec3(0.8619f, 1.f, 0.819f);
			scene->Lights[10].Range = 200.0f;

			scene->Lights[11].Position = glm::vec3(-400.0f, -90.0f, 100.0f);
			scene->Lights[11].Color = glm::vec3(0.45f, 0.678f, 0.1872f);
			scene->Lights[11].Range = 1000.0f;

			scene->Lights[12].Position = glm::vec3(-450.f, -90.0f, 100.0f);
			scene->Lights[12].Color = glm::vec3(0.45f, 0.678f, 0.1872f);
			scene->Lights[12].Range = 1000.0f;

			scene->Lights[13].Position = glm::vec3(-500.f, -90.0f, 100.0f);
			scene->Lights[13].Color = glm::vec3(0.45f, 0.678f, 0.1872f);
			scene->Lights[13].Range = 1000.0f;

			scene->Lights[14].Position = glm::vec3(-550.0f, -90.0f, 100.0f);
			scene->Lights[14].Color = glm::vec3(0.45f, 0.678f, 0.1872f);
			scene->Lights[14].Range = 1000.0f;

			scene->Lights[15].Position = glm::vec3(-500.0f, -90.0f, 100.0f);
			scene->Lights[15].Color = glm::vec3(0.45f, 0.678f, 0.1872f);
			scene->Lights[15].Range = 1000.0f;

			scene->Lights[16].Position = glm::vec3(-550.0f, -90.0f, 100.0f);
			scene->Lights[16].Color = glm::vec3(0.45f, 0.678f, 0.1872f);
			scene->Lights[16].Range = 1000.0f;

			scene->Lights[17].Position = glm::vec3(-600.0f, -90.0f, 100.0f);
			scene->Lights[17].Color = glm::vec3(0.45f, 0.678f, 0.1872f);
			scene->Lights[17].Range = 1000.0f;

			scene->Lights[18].Position = glm::vec3(-650.0f, -90.0f, 100.0f);
			scene->Lights[18].Color = glm::vec3(0.45f, 0.678f, 0.1872f);
			scene->Lights[18].Range = 1000.0f;

			scene->Lights[19].Position = glm::vec3(-700.0f, -90.0f, 100.0f);
			scene->Lights[19].Color = glm::vec3(0.45f, 0.678f, 0.1872f);
			scene->Lights[19].Range = 1000.0f;

			scene->Lights[20].Position = glm::vec3(-750.0f, -90.0f, 100.0f);
			scene->Lights[20].Color = glm::vec3(0.45f, 0.678f, 0.1872f);
			scene->Lights[20].Range = 1000.0f;

			scene->Lights[21].Position = glm::vec3(-800.0f, -90.0f, 100.0f);
			scene->Lights[21].Color = glm::vec3(0.45f, 0.678f, 0.1872f);
			scene->Lights[21].Range = 1000.0f;

			scene->Lights[22].Position = glm::vec3(-400.0f, 20.0f, 5.0f);
			scene->Lights[22].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[22].Range = 150.0f;

			scene->Lights[23].Position = glm::vec3(-450.0f, 20.0f, 5.0f);
			scene->Lights[23].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[23].Range = 150.0f;

			scene->Lights[24].Position = glm::vec3(-500.0f, 20.0f, 5.0f);
			scene->Lights[24].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[24].Range = 150.0f;

			scene->Lights[25].Position = glm::vec3(-550.0f, 20.0f, 5.0f);
			scene->Lights[25].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[25].Range = 150.0f;

			scene->Lights[26].Position = glm::vec3(-600.0f, 20.0f, 5.0f);
			scene->Lights[26].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[26].Range = 150.0f;

			scene->Lights[27].Position = glm::vec3(-650.0f, 20.0f, 5.0f);
			scene->Lights[27].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[27].Range = 150.0f;

			scene->Lights[28].Position = glm::vec3(-700.0f, 20.0f, 5.0f);
			scene->Lights[28].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[28].Range = 150.0f;

			scene->Lights[29].Position = glm::vec3(-750.0f, 20.0f, 5.0f);
			scene->Lights[29].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[29].Range = 150.0f;

			scene->Lights[30].Position = glm::vec3(-800.0f, 20.0f, 5.0f);
			scene->Lights[30].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[30].Range = 150.0f;

			// We'll create a mesh that is a simple plane that we can resize later
			planeMesh = ResourceManager::CreateAsset<MeshResource>();
			cubeMesh = ResourceManager::CreateAsset<MeshResource>("cube.obj");

			mushroomMesh = ResourceManager::CreateAsset<MeshResource>("Mushroom.obj");
			tmMesh = ResourceManager::CreateAsset<MeshResource>("tm.obj");
			bmMesh = ResourceManager::CreateAsset<MeshResource>("bm.obj");
			ToadMesh = ResourceManager::CreateAsset<MeshResource>("ToadStool.obj");


			BranchMesh = ResourceManager::CreateAsset<MeshResource>("Branch.obj");
			LogMesh = ResourceManager::CreateAsset<MeshResource>("Log.obj");
			Plant2Mesh = ResourceManager::CreateAsset<MeshResource>("Plant2.obj");
			Plant3Mesh = ResourceManager::CreateAsset<MeshResource>("Plant3.obj");

			SunflowerMesh = ResourceManager::CreateAsset<MeshResource>("Sunflower.obj");

			Rock1Mesh = ResourceManager::CreateAsset<MeshResource>("Rock1.obj");
			Rock2Mesh = ResourceManager::CreateAsset<MeshResource>("Rock2.obj");
			Rock3Mesh = ResourceManager::CreateAsset<MeshResource>("Rock3.obj");
			twigMesh = ResourceManager::CreateAsset<MeshResource>("Twig.obj");
			vinesMesh = ResourceManager::CreateAsset<MeshResource>("Vines.obj");

			ladybugMesh = ResourceManager::CreateAsset<MeshResource>("lbo2.obj"); //real lbm
			frogMesh = ResourceManager::CreateAsset<MeshResource>("frog.obj");

			BGMesh = ResourceManager::CreateAsset<MeshResource>("Background.obj");
			ExitTreeMesh = ResourceManager::CreateAsset<MeshResource>("ExitTree.obj");

			//Running Mesh
			runningMesh1 = ResourceManager::CreateAsset<MeshResource>("Running_000001.obj");
			runningMesh2 = ResourceManager::CreateAsset<MeshResource>("Running_000002.obj");
			runningMesh3 = ResourceManager::CreateAsset<MeshResource>("Running_000003.obj");
			runningMesh4 = ResourceManager::CreateAsset<MeshResource>("Running_000004.obj");
			runningMesh5 = ResourceManager::CreateAsset<MeshResource>("Running_000005.obj");
			runningMesh6 = ResourceManager::CreateAsset<MeshResource>("Running_000006.obj");
			runningMesh7 = ResourceManager::CreateAsset<MeshResource>("Running_000007.obj");
			runningMesh8 = ResourceManager::CreateAsset<MeshResource>("Running_000008.obj");
			runningMesh9 = ResourceManager::CreateAsset<MeshResource>("Running_000009.obj");
			runningMesh10 = ResourceManager::CreateAsset<MeshResource>("Running_000010.obj");
			runningMesh11 = ResourceManager::CreateAsset<MeshResource>("Running_000011.obj");
			runningMesh12 = ResourceManager::CreateAsset<MeshResource>("Running_000012.obj");
			runningMesh13 = ResourceManager::CreateAsset<MeshResource>("Running_000013.obj");
			runningMesh14 = ResourceManager::CreateAsset<MeshResource>("Running_000014.obj");
			runningMesh15 = ResourceManager::CreateAsset<MeshResource>("Running_000015.obj");
			runningMesh16 = ResourceManager::CreateAsset<MeshResource>("Running_000016.obj");
			runningMesh17 = ResourceManager::CreateAsset<MeshResource>("Running_000017.obj");
			runningMesh18 = ResourceManager::CreateAsset<MeshResource>("Running_000018.obj");

			//Flying Mesh
			flyingMesh1 = ResourceManager::CreateAsset<MeshResource>("NewLadybug_000001.obj");
			flyingMesh2 = ResourceManager::CreateAsset<MeshResource>("NewLadybug_000002.obj");
			flyingMesh3 = ResourceManager::CreateAsset<MeshResource>("NewLadybug_000003.obj");
			flyingMesh4 = ResourceManager::CreateAsset<MeshResource>("NewLadybug_000004.obj");
			flyingMesh5 = ResourceManager::CreateAsset<MeshResource>("NewLadybug_000005.obj");
			flyingMesh6 = ResourceManager::CreateAsset<MeshResource>("NewLadybug_000006.obj");
			flyingMesh7 = ResourceManager::CreateAsset<MeshResource>("NewLadybug_000007.obj");
			flyingMesh8 = ResourceManager::CreateAsset<MeshResource>("NewLadybug_000008.obj");

			//Sliding Mesh
			slidingMesh1 = ResourceManager::CreateAsset<MeshResource>("RunToCrawl_000001.obj");
			slidingMesh2 = ResourceManager::CreateAsset<MeshResource>("RunToCrawl_000002.obj");
			slidingMesh3 = ResourceManager::CreateAsset<MeshResource>("RunToCrawl_000003.obj");
			slidingMesh4 = ResourceManager::CreateAsset<MeshResource>("RunToCrawl_000004.obj");
			slidingMesh5 = ResourceManager::CreateAsset<MeshResource>("RunToCrawl_000005.obj");
			slidingMesh6 = ResourceManager::CreateAsset<MeshResource>("RunToCrawl_000006.obj");
			slidingMesh7 = ResourceManager::CreateAsset<MeshResource>("RunToCrawl_000007.obj");
			slidingMesh8 = ResourceManager::CreateAsset<MeshResource>("RunToCrawl_000007.obj");
			slidingMesh9 = ResourceManager::CreateAsset<MeshResource>("Crawling_000001.obj");
			slidingMesh10 = ResourceManager::CreateAsset<MeshResource>("Crawling_000002.obj");
			slidingMesh11 = ResourceManager::CreateAsset<MeshResource>("Crawling_000003.obj");
			slidingMesh12 = ResourceManager::CreateAsset<MeshResource>("Crawling_000004.obj");
			slidingMesh13 = ResourceManager::CreateAsset<MeshResource>("Crawling_000005.obj");
			slidingMesh14 = ResourceManager::CreateAsset<MeshResource>("Crawling_000006.obj");
			slidingMesh15 = ResourceManager::CreateAsset<MeshResource>("Crawling_000007.obj");
			slidingMesh16 = ResourceManager::CreateAsset<MeshResource>("Crawling_000008.obj");
			slidingMesh17 = ResourceManager::CreateAsset<MeshResource>("Crawling_000009.obj");
			slidingMesh18 = ResourceManager::CreateAsset<MeshResource>("Crawling_000010.obj");
			slidingMesh19 = ResourceManager::CreateAsset<MeshResource>("CrawlToRun_000001.obj");
			slidingMesh20 = ResourceManager::CreateAsset<MeshResource>("CrawlToRun_000002.obj");
			slidingMesh21 = ResourceManager::CreateAsset<MeshResource>("CrawlToRun_000003.obj");
			slidingMesh22 = ResourceManager::CreateAsset<MeshResource>("CrawlToRun_000004.obj");
			slidingMesh23 = ResourceManager::CreateAsset<MeshResource>("CrawlToRun_000005.obj");
			slidingMesh24 = ResourceManager::CreateAsset<MeshResource>("CrawlToRun_000006.obj");
			slidingMesh25 = ResourceManager::CreateAsset<MeshResource>("CrawlToRun_000007.obj");
			slidingMesh26 = ResourceManager::CreateAsset<MeshResource>("CrawlToRun_000007.obj");

			planeMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(1.0f)));
			planeMesh->GenerateMesh();



			//Background Assets
			createBackgroundAsset("1", glm::vec3(-401.f, 5.0f, -0.f), 0.5, glm::vec3(83.f, 5.0f, 0.0f), BranchMesh, BranchMaterial);
			createBackgroundAsset("2", glm::vec3(-600.f, 5.0f, -0.f), 0.5, glm::vec3(83.f, 5.0f, 0.0f), BranchMesh, BranchMaterial);

			createBackgroundAsset("3", glm::vec3(-586.f, 4.0f, -0.660), 3, glm::vec3(83.f, 5.0f, 0.0f), LogMesh, LogMaterial);
			createBackgroundAsset("4", glm::vec3(-420.f, 0.0f, -0.660), 0.5, glm::vec3(400.f, 5.0f, 0.0f), LogMesh, LogMaterial);

			createBackgroundAsset("6", glm::vec3(-540.5, 5.470f, -0.330), 6.0, glm::vec3(83.f, 5.0f, 0.0f), Plant2Mesh, PlantMaterial);
			createBackgroundAsset("7", glm::vec3(-640.5, 5.470f, -0.330), 6.0, glm::vec3(83.f, 5.0f, 0.0f), Plant3Mesh, PlantMaterial);

			createBackgroundAsset("8", glm::vec3(-420.f, 0.0f, -0.660), 6.0, glm::vec3(83.f, 5.0f, 0.0f), SunflowerMesh, SunflowerMaterial);

			createBackgroundAsset("9", glm::vec3(-413.f, 5.0f, -0.430), 0.5, glm::vec3(83.f, 5.0f, 90.0f), ToadMesh, ToadMaterial);
			createBackgroundAsset("10", glm::vec3(-490.f, 5.0f, -0.430), 0.5, glm::vec3(83.f, 5.0f, 90.0f), ToadMesh, ToadMaterial);
			createBackgroundAsset("11", glm::vec3(-730.f, 5.0f, -0.430), 0.5, glm::vec3(83.f, 5.0f, 90.0f), ToadMesh, ToadMaterial);

			createBackgroundAsset("12", glm::vec3(-420.f, 0.0f, -0.660), 0.5, glm::vec3(83.f, 5.0f, 90.0f), Rock1Mesh, boxMaterial);
			createBackgroundAsset("13", glm::vec3(-420.f, 0.0f, -0.660), 0.5, glm::vec3(83.f, 5.0f, 90.0f), Rock2Mesh, rockMaterial);
			createBackgroundAsset("14", glm::vec3(-420.f, 0.0f, -0.660), 0.5, glm::vec3(83.f, 5.0f, 90.0f), Rock3Mesh, rockMaterial);

			createBackgroundAsset("15", glm::vec3(-420.f, 6.0f, -0.660), 0.5, glm::vec3(83.f, 5.0f, 90.0f), twigMesh, twigMaterial);
			createBackgroundAsset("16", glm::vec3(-387.060f, 1.530f, 0.550), 0.05, glm::vec3(83.f, -7.0f, 90.0f), frogMesh, frogMaterial);

			createBackgroundAsset("17", glm::vec3(-392.5f, 4.6f, -1.8), 0.5, glm::vec3(83.f, -7.0f, 90.0f), bmMesh, bmMaterial);
			createBackgroundAsset("18", glm::vec3(-406.f, -12.660f, 0.400), 0.05, glm::vec3(83.f, -7.0f, 90.0f), tmMesh, tmMaterial);

			//Obstacles

			//createGroundObstacle("18", glm::vec3(-320.f, 0.0f, -0.660), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(90.f, 0.0f, 0.0f), mushroomMesh, mushroomMaterial); //mushroom 8 (small jump)
			//createGroundObstacle("20", glm::vec3(-340.f, 0.0f, 3.0), glm::vec3(1.f, 1.f, 1.f), glm::vec3(90.f, 0.0f, 73.f), vinesMesh, vinesMaterial); // vine 6 (jump blocking)
			//createGroundObstacle("24", glm::vec3(-380.f, 5.530f, 0.250f), glm::vec3(1.5f, 1.5f, 1.5f), glm::vec3(90.f, 0.0f, -25.f), vinesMesh, vinesMaterial); // vine 8 (squish blocking)

			createGroundObstacle("1", glm::vec3(-450.f, 0.0f, -0.660), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(90.f, 0.0f, 0.0f), mushroomMesh, mushroomMaterial); //red mushroom 1 (small jump)
			createGroundObstacle("2", glm::vec3(-500.f, 0.0f, -0.660), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(90.f, 0.0f, 0.0f), mushroomMesh, mushroomMaterial); //red mushroom 2 (small jump)
			createGroundObstacle("3", glm::vec3(-550.f, 0.0f, -0.660), glm::vec3(1.f), glm::vec3(90.f, 0.0f, 0.0f), tmMesh, tmMaterial); // tall mushroom 1 (small jump)
			createGroundObstacle("4", glm::vec3(-600.f, 0.0f, -0.660), glm::vec3(1.f), glm::vec3(90.f, 0.0f, 0.0f), bmMesh, bmMaterial); //branch mushroom 1 (small jump)

			createGroundObstacle("5", glm::vec3(-650.f, 0.0f, -0.660), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(90.f, 0.0f, 0.0f), mushroomMesh, mushroomMaterial); //red mushroom 3 (small jump)
			createGroundObstacle("6", glm::vec3(-680.f, 0.0f, -0.660), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(90.f, 0.0f, 0.0f), mushroomMesh, mushroomMaterial); //red mushroom 4 (small jump)

			createGroundObstacle("7", glm::vec3(-710.f, 0.0f, -0.660), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(90.f, 0.0f, 0.0f), mushroomMesh, mushroomMaterial); //red mushroom 5 (small jump)
			createGroundObstacle("8", glm::vec3(-750.f, 0.0f, -0.660), glm::vec3(1.f), glm::vec3(90.f, 0.0f, 0.0f), bmMesh, bmMaterial); //branch mushroom 2 (small jump)


			//3D Backgrounds
			createGroundObstacle("27", glm::vec3(-292.3f, -55.830f, -1.7f), glm::vec3(6.f, 6.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGMesh, BGMaterial);
			createGroundObstacle("28", glm::vec3(-400.f, -55.830f, -1.7f), glm::vec3(6.f, 6.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGMesh, BGMaterial);
			createGroundObstacle("29", glm::vec3(-507.7f, -55.830f, -1.7f), glm::vec3(6.f, 6.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGMesh, BGMaterial);

			createGroundObstacle("30", glm::vec3(-615.4f, -55.830f, -1.7f), glm::vec3(6.f, 6.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGMesh, BGMaterial);
			createGroundObstacle("31", glm::vec3(-723.1f, -55.830f, -1.7f), glm::vec3(6.f, 6.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGMesh, BGMaterial);
			createGroundObstacle("32", glm::vec3(-830.8f, -55.830f, -1.7f), glm::vec3(6.f, 6.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGMesh, BGMaterial);

			//2DBackGrounds
			createGroundObstacle("33", glm::vec3(-50.f, -130.0f, 64.130f), glm::vec3(375.0f, 125.0f, 250.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, bgMaterial);
			createGroundObstacle("34", glm::vec3(-450.f, -130.0f, 64.130f), glm::vec3(375.0f, 125.0f, 250.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, bgMaterial);
			createGroundObstacle("35", glm::vec3(-825.f, -130.0f, 64.130f), glm::vec3(375.0f, 125.0f, 250.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, bgMaterial);
			createGroundObstacle("36", glm::vec3(-1625.f, -130.0f, 64.130f), glm::vec3(375.0f, 125.0f, 250.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, bgMaterial);
			createGroundObstacle("37", glm::vec3(-2150.f, -130.0f, 64.130f), glm::vec3(375.0f, 125.0f, 250.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, bgMaterial);

			//Grass
			createGroundObstacle("38", glm::vec3(-418.170f, -38.230f, 5.250f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass1Material);
			createGroundObstacle("39", glm::vec3(-370.42f, 46.920f, 5.080f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass2Material);
			createGroundObstacle("40", glm::vec3(-397.91f, -47.360f, 5.090f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass3Material);
			createGroundObstacle("41", glm::vec3(-364.3f, -37.550f, 5.310f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass4Material);
			createGroundObstacle("42", glm::vec3(-447.340f, -37.120f, 5.560f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass5Material);

			createGroundObstacle("43", glm::vec3(-418.170f - 103.5f, -38.230f, 5.250f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass1Material);
			createGroundObstacle("44", glm::vec3(-370.42f - 103.5f, 46.920f, 5.080f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass2Material);
			createGroundObstacle("45", glm::vec3(-397.91f - 103.5f, -47.360f, 5.090f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass3Material);
			createGroundObstacle("46", glm::vec3(-364.3f - 103.5f, -37.550f, 5.310f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass4Material);
			createGroundObstacle("47", glm::vec3(-47.340f - 103.5f - 400.f, -37.120f, 5.560f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass5Material);

			createGroundObstacle("48", glm::vec3(-18.170f - 103.5f - 103.5f - 400.f, -38.230f, 5.250f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass1Material);
			createGroundObstacle("49", glm::vec3(29.580f - 103.5f - 103.5f - 400.f, 46.920f, 5.080f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass2Material);
			createGroundObstacle("50", glm::vec3(2.090f - 103.5f - 103.5f - 400.f, -47.360f, 5.090f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass3Material);
			createGroundObstacle("51", glm::vec3(35.700f - 103.5f - 103.5f - 400.f, -37.550f, 5.310f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass4Material);
			createGroundObstacle("52", glm::vec3(-47.340f - 103.5f - 103.5f - 400.f, -37.120f, 5.560f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass5Material);

			createGroundObstacle("53", glm::vec3(-18.170f - 103.5f - 103.5f - 103.5f - 400.f, -38.230f, 5.250f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass1Material);
			createGroundObstacle("54", glm::vec3(29.580f - 103.5f - 103.5f - 103.5f - 400.f, 46.920f, 5.080f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass2Material);
			createGroundObstacle("55", glm::vec3(2.090f - 103.5f - 103.5f - 103.5f - 400.f, -47.360f, 5.090f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass3Material);
			createGroundObstacle("56", glm::vec3(35.700f - 103.5f - 103.5f - 103.5f - 400.f, -37.550f, 5.310f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass4Material);
			createGroundObstacle("57", glm::vec3(-47.340f - 103.5f - 103.5f - 103.5f - 400.f, -37.120f, 5.560f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass5Material);

			//Exit Tree
			createGroundObstacle("58", glm::vec3(-409.f - 400.f, -3.380f, -0.340f), glm::vec3(3.0f, 3.0f, 3.0f), glm::vec3(90.0f, 0.0f, 140.f), ExitTreeMesh, ExitTreeMaterial);

			//Foreground Grass
			createGroundObstacle("59", glm::vec3(40.f - 400.f, -14.390f, 5.390f), glm::vec3(40.0f, 11.770f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, ForegroundMaterial);
			createGroundObstacle("60", glm::vec3(0.f - 400.f, -14.390f, 5.390f), glm::vec3(40.0f, 11.770f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, ForegroundMaterial);
			createGroundObstacle("61", glm::vec3(-40.f - 400.f, -14.390f, 5.390f), glm::vec3(40.0f, 11.770f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, ForegroundMaterial);
			createGroundObstacle("62", glm::vec3(-80.f - 400.f, -14.390f, 5.390f), glm::vec3(40.0f, 11.770f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, ForegroundMaterial);
			createGroundObstacle("63", glm::vec3(-120.f - 400.f, -14.390f, 5.390f), glm::vec3(40.0f, 11.770f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, ForegroundMaterial);
			createGroundObstacle("64", glm::vec3(-160.f - 400.f, -14.390f, 5.390f), glm::vec3(40.0f, 11.770f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, ForegroundMaterial);

			createGroundObstacle("65", glm::vec3(-200.f - 400.f, -14.390f, 5.390f), glm::vec3(40.0f, 11.770f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, ForegroundMaterial);
			createGroundObstacle("66", glm::vec3(-240.f - 400.f, -14.390f, 5.390f), glm::vec3(40.0f, 11.770f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, ForegroundMaterial);
			createGroundObstacle("67", glm::vec3(-280.f - 400.f, -14.390f, 5.390f), glm::vec3(40.0f, 11.770f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, ForegroundMaterial);
			createGroundObstacle("68", glm::vec3(-320.f - 400.f, -14.390f, 5.390f), glm::vec3(40.0f, 11.770f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, ForegroundMaterial);
			createGroundObstacle("69", glm::vec3(-360.f - 400.f, -14.390f, 5.390f), glm::vec3(40.0f, 11.770f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, ForegroundMaterial);
			createGroundObstacle("70", glm::vec3(-400.f - 400.f, -14.390f, 5.390f), glm::vec3(40.0f, 11.770f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, ForegroundMaterial);

			//red mushroom 1
			createCollision("101", -19.660f - 430.f, 1.560f, 1.f, 1.f);
			createCollision("102", -20.410f - 430.f, 1.560f, 1.f, 1.f);
			createCollision("103", -19.970f - 430.f, 1.860f, 1.f, 1.f);
			createCollision("104", -20.190f - 430.f, 0.450f, 1.f, 1.f);

			//red mushroom 2
			createCollision("111", -19.660f - 480.f, 1.560f, 1.f, 1.f);
			createCollision("111", -20.410f - 480.f, 1.560f, 1.f, 1.f);
			createCollision("111", -19.970f - 480.f, 1.860f, 1.f, 1.f);
			createCollision("111", -20.190f - 480.f, 0.450f, 1.f, 1.f);

			//tall mushroom
			createCollision("121", -19.660f - 530.f, 1.560f, 1.f, 1.f);
			createCollision("122", -20.410f - 530.f, 1.560f, 1.f, 1.f);
			createCollision("123", -19.970f - 530.f, 1.860f, 1.f, 1.f);
			createCollision("124", -20.190f - 530.f, 0.450f, 1.f, 1.f);
			createCollision("125", -550.f, 3.430f, 2.f, 4.f);

			//big mushroom
			createCollision("131", -19.660f - 580.f, 1.560f, 1.f, 1.f);
			createCollision("132", -20.410f - 580.f, 1.560f, 1.f, 1.f);
			createCollision("133", -19.970f - 580.f, 1.860f, 1.f, 1.f);
			createCollision("134", -20.190f - 580.f, 0.450f, 1.f, 1.f);
			createCollision("135", -600.f, 3.080f, 3.f, 4.f);
			createCollision("136", -600.f, 3.22f, 4.f, 2.f);

			//red mushroom 3
			createCollision("141", -19.660f - 630.f, 1.560f, 1.f, 1.f);
			createCollision("142", -20.410f - 630.f, 1.560f, 1.f, 1.f);
			createCollision("143", -19.970f - 630.f, 1.860f, 1.f, 1.f);
			createCollision("144", -20.190f - 630.f, 0.450f, 1.f, 1.f);

			//red mushroom 4
			createCollision("151", -19.660f - 660.f, 1.560f, 1.f, 1.f);
			createCollision("152", -20.410f - 660.f, 1.560f, 1.f, 1.f);
			createCollision("153", -19.970f - 660.f, 1.860f, 1.f, 1.f);
			createCollision("154", -20.190f - 660.f, 0.450f, 1.f, 1.f);

			//red mushroom 5
			createCollision("161", -19.660f - 690.f, 1.560f, 1.f, 1.f);
			createCollision("162", -20.410f - 690.f, 1.560f, 1.f, 1.f);
			createCollision("163", -19.970f - 690.f, 1.860f, 1.f, 1.f);
			createCollision("164", -20.190f - 690.f, 0.450f, 1.f, 1.f);

			//big mushroom 2
			createCollision("171", -19.660f - 730.f, 1.560f, 1.f, 1.f);
			createCollision("172", -20.410f - 730.f, 1.560f, 1.f, 1.f);
			createCollision("173", -19.970f - 730.f, 1.860f, 1.f, 1.f);
			createCollision("174", -20.190f - 730.f, 0.450f, 1.f, 1.f);
			createCollision("175", -750.f, 3.080f, 3.f, 4.f);
			createCollision("176", -750.f, 3.22f, 4.f, 2.f);



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
				player->SetPostion(glm::vec3(-406.f, 0.0f, 1.0f));
				player->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
				player->SetScale(glm::vec3(0.5f, 0.5f, 0.5f));

				// Add some behaviour that relies on the physics body
				//player->Add<JumpBehaviour>(player->GetPosition());
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

			GameObject::Sptr ButtonBack3 = scene->CreateGameObject("ButtonBack3");
			{
				// Set position in the scene
				ButtonBack3->SetPostion(glm::vec3(1.f, 6.5f, 5.f));
				// Scale down the plane
				ButtonBack3->SetScale(glm::vec3(3.0f, 0.8f, 0.5f));
				//spin things
				ButtonBack3->SetRotation(glm::vec3(-80.0f, 0.f, 0.f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = ButtonBack3->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(ButtonMaterial);
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

			GameObject::Sptr LSText = scene->CreateGameObject("LSText");
			{
				// Set position in the scene
				LSText->SetPostion(glm::vec3(1.0f, 8.0f, 6.1f));
				// Scale down the plane
				LSText->SetScale(glm::vec3(2.0f, 0.4f, 0.5f));
				LSText->SetRotation(glm::vec3(80.0f, 0.f, -180.f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = LSText->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(LStextMaterial);
			}


			GameObject::Sptr FrogTongue = scene->CreateGameObject("FrogTongue");
			{
				// Set position in the scene
				FrogTongue->SetPostion(glm::vec3(-3.4f, -1.05f, 3.59f));
				// Scale down the plane
				FrogTongue->SetScale(glm::vec3(1.0f, 0.1f, 1.0f));
				FrogTongue->SetRotation(glm::vec3(0.0f, 0.0f, 45.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = FrogTongue->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(FrogTongueMaterial);
			}

			GameObject::Sptr FrogBody = scene->CreateGameObject("FrogBody");
			{
				// Set position in the scene
				FrogBody->SetPostion(glm::vec3(-3.4f, -1.05f, 3.6f));
				// Scale down the plane
				FrogBody->SetScale(glm::vec3(1.5f, 1.5f, 1.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = FrogBody->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(FrogBodyMaterial);
			}

			GameObject::Sptr FrogHeadBot = scene->CreateGameObject("FrogHeadBot");
			{
				// Set position in the scene
				FrogHeadBot->SetPostion(glm::vec3(-3.4f, -1.05f, 3.61f));
				// Scale down the plane
				FrogHeadBot->SetScale(glm::vec3(1.5f, 1.5f, 1.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = FrogHeadBot->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(FrogHeadBotMaterial);
			}

			GameObject::Sptr FrogHeadTop = scene->CreateGameObject("FrogHeadTop");
			{
				// Set position in the scene
				FrogHeadTop->SetPostion(glm::vec3(-3.4f, -1.05f, 3.62f));
				// Scale down the plane
				FrogHeadTop->SetScale(glm::vec3(1.5f, 1.5f, 1.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = FrogHeadTop->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(FrogHeadTopMaterial);
			}

			GameObject::Sptr BushTransition = scene->CreateGameObject("BushTransition");
			{
				// Set position in the scene
				BushTransition->SetPostion(glm::vec3(5.f, 0.0f, 3.8f));
				// Scale down the plane
				BushTransition->SetScale(glm::vec3(5.8f, 2.5f, 1.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = BushTransition->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(BushTransitionMaterial);
			}

			scene->SetAmbientLight(glm::vec3(0.2f));

			// Kinematic rigid bodies are those controlled by some outside controller
			// and ONLY collide with dynamic objects
			RigidBody::Sptr physics = jumpingObstacle->Add<RigidBody>(RigidBodyType::Kinematic);
			physics->AddCollider(ConvexMeshCollider::Create());

			// Save the asset manifest for all the resources we just loaded
			ResourceManager::SaveManifest("manifest.json");
			// Save the scene to a JSON file
			scene->Save("Level1.json");

		}


		/// Working Level ///									//////		Level 3	////////// scenevalue == 3

		{
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
				BGMaterial->Texture = MBGTex;
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
				ProgressBarMaterial->Texture = Progress2Tex;
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
			Material::Sptr LStextMaterial = ResourceManager::CreateAsset<Material>();
			{
				LStextMaterial->Name = "LStext";
				LStextMaterial->MatShader = scene->BaseShader;
				LStextMaterial->Texture = LStextTex;
				LStextMaterial->Shininess = 2.0f;
			}
			Material::Sptr ExitRockMaterial = ResourceManager::CreateAsset<Material>();
			{
				ExitRockMaterial->Name = "ExitRock";
				ExitRockMaterial->MatShader = scene->BaseShader;
				ExitRockMaterial->Texture = ExitRockTex;
				ExitRockMaterial->Shininess = 2.0f;
			}


			Material::Sptr FrogBodyMaterial = ResourceManager::CreateAsset<Material>();
			{
				FrogBodyMaterial->Name = "FrogBody";
				FrogBodyMaterial->MatShader = scene->BaseShader;
				FrogBodyMaterial->Texture = FrogBodyTex;
				FrogBodyMaterial->Shininess = 2.0f;
			}

			Material::Sptr FrogHeadTopMaterial = ResourceManager::CreateAsset<Material>();
			{
				FrogHeadTopMaterial->Name = "FrogHeadTop";
				FrogHeadTopMaterial->MatShader = scene->BaseShader;
				FrogHeadTopMaterial->Texture = FrogHeadTopTex;
				FrogHeadTopMaterial->Shininess = 2.0f;
			}

			Material::Sptr FrogHeadBotMaterial = ResourceManager::CreateAsset<Material>();
			{
				FrogHeadBotMaterial->Name = "FrogHeadBot";
				FrogHeadBotMaterial->MatShader = scene->BaseShader;
				FrogHeadBotMaterial->Texture = FrogHeadBotTex;
				FrogHeadBotMaterial->Shininess = 2.0f;
			}

			Material::Sptr FrogTongueMaterial = ResourceManager::CreateAsset<Material>();
			{
				FrogTongueMaterial->Name = "FrogTongue";
				FrogTongueMaterial->MatShader = scene->BaseShader;
				FrogTongueMaterial->Texture = FrogTongueTex;
				FrogTongueMaterial->Shininess = 2.0f;
			}

			Material::Sptr BushTransitionMaterial = ResourceManager::CreateAsset<Material>();
			{
				BushTransitionMaterial->Name = "BushTransition";
				BushTransitionMaterial->MatShader = scene->BaseShader;
				BushTransitionMaterial->Texture = BushTransitionTex;
				BushTransitionMaterial->Shininess = 2.0f;
			}

			Material::Sptr SignPostMaterial = ResourceManager::CreateAsset<Material>();
			{
				SignPostMaterial->Name = "SignPost";
				SignPostMaterial->MatShader = scene->BaseShader;
				SignPostMaterial->Texture = SignPostTex;
				SignPostMaterial->Shininess = 2.0f;
			}

			Material::Sptr PuddleMaterial = ResourceManager::CreateAsset<Material>();
			{
				PuddleMaterial->Name = "Puddle";
				PuddleMaterial->MatShader = scene->BaseShader;
				PuddleMaterial->Texture = PuddleTex;
				PuddleMaterial->Shininess = 2.0f;
			}

			Material::Sptr CampfireMaterial = ResourceManager::CreateAsset<Material>();
			{
				CampfireMaterial->Name = "Campfire";
				CampfireMaterial->MatShader = scene->BaseShader;
				CampfireMaterial->Texture = CampfireTex;
				CampfireMaterial->Shininess = 2.0f;
			}

			Material::Sptr HangingRockMaterial = ResourceManager::CreateAsset<Material>();
			{
				HangingRockMaterial->Name = "HangingRock";
				HangingRockMaterial->MatShader = scene->BaseShader;
				HangingRockMaterial->Texture = HangingRockTex;
				HangingRockMaterial->Shininess = 2.0f;
			}

			Material::Sptr RockPileMaterial = ResourceManager::CreateAsset<Material>();
			{
				RockPileMaterial->Name = "RockPile";
				RockPileMaterial->MatShader = scene->BaseShader;
				RockPileMaterial->Texture = RockPileTex;
				RockPileMaterial->Shininess = 2.0f;
			}

			Material::Sptr RockTunnelMaterial = ResourceManager::CreateAsset<Material>();
			{
				RockTunnelMaterial->Name = "RockTunnel";
				RockTunnelMaterial->MatShader = scene->BaseShader;
				RockTunnelMaterial->Texture = RockTunnelTex;
				RockTunnelMaterial->Shininess = 2.0f;
			}

			Material::Sptr RockWallMaterial = ResourceManager::CreateAsset<Material>();
			{
				RockWallMaterial->Name = "RockWall";
				RockWallMaterial->MatShader = scene->BaseShader;
				RockWallMaterial->Texture = RockWallTex;
				RockWallMaterial->Shininess = 2.0f;
			}

			Material::Sptr MountainBackdropMaterial = ResourceManager::CreateAsset<Material>();
			{
				MountainBackdropMaterial->Name = "Backdrop";
				MountainBackdropMaterial->MatShader = scene->BaseShader;
				MountainBackdropMaterial->Texture = MountainBackdropTex;
				MountainBackdropMaterial->Shininess = 2.0f;
			}

			Material::Sptr MountainForegroundMaterial = ResourceManager::CreateAsset<Material>();
			{
				MountainForegroundMaterial->Name = "Foreground";
				MountainForegroundMaterial->MatShader = scene->BaseShader;
				MountainForegroundMaterial->Texture = MountainForegroundTex;
				MountainForegroundMaterial->Shininess = 2.0f;
			}


			// Create some lights for our scene
			scene->Lights.resize(31);
			scene->Lights[0].Position = glm::vec3(-400.0f - 400.f, 1.0f, 40.0f);
			scene->Lights[0].Color = glm::vec3(0.897f, 1.f, 1.f);
			scene->Lights[0].Range = 200.0f;

			scene->Lights[1].Position = glm::vec3(-450.f - 400.f, 0.0f, 40.0f);
			scene->Lights[1].Color = glm::vec3(0.897f, 1.f, 1.f);
			scene->Lights[1].Range = 200.0f;

			scene->Lights[2].Position = glm::vec3(-500.f - 400.f, 1.0f, 40.0f);
			scene->Lights[2].Color = glm::vec3(0.897f, 1.f, 1.f);
			scene->Lights[2].Range = 200.0f;

			scene->Lights[3].Position = glm::vec3(-550.0f - 400.f, 1.0f, 40.0f);
			scene->Lights[3].Color = glm::vec3(0.897f, 1.f, 1.f);
			scene->Lights[3].Range = 200.0f;

			scene->Lights[4].Position = glm::vec3(-500.0f - 400.f, 1.0f, 40.0f);
			scene->Lights[4].Color = glm::vec3(0.897f, 1.f, 1.f);
			scene->Lights[4].Range = 200.0f;

			scene->Lights[5].Position = glm::vec3(-550.0f - 400.f, 1.0f, 40.0f);
			scene->Lights[5].Color = glm::vec3(0.897f, 1.f, 1.f);
			scene->Lights[5].Range = 200.0f;

			scene->Lights[6].Position = glm::vec3(-600.0f - 400.f, 1.0f, 40.0f);
			scene->Lights[6].Color = glm::vec3(0.897f, 1.f, 1.f);
			scene->Lights[6].Range = 200.0f;

			scene->Lights[7].Position = glm::vec3(-650.0f - 400.f, 1.0f, 40.0f);
			scene->Lights[7].Color = glm::vec3(0.897f, 1.f, 1.f);
			scene->Lights[7].Range = 200.0f;

			scene->Lights[8].Position = glm::vec3(-700.0f - 400.f, 1.0f, 40.0f);
			scene->Lights[8].Color = glm::vec3(0.897f, 1.f, 1.f);
			scene->Lights[8].Range = 200.0f;

			scene->Lights[9].Position = glm::vec3(-750.0f - 400.f, 1.0f, 40.0f);
			scene->Lights[9].Color = glm::vec3(0.897f, 1.f, 1.f);
			scene->Lights[9].Range = 200.0f;

			scene->Lights[10].Position = glm::vec3(-800.0f - 400.f, 1.0f, 40.0f);
			scene->Lights[10].Color = glm::vec3(0.897f, 1.f, 1.f);
			scene->Lights[10].Range = 200.0f;

			scene->Lights[11].Position = glm::vec3(-400.0f - 400.f, 1.0f, 40.0f);
			scene->Lights[11].Color = glm::vec3(0.1092f, 0.1326f, 0.9204f);
			scene->Lights[11].Range = 1000.0f;

			scene->Lights[12].Position = glm::vec3(-450.f - 400.f, -90.0f, 150.0f);
			scene->Lights[12].Color = glm::vec3(0.1092f, 0.1326f, 0.9204f);
			scene->Lights[12].Range = 1000.0f;

			scene->Lights[13].Position = glm::vec3(-500.f - 400.f, -90.0f, 150.0f);
			scene->Lights[13].Color = glm::vec3(0.1092f, 0.1326f, 0.9204f);
			scene->Lights[13].Range = 1000.0f;

			scene->Lights[14].Position = glm::vec3(-550.0f - 400.f, -90.0f, 150.0f);
			scene->Lights[14].Color = glm::vec3(0.1092f, 0.1326f, 0.9204f);
			scene->Lights[14].Range = 1000.0f;

			scene->Lights[15].Position = glm::vec3(-500.0f - 400.f, -90.0f, 150.0f);
			scene->Lights[15].Color = glm::vec3(0.1092f, 0.1326f, 0.9204f);
			scene->Lights[15].Range = 1000.0f;

			scene->Lights[16].Position = glm::vec3(-550.0f - 400.f, -90.0f, 150.0f);
			scene->Lights[16].Color = glm::vec3(0.1092f, 0.1326f, 0.9204f);
			scene->Lights[16].Range = 1000.0f;

			scene->Lights[17].Position = glm::vec3(-600.0f - 400.f, -90.0f, 150.0f);
			scene->Lights[17].Color = glm::vec3(0.1092f, 0.1326f, 0.9204f);
			scene->Lights[17].Range = 1000.0f;

			scene->Lights[18].Position = glm::vec3(-650.0f - 400.f, -90.0f, 150.0f);
			scene->Lights[18].Color = glm::vec3(0.1092f, 0.1326f, 0.9204f);
			scene->Lights[18].Range = 1000.0f;

			scene->Lights[19].Position = glm::vec3(-700.0f - 400.f, -90.0f, 150.0f);
			scene->Lights[19].Color = glm::vec3(0.1092f, 0.1326f, 0.9204f);
			scene->Lights[19].Range = 1000.0f;

			scene->Lights[20].Position = glm::vec3(-750.0f - 400.f, -90.0f, 150.0f);
			scene->Lights[20].Color = glm::vec3(0.1092f, 0.1326f, 0.9204f);
			scene->Lights[20].Range = 1000.0f;

			scene->Lights[21].Position = glm::vec3(-800.0f - 400.f, -90.0f, 150.0f);
			scene->Lights[21].Color = glm::vec3(0.1092f, 0.1326f, 0.9204f);
			scene->Lights[21].Range = 1000.0f;

			scene->Lights[22].Position = glm::vec3(-400.0f - 400.f, 20.0f, 5.0f);
			scene->Lights[22].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[22].Range = 150.0f;

			scene->Lights[23].Position = glm::vec3(-450.0f - 400.f, 20.0f, 5.0f);
			scene->Lights[23].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[23].Range = 150.0f;

			scene->Lights[24].Position = glm::vec3(-500.0f - 400.f, 20.0f, 5.0f);
			scene->Lights[24].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[24].Range = 150.0f;

			scene->Lights[25].Position = glm::vec3(-550.0f - 400.f, 20.0f, 5.0f);
			scene->Lights[25].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[25].Range = 150.0f;

			scene->Lights[26].Position = glm::vec3(-600.0f - 400.f, 20.0f, 5.0f);
			scene->Lights[26].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[26].Range = 150.0f;

			scene->Lights[27].Position = glm::vec3(-650.0f - 400.f, 20.0f, 5.0f);
			scene->Lights[27].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[27].Range = 150.0f;

			scene->Lights[28].Position = glm::vec3(-700.0f - 400.f, 20.0f, 5.0f);
			scene->Lights[28].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[28].Range = 150.0f;

			scene->Lights[29].Position = glm::vec3(-750.0f - 400.f, 20.0f, 5.0f);
			scene->Lights[29].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[29].Range = 150.0f;

			scene->Lights[30].Position = glm::vec3(-800.0f - 400.f, 20.0f, 5.0f);
			scene->Lights[30].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[30].Range = 150.0f;

			// We'll create a mesh that is a simple plane that we can resize later
			planeMesh = ResourceManager::CreateAsset<MeshResource>();
			cubeMesh = ResourceManager::CreateAsset<MeshResource>("cube.obj");

			mushroomMesh = ResourceManager::CreateAsset<MeshResource>("Mushroom.obj");
			tmMesh = ResourceManager::CreateAsset<MeshResource>("tm.obj");
			bmMesh = ResourceManager::CreateAsset<MeshResource>("bm.obj");
			ToadMesh = ResourceManager::CreateAsset<MeshResource>("ToadStool.obj");


			BranchMesh = ResourceManager::CreateAsset<MeshResource>("Branch.obj");
			LogMesh = ResourceManager::CreateAsset<MeshResource>("Log.obj");
			Plant2Mesh = ResourceManager::CreateAsset<MeshResource>("Plant2.obj");
			Plant3Mesh = ResourceManager::CreateAsset<MeshResource>("Plant3.obj");

			CampfireMesh = ResourceManager::CreateAsset<MeshResource>("Campfire.obj");
			SignPostMesh = ResourceManager::CreateAsset<MeshResource>("SignPost.obj");
			PuddleMesh = ResourceManager::CreateAsset<MeshResource>("Puddle.obj");
			HangingRockMesh = ResourceManager::CreateAsset<MeshResource>("HangingRock.obj");
			RockPileMesh = ResourceManager::CreateAsset<MeshResource>("RockHole.obj");
			RockTunnelMesh = ResourceManager::CreateAsset<MeshResource>("RockTunnel.obj");
			RockWallMesh1 = ResourceManager::CreateAsset<MeshResource>("RockWall1.obj");
			RockWallMesh2 = ResourceManager::CreateAsset<MeshResource>("RockWall2.obj");



			ladybugMesh = ResourceManager::CreateAsset<MeshResource>("lbo.obj");
			frogMesh = ResourceManager::CreateAsset<MeshResource>("frog.obj");

			BGRockMesh = ResourceManager::CreateAsset<MeshResource>("RockBackground.obj");
			BGMesh = ResourceManager::CreateAsset<MeshResource>("Background.obj");
			ExitTreeMesh = ResourceManager::CreateAsset<MeshResource>("ExitTree.obj");
			ExitRockMesh = ResourceManager::CreateAsset<MeshResource>("ExitRock.obj");

			planeMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(1.0f)));
			planeMesh->GenerateMesh();



			//Background Assets
			createBackgroundAsset("1", glm::vec3(-401.f - 400.f, 5.0f, -0.f), 0.5, glm::vec3(83.f, 5.0f, 0.0f), BranchMesh, BranchMaterial);
			createBackgroundAsset("2", glm::vec3(-600.f - 400.f, 5.0f, -0.f), 0.5, glm::vec3(83.f, 5.0f, 0.0f), BranchMesh, BranchMaterial);

			createBackgroundAsset("3", glm::vec3(-586.f - 400.f, 4.0f, -0.660), 3, glm::vec3(83.f, 5.0f, 0.0f), LogMesh, LogMaterial);
			createBackgroundAsset("4", glm::vec3(-420.f - 400.f, 0.0f, -0.660), 0.5, glm::vec3(400.f, 5.0f, 0.0f), LogMesh, LogMaterial);

			createBackgroundAsset("6", glm::vec3(-540.5 - 400.f, 5.470f, -0.330), 6.0, glm::vec3(83.f, 5.0f, 0.0f), Plant2Mesh, PlantMaterial);
			createBackgroundAsset("7", glm::vec3(-640.5 - 400.f, 5.470f, -0.330), 6.0, glm::vec3(83.f, 5.0f, 0.0f), Plant3Mesh, PlantMaterial);

			createBackgroundAsset("8", glm::vec3(-420.f - 400.f, 0.0f, -0.660), 6.0, glm::vec3(83.f, 5.0f, 0.0f), SunflowerMesh, SunflowerMaterial);

			createBackgroundAsset("9", glm::vec3(-413.f - 400.f, 5.0f, -0.430), 0.5, glm::vec3(83.f, 5.0f, 90.0f), ToadMesh, ToadMaterial);
			createBackgroundAsset("10", glm::vec3(-490.f - 400.f, 5.0f, -0.430), 0.5, glm::vec3(83.f, 5.0f, 90.0f), ToadMesh, ToadMaterial);
			createBackgroundAsset("11", glm::vec3(-730.f - 400.f, 5.0f, -0.430), 0.5, glm::vec3(83.f, 5.0f, 90.0f), ToadMesh, ToadMaterial);

			createBackgroundAsset("12", glm::vec3(-420.f - 400.f, 0.0f, -0.660), 0.5, glm::vec3(83.f, 5.0f, 90.0f), Rock1Mesh, boxMaterial);
			createBackgroundAsset("13", glm::vec3(-420.f - 400.f, 0.0f, -0.660), 0.5, glm::vec3(83.f, 5.0f, 90.0f), Rock2Mesh, rockMaterial);
			createBackgroundAsset("14", glm::vec3(-420.f - 400.f, 0.0f, -0.660), 0.5, glm::vec3(83.f, 5.0f, 90.0f), Rock3Mesh, rockMaterial);

			createBackgroundAsset("15", glm::vec3(-420.f - 400.f, 6.0f, -0.660), 0.5, glm::vec3(83.f, 5.0f, 90.0f), twigMesh, twigMaterial);
			createBackgroundAsset("16", glm::vec3(-387.060f - 400.f, 1.530f, 0.550), 0.05, glm::vec3(83.f, -7.0f, 90.0f), frogMesh, frogMaterial);

			createBackgroundAsset("17", glm::vec3(-392.5f - 400.f, 4.6f, -1.8), 0.5, glm::vec3(83.f, -7.0f, 90.0f), bmMesh, bmMaterial);
			createBackgroundAsset("18", glm::vec3(-406.f - 400.f, -12.660f, 0.400), 0.05, glm::vec3(83.f, -7.0f, 90.0f), tmMesh, tmMaterial);

			//Obstacles
			createGroundObstacle("1", glm::vec3(-850.f, -4.0f, 0.0f), glm::vec3(5.0f, 5.0f, 5.0f), glm::vec3(90.0f, 0.0f, -168.0f), SignPostMesh, SignPostMaterial);
			createGroundObstacle("2", glm::vec3(-880.f, 0.0f, -2.830f), glm::vec3(1.0f, 4.0f, 3.0f), glm::vec3(90.0f, 0.0f, -21.0f), RockPileMesh, RockPileMaterial);
			createGroundObstacle("3", glm::vec3(-900.f, 0.0f, 0.0f), glm::vec3(3.0f, 2.0f, 2.0f), glm::vec3(90.0f, 0.0f, 0.0f), RockTunnelMesh, RockTunnelMaterial);
			createGroundObstacle("4", glm::vec3(-950.f, -2.50f, 0.0f), glm::vec3(1.0f, 2.0f, 2.0f), glm::vec3(90.0f, 0.0f, 0.0f), RockWallMesh1, RockWallMaterial);
			createGroundObstacle("5", glm::vec3(-980.f, -2.50f, 0.0f), glm::vec3(1.0f, 2.0f, 2.0f), glm::vec3(90.0f, 0.0f, 0.0f), RockWallMesh1, RockWallMaterial);
			createGroundObstacle("6", glm::vec3(-1010.f, 0.0f, 0.0f), glm::vec3(1.0f, 1.5f, 1.0f), glm::vec3(90.0f, 0.0f, 0.0f), RockWallMesh2, RockWallMaterial);
			createGroundObstacle("7", glm::vec3(-1050.f, 0.0f, 0.0f), glm::vec3(3.0f, 2.0f, 2.0f), glm::vec3(90.0f, 0.0f, 0.0f), RockTunnelMesh, RockTunnelMaterial);
			createGroundObstacle("8", glm::vec3(-1090.f, 1.390f, 0.3f), glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(90.0f, 0.0f, 158.0f), PuddleMesh, PuddleMaterial);
			createGroundObstacle("9", glm::vec3(-1120.f, 0.0f, 0.0f), glm::vec3(3.0f, 2.0f, 2.0f), glm::vec3(90.0f, 0.0f, 0.0f), RockTunnelMesh, RockTunnelMaterial);
			createGroundObstacle("10", glm::vec3(-1150.f, -2.22f, 0.0f), glm::vec3(0.4f, 0.4f, 0.4f), glm::vec3(90.0f, 0.0f, -67.0f), CampfireMesh, CampfireMaterial);

			//3D Backgrounds
			createGroundObstacle("27", glm::vec3(-292.3f - 400.f, -55.830f, -2.5f), glm::vec3(6.f, 6.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGRockMesh, BGMaterial);
			createGroundObstacle("28", glm::vec3(-400.f - 400.f, -55.830f, -2.5f), glm::vec3(6.f, 6.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGRockMesh, BGMaterial);
			createGroundObstacle("29", glm::vec3(-507.7f - 400.f, -55.830f, -2.5f), glm::vec3(6.f, 6.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGRockMesh, BGMaterial);

			createGroundObstacle("30", glm::vec3(-615.4f - 400.f, -55.830f, -2.5f), glm::vec3(6.f, 6.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGRockMesh, BGMaterial);
			createGroundObstacle("31", glm::vec3(-723.1f - 400.f, -55.830f, -2.5f), glm::vec3(6.f, 6.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGRockMesh, BGMaterial);
			createGroundObstacle("32", glm::vec3(-830.8f - 400.f, -55.830f, -2.5f), glm::vec3(6.f, 6.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGRockMesh, BGMaterial);
			createGroundObstacle("26", glm::vec3(-938.2f - 400.f, -55.830f, -2.5f), glm::vec3(6.f, 6.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGRockMesh, BGMaterial);

			//2DBackGrounds
			createGroundObstacle("33", glm::vec3(-75.f - 400.f, -130.0f, 64.130f), glm::vec3(375.0f, 125.0f, 250.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainBackdropMaterial);
			createGroundObstacle("34", glm::vec3(-450.f - 400.f, -130.0f, 64.130f), glm::vec3(375.0f, 125.0f, 250.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainBackdropMaterial);
			createGroundObstacle("35", glm::vec3(-825.f - 400.f, -130.0f, 64.130f), glm::vec3(375.0f, 125.0f, 250.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainBackdropMaterial);
			createGroundObstacle("36", glm::vec3(-1600.f, -130.0f, 64.130f), glm::vec3(375.0f, 125.0f, 250.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainBackdropMaterial);
			createGroundObstacle("37", glm::vec3(-2150.f - 400.f, -130.0f, 64.130f), glm::vec3(375.0f, 125.0f, 250.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainBackdropMaterial);

			//Grass
			createGroundObstacle("38", glm::vec3(-418.170f - 400.f, -38.230f, 5.250f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass1Material);
			createGroundObstacle("39", glm::vec3(-370.42f - 400.f, 46.920f, 5.080f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass2Material);
			createGroundObstacle("40", glm::vec3(-397.91f - 400.f, -47.360f, 5.090f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass3Material);
			createGroundObstacle("41", glm::vec3(-364.3f - 400.f, -37.550f, 5.310f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass4Material);
			createGroundObstacle("42", glm::vec3(-447.340f - 400.f, -37.120f, 5.560f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass5Material);

			createGroundObstacle("43", glm::vec3(-418.170f - 103.5f - 400.f, -38.230f, 5.250f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass1Material);
			createGroundObstacle("44", glm::vec3(-370.42f - 103.5f - 400.f, 46.920f, 5.080f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass2Material);
			createGroundObstacle("45", glm::vec3(-397.91f - 103.5f - 400.f, -47.360f, 5.090f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass3Material);
			createGroundObstacle("46", glm::vec3(-364.3f - 103.5f - 400.f, -37.550f, 5.310f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass4Material);
			createGroundObstacle("47", glm::vec3(-47.340f - 103.5f - 400.f - 400.f, -37.120f, 5.560f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass5Material);

			createGroundObstacle("48", glm::vec3(-18.170f - 103.5f - 103.5f - 400.f - 400.f, -38.230f, 5.250f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass1Material);
			createGroundObstacle("49", glm::vec3(29.580f - 103.5f - 103.5f - 400.f - 400.f, 46.920f, 5.080f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass2Material);
			createGroundObstacle("50", glm::vec3(2.090f - 103.5f - 103.5f - 400.f - 400.f, -47.360f, 5.090f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass3Material);
			createGroundObstacle("51", glm::vec3(35.700f - 103.5f - 103.5f - 400.f - 400.f, -37.550f, 5.310f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass4Material);
			createGroundObstacle("52", glm::vec3(-47.340f - 103.5f - 103.5f - 400.f - 400.f, -37.120f, 5.560f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass5Material);

			createGroundObstacle("53", glm::vec3(-18.170f - 103.5f - 103.5f - 103.5f - 400.f - 400.f, -38.230f, 5.250f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass1Material);
			createGroundObstacle("54", glm::vec3(29.580f - 103.5f - 103.5f - 103.5f - 400.f - 400.f, 46.920f, 5.080f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass2Material);
			createGroundObstacle("55", glm::vec3(2.090f - 103.5f - 103.5f - 103.5f - 400.f - 400.f, -47.360f, 5.090f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass3Material);
			createGroundObstacle("56", glm::vec3(35.700f - 103.5f - 103.5f - 103.5f - 400.f - 400.f, -37.550f, 5.310f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass4Material);
			createGroundObstacle("57", glm::vec3(-47.340f - 103.5f - 103.5f - 103.5f - 400.f - 400.f, -37.120f, 5.560f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass5Material);

			//Exit Rock
			createGroundObstacle("58", glm::vec3(-409.f - 400.f - 400.f, -1.f, 0.f), glm::vec3(1.f), glm::vec3(90.0f, 0.0f, 180.f), ExitRockMesh, ExitRockMaterial);

			//Foreground Grass
			createGroundObstacle("59", glm::vec3(40.f - 400.f - 400.f, -5.f, 10.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
			createGroundObstacle("60", glm::vec3(0.f - 400.f - 400.f, -5.f, 10.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
			createGroundObstacle("61", glm::vec3(-40.f - 400.f - 400.f, -5.f, 10.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
			createGroundObstacle("62", glm::vec3(-80.f - 400.f - 400.f, -5.f, 10.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
			createGroundObstacle("63", glm::vec3(-120.f - 400.f - 400.f, -5.f, 10.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
			createGroundObstacle("64", glm::vec3(-160.f - 400.f - 400.f, -5.f, 10.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);

			createGroundObstacle("65", glm::vec3(-200.f - 400.f - 400.f, -5.f, 10.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
			createGroundObstacle("66", glm::vec3(-240.f - 400.f - 400.f, -5.f, 10.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
			createGroundObstacle("67", glm::vec3(-280.f - 400.f - 400.f, -5.f, 10.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
			createGroundObstacle("68", glm::vec3(-320.f - 400.f - 400.f, -5.f, 10.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
			createGroundObstacle("69", glm::vec3(-360.f - 400.f - 400.f, -5.f, 10.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
			createGroundObstacle("70", glm::vec3(-400.f - 400.f - 400.f, -5.f, 10.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);


			//Collisions

			//sign
			createCollision("211", -850.0f, 8.11f, 1.f, 2.f); //2y
			createCollision("212", -849.6f, 7.79f, 1.f, 2.f);
			createCollision("213", -849.930f, 8.160f, 1.f, 2.f);
			createCollision("214", -850.f, 8.150f, 1.f, 2.f);

			//rock pile
			createCollision("221", -878.924f, 1.560f, 1.f, 2.f); //2y
			createCollision("222", -880.00f, 12.390f, 1.f, 2.f); //2y

			//rock tunnel
			createCollision("231", -896.0f, 5.0f, 1.f, 1.f);
			createCollision("232", -898.963f, 6.570f, 1.f, 1.f);
			createCollision("233", -902.823f, 5.0f, 1.f, 1.f);

			//rock wall
			createCollision("241", -950.f, 2.470f, 1.f, 2.f);

			//rock wall
			createCollision("251", -980.f, 2.470f, 1.f, 2.f);

			//tall rock wall
			createCollision("261", -1008.98f, 1.560f, 1.f, 4.f); //4y

			//rock tunnel
			createCollision("271", -1046.0f, 5.0f, 1.f, 1.f);
			createCollision("272", -1048.963f, 6.570f, 1.f, 1.f);
			createCollision("273", -1052.823f, 5.0f, 1.f, 1.f);

			//puddle
			createCollision("281", -1085.f, 0.44f, 1.f, 1.f);
			createCollision("282", -1091.f, 0.44f, 1.f, 1.f);
			createCollision("283", -1093.f, 0.44f, 1.f, 1.f);
			createCollision("284", -1095.f, 0.44f, 1.f, 1.f);
			createCollision("285", -1087.f, 0.44f, 1.f, 1.f);
			createCollision("286", -1089.f, 0.44f, 1.f, 1.f);

			//rock tunnel
			createCollision("291", -1116.0f, 5.0f, 1.f, 1.f);
			createCollision("292", -1118.963f, 6.570f, 1.f, 1.f);
			createCollision("293", -1122.823f, 5.0f, 1.f, 1.f);

			//campfire
			createCollision("201", -1147.44f, 6.3f, 1.f, 1.f);
			createCollision("202", -1151.720f, 6.3f, 1.f, 1.f);
			createCollision("203", -1150.f, 6.3f, 1.f, 1.f);






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
				player->SetPostion(glm::vec3(-806.f, 0.0f, 1.0f));
				player->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
				player->SetScale(glm::vec3(0.5f, 0.5f, 0.5f));

				// Add some behaviour that relies on the physics body
				//player->Add<JumpBehaviour>(player->GetPosition());
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

			createGroundObstacle("79", glm::vec3(40.f - 400.f - 400.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.22f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
			createGroundObstacle("80", glm::vec3(0.f - 400.f - 400.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
			createGroundObstacle("81", glm::vec3(-40.f - 400.f - 400.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
			createGroundObstacle("82", glm::vec3(-80.f - 400.f - 400.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
			createGroundObstacle("83", glm::vec3(-120.f - 400.f - 400.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
			createGroundObstacle("84", glm::vec3(-160.f - 400.f - 400.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);

			createGroundObstacle("85", glm::vec3(-200.f - 400.f - 400.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
			createGroundObstacle("86", glm::vec3(-240.f - 400.f - 400.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
			createGroundObstacle("87", glm::vec3(-280.f - 400.f - 400.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
			createGroundObstacle("88", glm::vec3(-320.f - 400.f - 400.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
			createGroundObstacle("89", glm::vec3(-360.f - 400.f - 400.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
			createGroundObstacle("90", glm::vec3(-400.f - 400.f - 400.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);


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

			GameObject::Sptr ButtonBack3 = scene->CreateGameObject("ButtonBack3");
			{
				// Set position in the scene
				ButtonBack3->SetPostion(glm::vec3(1.f, 6.5f, 5.f));
				// Scale down the plane
				ButtonBack3->SetScale(glm::vec3(3.0f, 0.8f, 0.5f));
				//spin things
				ButtonBack3->SetRotation(glm::vec3(-80.0f, 0.f, 0.f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = ButtonBack3->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(ButtonMaterial);
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

			GameObject::Sptr LSText = scene->CreateGameObject("LSText");
			{
				// Set position in the scene
				LSText->SetPostion(glm::vec3(1.0f, 8.0f, 6.1f));
				// Scale down the plane
				LSText->SetScale(glm::vec3(2.0f, 0.4f, 0.5f));
				LSText->SetRotation(glm::vec3(80.0f, 0.f, -180.f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = LSText->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(LStextMaterial);
			}


			GameObject::Sptr FrogTongue = scene->CreateGameObject("FrogTongue");
			{
				// Set position in the scene
				FrogTongue->SetPostion(glm::vec3(-3.4f, -1.05f, 3.59f));
				// Scale down the plane
				FrogTongue->SetScale(glm::vec3(1.0f, 0.1f, 1.0f));
				FrogTongue->SetRotation(glm::vec3(0.0f, 0.0f, 45.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = FrogTongue->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(FrogTongueMaterial);
			}

			GameObject::Sptr FrogBody = scene->CreateGameObject("FrogBody");
			{
				// Set position in the scene
				FrogBody->SetPostion(glm::vec3(-3.4f, -1.05f, 3.6f));
				// Scale down the plane
				FrogBody->SetScale(glm::vec3(1.5f, 1.5f, 1.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = FrogBody->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(FrogBodyMaterial);
			}

			GameObject::Sptr FrogHeadBot = scene->CreateGameObject("FrogHeadBot");
			{
				// Set position in the scene
				FrogHeadBot->SetPostion(glm::vec3(-3.4f, -1.05f, 3.61f));
				// Scale down the plane
				FrogHeadBot->SetScale(glm::vec3(1.5f, 1.5f, 1.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = FrogHeadBot->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(FrogHeadBotMaterial);
			}

			GameObject::Sptr FrogHeadTop = scene->CreateGameObject("FrogHeadTop");
			{
				// Set position in the scene
				FrogHeadTop->SetPostion(glm::vec3(-3.4f, -1.05f, 3.62f));
				// Scale down the plane
				FrogHeadTop->SetScale(glm::vec3(1.5f, 1.5f, 1.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = FrogHeadTop->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(FrogHeadTopMaterial);
			}

			GameObject::Sptr BushTransition = scene->CreateGameObject("BushTransition");
			{
				// Set position in the scene
				BushTransition->SetPostion(glm::vec3(5.f, 0.0f, 3.8f));
				// Scale down the plane
				BushTransition->SetScale(glm::vec3(5.8f, 2.5f, 1.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = BushTransition->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(BushTransitionMaterial);
			}

			scene->SetAmbientLight(glm::vec3(0.2f));

			// Kinematic rigid bodies are those controlled by some outside controller
			// and ONLY collide with dynamic objects
			RigidBody::Sptr physics = jumpingObstacle->Add<RigidBody>(RigidBodyType::Kinematic);
			physics->AddCollider(ConvexMeshCollider::Create());

			// Save the asset manifest for all the resources we just loaded
			ResourceManager::SaveManifest("manifest.json");
			// Save the scene to a JSON file
			scene->Save("Level3.json");

		}

		/// Working Level ///									//////		Level 4	////////// scenevalue == 4

		{
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
			BGMaterial->Texture = MBGTex;
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
			ProgressBarMaterial->Texture = Progress2Tex;
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
		Material::Sptr LStextMaterial = ResourceManager::CreateAsset<Material>();
		{
			LStextMaterial->Name = "LStext";
			LStextMaterial->MatShader = scene->BaseShader;
			LStextMaterial->Texture = LStextTex;
			LStextMaterial->Shininess = 2.0f;
		}
		Material::Sptr ExitRockMaterial = ResourceManager::CreateAsset<Material>();
		{
			ExitRockMaterial->Name = "ExitRock";
			ExitRockMaterial->MatShader = scene->BaseShader;
			ExitRockMaterial->Texture = ExitRockTex;
			ExitRockMaterial->Shininess = 2.0f;
		}


		Material::Sptr FrogBodyMaterial = ResourceManager::CreateAsset<Material>();
		{
			FrogBodyMaterial->Name = "FrogBody";
			FrogBodyMaterial->MatShader = scene->BaseShader;
			FrogBodyMaterial->Texture = FrogBodyTex;
			FrogBodyMaterial->Shininess = 2.0f;
		}

		Material::Sptr FrogHeadTopMaterial = ResourceManager::CreateAsset<Material>();
		{
			FrogHeadTopMaterial->Name = "FrogHeadTop";
			FrogHeadTopMaterial->MatShader = scene->BaseShader;
			FrogHeadTopMaterial->Texture = FrogHeadTopTex;
			FrogHeadTopMaterial->Shininess = 2.0f;
		}

		Material::Sptr FrogHeadBotMaterial = ResourceManager::CreateAsset<Material>();
		{
			FrogHeadBotMaterial->Name = "FrogHeadBot";
			FrogHeadBotMaterial->MatShader = scene->BaseShader;
			FrogHeadBotMaterial->Texture = FrogHeadBotTex;
			FrogHeadBotMaterial->Shininess = 2.0f;
		}

		Material::Sptr FrogTongueMaterial = ResourceManager::CreateAsset<Material>();
		{
			FrogTongueMaterial->Name = "FrogTongue";
			FrogTongueMaterial->MatShader = scene->BaseShader;
			FrogTongueMaterial->Texture = FrogTongueTex;
			FrogTongueMaterial->Shininess = 2.0f;
		}

		Material::Sptr BushTransitionMaterial = ResourceManager::CreateAsset<Material>();
		{
			BushTransitionMaterial->Name = "BushTransition";
			BushTransitionMaterial->MatShader = scene->BaseShader;
			BushTransitionMaterial->Texture = BushTransitionTex;
			BushTransitionMaterial->Shininess = 2.0f;
		}

		Material::Sptr SignPostMaterial = ResourceManager::CreateAsset<Material>();
		{
			SignPostMaterial->Name = "SignPost";
			SignPostMaterial->MatShader = scene->BaseShader;
			SignPostMaterial->Texture = SignPostTex;
			SignPostMaterial->Shininess = 2.0f;
		}

		Material::Sptr PuddleMaterial = ResourceManager::CreateAsset<Material>();
		{
			PuddleMaterial->Name = "Puddle";
			PuddleMaterial->MatShader = scene->BaseShader;
			PuddleMaterial->Texture = PuddleTex;
			PuddleMaterial->Shininess = 2.0f;
		}

		Material::Sptr CampfireMaterial = ResourceManager::CreateAsset<Material>();
		{
			CampfireMaterial->Name = "Campfire";
			CampfireMaterial->MatShader = scene->BaseShader;
			CampfireMaterial->Texture = CampfireTex;
			CampfireMaterial->Shininess = 2.0f;
		}

		Material::Sptr HangingRockMaterial = ResourceManager::CreateAsset<Material>();
		{
			HangingRockMaterial->Name = "HangingRock";
			HangingRockMaterial->MatShader = scene->BaseShader;
			HangingRockMaterial->Texture = HangingRockTex;
			HangingRockMaterial->Shininess = 2.0f;
		}

		Material::Sptr RockPileMaterial = ResourceManager::CreateAsset<Material>();
		{
			RockPileMaterial->Name = "RockPile";
			RockPileMaterial->MatShader = scene->BaseShader;
			RockPileMaterial->Texture = RockPileTex;
			RockPileMaterial->Shininess = 2.0f;
		}

		Material::Sptr RockTunnelMaterial = ResourceManager::CreateAsset<Material>();
		{
			RockTunnelMaterial->Name = "RockTunnel";
			RockTunnelMaterial->MatShader = scene->BaseShader;
			RockTunnelMaterial->Texture = RockTunnelTex;
			RockTunnelMaterial->Shininess = 2.0f;
		}

		Material::Sptr RockWallMaterial = ResourceManager::CreateAsset<Material>();
		{
			RockWallMaterial->Name = "RockWall";
			RockWallMaterial->MatShader = scene->BaseShader;
			RockWallMaterial->Texture = RockWallTex;
			RockWallMaterial->Shininess = 2.0f;
		}

		Material::Sptr MountainBackdropMaterial = ResourceManager::CreateAsset<Material>();
		{
			MountainBackdropMaterial->Name = "Backdrop";
			MountainBackdropMaterial->MatShader = scene->BaseShader;
			MountainBackdropMaterial->Texture = MountainBackdropTex;
			MountainBackdropMaterial->Shininess = 2.0f;
		}

		Material::Sptr MountainForegroundMaterial = ResourceManager::CreateAsset<Material>();
		{
			MountainForegroundMaterial->Name = "Foreground";
			MountainForegroundMaterial->MatShader = scene->BaseShader;
			MountainForegroundMaterial->Texture = MountainForegroundTex;
			MountainForegroundMaterial->Shininess = 2.0f;
		}


		// Create some lights for our scene
		scene->Lights.resize(31);
		scene->Lights[0].Position = glm::vec3(-400.0f - 800.f, 1.0f, 40.0f);
		scene->Lights[0].Color = glm::vec3(0.897f, 1.f, 1.f);
		scene->Lights[0].Range = 200.0f;

		scene->Lights[1].Position = glm::vec3(-450.f - 800.f, 0.0f, 40.0f);
		scene->Lights[1].Color = glm::vec3(0.897f, 1.f, 1.f);
		scene->Lights[1].Range = 200.0f;

		scene->Lights[2].Position = glm::vec3(-500.f - 800.f, 1.0f, 40.0f);
		scene->Lights[2].Color = glm::vec3(0.897f, 1.f, 1.f);
		scene->Lights[2].Range = 200.0f;

		scene->Lights[3].Position = glm::vec3(-550.0f - 800.f, 1.0f, 40.0f);
		scene->Lights[3].Color = glm::vec3(0.897f, 1.f, 1.f);
		scene->Lights[3].Range = 200.0f;

		scene->Lights[4].Position = glm::vec3(-500.0f - 800.f, 1.0f, 40.0f);
		scene->Lights[4].Color = glm::vec3(0.897f, 1.f, 1.f);
		scene->Lights[4].Range = 200.0f;

		scene->Lights[5].Position = glm::vec3(-550.0f - 800.f, 1.0f, 40.0f);
		scene->Lights[5].Color = glm::vec3(0.897f, 1.f, 1.f);
		scene->Lights[5].Range = 200.0f;

		scene->Lights[6].Position = glm::vec3(-600.0f - 800.f, 1.0f, 40.0f);
		scene->Lights[6].Color = glm::vec3(0.897f, 1.f, 1.f);
		scene->Lights[6].Range = 200.0f;

		scene->Lights[7].Position = glm::vec3(-650.0f - 800.f, 1.0f, 40.0f);
		scene->Lights[7].Color = glm::vec3(0.897f, 1.f, 1.f);
		scene->Lights[7].Range = 200.0f;

		scene->Lights[8].Position = glm::vec3(-700.0f - 800.f, 1.0f, 40.0f);
		scene->Lights[8].Color = glm::vec3(0.897f, 1.f, 1.f);
		scene->Lights[8].Range = 200.0f;

		scene->Lights[9].Position = glm::vec3(-750.0f - 800.f, 1.0f, 40.0f);
		scene->Lights[9].Color = glm::vec3(0.897f, 1.f, 1.f);
		scene->Lights[9].Range = 200.0f;

		scene->Lights[10].Position = glm::vec3(-800.0f - 800.f, 1.0f, 40.0f);
		scene->Lights[10].Color = glm::vec3(0.897f, 1.f, 1.f);
		scene->Lights[10].Range = 200.0f;

		scene->Lights[11].Position = glm::vec3(-400.0f - 800.f, 1.0f, 40.0f);
		scene->Lights[11].Color = glm::vec3(0.1092f, 0.1326f, 0.9204f);
		scene->Lights[11].Range = 1000.0f;

		scene->Lights[12].Position = glm::vec3(-450.f - 800.f, -90.0f, 150.0f);
		scene->Lights[12].Color = glm::vec3(0.1092f, 0.1326f, 0.9204f);
		scene->Lights[12].Range = 1000.0f;

		scene->Lights[13].Position = glm::vec3(-500.f - 800.f, -90.0f, 150.0f);
		scene->Lights[13].Color = glm::vec3(0.1092f, 0.1326f, 0.9204f);
		scene->Lights[13].Range = 1000.0f;

		scene->Lights[14].Position = glm::vec3(-550.0f - 800.f, -90.0f, 150.0f);
		scene->Lights[14].Color = glm::vec3(0.1092f, 0.1326f, 0.9204f);
		scene->Lights[14].Range = 1000.0f;

		scene->Lights[15].Position = glm::vec3(-500.0f - 800.f, -90.0f, 150.0f);
		scene->Lights[15].Color = glm::vec3(0.1092f, 0.1326f, 0.9204f);
		scene->Lights[15].Range = 1000.0f;

		scene->Lights[16].Position = glm::vec3(-550.0f - 800.f, -90.0f, 150.0f);
		scene->Lights[16].Color = glm::vec3(0.1092f, 0.1326f, 0.9204f);
		scene->Lights[16].Range = 1000.0f;

		scene->Lights[17].Position = glm::vec3(-600.0f - 800.f, -90.0f, 150.0f);
		scene->Lights[17].Color = glm::vec3(0.1092f, 0.1326f, 0.9204f);
		scene->Lights[17].Range = 1000.0f;

		scene->Lights[18].Position = glm::vec3(-650.0f - 800.f, -90.0f, 150.0f);
		scene->Lights[18].Color = glm::vec3(0.1092f, 0.1326f, 0.9204f);
		scene->Lights[18].Range = 1000.0f;

		scene->Lights[19].Position = glm::vec3(-700.0f - 800.f, -90.0f, 150.0f);
		scene->Lights[19].Color = glm::vec3(0.1092f, 0.1326f, 0.9204f);
		scene->Lights[19].Range = 1000.0f;

		scene->Lights[20].Position = glm::vec3(-750.0f - 800.f, -90.0f, 150.0f);
		scene->Lights[20].Color = glm::vec3(0.1092f, 0.1326f, 0.9204f);
		scene->Lights[20].Range = 1000.0f;

		scene->Lights[21].Position = glm::vec3(-800.0f - 800.f, -90.0f, 150.0f);
		scene->Lights[21].Color = glm::vec3(0.1092f, 0.1326f, 0.9204f);
		scene->Lights[21].Range = 1000.0f;

		scene->Lights[22].Position = glm::vec3(-400.0f - 800.f, 20.0f, 5.0f);
		scene->Lights[22].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[22].Range = 150.0f;

		scene->Lights[23].Position = glm::vec3(-450.0f - 800.f, 20.0f, 5.0f);
		scene->Lights[23].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[23].Range = 150.0f;

		scene->Lights[24].Position = glm::vec3(-500.0f - 800.f, 20.0f, 5.0f);
		scene->Lights[24].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[24].Range = 150.0f;

		scene->Lights[25].Position = glm::vec3(-550.0f - 800.f, 20.0f, 5.0f);
		scene->Lights[25].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[25].Range = 150.0f;

		scene->Lights[26].Position = glm::vec3(-600.0f - 800.f, 20.0f, 5.0f);
		scene->Lights[26].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[26].Range = 150.0f;

		scene->Lights[27].Position = glm::vec3(-650.0f - 800.f, 20.0f, 5.0f);
		scene->Lights[27].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[27].Range = 150.0f;

		scene->Lights[28].Position = glm::vec3(-700.0f - 800.f, 20.0f, 5.0f);
		scene->Lights[28].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[28].Range = 150.0f;

		scene->Lights[29].Position = glm::vec3(-750.0f - 800.f, 20.0f, 5.0f);
		scene->Lights[29].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[29].Range = 150.0f;

		scene->Lights[30].Position = glm::vec3(-800.0f - 800.f, 20.0f, 5.0f);
		scene->Lights[30].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[30].Range = 150.0f;

		// We'll create a mesh that is a simple plane that we can resize later
		planeMesh = ResourceManager::CreateAsset<MeshResource>();
		cubeMesh = ResourceManager::CreateAsset<MeshResource>("cube.obj");

		mushroomMesh = ResourceManager::CreateAsset<MeshResource>("Mushroom.obj");
		tmMesh = ResourceManager::CreateAsset<MeshResource>("tm.obj");
		bmMesh = ResourceManager::CreateAsset<MeshResource>("bm.obj");
		ToadMesh = ResourceManager::CreateAsset<MeshResource>("ToadStool.obj");


		BranchMesh = ResourceManager::CreateAsset<MeshResource>("Branch.obj");
		LogMesh = ResourceManager::CreateAsset<MeshResource>("Log.obj");
		Plant2Mesh = ResourceManager::CreateAsset<MeshResource>("Plant2.obj");
		Plant3Mesh = ResourceManager::CreateAsset<MeshResource>("Plant3.obj");

		CampfireMesh = ResourceManager::CreateAsset<MeshResource>("Campfire.obj");
		SignPostMesh = ResourceManager::CreateAsset<MeshResource>("SignPost.obj");
		PuddleMesh = ResourceManager::CreateAsset<MeshResource>("Puddle.obj");
		HangingRockMesh = ResourceManager::CreateAsset<MeshResource>("HangingRock.obj");
		RockPileMesh = ResourceManager::CreateAsset<MeshResource>("RockHole.obj");
		RockTunnelMesh = ResourceManager::CreateAsset<MeshResource>("RockTunnel.obj");
		RockWallMesh1 = ResourceManager::CreateAsset<MeshResource>("RockWall1.obj");
		RockWallMesh2 = ResourceManager::CreateAsset<MeshResource>("RockWall2.obj");



		ladybugMesh = ResourceManager::CreateAsset<MeshResource>("lbo.obj");
		frogMesh = ResourceManager::CreateAsset<MeshResource>("frog.obj");

		BGRockMesh = ResourceManager::CreateAsset<MeshResource>("RockBackground.obj");
		BGMesh = ResourceManager::CreateAsset<MeshResource>("Background.obj");
		ExitTreeMesh = ResourceManager::CreateAsset<MeshResource>("ExitTree.obj");
		ExitRockMesh = ResourceManager::CreateAsset<MeshResource>("ExitRock.obj");

		planeMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(1.0f)));
		planeMesh->GenerateMesh();



		//Background Assets
		createBackgroundAsset("1", glm::vec3(-401.f - 800.f, 5.0f, -0.f), 0.5, glm::vec3(83.f, 5.0f, 0.0f), BranchMesh, BranchMaterial);
		createBackgroundAsset("2", glm::vec3(-600.f - 800.f, 5.0f, -0.f), 0.5, glm::vec3(83.f, 5.0f, 0.0f), BranchMesh, BranchMaterial);

		createBackgroundAsset("3", glm::vec3(-586.f - 800.f, 4.0f, -0.660), 3, glm::vec3(83.f, 5.0f, 0.0f), LogMesh, LogMaterial);
		createBackgroundAsset("4", glm::vec3(-420.f - 800.f, 0.0f, -0.660), 0.5, glm::vec3(400.f, 5.0f, 0.0f), LogMesh, LogMaterial);

		createBackgroundAsset("6", glm::vec3(-540.5 - 800.f, 5.470f, -0.330), 6.0, glm::vec3(83.f, 5.0f, 0.0f), Plant2Mesh, PlantMaterial);
		createBackgroundAsset("7", glm::vec3(-640.5 - 800.f, 5.470f, -0.330), 6.0, glm::vec3(83.f, 5.0f, 0.0f), Plant3Mesh, PlantMaterial);

		createBackgroundAsset("8", glm::vec3(-420.f - 800.f, 0.0f, -0.660), 6.0, glm::vec3(83.f, 5.0f, 0.0f), SunflowerMesh, SunflowerMaterial);

		createBackgroundAsset("9", glm::vec3(-413.f - 800.f, 5.0f, -0.430), 0.5, glm::vec3(83.f, 5.0f, 90.0f), ToadMesh, ToadMaterial);
		createBackgroundAsset("10", glm::vec3(-490.f - 800.f, 5.0f, -0.430), 0.5, glm::vec3(83.f, 5.0f, 90.0f), ToadMesh, ToadMaterial);
		createBackgroundAsset("11", glm::vec3(-730.f - 800.f, 5.0f, -0.430), 0.5, glm::vec3(83.f, 5.0f, 90.0f), ToadMesh, ToadMaterial);

		createBackgroundAsset("12", glm::vec3(-420.f - 800.f, 0.0f, -0.660), 0.5, glm::vec3(83.f, 5.0f, 90.0f), Rock1Mesh, boxMaterial);
		createBackgroundAsset("13", glm::vec3(-420.f - 800.f, 0.0f, -0.660), 0.5, glm::vec3(83.f, 5.0f, 90.0f), Rock2Mesh, rockMaterial);
		createBackgroundAsset("14", glm::vec3(-420.f - 800.f, 0.0f, -0.660), 0.5, glm::vec3(83.f, 5.0f, 90.0f), Rock3Mesh, rockMaterial);

		createBackgroundAsset("15", glm::vec3(-420.f - 800.f, 6.0f, -0.660), 0.5, glm::vec3(83.f, 5.0f, 90.0f), twigMesh, twigMaterial);
		createBackgroundAsset("16", glm::vec3(-387.060f - 800.f, 1.530f, 0.550), 0.05, glm::vec3(83.f, -7.0f, 90.0f), frogMesh, frogMaterial);

		createBackgroundAsset("17", glm::vec3(-392.5f - 800.f, 4.6f, -1.8), 0.5, glm::vec3(83.f, -7.0f, 90.0f), bmMesh, bmMaterial);
		createBackgroundAsset("18", glm::vec3(-406.f - 800.f, -12.660f, 0.400), 0.05, glm::vec3(83.f, -7.0f, 90.0f), tmMesh, tmMaterial);

		//Obstacles scene 3
		/*createGroundObstacle("1", glm::vec3(-1250.f, -4.0f, 0.0f), glm::vec3(5.0f, 5.0f, 5.0f), glm::vec3(90.0f, 0.0f, -168.0f), SignPostMesh, SignPostMaterial);
		createGroundObstacle("2", glm::vec3(-1280.f, 0.0f, -2.830f), glm::vec3(1.0f, 4.0f, 3.0f), glm::vec3(90.0f, 0.0f, -21.0f), RockPileMesh, RockPileMaterial);
		createGroundObstacle("3", glm::vec3(-1300.f, 0.0f, 0.0f), glm::vec3(3.0f, 2.0f, 2.0f), glm::vec3(90.0f, 0.0f, 0.0f), RockTunnelMesh, RockTunnelMaterial);
		createGroundObstacle("4", glm::vec3(-1350.f, -2.50f, 0.0f), glm::vec3(1.0f, 2.0f, 2.0f), glm::vec3(90.0f, 0.0f, 0.0f), RockWallMesh1, RockWallMaterial);
		createGroundObstacle("5", glm::vec3(-1380.f, -2.50f, 0.0f), glm::vec3(1.0f, 2.0f, 2.0f), glm::vec3(90.0f, 0.0f, 0.0f), RockWallMesh1, RockWallMaterial);
		createGroundObstacle("6", glm::vec3(-1410.f, 0.0f, 0.0f), glm::vec3(1.0f, 1.5f, 1.0f), glm::vec3(90.0f, 0.0f, 0.0f), RockWallMesh2, RockWallMaterial);
		createGroundObstacle("7", glm::vec3(-1450.f, 0.0f, 0.0f), glm::vec3(3.0f, 2.0f, 2.0f), glm::vec3(90.0f, 0.0f, 0.0f), RockTunnelMesh, RockTunnelMaterial);
		createGroundObstacle("8", glm::vec3(-1490.f, 1.390f, 0.3f), glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(90.0f, 0.0f, 158.0f), PuddleMesh, PuddleMaterial);
		createGroundObstacle("9", glm::vec3(-1520.f, 0.0f, 0.0f), glm::vec3(3.0f, 2.0f, 2.0f), glm::vec3(90.0f, 0.0f, 0.0f), RockTunnelMesh, RockTunnelMaterial);
		createGroundObstacle("10", glm::vec3(-1550.f, -2.22f, 0.0f), glm::vec3(0.4f, 0.4f, 0.4f), glm::vec3(90.0f, 0.0f, -67.0f), CampfireMesh, CampfireMaterial);*/


		//createGroundObstacle("2", glm::vec3(-900.f, 1.390f, 0.3f), glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(90.0f, 0.0f, 158.0f), PuddleMesh, PuddleMaterial);
		//createGroundObstacle("3", glm::vec3(-950.f, -2.22f, 0.0f), glm::vec3(0.4f, 0.4f, 0.4f), glm::vec3(90.0f, 0.0f, -67.0f), CampfireMesh, CampfireMaterial);
		//createGroundObstacle("4", glm::vec3(-1000.f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), HangingRockMesh, HangingRockMaterial);
		//createGroundObstacle("5", glm::vec3(-1050.f, 0.0f, -2.830f), glm::vec3(1.0f, 4.0f, 3.0f), glm::vec3(90.0f, 0.0f, -21.0f), RockPileMesh, RockPileMaterial);
		//createGroundObstacle("6", glm::vec3(-1100.f, 0.0f, 0.0f), glm::vec3(3.0f, 2.0f, 2.0f), glm::vec3(90.0f, 0.0f, 0.0f), RockTunnelMesh, RockTunnelMaterial);
		//createGroundObstacle("7", glm::vec3(-1150.f, -2.50f, 0.0f), glm::vec3(1.0f, 2.0f, 2.0f), glm::vec3(90.0f, 0.0f, 0.0f), RockWallMesh1, RockWallMaterial);
		//createGroundObstacle("8", glm::vec3(-1200.f, 0.0f, 0.0f), glm::vec3(1.0f, 1.5f, 1.0f), glm::vec3(90.0f, 0.0f, 0.0f), RockWallMesh2, RockWallMaterial);

		createGroundObstacle("1", glm::vec3(-1250.f, -2.50f, 0.0f), glm::vec3(1.0f, 2.0f, 2.0f), glm::vec3(90.0f, 0.0f, 0.0f), RockWallMesh1, RockWallMaterial);
		createGroundObstacle("2", glm::vec3(-1280.f, -12.0f, -1.5f), glm::vec3(2.0f, 2.750f, 1.590f), glm::vec3(90.0f, 0.0f, -180.0f), HangingRockMesh, HangingRockMaterial);
		createGroundObstacle("3", glm::vec3(-1300.f, 0.0f, 0.0f), glm::vec3(3.0f, 2.0f, 2.0f), glm::vec3(90.0f, 0.0f, 0.0f), RockTunnelMesh, RockTunnelMaterial);
		createGroundObstacle("4", glm::vec3(-1360.f, 0.0f, 0.0f), glm::vec3(1.0f, 1.5f, 1.0f), glm::vec3(90.0f, 0.0f, 0.0f), RockWallMesh2, RockWallMaterial);
		createGroundObstacle("5", glm::vec3(-1370.f, 0.0f, 0.0f), glm::vec3(1.0f, 1.5f, 1.0f), glm::vec3(90.0f, 0.0f, 0.0f), RockWallMesh2, RockWallMaterial);
		createGroundObstacle("6", glm::vec3(-1380.f, - 12.0f, -1.5f), glm::vec3(2.0f, 2.750f, 1.590f), glm::vec3(90.0f, 0.0f, -180.0f), HangingRockMesh, HangingRockMaterial);
		createGroundObstacle("7", glm::vec3(-1400.f, -2.50f, 0.0f), glm::vec3(1.0f, 2.0f, 2.0f), glm::vec3(90.0f, 0.0f, 0.0f), RockWallMesh1, RockWallMaterial);
		createGroundObstacle("8", glm::vec3(-1420.f, -2.50f, 0.0f), glm::vec3(1.0f, 2.0f, 2.0f), glm::vec3(90.0f, 0.0f, 0.0f), RockWallMesh1, RockWallMaterial);
		createGroundObstacle("9", glm::vec3(-1425.f, 0.0f, -2.830f), glm::vec3(1.0f, 4.0f, 3.0f), glm::vec3(90.0f, 0.0f, -21.0f), RockPileMesh, RockPileMaterial);
		createGroundObstacle("10", glm::vec3(-1450.f, 1.390f, 0.3f), glm::vec3(4.0f, 2.0f, 4.0f), glm::vec3(90.0f, 0.0f, 90.0f), PuddleMesh, PuddleMaterial);
		createGroundObstacle("11", glm::vec3(-1485.f, 1.390f, 0.3f), glm::vec3(4.0f, 2.0f, 4.0f), glm::vec3(90.0f, 0.0f, 90.0f), PuddleMesh, PuddleMaterial);
		createGroundObstacle("12", glm::vec3(-1520.f, -2.50f, 0.0f), glm::vec3(1.0f, 2.0f, 2.0f), glm::vec3(90.0f, 0.0f, 0.0f), RockWallMesh1, RockWallMaterial);
		createGroundObstacle("13", glm::vec3(-1530.f, -12.0f, -1.5f), glm::vec3(2.0f, 2.750f, 1.590f), glm::vec3(90.0f, 0.0f, -180.0f), HangingRockMesh, HangingRockMaterial);
		createGroundObstacle("14", glm::vec3(-1540.f, -2.50f, 0.0f), glm::vec3(1.0f, 2.0f, 2.0f), glm::vec3(90.0f, 0.0f, 0.0f), RockWallMesh1, RockWallMaterial);
		createGroundObstacle("15", glm::vec3(-1580.f, -4.0f, 0.0f), glm::vec3(5.0f, 5.0f, 5.0f), glm::vec3(90.0f, 0.0f, -168.0f), SignPostMesh, SignPostMaterial);

		


		//3D Backgrounds
		createGroundObstacle("27", glm::vec3(-292.3f - 800.f, -55.830f, -2.5f), glm::vec3(6.f, 6.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGRockMesh, BGMaterial);
		createGroundObstacle("28", glm::vec3(-400.f - 800.f, -55.830f, -2.5f), glm::vec3(6.f, 6.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGRockMesh, BGMaterial);
		createGroundObstacle("29", glm::vec3(-507.7f - 800.f, -55.830f, -2.5f), glm::vec3(6.f, 6.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGRockMesh, BGMaterial);

		createGroundObstacle("30", glm::vec3(-615.4f - 800.f, -55.830f, -2.5f), glm::vec3(6.f, 6.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGRockMesh, BGMaterial);
		createGroundObstacle("31", glm::vec3(-723.1f - 800.f, -55.830f, -2.5f), glm::vec3(6.f, 6.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGRockMesh, BGMaterial);
		createGroundObstacle("32", glm::vec3(-830.8f - 800.f, -55.830f, -2.5f), glm::vec3(6.f, 6.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGRockMesh, BGMaterial);
		createGroundObstacle("26", glm::vec3(-938.2f - 800.f, -55.830f, -2.5f), glm::vec3(6.f, 6.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGRockMesh, BGMaterial);

		//2DBackGrounds
		createGroundObstacle("33", glm::vec3(-75.f - 800.f, -130.0f, 64.130f), glm::vec3(375.0f, 125.0f, 250.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainBackdropMaterial);
		createGroundObstacle("34", glm::vec3(-450.f - 800.f, -130.0f, 64.130f), glm::vec3(375.0f, 125.0f, 250.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainBackdropMaterial);
		createGroundObstacle("35", glm::vec3(-825.f - 800.f, -130.0f, 64.130f), glm::vec3(375.0f, 125.0f, 250.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainBackdropMaterial);
		createGroundObstacle("36", glm::vec3(-2000.f, -130.0f, 64.130f), glm::vec3(375.0f, 125.0f, 250.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainBackdropMaterial);
		createGroundObstacle("37", glm::vec3(-2150.f - 800.f, -130.0f, 64.130f), glm::vec3(375.0f, 125.0f, 250.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainBackdropMaterial);

		//Grass
		createGroundObstacle("38", glm::vec3(-418.170f - 800.f, -38.230f, 5.250f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass1Material);
		createGroundObstacle("39", glm::vec3(-370.42f - 800.f, 46.920f, 5.080f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass2Material);
		createGroundObstacle("40", glm::vec3(-397.91f - 800.f, -47.360f, 5.090f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass3Material);
		createGroundObstacle("41", glm::vec3(-364.3f - 800.f, -37.550f, 5.310f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass4Material);
		createGroundObstacle("42", glm::vec3(-447.340f - 800.f, -37.120f, 5.560f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass5Material);

		createGroundObstacle("43", glm::vec3(-418.170f - 103.5f - 800.f, -38.230f, 5.250f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass1Material);
		createGroundObstacle("44", glm::vec3(-370.42f - 103.5f - 800.f, 46.920f, 5.080f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass2Material);
		createGroundObstacle("45", glm::vec3(-397.91f - 103.5f - 800.f, -47.360f, 5.090f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass3Material);
		createGroundObstacle("46", glm::vec3(-364.3f - 103.5f - 800.f, -37.550f, 5.310f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass4Material);
		createGroundObstacle("47", glm::vec3(-47.340f - 103.5f - 800.f - 400.f, -37.120f, 5.560f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass5Material);

		createGroundObstacle("48", glm::vec3(-18.170f - 103.5f - 103.5f - 800.f - 400.f, -38.230f, 5.250f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass1Material);
		createGroundObstacle("49", glm::vec3(29.580f - 103.5f - 103.5f - 800.f - 400.f, 46.920f, 5.080f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass2Material);
		createGroundObstacle("50", glm::vec3(2.090f - 103.5f - 103.5f - 800.f - 400.f, -47.360f, 5.090f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass3Material);
		createGroundObstacle("51", glm::vec3(35.700f - 103.5f - 103.5f - 800.f - 400.f, -37.550f, 5.310f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass4Material);
		createGroundObstacle("52", glm::vec3(-47.340f - 103.5f - 103.5f - 800.f - 400.f, -37.120f, 5.560f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass5Material);

		createGroundObstacle("53", glm::vec3(-18.170f - 103.5f - 103.5f - 103.5f - 800.f - 400.f, -38.230f, 5.250f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass1Material);
		createGroundObstacle("54", glm::vec3(29.580f - 103.5f - 103.5f - 103.5f - 800.f - 400.f, 46.920f, 5.080f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass2Material);
		createGroundObstacle("55", glm::vec3(2.090f - 103.5f - 103.5f - 103.5f - 800.f - 400.f, -47.360f, 5.090f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass3Material);
		createGroundObstacle("56", glm::vec3(35.700f - 103.5f - 103.5f - 103.5f - 800.f - 400.f, -37.550f, 5.310f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass4Material);
		createGroundObstacle("57", glm::vec3(-47.340f - 103.5f - 103.5f - 103.5f - 800.f - 400.f, -37.120f, 5.560f), glm::vec3(9.0f, 9.0f, 9.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, grass5Material);

		//Exit Rock
		createGroundObstacle("58", glm::vec3(-409.f - 400.f - 800.f, -1.f, 0.f), glm::vec3(1.f), glm::vec3(90.0f, 0.0f, 180.f), ExitRockMesh, ExitRockMaterial);

		//Foreground Grass
		createGroundObstacle("59", glm::vec3(40.f - 400.f - 800.f, -5.f, 10.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
		createGroundObstacle("60", glm::vec3(0.f - 400.f - 800.f, -5.f, 10.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
		createGroundObstacle("61", glm::vec3(-40.f - 400.f - 800.f, -5.f, 10.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
		createGroundObstacle("62", glm::vec3(-80.f - 400.f - 800.f, -5.f, 10.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
		createGroundObstacle("63", glm::vec3(-120.f - 400.f - 800.f, -5.f, 10.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
		createGroundObstacle("64", glm::vec3(-160.f - 400.f - 800.f, -5.f, 10.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);

		createGroundObstacle("65", glm::vec3(-200.f - 400.f - 800.f, -5.f, 10.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
		createGroundObstacle("66", glm::vec3(-240.f - 400.f - 800.f, -5.f, 10.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
		createGroundObstacle("67", glm::vec3(-280.f - 400.f - 800.f, -5.f, 10.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
		createGroundObstacle("68", glm::vec3(-320.f - 400.f - 800.f, -5.f, 10.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
		createGroundObstacle("69", glm::vec3(-360.f - 400.f - 800.f, -5.f, 10.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
		createGroundObstacle("70", glm::vec3(-400.f - 400.f - 800.f, -5.f, 10.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);

		//rock wall
		createCollision("311", -1250.f, 2.470f, 1.f, 2.f);

		//hanging rock
		//-1275 -> -1280
		createCollision("322", -1277.f, 7.470f, 1.f, 2.f);
		createCollision("323", -1278.f, 7.470f, 1.f, 2.f);
		createCollision("324", -1280.f, 7.470f, 1.f, 2.f);

		//rock tunnel // based off 1120
		createCollision("331", -1296.0f, 5.0f, 1.f, 1.f);
		createCollision("332", -1298.963f, 6.570f, 1.f, 1.f);
		createCollision("333", -1302.823f, 5.0f, 1.f, 1.f);

		//tall wall
		createCollision("341", -1360.f, 1.56f, 1.f, 4.f);

		//tall wall
		createCollision("351", -1370.f, 1.56f, 1.f, 4.f);

		//hanging rock
		createCollision("362", -1377.f, 7.470f, 1.f, 2.f);
		createCollision("363", -1378.f, 7.470f, 1.f, 2.f);
		createCollision("364", -1380.f, 7.470f, 1.f, 2.f);

		//rock wall
		createCollision("371", -1400.f, 2.470f, 1.f, 2.f);

		//rock wall
		createCollision("381", -1420.f, 2.470f, 1.f, 2.f);

		//rock pile
		createCollision("391", -1423.924f, 1.560f, 1.f, 2.f); //2y
		createCollision("392", -1425.00f, 12.390f, 1.f, 2.f); //2y

		//puddle
		//-1440 -> 1464
		createCollision("3101", -1440.f, 0.44f, 1.f, 1.f);
		createCollision("3102", -1442.f, 0.44f, 1.f, 1.f);
		createCollision("3103", -1444.f, 0.44f, 1.f, 1.f);
		createCollision("3104", -1446.f, 0.44f, 1.f, 1.f);
		createCollision("3105", -1448.f, 0.44f, 1.f, 1.f);
		createCollision("3106", -1450.f, 0.44f, 1.f, 1.f);
		createCollision("3107", -1452.f, 0.44f, 1.f, 1.f);
		createCollision("3108", -1454.f, 0.44f, 1.f, 1.f);
		createCollision("3109", -1456.f, 0.44f, 1.f, 1.f);
		createCollision("31010", -1458.f, 0.44f, 1.f, 1.f);
		createCollision("31011", -1460.f, 0.44f, 1.f, 1.f);
		createCollision("31012", -1462.f, 0.44f, 1.f, 1.f);
		createCollision("31013", -1464.f, 0.44f, 1.f, 1.f);


		//puddle
		//-1475 -> -1499
		createCollision("3111", -1475.f, 0.44f, 1.f, 1.f);
		createCollision("3112", -1477.f, 0.44f, 1.f, 1.f);
		createCollision("3113", -1479.f, 0.44f, 1.f, 1.f);
		createCollision("3114", -1481.f, 0.44f, 1.f, 1.f);
		createCollision("3115", -1483.f, 0.44f, 1.f, 1.f);
		createCollision("3116", -1485.f, 0.44f, 1.f, 1.f);
		createCollision("3117", -1487.f, 0.44f, 1.f, 1.f);
		createCollision("3118", -1489.f, 0.44f, 1.f, 1.f);
		createCollision("3119", -1491.f, 0.44f, 1.f, 1.f);
		createCollision("31110", -1493.f, 0.44f, 1.f, 1.f);
		createCollision("31111", -1495.f, 0.44f, 1.f, 1.f);
		createCollision("31112", -1497.f, 0.44f, 1.f, 1.f);
		createCollision("31113", -1499.f, 0.44f, 1.f, 1.f);

		//rock wall
		createCollision("3121", -1520.f, 2.470f, 1.f, 2.f);

		//hanging rock
		createCollision("3132", -1527.f, 7.470f, 1.f, 2.f);
		createCollision("3133", -1528.f, 7.470f, 1.f, 2.f);
		createCollision("3134", -1530.f, 7.470f, 1.f, 2.f);

		//rock wall
		createCollision("3141", -1540.f, 2.470f, 1.f, 2.f);

		//sign post
		createCollision("3151", -1580.0f, 8.11f, 1.f, 2.f); //2y
		createCollision("3152", -1579.6f, 7.79f, 1.f, 2.f);
		createCollision("3153", -1579.930f, 8.160f, 1.f, 2.f);
		createCollision("3154", -1580.f, 8.150f, 1.f, 2.f);

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
			player->SetPostion(glm::vec3(-1206.f, 0.0f, 1.0f));
			player->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			player->SetScale(glm::vec3(0.5f, 0.5f, 0.5f));

			// Add some behaviour that relies on the physics body
			//player->Add<JumpBehaviour>(player->GetPosition());
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

		createGroundObstacle("79", glm::vec3(40.f - 400.f - 800.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.22f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
		createGroundObstacle("80", glm::vec3(0.f - 400.f - 800.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
		createGroundObstacle("81", glm::vec3(-40.f - 400.f - 800.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
		createGroundObstacle("82", glm::vec3(-80.f - 400.f - 800.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
		createGroundObstacle("83", glm::vec3(-120.f - 400.f - 800.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
		createGroundObstacle("84", glm::vec3(-160.f - 400.f - 800.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);

		createGroundObstacle("85", glm::vec3(-200.f - 400.f - 800.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
		createGroundObstacle("86", glm::vec3(-240.f - 400.f - 800.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
		createGroundObstacle("87", glm::vec3(-280.f - 400.f - 800.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
		createGroundObstacle("88", glm::vec3(-320.f - 400.f - 800.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
		createGroundObstacle("89", glm::vec3(-360.f - 400.f - 800.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);
		createGroundObstacle("90", glm::vec3(-400.f - 400.f - 800.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MountainForegroundMaterial);


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

		GameObject::Sptr ButtonBack3 = scene->CreateGameObject("ButtonBack3");
		{
			// Set position in the scene
			ButtonBack3->SetPostion(glm::vec3(1.f, 6.5f, 5.f));
			// Scale down the plane
			ButtonBack3->SetScale(glm::vec3(3.0f, 0.8f, 0.5f));
			//spin things
			ButtonBack3->SetRotation(glm::vec3(-80.0f, 0.f, 0.f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = ButtonBack3->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(ButtonMaterial);
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

		GameObject::Sptr LSText = scene->CreateGameObject("LSText");
		{
			// Set position in the scene
			LSText->SetPostion(glm::vec3(1.0f, 8.0f, 6.1f));
			// Scale down the plane
			LSText->SetScale(glm::vec3(2.0f, 0.4f, 0.5f));
			LSText->SetRotation(glm::vec3(80.0f, 0.f, -180.f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = LSText->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(LStextMaterial);
		}


		GameObject::Sptr FrogTongue = scene->CreateGameObject("FrogTongue");
		{
			// Set position in the scene
			FrogTongue->SetPostion(glm::vec3(-3.4f, -1.05f, 3.59f));
			// Scale down the plane
			FrogTongue->SetScale(glm::vec3(1.0f, 0.1f, 1.0f));
			FrogTongue->SetRotation(glm::vec3(0.0f, 0.0f, 45.0f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = FrogTongue->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(FrogTongueMaterial);
		}

		GameObject::Sptr FrogBody = scene->CreateGameObject("FrogBody");
		{
			// Set position in the scene
			FrogBody->SetPostion(glm::vec3(-3.4f, -1.05f, 3.6f));
			// Scale down the plane
			FrogBody->SetScale(glm::vec3(1.5f, 1.5f, 1.0f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = FrogBody->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(FrogBodyMaterial);
		}

		GameObject::Sptr FrogHeadBot = scene->CreateGameObject("FrogHeadBot");
		{
			// Set position in the scene
			FrogHeadBot->SetPostion(glm::vec3(-3.4f, -1.05f, 3.61f));
			// Scale down the plane
			FrogHeadBot->SetScale(glm::vec3(1.5f, 1.5f, 1.0f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = FrogHeadBot->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(FrogHeadBotMaterial);
		}

		GameObject::Sptr FrogHeadTop = scene->CreateGameObject("FrogHeadTop");
		{
			// Set position in the scene
			FrogHeadTop->SetPostion(glm::vec3(-3.4f, -1.05f, 3.62f));
			// Scale down the plane
			FrogHeadTop->SetScale(glm::vec3(1.5f, 1.5f, 1.0f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = FrogHeadTop->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(FrogHeadTopMaterial);
		}

		GameObject::Sptr BushTransition = scene->CreateGameObject("BushTransition");
		{
			// Set position in the scene
			BushTransition->SetPostion(glm::vec3(5.f, 0.0f, 3.8f));
			// Scale down the plane
			BushTransition->SetScale(glm::vec3(5.8f, 2.5f, 1.0f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = BushTransition->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(BushTransitionMaterial);
		}

		scene->SetAmbientLight(glm::vec3(0.2f));

		// Kinematic rigid bodies are those controlled by some outside controller
		// and ONLY collide with dynamic objects
		RigidBody::Sptr physics = jumpingObstacle->Add<RigidBody>(RigidBodyType::Kinematic);
		physics->AddCollider(ConvexMeshCollider::Create());

		// Save the asset manifest for all the resources we just loaded
		ResourceManager::SaveManifest("manifest.json");
		// Save the scene to a JSON file
		scene->Save("Level4.json");

		}

		
		/// Working Level ///									//////		Level 5	////////// scenevalue == 5

		{
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
				BGMaterial->Texture = MBGTex;
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
				ProgressBarMaterial->Texture = Progress3Tex;
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

			Material::Sptr LStextMaterial = ResourceManager::CreateAsset<Material>();
			{
				LStextMaterial->Name = "LStext";
				LStextMaterial->MatShader = scene->BaseShader;
				LStextMaterial->Texture = LStextTex;
				LStextMaterial->Shininess = 2.0f;
			}
			Material::Sptr ExitRockMaterial = ResourceManager::CreateAsset<Material>();
			{
				ExitRockMaterial->Name = "ExitRock";
				ExitRockMaterial->MatShader = scene->BaseShader;
				ExitRockMaterial->Texture = ExitRockTex;
				ExitRockMaterial->Shininess = 2.0f;
			}


			Material::Sptr FrogBodyMaterial = ResourceManager::CreateAsset<Material>();
			{
				FrogBodyMaterial->Name = "FrogBody";
				FrogBodyMaterial->MatShader = scene->BaseShader;
				FrogBodyMaterial->Texture = FrogBodyTex;
				FrogBodyMaterial->Shininess = 2.0f;
			}

			Material::Sptr FrogHeadTopMaterial = ResourceManager::CreateAsset<Material>();
			{
				FrogHeadTopMaterial->Name = "FrogHeadTop";
				FrogHeadTopMaterial->MatShader = scene->BaseShader;
				FrogHeadTopMaterial->Texture = FrogHeadTopTex;
				FrogHeadTopMaterial->Shininess = 2.0f;
			}

			Material::Sptr FrogHeadBotMaterial = ResourceManager::CreateAsset<Material>();
			{
				FrogHeadBotMaterial->Name = "FrogHeadBot";
				FrogHeadBotMaterial->MatShader = scene->BaseShader;
				FrogHeadBotMaterial->Texture = FrogHeadBotTex;
				FrogHeadBotMaterial->Shininess = 2.0f;
			}

			Material::Sptr FrogTongueMaterial = ResourceManager::CreateAsset<Material>();
			{
				FrogTongueMaterial->Name = "FrogTongue";
				FrogTongueMaterial->MatShader = scene->BaseShader;
				FrogTongueMaterial->Texture = FrogTongueTex;
				FrogTongueMaterial->Shininess = 2.0f;
			}

			Material::Sptr BushTransitionMaterial = ResourceManager::CreateAsset<Material>();
			{
				BushTransitionMaterial->Name = "BushTransition";
				BushTransitionMaterial->MatShader = scene->BaseShader;
				BushTransitionMaterial->Texture = BushTransitionTex;
				BushTransitionMaterial->Shininess = 2.0f;
			}

			Material::Sptr SignPostMaterial = ResourceManager::CreateAsset<Material>();
			{
				SignPostMaterial->Name = "SignPost";
				SignPostMaterial->MatShader = scene->BaseShader;
				SignPostMaterial->Texture = SignPostTex;
				SignPostMaterial->Shininess = 2.0f;
			}

			Material::Sptr PuddleMaterial = ResourceManager::CreateAsset<Material>();
			{
				PuddleMaterial->Name = "Puddle";
				PuddleMaterial->MatShader = scene->BaseShader;
				PuddleMaterial->Texture = PuddleTex;
				PuddleMaterial->Shininess = 2.0f;
			}

			Material::Sptr CampfireMaterial = ResourceManager::CreateAsset<Material>();
			{
				CampfireMaterial->Name = "Campfire";
				CampfireMaterial->MatShader = scene->BaseShader;
				CampfireMaterial->Texture = CampfireTex;
				CampfireMaterial->Shininess = 2.0f;
			}

			Material::Sptr HangingRockMaterial = ResourceManager::CreateAsset<Material>();
			{
				HangingRockMaterial->Name = "HangingRock";
				HangingRockMaterial->MatShader = scene->BaseShader;
				HangingRockMaterial->Texture = HangingRockTex;
				HangingRockMaterial->Shininess = 2.0f;
			}

			Material::Sptr RockPileMaterial = ResourceManager::CreateAsset<Material>();
			{
				RockPileMaterial->Name = "RockPile";
				RockPileMaterial->MatShader = scene->BaseShader;
				RockPileMaterial->Texture = RockPileTex;
				RockPileMaterial->Shininess = 2.0f;
			}

			Material::Sptr RockTunnelMaterial = ResourceManager::CreateAsset<Material>();
			{
				RockTunnelMaterial->Name = "RockTunnel";
				RockTunnelMaterial->MatShader = scene->BaseShader;
				RockTunnelMaterial->Texture = RockTunnelTex;
				RockTunnelMaterial->Shininess = 2.0f;
			}

			Material::Sptr RockWallMaterial = ResourceManager::CreateAsset<Material>();
			{
				RockWallMaterial->Name = "RockWall";
				RockWallMaterial->MatShader = scene->BaseShader;
				RockWallMaterial->Texture = RockWallTex;
				RockWallMaterial->Shininess = 2.0f;
			}

			Material::Sptr MineBackdropMaterial = ResourceManager::CreateAsset<Material>();
			{
				MineBackdropMaterial->Name = "Backdrop";
				MineBackdropMaterial->MatShader = scene->BaseShader;
				MineBackdropMaterial->Texture = MineBackgroundTex;
				MineBackdropMaterial->Shininess = 2.0f;
			}

			Material::Sptr MineForegroundMaterial = ResourceManager::CreateAsset<Material>();
			{
				MineForegroundMaterial->Name = "Foreground";
				MineForegroundMaterial->MatShader = scene->BaseShader;
				MineForegroundMaterial->Texture = MineForegroundTex;
				MineForegroundMaterial->Shininess = 2.0f;
			}

			Material::Sptr MineBackgroundMaterial = ResourceManager::CreateAsset<Material>();
			{
				MineBackgroundMaterial->Name = "Background";
				MineBackgroundMaterial->MatShader = scene->BaseShader;
				MineBackgroundMaterial->Texture = MineUVTex;
				MineBackgroundMaterial->Shininess = 2.0f;
			}

			Material::Sptr CaveExitMaterial = ResourceManager::CreateAsset<Material>();
			{
				CaveExitMaterial->Name = "CaveEntrance";
				CaveExitMaterial->MatShader = scene->BaseShader;
				CaveExitMaterial->Texture = CaveEntranceTex;
				CaveExitMaterial->Shininess = 2.0f;
			}

			Material::Sptr GoldBarMaterial = ResourceManager::CreateAsset<Material>();
			{
				GoldBarMaterial->Name = "GoldBar";
				GoldBarMaterial->MatShader = scene->BaseShader;
				GoldBarMaterial->Texture = GoldBarTex;
				GoldBarMaterial->Shininess = 2.0f;
			}

			Material::Sptr GoldPile1Material = ResourceManager::CreateAsset<Material>();
			{
				GoldPile1Material->Name = "GoldPile1";
				GoldPile1Material->MatShader = scene->BaseShader;
				GoldPile1Material->Texture = GoldPile1Tex;
				GoldPile1Material->Shininess = 2.0f;
			}

			Material::Sptr GoldPile2Material = ResourceManager::CreateAsset<Material>();
			{
				GoldPile2Material->Name = "GoldPile2";
				GoldPile2Material->MatShader = scene->BaseShader;
				GoldPile2Material->Texture = GoldPile2Tex;
				GoldPile2Material->Shininess = 2.0f;
			}

			Material::Sptr Crystal1BlueMaterial = ResourceManager::CreateAsset<Material>();
			{
				Crystal1BlueMaterial->Name = "Crystal1Blue";
				Crystal1BlueMaterial->MatShader = scene->BaseShader;
				Crystal1BlueMaterial->Texture = Crystal1BlueTex;
				Crystal1BlueMaterial->Shininess = 2.0f;
			}

			Material::Sptr Crystal1GreenMaterial = ResourceManager::CreateAsset<Material>();
			{
				Crystal1GreenMaterial->Name = "Crystal1Green";
				Crystal1GreenMaterial->MatShader = scene->BaseShader;
				Crystal1GreenMaterial->Texture = Crystal1GreenTex;
				Crystal1GreenMaterial->Shininess = 2.0f;
			}

			Material::Sptr Crystal1PurpleMaterial = ResourceManager::CreateAsset<Material>();
			{
				Crystal1PurpleMaterial->Name = "Crystal1Purple";
				Crystal1PurpleMaterial->MatShader = scene->BaseShader;
				Crystal1PurpleMaterial->Texture = Crystal1PurpleTex;
				Crystal1PurpleMaterial->Shininess = 2.0f;
			}

			Material::Sptr Crystal1RedMaterial = ResourceManager::CreateAsset<Material>();
			{
				Crystal1RedMaterial->Name = "Crystal1Red";
				Crystal1RedMaterial->MatShader = scene->BaseShader;
				Crystal1RedMaterial->Texture = Crystal1RedTex;
				Crystal1RedMaterial->Shininess = 2.0f;
			}

			Material::Sptr Crystal2BlueMaterial = ResourceManager::CreateAsset<Material>();
			{
				Crystal2BlueMaterial->Name = "Crystal2Blue";
				Crystal2BlueMaterial->MatShader = scene->BaseShader;
				Crystal2BlueMaterial->Texture = Crystal2BlueTex;
				Crystal2BlueMaterial->Shininess = 2.0f;
			}

			Material::Sptr Crystal2GreenMaterial = ResourceManager::CreateAsset<Material>();
			{
				Crystal2GreenMaterial->Name = "Crystal2Green";
				Crystal2GreenMaterial->MatShader = scene->BaseShader;
				Crystal2GreenMaterial->Texture = Crystal2GreenTex;
				Crystal2GreenMaterial->Shininess = 2.0f;
			}

			Material::Sptr Crystal2YellowMaterial = ResourceManager::CreateAsset<Material>();
			{
				Crystal2YellowMaterial->Name = "Crystal2Yellow";
				Crystal2YellowMaterial->MatShader = scene->BaseShader;
				Crystal2YellowMaterial->Texture = Crystal2YellowTex;
				Crystal2YellowMaterial->Shininess = 2.0f;
			}

			Material::Sptr StalagmiteMaterial = ResourceManager::CreateAsset<Material>();
			{
				StalagmiteMaterial->Name = "Stalagmite";
				StalagmiteMaterial->MatShader = scene->BaseShader;
				StalagmiteMaterial->Texture = StalagmiteTex;
				StalagmiteMaterial->Shininess = 2.0f;
			}

			Material::Sptr StalagtiteMaterial = ResourceManager::CreateAsset<Material>();
			{
				StalagtiteMaterial->Name = "Stalagtite";
				StalagtiteMaterial->MatShader = scene->BaseShader;
				StalagtiteMaterial->Texture = StalagtiteTex;
				StalagtiteMaterial->Shininess = 2.0f;
			}

			// Create some lights for our scene
			scene->Lights.resize(31);
			scene->Lights[0].Position = glm::vec3(-400.0f - 1200.f, 1.0f, 40.0f);
			scene->Lights[0].Color = glm::vec3(1.f, 0.9867f, 0.8463f);
			scene->Lights[0].Range = 200.0f;

			scene->Lights[1].Position = glm::vec3(-450.f - 1200.f, 0.0f, 40.0f);
			scene->Lights[1].Color = glm::vec3(1.f, 0.9867f, 0.8463f);
			scene->Lights[1].Range = 200.0f;

			scene->Lights[2].Position = glm::vec3(-500.f - 1200.f, 1.0f, 40.0f);
			scene->Lights[2].Color = glm::vec3(1.f, 0.9867f, 0.8463f);
			scene->Lights[2].Range = 200.0f;

			scene->Lights[3].Position = glm::vec3(-550.0f - 1200.f, 1.0f, 40.0f);
			scene->Lights[3].Color = glm::vec3(1.f, 0.9867f, 0.8463f);
			scene->Lights[3].Range = 200.0f;

			scene->Lights[4].Position = glm::vec3(-500.0f - 1200.f, 1.0f, 40.0f);
			scene->Lights[4].Color = glm::vec3(1.f, 0.9867f, 0.8463f);
			scene->Lights[4].Range = 200.0f;

			scene->Lights[5].Position = glm::vec3(-550.0f - 1200.f, 1.0f, 40.0f);
			scene->Lights[5].Color = glm::vec3(1.f, 0.9867f, 0.8463f);
			scene->Lights[5].Range = 200.0f;

			scene->Lights[6].Position = glm::vec3(-600.0f - 1200.f, 1.0f, 40.0f);
			scene->Lights[6].Color = glm::vec3(1.f, 0.9867f, 0.8463f);
			scene->Lights[6].Range = 200.0f;

			scene->Lights[7].Position = glm::vec3(-650.0f - 1200.f, 1.0f, 40.0f);
			scene->Lights[7].Color = glm::vec3(1.f, 0.9867f, 0.8463f);
			scene->Lights[7].Range = 200.0f;

			scene->Lights[8].Position = glm::vec3(-700.0f - 1200.f, 1.0f, 40.0f);
			scene->Lights[8].Color = glm::vec3(1.f, 0.9867f, 0.8463f);
			scene->Lights[8].Range = 200.0f;

			scene->Lights[9].Position = glm::vec3(-750.0f - 1200.f, 1.0f, 40.0f);
			scene->Lights[9].Color = glm::vec3(1.f, 0.9867f, 0.8463f);
			scene->Lights[9].Range = 200.0f;

			scene->Lights[10].Position = glm::vec3(-800.0f - 1200.f, 1.0f, 40.0f);
			scene->Lights[10].Color = glm::vec3(1.f, 0.9867f, 0.8463f);
			scene->Lights[10].Range = 200.0f;

			scene->Lights[11].Position = glm::vec3(-400.0f - 1200.f, 1.0f, 40.0f);
			scene->Lights[11].Color = glm::vec3(0.8268f, 0.6162f, 0.2301f);
			scene->Lights[11].Range = 1000.0f;

			scene->Lights[12].Position = glm::vec3(-450.f - 1200.f, -90.0f, 150.0f);
			scene->Lights[12].Color = glm::vec3(0.8268f, 0.6162f, 0.2301f);
			scene->Lights[12].Range = 1000.0f;

			scene->Lights[13].Position = glm::vec3(-500.f - 1200.f, -90.0f, 150.0f);
			scene->Lights[13].Color = glm::vec3(0.8268f, 0.6162f, 0.2301f);
			scene->Lights[13].Range = 1000.0f;

			scene->Lights[14].Position = glm::vec3(-550.0f - 1200.f, -90.0f, 150.0f);
			scene->Lights[14].Color = glm::vec3(0.8268f, 0.6162f, 0.2301f);
			scene->Lights[14].Range = 1000.0f;

			scene->Lights[15].Position = glm::vec3(-500.0f - 1200.f, -90.0f, 150.0f);
			scene->Lights[15].Color = glm::vec3(0.8268f, 0.6162f, 0.2301f);
			scene->Lights[15].Range = 1000.0f;

			scene->Lights[16].Position = glm::vec3(-550.0f - 1200.f, -90.0f, 150.0f);
			scene->Lights[16].Color = glm::vec3(0.8268f, 0.6162f, 0.2301f);
			scene->Lights[16].Range = 1000.0f;

			scene->Lights[17].Position = glm::vec3(-600.0f - 1200.f, -90.0f, 150.0f);
			scene->Lights[17].Color = glm::vec3(0.8268f, 0.6162f, 0.2301f);
			scene->Lights[17].Range = 1000.0f;

			scene->Lights[18].Position = glm::vec3(-650.0f - 1200.f, -90.0f, 150.0f);
			scene->Lights[18].Color = glm::vec3(0.8268f, 0.6162f, 0.2301f);
			scene->Lights[18].Range = 1000.0f;

			scene->Lights[19].Position = glm::vec3(-700.0f - 1200.f, -90.0f, 150.0f);
			scene->Lights[19].Color = glm::vec3(0.8268f, 0.6162f, 0.2301f);
			scene->Lights[19].Range = 1000.0f;

			scene->Lights[20].Position = glm::vec3(-750.0f - 1200.f, -90.0f, 150.0f);
			scene->Lights[20].Color = glm::vec3(0.8268f, 0.6162f, 0.2301f);
			scene->Lights[20].Range = 1000.0f;

			scene->Lights[21].Position = glm::vec3(-800.0f - 1200.f, -90.0f, 150.0f);
			scene->Lights[21].Color = glm::vec3(0.8268f, 0.6162f, 0.2301f);
			scene->Lights[21].Range = 1000.0f;

			scene->Lights[22].Position = glm::vec3(-400.0f - 1200.f, 20.0f, 5.0f);
			scene->Lights[22].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[22].Range = 150.0f;

			scene->Lights[23].Position = glm::vec3(-450.0f - 1200.f, 20.0f, 5.0f);
			scene->Lights[23].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[23].Range = 150.0f;

			scene->Lights[24].Position = glm::vec3(-500.0f - 1200.f, 20.0f, 5.0f);
			scene->Lights[24].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[24].Range = 150.0f;

			scene->Lights[25].Position = glm::vec3(-550.0f - 1200.f, 20.0f, 5.0f);
			scene->Lights[25].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[25].Range = 150.0f;

			scene->Lights[26].Position = glm::vec3(-600.0f - 1200.f, 20.0f, 5.0f);
			scene->Lights[26].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[26].Range = 150.0f;

			scene->Lights[27].Position = glm::vec3(-650.0f - 1200.f, 20.0f, 5.0f);
			scene->Lights[27].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[27].Range = 150.0f;

			scene->Lights[28].Position = glm::vec3(-700.0f - 1200.f, 20.0f, 5.0f);
			scene->Lights[28].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[28].Range = 150.0f;

			scene->Lights[29].Position = glm::vec3(-750.0f - 1200.f, 20.0f, 5.0f);
			scene->Lights[29].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[29].Range = 150.0f;

			scene->Lights[30].Position = glm::vec3(-800.0f - 1200.f, 20.0f, 5.0f);
			scene->Lights[30].Color = glm::vec3(1.f, 1.f, 1.f);
			scene->Lights[30].Range = 150.0f;

			// We'll create a mesh that is a simple plane that we can resize later
			planeMesh = ResourceManager::CreateAsset<MeshResource>();
			cubeMesh = ResourceManager::CreateAsset<MeshResource>("cube.obj");

			mushroomMesh = ResourceManager::CreateAsset<MeshResource>("Mushroom.obj");
			tmMesh = ResourceManager::CreateAsset<MeshResource>("tm.obj");
			bmMesh = ResourceManager::CreateAsset<MeshResource>("bm.obj");
			ToadMesh = ResourceManager::CreateAsset<MeshResource>("ToadStool.obj");


			BranchMesh = ResourceManager::CreateAsset<MeshResource>("Branch.obj");
			LogMesh = ResourceManager::CreateAsset<MeshResource>("Log.obj");
			Plant2Mesh = ResourceManager::CreateAsset<MeshResource>("Plant2.obj");
			Plant3Mesh = ResourceManager::CreateAsset<MeshResource>("Plant3.obj");



			ladybugMesh = ResourceManager::CreateAsset<MeshResource>("lbo.obj");
			frogMesh = ResourceManager::CreateAsset<MeshResource>("frog.obj");

			BGMineMesh = ResourceManager::CreateAsset<MeshResource>("MineBackground.obj");
			BGRockMesh = ResourceManager::CreateAsset<MeshResource>("RockBackground.obj");
			BGMesh = ResourceManager::CreateAsset<MeshResource>("Background.obj");

			CaveEntranceMesh = ResourceManager::CreateAsset<MeshResource>("CaveEntrance.obj");
			Crystal1Mesh = ResourceManager::CreateAsset<MeshResource>("Crystals1.obj");
			Crystal2Mesh = ResourceManager::CreateAsset<MeshResource>("Crystals2.obj");
			StalagmiteMesh = ResourceManager::CreateAsset<MeshResource>("Stalagmite.obj");
			StalagtiteMesh = ResourceManager::CreateAsset<MeshResource>("Stalagtite.obj");
			GoldbarMesh = ResourceManager::CreateAsset<MeshResource>("Goldbar.obj");
			GoldPile1Mesh = ResourceManager::CreateAsset<MeshResource>("GoldPile1.obj");
			GoldPile2Mesh = ResourceManager::CreateAsset<MeshResource>("GoldPile2.obj");


			planeMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(1.0f)));
			planeMesh->GenerateMesh();



			//Background Assets
			//createBackgroundAsset("16", glm::vec3(-387.060f - 400.f, 1.530f, 0.550), 0.05, glm::vec3(83.f, -7.0f, 90.0f), frogMesh, frogMaterial);

			//Obstacles

			//createGroundObstacle("18", glm::vec3(-320.f, 0.0f, -0.660), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(90.f, 0.0f, 0.0f), mushroomMesh, mushroomMaterial); //mushroom 8 (small jump)
			//createGroundObstacle("20", glm::vec3(-340.f, 0.0f, 3.0), glm::vec3(1.f, 1.f, 1.f), glm::vec3(90.f, 0.0f, 73.f), vinesMesh, vinesMaterial); // vine 6 (jump blocking)
			//createGroundObstacle("24", glm::vec3(-380.f, 5.530f, 0.250f), glm::vec3(1.5f, 1.5f, 1.5f), glm::vec3(90.f, 0.0f, -25.f), vinesMesh, vinesMaterial); // vine 8 (squish blocking)
			//createGroundObstacle("26", glm::vec3(-395.f, 0.0f, 3.3f), glm::vec3(0.25f, 0.25f, 0.25f), glm::vec3(0.0f, 0.0f, -75.f), cobwebMesh, cobwebMaterial); //cobweb 9 (tall jump)


			createGroundObstacle("1", glm::vec3(-1650.f, 0.0f, 0.215f), glm::vec3(6.f, 4.f, 8.f), glm::vec3(90.f, 0.0f, 80.0f), Crystal1Mesh, Crystal1BlueMaterial); // (beeg jump)
			createGroundObstacle("2", glm::vec3(-1700.f, 0.0f, -0.f), glm::vec3(4.f, 1.5f, 6.f), glm::vec3(90.f, 0.0f, -162.0f), Crystal2Mesh, Crystal2BlueMaterial); //(small jump)
			createGroundObstacle("3", glm::vec3(-1750.f, 0.0f, -0.f), glm::vec3(4.f, 1.5f, 6.f), glm::vec3(90.f, 0.0f, -162.0f), Crystal2Mesh, Crystal2BlueMaterial); //(small jump)
			createGroundObstacle("4", glm::vec3(-1800.f, 0.0f, 0.215f), glm::vec3(6.f, 4.f, 8.f), glm::vec3(90.f, 0.0f, 80.0f), Crystal1Mesh, Crystal1BlueMaterial); // (beeg jump)
			createGroundObstacle("5", glm::vec3(-1850.f, 0.0f, -0.f), glm::vec3(4.f, 1.5f, 6.f), glm::vec3(90.f, 0.0f, -162.0f), Crystal2Mesh, Crystal2BlueMaterial); //(small jump)
			createGroundObstacle("6", glm::vec3(-1880.f, 0.0f, -0.f), glm::vec3(4.f, 1.5f, 6.f), glm::vec3(90.f, 0.0f, -162.0f), Crystal2Mesh, Crystal2BlueMaterial); //(small jump)
			createGroundObstacle("7", glm::vec3(-1900.f, 0.0f, 0.215f), glm::vec3(6.f, 4.f, 8.f), glm::vec3(90.f, 0.0f, 80.0f), Crystal1Mesh, Crystal1BlueMaterial); // (beeg jump)
			createGroundObstacle("8", glm::vec3(-1910.f, 0.0f, 0.215f), glm::vec3(6.f, 4.f, 8.f), glm::vec3(90.f, 0.0f, 80.0f), Crystal1Mesh, Crystal1BlueMaterial); // (beeg jump)
			createGroundObstacle("9", glm::vec3(-1930.f, 0.0f, -0.f), glm::vec3(4.f, 1.5f, 6.f), glm::vec3(90.f, 0.0f, -162.0f), Crystal2Mesh, Crystal2BlueMaterial); //(small jump)
			createGroundObstacle("10", glm::vec3(-1950.f, 0.0f, 0.215f), glm::vec3(6.f, 4.f, 8.f), glm::vec3(90.f, 0.0f, 80.0f), Crystal1Mesh, Crystal1BlueMaterial); // (beeg jump)
			createGroundObstacle("11", glm::vec3(-1960.f, 0.0f, 0.215f), glm::vec3(6.f, 4.f, 8.f), glm::vec3(90.f, 0.0f, 80.0f), Crystal1Mesh, Crystal1BlueMaterial); // (beeg jump)

			//3D Backgrounds
			createGroundObstacle("27", glm::vec3(-292.3f - 1200.f, -53.250f, -4.5f), glm::vec3(6.f, 12.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGMineMesh, MineBackgroundMaterial);
			createGroundObstacle("28", glm::vec3(-400.f - 1200.f, -53.250f, -4.5f), glm::vec3(6.f, 12.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGMineMesh, MineBackgroundMaterial);
			createGroundObstacle("29", glm::vec3(-507.7f - 1200.f, -53.250f, -4.5f), glm::vec3(6.f, 12.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGMineMesh, MineBackgroundMaterial);

			createGroundObstacle("30", glm::vec3(-615.4f - 1200.f, -53.250f, -4.5f), glm::vec3(6.f, 12.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGMineMesh, MineBackgroundMaterial);
			createGroundObstacle("31", glm::vec3(-723.1f - 1200.f, -53.250f, -4.5f), glm::vec3(6.f, 12.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGMineMesh, MineBackgroundMaterial);
			createGroundObstacle("32", glm::vec3(-830.8f - 1200.f, -53.250f, -4.5f), glm::vec3(6.f, 12.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGMineMesh, MineBackgroundMaterial);
			createGroundObstacle("26", glm::vec3(-938.2f - 1200.f, -53.250f, -4.5f), glm::vec3(6.f, 12.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGMineMesh, MineBackgroundMaterial);

			//2DBackGrounds
			createGroundObstacle("33", glm::vec3(-75.f - 1200.f, -130.0f, 64.130f), glm::vec3(375.0f, 125.0f, 250.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineBackdropMaterial);
			createGroundObstacle("34", glm::vec3(-450.f - 1200.f, -130.0f, 64.130f), glm::vec3(375.0f, 125.0f, 250.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineBackdropMaterial);
			createGroundObstacle("35", glm::vec3(-825.f - 1200.f, -130.0f, 64.130f), glm::vec3(375.0f, 125.0f, 250.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineBackdropMaterial);
			createGroundObstacle("36", glm::vec3(-2000.f, -130.0f, 64.130f), glm::vec3(375.0f, 125.0f, 250.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineBackdropMaterial);
			createGroundObstacle("37", glm::vec3(-2150.f - 400.f, -130.0f, 64.130f), glm::vec3(375.0f, 125.0f, 250.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineBackdropMaterial);

			//Exit Rock
			createGroundObstacle("58", glm::vec3(-409.f - 400.f - 1200.f, -1.f, 0.f), glm::vec3(2.f, 2.f, 1.f), glm::vec3(90.0f, 0.0f, -68.f), CaveEntranceMesh, CaveExitMaterial);

			//Foreground 
			createGroundObstacle("59", glm::vec3(40.f - 400.f - 1200.f, -5.f, 8.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
			createGroundObstacle("60", glm::vec3(0.f - 400.f - 1200.f, -5.f, 8.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
			createGroundObstacle("61", glm::vec3(-40.f - 400.f - 1200.f, -5.f, 8.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
			createGroundObstacle("62", glm::vec3(-80.f - 400.f - 1200.f, -5.f, 8.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
			createGroundObstacle("63", glm::vec3(-120.f - 400.f - 1200.f, -5.f, 8.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
			createGroundObstacle("64", glm::vec3(-160.f - 400.f - 1200.f, -5.f, 8.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);

			createGroundObstacle("65", glm::vec3(-200.f - 400.f - 1200.f, -5.f, 8.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
			createGroundObstacle("66", glm::vec3(-240.f - 400.f - 1200.f, -5.f, 8.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
			createGroundObstacle("67", glm::vec3(-280.f - 400.f - 1200.f, -5.f, 8.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
			createGroundObstacle("68", glm::vec3(-320.f - 400.f - 1200.f, -5.f, 8.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
			createGroundObstacle("69", glm::vec3(-360.f - 400.f - 1200.f, -5.f, 8.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
			createGroundObstacle("70", glm::vec3(-400.f - 400.f - 1200.f, -5.f, 8.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);

			//large rock
			createCollision("511", -1646.f, 1.560f, 1.f, 1.f);
			createCollision("512", -1649.44f, 4.f, 1.f, 1.f);
			createCollision("513", -1652.f, 3.8f, 1.f, 1.f);
			createCollision("514", -1648.f, 2.82f, 1.f, 1.f);
			createCollision("515", -1646.f, 1.f, 1.f, 1.f);
			createCollision("516", -1654.f, 1.56f, 1.f, 1.f);

			//small rock
			createCollision("521", -1698.f, 0.8f, 1.f, 1.f);
			createCollision("522", -1699.f, 1.f, 1.f, 1.f);
			createCollision("523", -1700.f, 1.f, 1.f, 1.f);
			createCollision("524", -1702.f, 0.7f, 1.f, 1.f);

			//small rock
			createCollision("531", -1748.f, 0.8f, 1.f, 1.f);
			createCollision("532", -1749.f, 1.f, 1.f, 1.f);
			createCollision("533", -1750.f, 1.f, 1.f, 1.f);
			createCollision("534", -1752.f, 0.7f, 1.f, 1.f);

			//large rock
			createCollision("541", -1796.f, 1.560f, 1.f, 1.f);
			createCollision("542", -1796.f, 1.f, 1.f, 1.f);
			createCollision("543", -1798.f, 2.82f, 1.f, 1.f);
			createCollision("544", -1799.44f, 4.f, 1.f, 1.f);
			createCollision("545", -1802.f, 3.8f, 1.f, 1.f);
			createCollision("546", -1804.f, 1.56f, 1.f, 1.f);

			//small rock 1850
			createCollision("551", -1848.f, 0.8f, 1.f, 1.f);
			createCollision("552", -1849.f, 1.f, 1.f, 1.f);
			createCollision("553", -1850.f, 1.f, 1.f, 1.f);
			createCollision("554", -1852.f, 0.7f, 1.f, 1.f);

			//small rock 1880
			createCollision("561", -1878.f, 0.8f, 1.f, 1.f);
			createCollision("562", -1879.f, 1.f, 1.f, 1.f);
			createCollision("563", -1880.f, 1.f, 1.f, 1.f);
			createCollision("564", -1882.f, 0.7f, 1.f, 1.f);

			//large rock 1900
			createCollision("571", -1896.f, 1.560f, 1.f, 1.f);
			createCollision("572", -1896.f, 1.f, 1.f, 1.f);
			createCollision("573", -1898.f, 2.82f, 1.f, 1.f);
			createCollision("574", -1899.44f, 4.f, 1.f, 1.f);
			createCollision("575", -1902.f, 3.8f, 1.f, 1.f);
			createCollision("576", -1904.f, 1.56f, 1.f, 1.f);

			//large rock 1910
			createCollision("581", -1906.f, 1.560f, 1.f, 1.f);
			createCollision("582", -1906.f, 1.f, 1.f, 1.f);
			createCollision("583", -1908.f, 2.82f, 1.f, 1.f);
			createCollision("584", -1909.44f, 4.f, 1.f, 1.f);
			createCollision("585", -1912.f, 3.8f, 1.f, 1.f);
			createCollision("586", -1914.f, 1.56f, 1.f, 1.f);

			//small rock 1930
			createCollision("591", -1928.f, 0.8f, 1.f, 1.f);
			createCollision("592", -1929.f, 1.f, 1.f, 1.f);
			createCollision("593", -1930.f, 1.f, 1.f, 1.f);
			createCollision("594", -1932.f, 0.7f, 1.f, 1.f);

			//large rock 1950
			createCollision("5101", -1946.f, 1.560f, 1.f, 1.f);
			createCollision("5102", -1946.f, 1.f, 1.f, 1.f);
			createCollision("5103", -1948.f, 2.82f, 1.f, 1.f);
			createCollision("5104", -1949.44f, 4.f, 1.f, 1.f);
			createCollision("5105", -1952.f, 3.8f, 1.f, 1.f);
			createCollision("5106", -1954.f, 1.56f, 1.f, 1.f);

			//large rock 1960
			createCollision("5111", -1956.f, 1.560f, 1.f, 1.f);
			createCollision("5112", -1956.f, 1.f, 1.f, 1.f);
			createCollision("5113", -1958.f, 2.82f, 1.f, 1.f);
			createCollision("5114", -1959.44f, 4.f, 1.f, 1.f);
			createCollision("5115", -1962.f, 3.8f, 1.f, 1.f);
			createCollision("5116", -1964.f, 1.56f, 1.f, 1.f);



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
				player->SetPostion(glm::vec3(-1606.f, 0.0f, 1.0f));
				player->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
				player->SetScale(glm::vec3(0.5f, 0.5f, 0.5f));

				// Add some behaviour that relies on the physics body
				//player->Add<JumpBehaviour>(player->GetPosition());
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

			createGroundObstacle("79", glm::vec3(40.f - 400.f - 1200.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.22f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
			createGroundObstacle("80", glm::vec3(0.f - 400.f - 1200.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
			createGroundObstacle("81", glm::vec3(-40.f - 400.f - 1200.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
			createGroundObstacle("82", glm::vec3(-80.f - 400.f - 1200.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
			createGroundObstacle("83", glm::vec3(-120.f - 400.f - 1200.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
			createGroundObstacle("84", glm::vec3(-160.f - 400.f - 1200.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);

			createGroundObstacle("85", glm::vec3(-200.f - 400.f - 1200.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
			createGroundObstacle("86", glm::vec3(-240.f - 400.f - 1200.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
			createGroundObstacle("87", glm::vec3(-280.f - 400.f - 1200.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
			createGroundObstacle("88", glm::vec3(-320.f - 400.f - 1200.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
			createGroundObstacle("89", glm::vec3(-360.f - 400.f - 1200.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
			createGroundObstacle("90", glm::vec3(-400.f - 400.f - 1200.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);


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

			GameObject::Sptr ButtonBack3 = scene->CreateGameObject("ButtonBack3");
			{
				// Set position in the scene
				ButtonBack3->SetPostion(glm::vec3(1.f, 6.5f, 5.f));
				// Scale down the plane
				ButtonBack3->SetScale(glm::vec3(3.0f, 0.8f, 0.5f));
				//spin things
				ButtonBack3->SetRotation(glm::vec3(-80.0f, 0.f, 0.f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = ButtonBack3->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(ButtonMaterial);
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

			GameObject::Sptr LSText = scene->CreateGameObject("LSText");
			{
				// Set position in the scene
				LSText->SetPostion(glm::vec3(1.0f, 8.0f, 6.1f));
				// Scale down the plane
				LSText->SetScale(glm::vec3(2.0f, 0.4f, 0.5f));
				LSText->SetRotation(glm::vec3(80.0f, 0.f, -180.f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = LSText->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(LStextMaterial);
			}


			GameObject::Sptr FrogTongue = scene->CreateGameObject("FrogTongue");
			{
				// Set position in the scene
				FrogTongue->SetPostion(glm::vec3(-3.4f, -1.05f, 3.59f));
				// Scale down the plane
				FrogTongue->SetScale(glm::vec3(1.0f, 0.1f, 1.0f));
				FrogTongue->SetRotation(glm::vec3(0.0f, 0.0f, 45.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = FrogTongue->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(FrogTongueMaterial);
			}

			GameObject::Sptr FrogBody = scene->CreateGameObject("FrogBody");
			{
				// Set position in the scene
				FrogBody->SetPostion(glm::vec3(-3.4f, -1.05f, 3.6f));
				// Scale down the plane
				FrogBody->SetScale(glm::vec3(1.5f, 1.5f, 1.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = FrogBody->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(FrogBodyMaterial);
			}

			GameObject::Sptr FrogHeadBot = scene->CreateGameObject("FrogHeadBot");
			{
				// Set position in the scene
				FrogHeadBot->SetPostion(glm::vec3(-3.4f, -1.05f, 3.61f));
				// Scale down the plane
				FrogHeadBot->SetScale(glm::vec3(1.5f, 1.5f, 1.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = FrogHeadBot->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(FrogHeadBotMaterial);
			}

			GameObject::Sptr FrogHeadTop = scene->CreateGameObject("FrogHeadTop");
			{
				// Set position in the scene
				FrogHeadTop->SetPostion(glm::vec3(-3.4f, -1.05f, 3.62f));
				// Scale down the plane
				FrogHeadTop->SetScale(glm::vec3(1.5f, 1.5f, 1.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = FrogHeadTop->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(FrogHeadTopMaterial);
			}

			GameObject::Sptr BushTransition = scene->CreateGameObject("BushTransition");
			{
				// Set position in the scene
				BushTransition->SetPostion(glm::vec3(5.f, 0.0f, 3.8f));
				// Scale down the plane
				BushTransition->SetScale(glm::vec3(5.8f, 2.5f, 1.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = BushTransition->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(BushTransitionMaterial);
			}

			scene->SetAmbientLight(glm::vec3(0.2f));

			// Kinematic rigid bodies are those controlled by some outside controller
			// and ONLY collide with dynamic objects
			RigidBody::Sptr physics = jumpingObstacle->Add<RigidBody>(RigidBodyType::Kinematic);
			physics->AddCollider(ConvexMeshCollider::Create());

			// Save the asset manifest for all the resources we just loaded
			ResourceManager::SaveManifest("manifest.json");
			// Save the scene to a JSON file
			scene->Save("Level5.json");

		}

		/// Working Level ///									//////		Level 6	////////// scenevalue == 6

		{
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
			BGMaterial->Texture = MBGTex;
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
			ProgressBarMaterial->Texture = Progress3Tex;
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
		Material::Sptr LStextMaterial = ResourceManager::CreateAsset<Material>();
		{
			LStextMaterial->Name = "LStext";
			LStextMaterial->MatShader = scene->BaseShader;
			LStextMaterial->Texture = LStextTex;
			LStextMaterial->Shininess = 2.0f;
		}
		Material::Sptr ExitRockMaterial = ResourceManager::CreateAsset<Material>();
		{
			ExitRockMaterial->Name = "ExitRock";
			ExitRockMaterial->MatShader = scene->BaseShader;
			ExitRockMaterial->Texture = ExitRockTex;
			ExitRockMaterial->Shininess = 2.0f;
		}


		Material::Sptr FrogBodyMaterial = ResourceManager::CreateAsset<Material>();
		{
			FrogBodyMaterial->Name = "FrogBody";
			FrogBodyMaterial->MatShader = scene->BaseShader;
			FrogBodyMaterial->Texture = FrogBodyTex;
			FrogBodyMaterial->Shininess = 2.0f;
		}

		Material::Sptr FrogHeadTopMaterial = ResourceManager::CreateAsset<Material>();
		{
			FrogHeadTopMaterial->Name = "FrogHeadTop";
			FrogHeadTopMaterial->MatShader = scene->BaseShader;
			FrogHeadTopMaterial->Texture = FrogHeadTopTex;
			FrogHeadTopMaterial->Shininess = 2.0f;
		}

		Material::Sptr FrogHeadBotMaterial = ResourceManager::CreateAsset<Material>();
		{
			FrogHeadBotMaterial->Name = "FrogHeadBot";
			FrogHeadBotMaterial->MatShader = scene->BaseShader;
			FrogHeadBotMaterial->Texture = FrogHeadBotTex;
			FrogHeadBotMaterial->Shininess = 2.0f;
		}

		Material::Sptr FrogTongueMaterial = ResourceManager::CreateAsset<Material>();
		{
			FrogTongueMaterial->Name = "FrogTongue";
			FrogTongueMaterial->MatShader = scene->BaseShader;
			FrogTongueMaterial->Texture = FrogTongueTex;
			FrogTongueMaterial->Shininess = 2.0f;
		}

		Material::Sptr BushTransitionMaterial = ResourceManager::CreateAsset<Material>();
		{
			BushTransitionMaterial->Name = "BushTransition";
			BushTransitionMaterial->MatShader = scene->BaseShader;
			BushTransitionMaterial->Texture = BushTransitionTex;
			BushTransitionMaterial->Shininess = 2.0f;
		}

		Material::Sptr SignPostMaterial = ResourceManager::CreateAsset<Material>();
		{
			SignPostMaterial->Name = "SignPost";
			SignPostMaterial->MatShader = scene->BaseShader;
			SignPostMaterial->Texture = SignPostTex;
			SignPostMaterial->Shininess = 2.0f;
		}

		Material::Sptr PuddleMaterial = ResourceManager::CreateAsset<Material>();
		{
			PuddleMaterial->Name = "Puddle";
			PuddleMaterial->MatShader = scene->BaseShader;
			PuddleMaterial->Texture = PuddleTex;
			PuddleMaterial->Shininess = 2.0f;
		}

		Material::Sptr CampfireMaterial = ResourceManager::CreateAsset<Material>();
		{
			CampfireMaterial->Name = "Campfire";
			CampfireMaterial->MatShader = scene->BaseShader;
			CampfireMaterial->Texture = CampfireTex;
			CampfireMaterial->Shininess = 2.0f;
		}

		Material::Sptr HangingRockMaterial = ResourceManager::CreateAsset<Material>();
		{
			HangingRockMaterial->Name = "HangingRock";
			HangingRockMaterial->MatShader = scene->BaseShader;
			HangingRockMaterial->Texture = HangingRockTex;
			HangingRockMaterial->Shininess = 2.0f;
		}

		Material::Sptr RockPileMaterial = ResourceManager::CreateAsset<Material>();
		{
			RockPileMaterial->Name = "RockPile";
			RockPileMaterial->MatShader = scene->BaseShader;
			RockPileMaterial->Texture = RockPileTex;
			RockPileMaterial->Shininess = 2.0f;
		}

		Material::Sptr RockTunnelMaterial = ResourceManager::CreateAsset<Material>();
		{
			RockTunnelMaterial->Name = "RockTunnel";
			RockTunnelMaterial->MatShader = scene->BaseShader;
			RockTunnelMaterial->Texture = RockTunnelTex;
			RockTunnelMaterial->Shininess = 2.0f;
		}

		Material::Sptr RockWallMaterial = ResourceManager::CreateAsset<Material>();
		{
			RockWallMaterial->Name = "RockWall";
			RockWallMaterial->MatShader = scene->BaseShader;
			RockWallMaterial->Texture = RockWallTex;
			RockWallMaterial->Shininess = 2.0f;
		}

		Material::Sptr MineBackdropMaterial = ResourceManager::CreateAsset<Material>();
		{
			MineBackdropMaterial->Name = "Backdrop";
			MineBackdropMaterial->MatShader = scene->BaseShader;
			MineBackdropMaterial->Texture = MineBackgroundTex;
			MineBackdropMaterial->Shininess = 2.0f;
		}

		Material::Sptr MineForegroundMaterial = ResourceManager::CreateAsset<Material>();
		{
			MineForegroundMaterial->Name = "Foreground";
			MineForegroundMaterial->MatShader = scene->BaseShader;
			MineForegroundMaterial->Texture = MineForegroundTex;
			MineForegroundMaterial->Shininess = 2.0f;
		}

		Material::Sptr MineBackgroundMaterial = ResourceManager::CreateAsset<Material>();
		{
			MineBackgroundMaterial->Name = "Background";
			MineBackgroundMaterial->MatShader = scene->BaseShader;
			MineBackgroundMaterial->Texture = MineUVTex;
			MineBackgroundMaterial->Shininess = 2.0f;
		}

		Material::Sptr CaveEntranceMaterial = ResourceManager::CreateAsset<Material>();
		{
			CaveEntranceMaterial->Name = "CaveEntrance";
			CaveEntranceMaterial->MatShader = scene->BaseShader;
			CaveEntranceMaterial->Texture = CaveEntranceTex;
			CaveEntranceMaterial->Shininess = 2.0f;
		}

		Material::Sptr GoldBarMaterial = ResourceManager::CreateAsset<Material>();
		{
			GoldBarMaterial->Name = "GoldBar";
			GoldBarMaterial->MatShader = scene->BaseShader;
			GoldBarMaterial->Texture = GoldBarTex;
			GoldBarMaterial->Shininess = 2.0f;
		}

		Material::Sptr GoldPile1Material = ResourceManager::CreateAsset<Material>();
		{
			GoldPile1Material->Name = "GoldPile1";
			GoldPile1Material->MatShader = scene->BaseShader;
			GoldPile1Material->Texture = GoldPile1Tex;
			GoldPile1Material->Shininess = 2.0f;
		}

		Material::Sptr GoldPile2Material = ResourceManager::CreateAsset<Material>();
		{
			GoldPile2Material->Name = "GoldPile2";
			GoldPile2Material->MatShader = scene->BaseShader;
			GoldPile2Material->Texture = GoldPile2Tex;
			GoldPile2Material->Shininess = 2.0f;
		}

		Material::Sptr Crystal1BlueMaterial = ResourceManager::CreateAsset<Material>();
		{
			Crystal1BlueMaterial->Name = "Crystal1Blue";
			Crystal1BlueMaterial->MatShader = scene->BaseShader;
			Crystal1BlueMaterial->Texture = Crystal1BlueTex;
			Crystal1BlueMaterial->Shininess = 2.0f;
		}

		Material::Sptr Crystal1GreenMaterial = ResourceManager::CreateAsset<Material>();
		{
			Crystal1GreenMaterial->Name = "Crystal1Green";
			Crystal1GreenMaterial->MatShader = scene->BaseShader;
			Crystal1GreenMaterial->Texture = Crystal1GreenTex;
			Crystal1GreenMaterial->Shininess = 2.0f;
		}

		Material::Sptr Crystal1PurpleMaterial = ResourceManager::CreateAsset<Material>();
		{
			Crystal1PurpleMaterial->Name = "Crystal1Purple";
			Crystal1PurpleMaterial->MatShader = scene->BaseShader;
			Crystal1PurpleMaterial->Texture = Crystal1PurpleTex;
			Crystal1PurpleMaterial->Shininess = 2.0f;
		}

		Material::Sptr Crystal1RedMaterial = ResourceManager::CreateAsset<Material>();
		{
			Crystal1RedMaterial->Name = "Crystal1Red";
			Crystal1RedMaterial->MatShader = scene->BaseShader;
			Crystal1RedMaterial->Texture = Crystal1RedTex;
			Crystal1RedMaterial->Shininess = 2.0f;
		}

		Material::Sptr Crystal2BlueMaterial = ResourceManager::CreateAsset<Material>();
		{
			Crystal2BlueMaterial->Name = "Crystal2Blue";
			Crystal2BlueMaterial->MatShader = scene->BaseShader;
			Crystal2BlueMaterial->Texture = Crystal2BlueTex;
			Crystal2BlueMaterial->Shininess = 2.0f;
		}

		Material::Sptr Crystal2GreenMaterial = ResourceManager::CreateAsset<Material>();
		{
			Crystal2GreenMaterial->Name = "Crystal2Green";
			Crystal2GreenMaterial->MatShader = scene->BaseShader;
			Crystal2GreenMaterial->Texture = Crystal2GreenTex;
			Crystal2GreenMaterial->Shininess = 2.0f;
		}

		Material::Sptr Crystal2YellowMaterial = ResourceManager::CreateAsset<Material>();
		{
			Crystal2YellowMaterial->Name = "Crystal2Yellow";
			Crystal2YellowMaterial->MatShader = scene->BaseShader;
			Crystal2YellowMaterial->Texture = Crystal2YellowTex;
			Crystal2YellowMaterial->Shininess = 2.0f;
		}

		Material::Sptr StalagmiteMaterial = ResourceManager::CreateAsset<Material>();
		{
			StalagmiteMaterial->Name = "Stalagmite";
			StalagmiteMaterial->MatShader = scene->BaseShader;
			StalagmiteMaterial->Texture = StalagmiteTex;
			StalagmiteMaterial->Shininess = 2.0f;
		}

		Material::Sptr StalagtiteMaterial = ResourceManager::CreateAsset<Material>();
		{
			StalagtiteMaterial->Name = "Stalagtite";
			StalagtiteMaterial->MatShader = scene->BaseShader;
			StalagtiteMaterial->Texture = StalagtiteTex;
			StalagtiteMaterial->Shininess = 2.0f;
		}

		// Create some lights for our scene
		scene->Lights.resize(31);
		scene->Lights[0].Position = glm::vec3(-400.0f - 1600.f, 1.0f, 40.0f);
		scene->Lights[0].Color = glm::vec3(1.f, 0.9867f, 0.8463f);
		scene->Lights[0].Range = 200.0f;

		scene->Lights[1].Position = glm::vec3(-450.f - 1600.f, 0.0f, 40.0f);
		scene->Lights[1].Color = glm::vec3(1.f, 0.9867f, 0.8463f);
		scene->Lights[1].Range = 200.0f;

		scene->Lights[2].Position = glm::vec3(-500.f - 1600.f, 1.0f, 40.0f);
		scene->Lights[2].Color = glm::vec3(1.f, 0.9867f, 0.8463f);
		scene->Lights[2].Range = 200.0f;

		scene->Lights[3].Position = glm::vec3(-550.0f - 1600.f, 1.0f, 40.0f);
		scene->Lights[3].Color = glm::vec3(1.f, 0.9867f, 0.8463f);
		scene->Lights[3].Range = 200.0f;

		scene->Lights[4].Position = glm::vec3(-500.0f - 1600.f, 1.0f, 40.0f);
		scene->Lights[4].Color = glm::vec3(1.f, 0.9867f, 0.8463f);
		scene->Lights[4].Range = 200.0f;

		scene->Lights[5].Position = glm::vec3(-550.0f - 1600.f, 1.0f, 40.0f);
		scene->Lights[5].Color = glm::vec3(1.f, 0.9867f, 0.8463f);
		scene->Lights[5].Range = 200.0f;

		scene->Lights[6].Position = glm::vec3(-600.0f - 1600.f, 1.0f, 40.0f);
		scene->Lights[6].Color = glm::vec3(1.f, 0.9867f, 0.8463f);
		scene->Lights[6].Range = 200.0f;

		scene->Lights[7].Position = glm::vec3(-650.0f - 1600.f, 1.0f, 40.0f);
		scene->Lights[7].Color = glm::vec3(1.f, 0.9867f, 0.8463f);
		scene->Lights[7].Range = 200.0f;

		scene->Lights[8].Position = glm::vec3(-700.0f - 1600.f, 1.0f, 40.0f);
		scene->Lights[8].Color = glm::vec3(1.f, 0.9867f, 0.8463f);
		scene->Lights[8].Range = 200.0f;

		scene->Lights[9].Position = glm::vec3(-750.0f - 1600.f, 1.0f, 40.0f);
		scene->Lights[9].Color = glm::vec3(1.f, 0.9867f, 0.8463f);
		scene->Lights[9].Range = 200.0f;

		scene->Lights[10].Position = glm::vec3(-800.0f - 1600.f, 1.0f, 40.0f);
		scene->Lights[10].Color = glm::vec3(1.f, 0.9867f, 0.8463f);
		scene->Lights[10].Range = 200.0f;

		scene->Lights[11].Position = glm::vec3(-400.0f - 1600.f, 1.0f, 40.0f);
		scene->Lights[11].Color = glm::vec3(0.8268f, 0.6162f, 0.2301f);
		scene->Lights[11].Range = 1000.0f;

		scene->Lights[12].Position = glm::vec3(-450.f - 1600.f, -90.0f, 150.0f);
		scene->Lights[12].Color = glm::vec3(0.8268f, 0.6162f, 0.2301f);
		scene->Lights[12].Range = 1000.0f;

		scene->Lights[13].Position = glm::vec3(-500.f - 1600.f, -90.0f, 150.0f);
		scene->Lights[13].Color = glm::vec3(0.8268f, 0.6162f, 0.2301f);
		scene->Lights[13].Range = 1000.0f;

		scene->Lights[14].Position = glm::vec3(-550.0f - 1600.f, -90.0f, 150.0f);
		scene->Lights[14].Color = glm::vec3(0.8268f, 0.6162f, 0.2301f);
		scene->Lights[14].Range = 1000.0f;

		scene->Lights[15].Position = glm::vec3(-500.0f - 1600.f, -90.0f, 150.0f);
		scene->Lights[15].Color = glm::vec3(0.8268f, 0.6162f, 0.2301f);
		scene->Lights[15].Range = 1000.0f;

		scene->Lights[16].Position = glm::vec3(-550.0f - 1600.f, -90.0f, 150.0f);
		scene->Lights[16].Color = glm::vec3(0.8268f, 0.6162f, 0.2301f);
		scene->Lights[16].Range = 1000.0f;

		scene->Lights[17].Position = glm::vec3(-600.0f - 1600.f, -90.0f, 150.0f);
		scene->Lights[17].Color = glm::vec3(0.8268f, 0.6162f, 0.2301f);
		scene->Lights[17].Range = 1000.0f;

		scene->Lights[18].Position = glm::vec3(-650.0f - 1600.f, -90.0f, 150.0f);
		scene->Lights[18].Color = glm::vec3(0.8268f, 0.6162f, 0.2301f);
		scene->Lights[18].Range = 1000.0f;

		scene->Lights[19].Position = glm::vec3(-700.0f - 1600.f, -90.0f, 150.0f);
		scene->Lights[19].Color = glm::vec3(0.8268f, 0.6162f, 0.2301f);
		scene->Lights[19].Range = 1000.0f;

		scene->Lights[20].Position = glm::vec3(-750.0f - 1600.f, -90.0f, 150.0f);
		scene->Lights[20].Color = glm::vec3(0.8268f, 0.6162f, 0.2301f);
		scene->Lights[20].Range = 1000.0f;

		scene->Lights[21].Position = glm::vec3(-800.0f - 1600.f, -90.0f, 150.0f);
		scene->Lights[21].Color = glm::vec3(0.8268f, 0.6162f, 0.2301f);
		scene->Lights[21].Range = 1000.0f;

		scene->Lights[22].Position = glm::vec3(-400.0f - 1600.f, 20.0f, 5.0f);
		scene->Lights[22].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[22].Range = 150.0f;

		scene->Lights[23].Position = glm::vec3(-450.0f - 1600.f, 20.0f, 5.0f);
		scene->Lights[23].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[23].Range = 150.0f;

		scene->Lights[24].Position = glm::vec3(-500.0f - 1600.f, 20.0f, 5.0f);
		scene->Lights[24].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[24].Range = 150.0f;

		scene->Lights[25].Position = glm::vec3(-550.0f - 1600.f, 20.0f, 5.0f);
		scene->Lights[25].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[25].Range = 150.0f;

		scene->Lights[26].Position = glm::vec3(-600.0f - 1600.f, 20.0f, 5.0f);
		scene->Lights[26].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[26].Range = 150.0f;

		scene->Lights[27].Position = glm::vec3(-650.0f - 1600.f, 20.0f, 5.0f);
		scene->Lights[27].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[27].Range = 150.0f;

		scene->Lights[28].Position = glm::vec3(-700.0f - 1600.f, 20.0f, 5.0f);
		scene->Lights[28].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[28].Range = 150.0f;

		scene->Lights[29].Position = glm::vec3(-750.0f - 1600.f, 20.0f, 5.0f);
		scene->Lights[29].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[29].Range = 150.0f;

		scene->Lights[30].Position = glm::vec3(-800.0f - 1600.f, 20.0f, 5.0f);
		scene->Lights[30].Color = glm::vec3(1.f, 1.f, 1.f);
		scene->Lights[30].Range = 150.0f;

		// We'll create a mesh that is a simple plane that we can resize later
		planeMesh = ResourceManager::CreateAsset<MeshResource>();
		cubeMesh = ResourceManager::CreateAsset<MeshResource>("cube.obj");

		mushroomMesh = ResourceManager::CreateAsset<MeshResource>("Mushroom.obj");
		tmMesh = ResourceManager::CreateAsset<MeshResource>("tm.obj");
		bmMesh = ResourceManager::CreateAsset<MeshResource>("bm.obj");
		ToadMesh = ResourceManager::CreateAsset<MeshResource>("ToadStool.obj");


		cobwebMesh = ResourceManager::CreateAsset<MeshResource>("Cobweb.obj");
		BranchMesh = ResourceManager::CreateAsset<MeshResource>("Branch.obj");
		LogMesh = ResourceManager::CreateAsset<MeshResource>("Log.obj");
		Plant2Mesh = ResourceManager::CreateAsset<MeshResource>("Plant2.obj");
		Plant3Mesh = ResourceManager::CreateAsset<MeshResource>("Plant3.obj");



		ladybugMesh = ResourceManager::CreateAsset<MeshResource>("lbo.obj");
		frogMesh = ResourceManager::CreateAsset<MeshResource>("frog.obj");

		BGMineMesh = ResourceManager::CreateAsset<MeshResource>("MineBackground.obj");
		BGRockMesh = ResourceManager::CreateAsset<MeshResource>("RockBackground.obj");
		BGMesh = ResourceManager::CreateAsset<MeshResource>("Background.obj");
		ExitTreeMesh = ResourceManager::CreateAsset<MeshResource>("ExitTree.obj");
		ExitRockMesh = ResourceManager::CreateAsset<MeshResource>("ExitRock.obj");

		CaveEntranceMesh = ResourceManager::CreateAsset<MeshResource>("CaveEntrance.obj");
		Crystal1Mesh = ResourceManager::CreateAsset<MeshResource>("Crystals1.obj");
		Crystal2Mesh = ResourceManager::CreateAsset<MeshResource>("Crystals2.obj");
		StalagmiteMesh = ResourceManager::CreateAsset<MeshResource>("Stalagmite.obj");
		StalagtiteMesh = ResourceManager::CreateAsset<MeshResource>("Stalagite.obj");
		GoldbarMesh = ResourceManager::CreateAsset<MeshResource>("Goldbar.obj");
		GoldPile1Mesh = ResourceManager::CreateAsset<MeshResource>("GoldPile1.obj");
		GoldPile2Mesh = ResourceManager::CreateAsset<MeshResource>("GoldPile2.obj");


		planeMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(1.0f)));
		planeMesh->GenerateMesh();



		//Background Assets
		//createBackgroundAsset("16", glm::vec3(-387.060f - 400.f, 1.530f, 0.550), 0.05, glm::vec3(83.f, -7.0f, 90.0f), frogMesh, frogMaterial);

		//Obstacles

		//createGroundObstacle("18", glm::vec3(-320.f, 0.0f, -0.660), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(90.f, 0.0f, 0.0f), mushroomMesh, mushroomMaterial); //mushroom 8 (small jump)
		//createGroundObstacle("20", glm::vec3(-340.f, 0.0f, 3.0), glm::vec3(1.f, 1.f, 1.f), glm::vec3(90.f, 0.0f, 73.f), vinesMesh, vinesMaterial); // vine 6 (jump blocking)
		//createGroundObstacle("24", glm::vec3(-380.f, 5.530f, 0.250f), glm::vec3(1.5f, 1.5f, 1.5f), glm::vec3(90.f, 0.0f, -25.f), vinesMesh, vinesMaterial); // vine 8 (squish blocking)
		//createGroundObstacle("26", glm::vec3(-395.f, 0.0f, 3.3f), glm::vec3(0.25f, 0.25f, 0.25f), glm::vec3(0.0f, 0.0f, -75.f), cobwebMesh, cobwebMaterial); //cobweb 9 (tall jump)


		createGroundObstacle("1", glm::vec3(-2050.f, 0.0f, 0.215f), glm::vec3(6.f, 4.f, 8.f), glm::vec3(90.f, 0.0f, 80.0f), Crystal1Mesh, Crystal1BlueMaterial); // (beeg jump)
		createGroundObstacle("2", glm::vec3(-2065.f, 0.0f, -0.f), glm::vec3(4.f, 1.5f, 6.f), glm::vec3(90.f, 0.0f, -162.0f), Crystal2Mesh, Crystal2BlueMaterial); //(small jump)
		createGroundObstacle("3", glm::vec3(-2080.f, 0.0f, -0.f), glm::vec3(4.f, 1.5f, 6.f), glm::vec3(90.f, 0.0f, -162.0f), Crystal2Mesh, Crystal2BlueMaterial); //(small jump)
		createGroundObstacle("4", glm::vec3(-2100.f, 0.0f, 0.215f), glm::vec3(6.f, 4.f, 8.f), glm::vec3(90.f, 0.0f, 80.0f), Crystal1Mesh, Crystal1BlueMaterial); // (beeg jump)
		createGroundObstacle("5", glm::vec3(-2115.f, 0.0f, -0.f), glm::vec3(4.f, 1.5f, 6.f), glm::vec3(90.f, 0.0f, -162.0f), Crystal2Mesh, Crystal2BlueMaterial); //(small jump)
		createGroundObstacle("6", glm::vec3(-2130.f, 0.0f, -0.f), glm::vec3(4.f, 1.5f, 6.f), glm::vec3(90.f, 0.0f, -162.0f), Crystal2Mesh, Crystal2BlueMaterial); //(small jump)
		createGroundObstacle("7", glm::vec3(-2150.f, 0.0f, 0.215f), glm::vec3(6.f, 4.f, 8.f), glm::vec3(90.f, 0.0f, 80.0f), Crystal1Mesh, Crystal1BlueMaterial); // (beeg jump)
		createGroundObstacle("8", glm::vec3(-2160.f, 0.0f, 0.215f), glm::vec3(6.f, 4.f, 8.f), glm::vec3(90.f, 0.0f, 80.0f), Crystal1Mesh, Crystal1BlueMaterial); // (beeg jump)
		createGroundObstacle("9", glm::vec3(-2180.f, 0.0f, -0.f), glm::vec3(4.f, 1.5f, 6.f), glm::vec3(90.f, 0.0f, -162.0f), Crystal2Mesh, Crystal2BlueMaterial); //(small jump)
		createGroundObstacle("10", glm::vec3(-2200.f, 0.0f, 0.215f), glm::vec3(6.f, 4.f, 8.f), glm::vec3(90.f, 0.0f, 80.0f), Crystal1Mesh, Crystal1BlueMaterial); // (beeg jump)
		createGroundObstacle("11", glm::vec3(-2210.f, 0.0f, 0.215f), glm::vec3(6.f, 4.f, 8.f), glm::vec3(90.f, 0.0f, 80.0f), Crystal1Mesh, Crystal1BlueMaterial); // (beeg jump)
		createGroundObstacle("12", glm::vec3(-2240.f, 0.0f, 0.215f), glm::vec3(6.f, 4.f, 8.f), glm::vec3(90.f, 0.0f, 80.0f), Crystal1Mesh, Crystal1BlueMaterial); // (beeg jump)
		createGroundObstacle("13", glm::vec3(-2245.f, 0.0f, -0.f), glm::vec3(4.f, 1.5f, 6.f), glm::vec3(90.f, 0.0f, -162.0f), Crystal2Mesh, Crystal2BlueMaterial); //(small jump)
		createGroundObstacle("14", glm::vec3(-2260.f, 0.0f, -0.f), glm::vec3(4.f, 1.5f, 6.f), glm::vec3(90.f, 0.0f, -162.0f), Crystal2Mesh, Crystal2BlueMaterial); //(small jump)
		createGroundObstacle("15", glm::vec3(-2270.f, 0.0f, -0.f), glm::vec3(4.f, 1.5f, 6.f), glm::vec3(90.f, 0.0f, -162.0f), Crystal2Mesh, Crystal2BlueMaterial); //(small jump)
		createGroundObstacle("16", glm::vec3(-2280.f, 0.0f, -0.f), glm::vec3(4.f, 1.5f, 6.f), glm::vec3(90.f, 0.0f, -162.0f), Crystal2Mesh, Crystal2BlueMaterial); //(small jump)
		createGroundObstacle("17", glm::vec3(-2300.f, 0.0f, 0.215f), glm::vec3(6.f, 4.f, 8.f), glm::vec3(90.f, 0.0f, 80.0f), Crystal1Mesh, Crystal1BlueMaterial); // (beeg jump)
		createGroundObstacle("18", glm::vec3(-2310.f, 0.0f, 0.215f), glm::vec3(6.f, 4.f, 8.f), glm::vec3(90.f, 0.0f, 80.0f), Crystal1Mesh, Crystal1BlueMaterial); // (beeg jump)
		createGroundObstacle("19", glm::vec3(-2345.f, 0.0f, -0.f), glm::vec3(4.f, 1.5f, 6.f), glm::vec3(90.f, 0.0f, -162.0f), Crystal2Mesh, Crystal2BlueMaterial); //(small jump)
		createGroundObstacle("20", glm::vec3(-2360.f, 0.0f, -0.f), glm::vec3(4.f, 1.5f, 6.f), glm::vec3(90.f, 0.0f, -162.0f), Crystal2Mesh, Crystal2BlueMaterial); //(small jump)
		createGroundObstacle("21", glm::vec3(-2370.f, 0.0f, -0.f), glm::vec3(4.f, 1.5f, 6.f), glm::vec3(90.f, 0.0f, -162.0f), Crystal2Mesh, Crystal2BlueMaterial); //(small jump)
		createGroundObstacle("22", glm::vec3(-2380.f, 0.0f, -0.f), glm::vec3(4.f, 1.5f, 6.f), glm::vec3(90.f, 0.0f, -162.0f), Crystal2Mesh, Crystal2BlueMaterial); //(small jump)



		//3D Backgrounds
		createGroundObstacle("27", glm::vec3(-292.3f - 1600.f, -53.250f, -4.5f), glm::vec3(6.f, 12.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGMineMesh, MineBackgroundMaterial);
		createGroundObstacle("28", glm::vec3(-400.f - 1600.f, -53.250f, -4.5f), glm::vec3(6.f, 12.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGMineMesh, MineBackgroundMaterial);
		createGroundObstacle("29", glm::vec3(-507.7f - 1600.f, -53.250f, -4.5f), glm::vec3(6.f, 12.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGMineMesh, MineBackgroundMaterial);

		createGroundObstacle("30", glm::vec3(-615.4f - 1600.f, -53.250f, -4.5f), glm::vec3(6.f, 12.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGMineMesh, MineBackgroundMaterial);
		createGroundObstacle("31", glm::vec3(-723.1f - 1600.f, -53.250f, -4.5f), glm::vec3(6.f, 12.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGMineMesh, MineBackgroundMaterial);
		createGroundObstacle("32", glm::vec3(-830.8f - 1600.f, -53.250f, -4.5f), glm::vec3(6.f, 12.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGMineMesh, MineBackgroundMaterial);
		createGroundObstacle("26", glm::vec3(-938.2f - 1600.f, -53.250f, -4.5f), glm::vec3(6.f, 12.f, 6.f), glm::vec3(90.0f, 0.0f, -180.f), BGMineMesh, MineBackgroundMaterial);

		//2DBackGrounds
		createGroundObstacle("33", glm::vec3(-75.f - 1600.f, -130.0f, 64.130f), glm::vec3(375.0f, 125.0f, 250.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineBackdropMaterial);
		createGroundObstacle("34", glm::vec3(-450.f - 1600.f, -130.0f, 64.130f), glm::vec3(375.0f, 125.0f, 250.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineBackdropMaterial);
		createGroundObstacle("35", glm::vec3(-825.f - 1600.f, -130.0f, 64.130f), glm::vec3(375.0f, 125.0f, 250.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineBackdropMaterial);
		createGroundObstacle("36", glm::vec3(-2400.f, -130.0f, 64.130f), glm::vec3(375.0f, 125.0f, 250.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineBackdropMaterial);
		createGroundObstacle("37", glm::vec3(-2550.f - 400.f, -130.0f, 64.130f), glm::vec3(375.0f, 125.0f, 250.f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineBackdropMaterial);

		//Exit Rock
		createGroundObstacle("58", glm::vec3(-409.f - 800.f - 1200.f, -1.f, 0.f), glm::vec3(2.f, 2.f, 1.f), glm::vec3(90.0f, 0.0f, -68.f), CaveEntranceMesh, CaveEntranceMaterial);

		//Foreground 
		createGroundObstacle("59", glm::vec3(40.f - 800.f - 1200.f, -5.f, 8.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
		createGroundObstacle("60", glm::vec3(0.f - 800.f - 1200.f, -5.f, 8.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
		createGroundObstacle("61", glm::vec3(-40.f - 800.f - 1200.f, -5.f, 8.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
		createGroundObstacle("62", glm::vec3(-80.f - 800.f - 1200.f, -5.f, 8.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
		createGroundObstacle("63", glm::vec3(-120.f - 800.f - 1200.f, -5.f, 8.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
		createGroundObstacle("64", glm::vec3(-160.f - 800.f - 1200.f, -5.f, 8.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);

		createGroundObstacle("65", glm::vec3(-200.f - 800.f - 1200.f, -5.f, 8.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
		createGroundObstacle("66", glm::vec3(-240.f - 800.f - 1200.f, -5.f, 8.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
		createGroundObstacle("67", glm::vec3(-280.f - 800.f - 1200.f, -5.f, 8.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
		createGroundObstacle("68", glm::vec3(-320.f - 800.f - 1200.f, -5.f, 8.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
		createGroundObstacle("69", glm::vec3(-360.f - 800.f - 1200.f, -5.f, 8.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
		createGroundObstacle("70", glm::vec3(-400.f - 800.f - 1200.f, -5.f, 8.f), glm::vec3(42.0f, 20.f, 5.0f), glm::vec3(90.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);


		//large rock 
		createCollision("611", -2046.f, 1.560f, 1.f, 1.f);
		createCollision("612", -2046.f, 1.f, 1.f, 1.f);
		createCollision("613", -2048.f, 2.82f, 1.f, 1.f);
		createCollision("614", -2049.44f, 4.f, 1.f, 1.f);
		createCollision("615", -2052.f, 3.8f, 1.f, 1.f);
		createCollision("616", -2054.f, 1.56f, 1.f, 1.f);

		//small rock 
		createCollision("621", -2063.f, 0.8f, 1.f, 1.f);
		createCollision("622", -2064.f, 1.f, 1.f, 1.f);
		createCollision("623", -2065.f, 1.f, 1.f, 1.f);
		createCollision("624", -2067.f, 0.7f, 1.f, 1.f);

		//small rock 1930
		createCollision("631", -2078.f, 0.8f, 1.f, 1.f);
		createCollision("632", -2079.f, 1.f, 1.f, 1.f);
		createCollision("633", -2080.f, 1.f, 1.f, 1.f);
		createCollision("634", -2082.f, 0.7f, 1.f, 1.f);

		//large rock 
		createCollision("641", -2096.f, 1.560f, 1.f, 1.f);
		createCollision("642", -2096.f, 1.f, 1.f, 1.f);
		createCollision("643", -2098.f, 2.82f, 1.f, 1.f);
		createCollision("644", -2099.44f, 4.f, 1.f, 1.f);
		createCollision("645", -2102.f, 3.8f, 1.f, 1.f);
		createCollision("646", -2104.f, 1.56f, 1.f, 1.f);

		//small rock 
		createCollision("651", -2113.f, 0.8f, 1.f, 1.f);
		createCollision("652", -2114.f, 1.f, 1.f, 1.f);
		createCollision("653", -2115.f, 1.f, 1.f, 1.f);
		createCollision("654", -2117.f, 0.7f, 1.f, 1.f);

		//small rock 1930
		createCollision("661", -2128.f, 0.8f, 1.f, 1.f);
		createCollision("662", -2129.f, 1.f, 1.f, 1.f);
		createCollision("663", -2130.f, 1.f, 1.f, 1.f);
		createCollision("664", -2132.f, 0.7f, 1.f, 1.f);

		//large rock 
		createCollision("671", -2146.f, 1.560f, 1.f, 1.f);
		createCollision("672", -2146.f, 1.f, 1.f, 1.f);
		createCollision("673", -2148.f, 2.82f, 1.f, 1.f);
		createCollision("674", -2149.44f, 4.f, 1.f, 1.f);
		createCollision("675", -2152.f, 3.8f, 1.f, 1.f);
		createCollision("676", -2154.f, 1.56f, 1.f, 1.f);

		//large rock 
		createCollision("681", -2156.f, 1.560f, 1.f, 1.f);
		createCollision("682", -2156.f, 1.f, 1.f, 1.f);
		createCollision("683", -2158.f, 2.82f, 1.f, 1.f);
		createCollision("684", -2159.44f, 4.f, 1.f, 1.f);
		createCollision("685", -2162.f, 3.8f, 1.f, 1.f);
		createCollision("686", -2164.f, 1.56f, 1.f, 1.f);

		//small rock 1930
		createCollision("691", -2178.f, 0.8f, 1.f, 1.f);
		createCollision("692", -2179.f, 1.f, 1.f, 1.f);
		createCollision("693", -2180.f, 1.f, 1.f, 1.f);
		createCollision("694", -2182.f, 0.7f, 1.f, 1.f);

		//large rock 
		createCollision("6101", -2196.f, 1.560f, 1.f, 1.f);
		createCollision("6102", -2196.f, 1.f, 1.f, 1.f);
		createCollision("6103", -2198.f, 2.82f, 1.f, 1.f);
		createCollision("6104", -2199.44f, 4.f, 1.f, 1.f);
		createCollision("6105", -2202.f, 3.8f, 1.f, 1.f);
		createCollision("6106", -2204.f, 1.56f, 1.f, 1.f);

		//large rock 
		createCollision("6111", -2206.f, 1.560f, 1.f, 1.f);
		createCollision("6112", -2206.f, 1.f, 1.f, 1.f);
		createCollision("6113", -2208.f, 2.82f, 1.f, 1.f);
		createCollision("6114", -2209.44f, 4.f, 1.f, 1.f);
		createCollision("6115", -2212.f, 3.8f, 1.f, 1.f);
		createCollision("6116", -2214.f, 1.56f, 1.f, 1.f);

		//large rock 
		createCollision("6121", -2236.f, 1.560f, 1.f, 1.f);
		createCollision("6122", -2236.f, 1.f, 1.f, 1.f);
		createCollision("6123", -2238.f, 2.82f, 1.f, 1.f);
		createCollision("6124", -2239.44f, 4.f, 1.f, 1.f);
		createCollision("6125", -2242.f, 3.8f, 1.f, 1.f);
		createCollision("6126", -2244.f, 1.56f, 1.f, 1.f);

		//small rock 1930
		createCollision("6131", -2243.f, 0.8f, 1.f, 1.f);
		createCollision("6132", -2243.f, 1.f, 1.f, 1.f);
		createCollision("6133", -2245.f, 1.f, 1.f, 1.f);
		createCollision("6134", -2247.f, 0.7f, 1.f, 1.f);

		//small rock 1930
		createCollision("6141", -2258.f, 0.8f, 1.f, 1.f);
		createCollision("6142", -2259.f, 1.f, 1.f, 1.f);
		createCollision("6143", -2260.f, 1.f, 1.f, 1.f);
		createCollision("6144", -2262.f, 0.7f, 1.f, 1.f);

		//small rock 1930
		createCollision("6151", -2268.f, 0.8f, 1.f, 1.f);
		createCollision("6152", -2269.f, 1.f, 1.f, 1.f);
		createCollision("6153", -2270.f, 1.f, 1.f, 1.f);
		createCollision("6154", -2272.f, 0.7f, 1.f, 1.f);

		//small rock 1930
		createCollision("6161", -2278.f, 0.8f, 1.f, 1.f);
		createCollision("6162", -2279.f, 1.f, 1.f, 1.f);
		createCollision("6163", -2280.f, 1.f, 1.f, 1.f);
		createCollision("6164", -2282.f, 0.7f, 1.f, 1.f);

		//large rock 
		createCollision("6171", -2296.f, 1.560f, 1.f, 1.f);
		createCollision("6172", -2296.f, 1.f, 1.f, 1.f);
		createCollision("6173", -2298.f, 2.82f, 1.f, 1.f);
		createCollision("6174", -2299.44f, 4.f, 1.f, 1.f);
		createCollision("6175", -2302.f, 3.8f, 1.f, 1.f);
		createCollision("6176", -2304.f, 1.56f, 1.f, 1.f);

		//large rock 
		createCollision("6181", -2306.f, 1.560f, 1.f, 1.f);
		createCollision("6182", -2306.f, 1.f, 1.f, 1.f);
		createCollision("6183", -2308.f, 2.82f, 1.f, 1.f);
		createCollision("6184", -2309.44f, 4.f, 1.f, 1.f);
		createCollision("6185", -2312.f, 3.8f, 1.f, 1.f);
		createCollision("6186", -2314.f, 1.56f, 1.f, 1.f);

		//small rock 
		createCollision("6191", -2343.f, 0.8f, 1.f, 1.f);
		createCollision("6192", -2344.f, 1.f, 1.f, 1.f);
		createCollision("6193", -2345.f, 1.f, 1.f, 1.f);
		createCollision("6194", -2347.f, 0.7f, 1.f, 1.f);

		//small rock 
		createCollision("6201", -2358.f, 0.8f, 1.f, 1.f);
		createCollision("6202", -2359.f, 1.f, 1.f, 1.f);
		createCollision("6203", -2360.f, 1.f, 1.f, 1.f);
		createCollision("6204", -2362.f, 0.7f, 1.f, 1.f);

		//small rock 
		createCollision("6211", -2368.f, 0.8f, 1.f, 1.f);
		createCollision("6212", -2369.f, 1.f, 1.f, 1.f);
		createCollision("6213", -2370.f, 1.f, 1.f, 1.f);
		createCollision("6214", -2372.f, 0.7f, 1.f, 1.f);

		//small rock 
		createCollision("6221", -2378.f, 0.8f, 1.f, 1.f);
		createCollision("6222", -2379.f, 1.f, 1.f, 1.f);
		createCollision("6223", -2380.f, 1.f, 1.f, 1.f);
		createCollision("6224", -2382.f, 0.7f, 1.f, 1.f);

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
			player->SetPostion(glm::vec3(-2006.f, 0.0f, 1.0f));
			player->SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
			player->SetScale(glm::vec3(0.5f, 0.5f, 0.5f));

			// Add some behaviour that relies on the physics body
			//player->Add<JumpBehaviour>(player->GetPosition());
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

		createGroundObstacle("79", glm::vec3(40.f - 400.f - 1600.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.22f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
		createGroundObstacle("80", glm::vec3(0.f - 400.f - 1600.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
		createGroundObstacle("81", glm::vec3(-40.f - 400.f - 1600.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
		createGroundObstacle("82", glm::vec3(-80.f - 400.f - 1600.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
		createGroundObstacle("83", glm::vec3(-120.f - 400.f - 1600.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
		createGroundObstacle("84", glm::vec3(-160.f - 400.f - 1600.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);

		createGroundObstacle("85", glm::vec3(-200.f - 400.f - 1600.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
		createGroundObstacle("86", glm::vec3(-240.f - 400.f - 1600.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
		createGroundObstacle("87", glm::vec3(-280.f - 400.f - 1600.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
		createGroundObstacle("88", glm::vec3(-320.f - 400.f - 1600.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
		createGroundObstacle("89", glm::vec3(-360.f - 400.f - 1600.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);
		createGroundObstacle("90", glm::vec3(-400.f - 400.f - 1600.f, 5.76f, 3.390f), glm::vec3(40.0f, 9.220f, 5.0f), glm::vec3(77.0f, 0.0f, -180.f), planeMesh, MineForegroundMaterial);


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

		GameObject::Sptr ButtonBack3 = scene->CreateGameObject("ButtonBack3");
		{
			// Set position in the scene
			ButtonBack3->SetPostion(glm::vec3(1.f, 6.5f, 5.f));
			// Scale down the plane
			ButtonBack3->SetScale(glm::vec3(3.0f, 0.8f, 0.5f));
			//spin things
			ButtonBack3->SetRotation(glm::vec3(-80.0f, 0.f, 0.f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = ButtonBack3->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(ButtonMaterial);
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

		GameObject::Sptr LSText = scene->CreateGameObject("LSText");
		{
			// Set position in the scene
			LSText->SetPostion(glm::vec3(1.0f, 8.0f, 6.1f));
			// Scale down the plane
			LSText->SetScale(glm::vec3(2.0f, 0.4f, 0.5f));
			LSText->SetRotation(glm::vec3(80.0f, 0.f, -180.f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = LSText->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(LStextMaterial);
		}


		GameObject::Sptr FrogTongue = scene->CreateGameObject("FrogTongue");
		{
			// Set position in the scene
			FrogTongue->SetPostion(glm::vec3(-3.4f, -1.05f, 3.59f));
			// Scale down the plane
			FrogTongue->SetScale(glm::vec3(1.0f, 0.1f, 1.0f));
			FrogTongue->SetRotation(glm::vec3(0.0f, 0.0f, 45.0f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = FrogTongue->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(FrogTongueMaterial);
		}

		GameObject::Sptr FrogBody = scene->CreateGameObject("FrogBody");
		{
			// Set position in the scene
			FrogBody->SetPostion(glm::vec3(-3.4f, -1.05f, 3.6f));
			// Scale down the plane
			FrogBody->SetScale(glm::vec3(1.5f, 1.5f, 1.0f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = FrogBody->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(FrogBodyMaterial);
		}

		GameObject::Sptr FrogHeadBot = scene->CreateGameObject("FrogHeadBot");
		{
			// Set position in the scene
			FrogHeadBot->SetPostion(glm::vec3(-3.4f, -1.05f, 3.61f));
			// Scale down the plane
			FrogHeadBot->SetScale(glm::vec3(1.5f, 1.5f, 1.0f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = FrogHeadBot->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(FrogHeadBotMaterial);
		}

		GameObject::Sptr FrogHeadTop = scene->CreateGameObject("FrogHeadTop");
		{
			// Set position in the scene
			FrogHeadTop->SetPostion(glm::vec3(-3.4f, -1.05f, 3.62f));
			// Scale down the plane
			FrogHeadTop->SetScale(glm::vec3(1.5f, 1.5f, 1.0f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = FrogHeadTop->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(FrogHeadTopMaterial);
		}

		GameObject::Sptr BushTransition = scene->CreateGameObject("BushTransition");
		{
			// Set position in the scene
			BushTransition->SetPostion(glm::vec3(5.f, 0.0f, 3.8f));
			// Scale down the plane
			BushTransition->SetScale(glm::vec3(5.8f, 2.5f, 1.0f));

			// Create and attach a render component
			RenderComponent::Sptr renderer = BushTransition->Add<RenderComponent>();
			renderer->SetMesh(planeMesh);
			renderer->SetMaterial(BushTransitionMaterial);
		}

		scene->SetAmbientLight(glm::vec3(0.2f));

		// Kinematic rigid bodies are those controlled by some outside controller
		// and ONLY collide with dynamic objects
		RigidBody::Sptr physics = jumpingObstacle->Add<RigidBody>(RigidBodyType::Kinematic);
		physics->AddCollider(ConvexMeshCollider::Create());

		// Save the asset manifest for all the resources we just loaded
		ResourceManager::SaveManifest("manifest.json");
		// Save the scene to a JSON file
		scene->Save("Level6.json");

		}


		//// Making a 'Menu' Scene ////

		{

			// Create an empty scene
			scene = std::make_shared<Scene>();

			// I hate this
			scene->BaseShader = uboShader;

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

			Material::Sptr ControlsMaterial = ResourceManager::CreateAsset<Material>();
			{
				ControlsMaterial->Name = "Controls Text";
				ControlsMaterial->MatShader = scene->BaseShader;
				ControlsMaterial->Texture = ControlsTex;
				ControlsMaterial->Shininess = 2.0f;
			}

			Material::Sptr FilterMaterial = ResourceManager::CreateAsset<Material>();
			{
				FilterMaterial->Name = "Button Filter";
				FilterMaterial->MatShader = scene->BaseShader;
				FilterMaterial->Texture = FilterTex;
				FilterMaterial->Shininess = 2.0f;
			}

			Material::Sptr FrogBodyMaterial = ResourceManager::CreateAsset<Material>();
			{
				FrogBodyMaterial->Name = "FrogBody";
				FrogBodyMaterial->MatShader = scene->BaseShader;
				FrogBodyMaterial->Texture = FrogBodyTex;
				FrogBodyMaterial->Shininess = 2.0f;
			}

			Material::Sptr FrogHeadTopMaterial = ResourceManager::CreateAsset<Material>();
			{
				FrogHeadTopMaterial->Name = "FrogHeadTop";
				FrogHeadTopMaterial->MatShader = scene->BaseShader;
				FrogHeadTopMaterial->Texture = FrogHeadTopTex;
				FrogHeadTopMaterial->Shininess = 2.0f;
			}

			Material::Sptr FrogHeadBotMaterial = ResourceManager::CreateAsset<Material>();
			{
				FrogHeadBotMaterial->Name = "FrogHeadBot";
				FrogHeadBotMaterial->MatShader = scene->BaseShader;
				FrogHeadBotMaterial->Texture = FrogHeadBotTex;
				FrogHeadBotMaterial->Shininess = 2.0f;
			}

			Material::Sptr FrogTongueMaterial = ResourceManager::CreateAsset<Material>();
			{
				FrogTongueMaterial->Name = "FrogTongue";
				FrogTongueMaterial->MatShader = scene->BaseShader;
				FrogTongueMaterial->Texture = FrogTongueTex;
				FrogTongueMaterial->Shininess = 2.0f;
			}

			Material::Sptr BushTransitionMaterial = ResourceManager::CreateAsset<Material>();
			{
				BushTransitionMaterial->Name = "BushTransition";
				BushTransitionMaterial->MatShader = scene->BaseShader;
				BushTransitionMaterial->Texture = BushTransitionTex;
				BushTransitionMaterial->Shininess = 2.0f;
			}

			// Create some lights for our scene
			scene->Lights.resize(8);
			scene->Lights[0].Position = glm::vec3(3.0f, 3.0f, 3.0f);
			scene->Lights[0].Color = glm::vec3(0.892f, 1.0f, 0.882f);
			scene->Lights[0].Range = 5.0f;

			scene->Lights[1].Position = glm::vec3(3.0f, -3.0f, 3.0f);
			scene->Lights[1].Color = glm::vec3(0.892f, 1.0f, 0.882f);
			scene->Lights[1].Range = 5.0f;

			scene->Lights[2].Position = glm::vec3(-3.0f, 3.0f, 5.0f);
			scene->Lights[2].Color = glm::vec3(0.892f, 1.0f, 0.882f);
			scene->Lights[2].Range = 5.0f;

			scene->Lights[3].Position = glm::vec3(-3.0f, -3.0f, 5.0f);
			scene->Lights[3].Color = glm::vec3(0.892f, 1.0f, 0.882f);
			scene->Lights[3].Range = 5.0f;

			scene->Lights[4].Position = glm::vec3(-8.0f, -3.0f, 5.0f);
			scene->Lights[4].Color = glm::vec3(0.892f, 1.0f, 0.882f);
			scene->Lights[4].Range = 10.0f;

			scene->Lights[5].Position = glm::vec3(-8.0f, 3.0f, 5.0f);
			scene->Lights[5].Color = glm::vec3(0.892f, 1.0f, 0.882f);
			scene->Lights[5].Range = 10.0f;

			scene->Lights[6].Position = glm::vec3(8.0f, -3.0f, 5.0f);
			scene->Lights[6].Color = glm::vec3(0.892f, 1.0f, 0.882f);
			scene->Lights[6].Range = 10.0f;

			scene->Lights[7].Position = glm::vec3(8.0f, 3.0f, 5.0f);
			scene->Lights[7].Color = glm::vec3(0.892f, 1.0f, 0.882f);
			scene->Lights[7].Range = 10.0f;

			// We'll create a mesh that is a simple plane that we can resize later
			planeMesh = ResourceManager::CreateAsset<MeshResource>();
			cubeMesh = ResourceManager::CreateAsset<MeshResource>("cube.obj");
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
				plane->SetScale(glm::vec3(19.0F));
				plane->SetPostion(glm::vec3(0.0f, -4.0f, 0.0f));

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
				GameLogo->SetPostion(glm::vec3(1.75f, 1.15f, 3.0f));
				// Scale down the plane
				GameLogo->SetScale(glm::vec3(2.0f, 1.45f, 1.0f));

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
				ButtonBackground1->SetPostion(glm::vec3(1.75f, 0.1f, 3.0f));
				// Scale down the plane
				ButtonBackground1->SetScale(glm::vec3(1.75f, 0.30f, 1.0f));

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
				ButtonBackground2->SetPostion(glm::vec3(1.75f, -0.350f, 3.0f));
				// Scale down the plane
				ButtonBackground2->SetScale(glm::vec3(1.75f, 0.30f, 1.0f));

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
				ButtonBackground3->SetPostion(glm::vec3(1.75f, -0.8f, 3.0f));
				// Scale down the plane
				ButtonBackground3->SetScale(glm::vec3(1.75f, 0.30f, 1.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = ButtonBackground3->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(ButtonBackMaterial);

				// This object is a renderable only, it doesn't have any behaviours or
				// physics bodies attached!
			}

			GameObject::Sptr Filter = scene->CreateGameObject("Filter");
			{
				// Set position in the scene
				Filter->SetPostion(glm::vec3(1.0f, 0.5f, 3.010f));
				// Scale down the plane
				Filter->SetScale(glm::vec3(1.75f, 0.3f, 1.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = Filter->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(FilterMaterial);

				// This object is a renderable only, it doesn't have any behaviours or
				// physics bodies attached!
			}

			GameObject::Sptr StartButton = scene->CreateGameObject("StartButton");
			{
				// Set position in the scene
				StartButton->SetPostion(glm::vec3(1.3f, 0.080f, 3.5f));
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
				BackButton->SetPostion(glm::vec3(1.3f, -0.6f, 3.5f));
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
				ControlsButton->SetPostion(glm::vec3(1.3f, -0.26f, 3.5f));
				// Scale down the plane
				ControlsButton->SetScale(glm::vec3(1.0f, 0.250f, 1.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = ControlsButton->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(ControlsMaterial);

				// This object is a renderable only, it doesn't have any behaviours or
				// physics bodies attached!
			}

			GameObject::Sptr FrogTongue = scene->CreateGameObject("FrogTongue");
			{
				// Set position in the scene
				FrogTongue->SetPostion(glm::vec3(-3.4f, -1.05f, 3.59f));
				// Scale down the plane
				FrogTongue->SetScale(glm::vec3(1.0f, 0.1f, 1.0f));
				FrogTongue->SetRotation(glm::vec3(0.0f, 0.0f, 45.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = FrogTongue->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(FrogTongueMaterial);
			}

			GameObject::Sptr FrogBody = scene->CreateGameObject("FrogBody");
			{
				// Set position in the scene
				FrogBody->SetPostion(glm::vec3(-3.4f, -1.05f, 3.6f));
				// Scale down the plane
				FrogBody->SetScale(glm::vec3(1.5f, 1.5f, 1.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = FrogBody->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(FrogBodyMaterial);
			}

			GameObject::Sptr FrogHeadBot = scene->CreateGameObject("FrogHeadBot");
			{
				// Set position in the scene
				FrogHeadBot->SetPostion(glm::vec3(-3.4f, -1.05f, 3.61f));
				// Scale down the plane
				FrogHeadBot->SetScale(glm::vec3(1.5f, 1.5f, 1.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = FrogHeadBot->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(FrogHeadBotMaterial);
			}

			GameObject::Sptr FrogHeadTop = scene->CreateGameObject("FrogHeadTop");
			{
				// Set position in the scene
				FrogHeadTop->SetPostion(glm::vec3(-3.4f, -1.05f, 3.62f));
				// Scale down the plane
				FrogHeadTop->SetScale(glm::vec3(1.5f, 1.5f, 1.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = FrogHeadTop->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(FrogHeadTopMaterial);
			}

			GameObject::Sptr BushTransition = scene->CreateGameObject("BushTransition");
			{
				// Set position in the scene
				BushTransition->SetPostion(glm::vec3(5.f, 0.0f, 3.8f));
				// Scale down the plane
				BushTransition->SetScale(glm::vec3(5.8f, 2.5f, 1.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = BushTransition->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(BushTransitionMaterial);
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

			// Create an empty scene
			scene = std::make_shared<Scene>();

			// I hate this
			scene->BaseShader = uboShader;

			Material::Sptr ControlsMaterial = ResourceManager::CreateAsset<Material>();
			{
				ControlsMaterial->Name = "Controls";
				ControlsMaterial->MatShader = scene->BaseShader;
				ControlsMaterial->Texture = ControlTex;
				ControlsMaterial->Shininess = 2.0f;
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

			Material::Sptr FrogBodyMaterial = ResourceManager::CreateAsset<Material>();
			{
				FrogBodyMaterial->Name = "FrogBody";
				FrogBodyMaterial->MatShader = scene->BaseShader;
				FrogBodyMaterial->Texture = FrogBodyTex;
				FrogBodyMaterial->Shininess = 2.0f;
			}

			Material::Sptr FrogHeadTopMaterial = ResourceManager::CreateAsset<Material>();
			{
				FrogHeadTopMaterial->Name = "FrogHeadTop";
				FrogHeadTopMaterial->MatShader = scene->BaseShader;
				FrogHeadTopMaterial->Texture = FrogHeadTopTex;
				FrogHeadTopMaterial->Shininess = 2.0f;
			}

			Material::Sptr FrogHeadBotMaterial = ResourceManager::CreateAsset<Material>();
			{
				FrogHeadBotMaterial->Name = "FrogHeadBot";
				FrogHeadBotMaterial->MatShader = scene->BaseShader;
				FrogHeadBotMaterial->Texture = FrogHeadBotTex;
				FrogHeadBotMaterial->Shininess = 2.0f;
			}

			Material::Sptr FrogTongueMaterial = ResourceManager::CreateAsset<Material>();
			{
				FrogTongueMaterial->Name = "FrogTongue";
				FrogTongueMaterial->MatShader = scene->BaseShader;
				FrogTongueMaterial->Texture = FrogTongueTex;
				FrogTongueMaterial->Shininess = 2.0f;
			}

			Material::Sptr BushTransitionMaterial = ResourceManager::CreateAsset<Material>();
			{
				BushTransitionMaterial->Name = "BushTransition";
				BushTransitionMaterial->MatShader = scene->BaseShader;
				BushTransitionMaterial->Texture = BushTransitionTex;
				BushTransitionMaterial->Shininess = 2.0f;
			}

			Material::Sptr PanelMaterial = ResourceManager::CreateAsset<Material>();
			{
				PanelMaterial->Name = "Panel";
				PanelMaterial->MatShader = scene->BaseShader;
				PanelMaterial->Texture = PanelTex;
				PanelMaterial->Shininess = 2.0f;
			}

			// Create some lights for our scene
			scene->Lights.resize(8);
			scene->Lights[0].Position = glm::vec3(3.0f, 3.0f, 3.0f);
			scene->Lights[0].Color = glm::vec3(0.892f, 1.0f, 0.882f);
			scene->Lights[0].Range = 5.0f;

			scene->Lights[1].Position = glm::vec3(3.0f, -3.0f, 3.0f);
			scene->Lights[1].Color = glm::vec3(0.892f, 1.0f, 0.882f);
			scene->Lights[1].Range = 5.0f;

			scene->Lights[2].Position = glm::vec3(-3.0f, 3.0f, 5.0f);
			scene->Lights[2].Color = glm::vec3(0.892f, 1.0f, 0.882f);
			scene->Lights[2].Range = 5.0f;

			scene->Lights[3].Position = glm::vec3(-3.0f, -3.0f, 5.0f);
			scene->Lights[3].Color = glm::vec3(0.892f, 1.0f, 0.882f);
			scene->Lights[3].Range = 5.0f;

			scene->Lights[4].Position = glm::vec3(-8.0f, -3.0f, 5.0f);
			scene->Lights[4].Color = glm::vec3(0.892f, 1.0f, 0.882f);
			scene->Lights[4].Range = 10.0f;

			scene->Lights[5].Position = glm::vec3(-8.0f, 3.0f, 5.0f);
			scene->Lights[5].Color = glm::vec3(0.892f, 1.0f, 0.882f);
			scene->Lights[5].Range = 10.0f;

			scene->Lights[6].Position = glm::vec3(8.0f, -3.0f, 5.0f);
			scene->Lights[6].Color = glm::vec3(0.892f, 1.0f, 0.882f);
			scene->Lights[6].Range = 10.0f;

			scene->Lights[7].Position = glm::vec3(8.0f, 3.0f, 5.0f);
			scene->Lights[7].Color = glm::vec3(0.892f, 1.0f, 0.882f);
			scene->Lights[7].Range = 10.0f;

			// We'll create a mesh that is a simple plane that we can resize later
			planeMesh = ResourceManager::CreateAsset<MeshResource>();
			cubeMesh = ResourceManager::CreateAsset<MeshResource>("cube.obj");
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
				plane->SetScale(glm::vec3(19.0F));
				plane->SetPostion(glm::vec3(0.0f, -4.0f, 0.0f));

				// Create and attach a RenderComponent to the object to draw our mesh
				RenderComponent::Sptr renderer = plane->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(MenuMaterial);

				// Attach a plane collider that extends infinitely along the X/Y axis
				RigidBody::Sptr physics = plane->Add<RigidBody>(/*static by default*/);
				physics->AddCollider(PlaneCollider::Create());
			}

			//Objects with transparency need to be loaded in last otherwise it creates issues
			GameObject::Sptr Panel = scene->CreateGameObject("Panel");
			{
				// Set position in the scene
				Panel->SetPostion(glm::vec3(0.0f, 0.0f, 0.001f));
				// Scale down the plane
				Panel->SetScale(glm::vec3(10.0f, 10.0f, 10.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = Panel->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(PanelMaterial);
			}

			GameObject::Sptr plane3 = scene->CreateGameObject("Controls");
			{
				// Scale up the plane
				plane3->SetScale(glm::vec3(10.0F));
				plane3->SetPostion(glm::vec3(0.0f, 0.0f, 0.002f));

				// Create and attach a RenderComponent to the object to draw our mesh
				RenderComponent::Sptr renderer = plane3->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(ControlsMaterial);

				// Attach a plane collider that extends infinitely along the X/Y axis
				RigidBody::Sptr physics = plane3->Add<RigidBody>(/*static by default*/);
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

			GameObject::Sptr FrogTongue = scene->CreateGameObject("FrogTongue");
			{
				// Set position in the scene
				FrogTongue->SetPostion(glm::vec3(-3.4f, -1.05f, 3.59f));
				// Scale down the plane
				FrogTongue->SetScale(glm::vec3(1.0f, 0.1f, 1.0f));
				FrogTongue->SetRotation(glm::vec3(0.0f, 0.0f, 45.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = FrogTongue->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(FrogTongueMaterial);
			}

			GameObject::Sptr FrogBody = scene->CreateGameObject("FrogBody");
			{
				// Set position in the scene
				FrogBody->SetPostion(glm::vec3(-3.4f, -1.05f, 3.6f));
				// Scale down the plane
				FrogBody->SetScale(glm::vec3(1.5f, 1.5f, 1.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = FrogBody->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(FrogBodyMaterial);
			}

			GameObject::Sptr FrogHeadBot = scene->CreateGameObject("FrogHeadBot");
			{
				// Set position in the scene
				FrogHeadBot->SetPostion(glm::vec3(-3.4f, -1.05f, 3.61f));
				// Scale down the plane
				FrogHeadBot->SetScale(glm::vec3(1.5f, 1.5f, 1.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = FrogHeadBot->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(FrogHeadBotMaterial);
			}

			GameObject::Sptr FrogHeadTop = scene->CreateGameObject("FrogHeadTop");
			{
				// Set position in the scene
				FrogHeadTop->SetPostion(glm::vec3(-3.4f, -1.05f, 3.62f));
				// Scale down the plane
				FrogHeadTop->SetScale(glm::vec3(1.5f, 1.5f, 1.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = FrogHeadTop->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(FrogHeadTopMaterial);
			}

			GameObject::Sptr BushTransition = scene->CreateGameObject("BushTransition");
			{
				// Set position in the scene
				BushTransition->SetPostion(glm::vec3(5.f, 0.0f, 3.8f));
				// Scale down the plane
				BushTransition->SetScale(glm::vec3(5.8f, 2.5f, 1.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = BushTransition->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(BushTransitionMaterial);
			}



			// Save the asset manifest for all the resources we just loaded
			ResourceManager::SaveManifest("manifest.json");
			// Save the scene to a JSON file
			scene->Save("CS.json");

		}



		//// Level Select Scene ////

		{
			// Create an empty scene
			scene = std::make_shared<Scene>();

			// I hate this
			scene->BaseShader = uboShader;

			Material::Sptr MenuMaterial = ResourceManager::CreateAsset<Material>();
			{
				MenuMaterial->Name = "Menu";
				MenuMaterial->MatShader = scene->BaseShader;
				MenuMaterial->Texture = LSMenuTex;
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

			Material::Sptr FilterMaterial = ResourceManager::CreateAsset<Material>();
			{
				FilterMaterial->Name = "Button Filter";
				FilterMaterial->MatShader = scene->BaseShader;
				FilterMaterial->Texture = FilterTex;
				FilterMaterial->Shininess = 2.0f;
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

			Material::Sptr FrogBodyMaterial = ResourceManager::CreateAsset<Material>();
			{
				FrogBodyMaterial->Name = "FrogBody";
				FrogBodyMaterial->MatShader = scene->BaseShader;
				FrogBodyMaterial->Texture = FrogBodyTex;
				FrogBodyMaterial->Shininess = 2.0f;
			}

			Material::Sptr FrogHeadTopMaterial = ResourceManager::CreateAsset<Material>();
			{
				FrogHeadTopMaterial->Name = "FrogHeadTop";
				FrogHeadTopMaterial->MatShader = scene->BaseShader;
				FrogHeadTopMaterial->Texture = FrogHeadTopTex;
				FrogHeadTopMaterial->Shininess = 2.0f;
			}

			Material::Sptr FrogHeadBotMaterial = ResourceManager::CreateAsset<Material>();
			{
				FrogHeadBotMaterial->Name = "FrogHeadBot";
				FrogHeadBotMaterial->MatShader = scene->BaseShader;
				FrogHeadBotMaterial->Texture = FrogHeadBotTex;
				FrogHeadBotMaterial->Shininess = 2.0f;
			}

			Material::Sptr FrogTongueMaterial = ResourceManager::CreateAsset<Material>();
			{
				FrogTongueMaterial->Name = "FrogTongue";
				FrogTongueMaterial->MatShader = scene->BaseShader;
				FrogTongueMaterial->Texture = FrogTongueTex;
				FrogTongueMaterial->Shininess = 2.0f;
			}

			Material::Sptr ForestButtonMaterial = ResourceManager::CreateAsset<Material>();
			{
				ForestButtonMaterial->Name = "ForestButton";
				ForestButtonMaterial->MatShader = scene->BaseShader;
				ForestButtonMaterial->Texture = ForestButtonTex;
				ForestButtonMaterial->Shininess = 2.0f;
			}

			Material::Sptr MountainButtonMaterial = ResourceManager::CreateAsset<Material>();
			{
				MountainButtonMaterial->Name = "MountainButton";
				MountainButtonMaterial->MatShader = scene->BaseShader;
				MountainButtonMaterial->Texture = MountainButtonTex;
				MountainButtonMaterial->Shininess = 2.0f;
			}

			Material::Sptr MineButtonMaterial = ResourceManager::CreateAsset<Material>();
			{
				MineButtonMaterial->Name = "MineButton";
				MineButtonMaterial->MatShader = scene->BaseShader;
				MineButtonMaterial->Texture = MineButtonTex;
				MineButtonMaterial->Shininess = 2.0f;
			}

			Material::Sptr BushTransitionMaterial = ResourceManager::CreateAsset<Material>();
			{
				BushTransitionMaterial->Name = "BushTransition";
				BushTransitionMaterial->MatShader = scene->BaseShader;
				BushTransitionMaterial->Texture = BushTransitionTex;
				BushTransitionMaterial->Shininess = 2.0f;
			}

			Material::Sptr Red1Material = ResourceManager::CreateAsset<Material>();
			{
				Red1Material->Name = "Red 1";
				Red1Material->MatShader = scene->BaseShader;
				Red1Material->Texture = Tex1R;
				Red1Material->Shininess = 2.0f;
			}

			Material::Sptr Red2Material = ResourceManager::CreateAsset<Material>();
			{
				Red2Material->Name = "Red 2";
				Red2Material->MatShader = scene->BaseShader;
				Red2Material->Texture = Tex2R;
				Red2Material->Shininess = 2.0f;
			}
			
			Material::Sptr Red3Material = ResourceManager::CreateAsset<Material>();
			{
				Red3Material->Name = "Red 3";
				Red3Material->MatShader = scene->BaseShader;
				Red3Material->Texture = Tex3R;
				Red3Material->Shininess = 2.0f;
			}

			Material::Sptr Red4Material = ResourceManager::CreateAsset<Material>();
			{
				Red4Material->Name = "Red 4";
				Red4Material->MatShader = scene->BaseShader;
				Red4Material->Texture = Tex4R;
				Red4Material->Shininess = 2.0f;
			}

			Material::Sptr Red5Material = ResourceManager::CreateAsset<Material>();
			{
				Red5Material->Name = "Red 5";
				Red5Material->MatShader = scene->BaseShader;
				Red5Material->Texture = Tex5R;
				Red5Material->Shininess = 2.0f;
			}

			Material::Sptr Red6Material = ResourceManager::CreateAsset<Material>();
			{
				Red6Material->Name = "Red 6";
				Red6Material->MatShader = scene->BaseShader;
				Red6Material->Texture = Tex6R;
				Red6Material->Shininess = 2.0f;
			}

			Material::Sptr Black1Material = ResourceManager::CreateAsset<Material>();
			{
				Black1Material->Name = "Black 1";
				Black1Material->MatShader = scene->BaseShader;
				Black1Material->Texture = Tex1B;
				Black1Material->Shininess = 2.0f;
			}

			Material::Sptr Black2Material = ResourceManager::CreateAsset<Material>();
			{
				Black2Material->Name = "Black 2";
				Black2Material->MatShader = scene->BaseShader;
				Black2Material->Texture = Tex2B;
				Black2Material->Shininess = 2.0f;
			}

			Material::Sptr Black3Material = ResourceManager::CreateAsset<Material>();
			{
				Black3Material->Name = "Black 3";
				Black3Material->MatShader = scene->BaseShader;
				Black3Material->Texture = Tex3B;
				Black3Material->Shininess = 2.0f;
			}

			Material::Sptr Black4Material = ResourceManager::CreateAsset<Material>();
			{
				Black4Material->Name = "Black 4";
				Black4Material->MatShader = scene->BaseShader;
				Black4Material->Texture = Tex4B;
				Black4Material->Shininess = 2.0f;
			}

			Material::Sptr Black5Material = ResourceManager::CreateAsset<Material>();
			{
				Black5Material->Name = "Black 5";
				Black5Material->MatShader = scene->BaseShader;
				Black5Material->Texture = Tex5B;
				Black5Material->Shininess = 2.0f;
			}

			Material::Sptr Black6Material = ResourceManager::CreateAsset<Material>();
			{
				Black6Material->Name = "Black 6";
				Black6Material->MatShader = scene->BaseShader;
				Black6Material->Texture = Tex6B;
				Black6Material->Shininess = 2.0f;
			}


			// Create some lights for our scene
			scene->Lights.resize(8);
			scene->Lights[0].Position = glm::vec3(3.0f, 3.0f, 3.0f);
			scene->Lights[0].Color = glm::vec3(0.892f, 1.0f, 0.882f);
			scene->Lights[0].Range = 5.0f;

			scene->Lights[1].Position = glm::vec3(3.0f, -3.0f, 3.0f);
			scene->Lights[1].Color = glm::vec3(0.892f, 1.0f, 0.882f);
			scene->Lights[1].Range = 5.0f;

			scene->Lights[2].Position = glm::vec3(-3.0f, 3.0f, 5.0f);
			scene->Lights[2].Color = glm::vec3(0.892f, 1.0f, 0.882f);
			scene->Lights[2].Range = 5.0f;

			scene->Lights[3].Position = glm::vec3(-3.0f, -3.0f, 5.0f);
			scene->Lights[3].Color = glm::vec3(0.892f, 1.0f, 0.882f);
			scene->Lights[3].Range = 5.0f;

			scene->Lights[4].Position = glm::vec3(-8.0f, -3.0f, 5.0f);
			scene->Lights[4].Color = glm::vec3(0.892f, 1.0f, 0.882f);
			scene->Lights[4].Range = 10.0f;

			scene->Lights[5].Position = glm::vec3(-8.0f, 3.0f, 5.0f);
			scene->Lights[5].Color = glm::vec3(0.892f, 1.0f, 0.882f);
			scene->Lights[5].Range = 10.0f;

			scene->Lights[6].Position = glm::vec3(8.0f, -3.0f, 5.0f);
			scene->Lights[6].Color = glm::vec3(0.892f, 1.0f, 0.882f);
			scene->Lights[6].Range = 10.0f;

			scene->Lights[7].Position = glm::vec3(8.0f, 3.0f, 5.0f);
			scene->Lights[7].Color = glm::vec3(0.892f, 1.0f, 0.882f);
			scene->Lights[7].Range = 10.0f;

			// We'll create a mesh that is a simple plane that we can resize later
			planeMesh = ResourceManager::CreateAsset<MeshResource>();
			cubeMesh = ResourceManager::CreateAsset<MeshResource>("cube.obj");
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
				plane->SetScale(glm::vec3(20.0F, 12.0f, 10.0f));

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
				LsButton1->SetPostion(glm::vec3(0.7f, 0.4f, 3.0f));
				// Scale down the plane
				LsButton1->SetScale(glm::vec3(0.7f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = LsButton1->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(ForestButtonMaterial);

				// This object is a renderable only, it doesn't have any behaviours or
				// physics bodies attached!
			}

			GameObject::Sptr LsButton2 = scene->CreateGameObject("LSButton2");
			{
				// Set position in the scene
				LsButton2->SetPostion(glm::vec3(0.7f, -0.4f, 3.0f));
				// Scale down the plane
				LsButton2->SetScale(glm::vec3(0.7f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = LsButton2->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(ForestButtonMaterial);

				// This object is a renderable only, it doesn't have any behaviours or
				// physics bodies attached!
			}

			GameObject::Sptr LsButton3 = scene->CreateGameObject("LSButton3");
			{
				// Set position in the scene
				LsButton3->SetPostion(glm::vec3(1.6f, 0.4f, 3.0f));
				// Scale down the plane
				LsButton3->SetScale(glm::vec3(0.7f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = LsButton3->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(MountainButtonMaterial);

				// This object is a renderable only, it doesn't have any behaviours or
				// physics bodies attached!
			}

			GameObject::Sptr LsButton4 = scene->CreateGameObject("LSButton4");
			{
				// Set position in the scene
				LsButton4->SetPostion(glm::vec3(1.6f, -0.4f, 3.0f));
				// Scale down the plane
				LsButton4->SetScale(glm::vec3(0.7f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = LsButton4->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(MountainButtonMaterial);

				// This object is a renderable only, it doesn't have any behaviours or
				// physics bodies attached!
			}

			GameObject::Sptr LsButton5 = scene->CreateGameObject("LSButton5");
			{
				// Set position in the scene
				LsButton5->SetPostion(glm::vec3(2.5f, 0.4f, 3.0f));
				// Scale down the plane
				LsButton5->SetScale(glm::vec3(0.7f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = LsButton5->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(MineButtonMaterial);

				// This object is a renderable only, it doesn't have any behaviours or
				// physics bodies attached!
			}

			GameObject::Sptr LsButton6 = scene->CreateGameObject("LSButton6");
			{
				// Set position in the scene
				LsButton6->SetPostion(glm::vec3(2.5f, -0.4f, 3.0f));
				// Scale down the plane
				LsButton6->SetScale(glm::vec3(0.7f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = LsButton6->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(MineButtonMaterial);

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
				Num1->SetPostion(glm::vec3(0.525f, 0.3f, 3.5f));
				Num1->SetScale(glm::vec3(0.6f));

				RenderComponent::Sptr renderer = Num1->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(Black1Material);
			}

			GameObject::Sptr Num2 = scene->CreateGameObject("Num2");
			{
				Num2->SetPostion(glm::vec3(0.525f, -0.3f, 3.5f));
				Num2->SetScale(glm::vec3(0.6f));

				RenderComponent::Sptr renderer = Num2->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(Black2Material);
			}

			GameObject::Sptr Num3 = scene->CreateGameObject("Num3");
			{
				Num3->SetPostion(glm::vec3(1.2f, 0.3f, 3.5f));
				Num3->SetScale(glm::vec3(0.6f));

				RenderComponent::Sptr renderer = Num3->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(Black3Material);
			}

			GameObject::Sptr Num4 = scene->CreateGameObject("Num4");
			{
				Num4->SetPostion(glm::vec3(1.2f, -0.3f, 3.5f));
				Num4->SetScale(glm::vec3(0.6f));

				RenderComponent::Sptr renderer = Num4->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(Black4Material);
			}

			GameObject::Sptr Num5 = scene->CreateGameObject("Num5");
			{
				Num5->SetPostion(glm::vec3(1.875f, 0.3f, 3.5f));
				Num5->SetScale(glm::vec3(0.6f));

				RenderComponent::Sptr renderer = Num5->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(Black5Material);
			}

			GameObject::Sptr Num6 = scene->CreateGameObject("Num6");
			{
				Num6->SetPostion(glm::vec3(1.875f, -0.3f, 3.5f));
				Num6->SetScale(glm::vec3(0.6f));

				RenderComponent::Sptr renderer = Num6->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(Black6Material);
			}

			GameObject::Sptr FrogTongue = scene->CreateGameObject("FrogTongue");
			{
				// Set position in the scene
				FrogTongue->SetPostion(glm::vec3(-3.4f, -1.05f, 3.59f));
				// Scale down the plane
				FrogTongue->SetScale(glm::vec3(1.0f, 0.1f, 1.0f));
				FrogTongue->SetRotation(glm::vec3(0.0f, 0.0f, 45.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = FrogTongue->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(FrogTongueMaterial);
			}

			GameObject::Sptr FrogBody = scene->CreateGameObject("FrogBody");
			{
				// Set position in the scene
				FrogBody->SetPostion(glm::vec3(-3.4f, -1.05f, 3.6f));
				// Scale down the plane
				FrogBody->SetScale(glm::vec3(1.5f, 1.5f, 1.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = FrogBody->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(FrogBodyMaterial);
			}

			GameObject::Sptr FrogHeadBot = scene->CreateGameObject("FrogHeadBot");
			{
				// Set position in the scene
				FrogHeadBot->SetPostion(glm::vec3(-3.4f, -1.05f, 3.61f));
				// Scale down the plane
				FrogHeadBot->SetScale(glm::vec3(1.5f, 1.5f, 1.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = FrogHeadBot->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(FrogHeadBotMaterial);
			}

			GameObject::Sptr FrogHeadTop = scene->CreateGameObject("FrogHeadTop");
			{
				// Set position in the scene
				FrogHeadTop->SetPostion(glm::vec3(-3.4f, -1.05f, 3.62f));
				// Scale down the plane
				FrogHeadTop->SetScale(glm::vec3(1.5f, 1.5f, 1.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = FrogHeadTop->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(FrogHeadTopMaterial);
			}

			GameObject::Sptr BushTransition = scene->CreateGameObject("BushTransition");
			{
				// Set position in the scene
				BushTransition->SetPostion(glm::vec3(5.f, 0.0f, 3.8f));
				// Scale down the plane
				BushTransition->SetScale(glm::vec3(5.8f, 2.5f, 1.0f));

				// Create and attach a render component
				RenderComponent::Sptr renderer = BushTransition->Add<RenderComponent>();
				renderer->SetMesh(planeMesh);
				renderer->SetMaterial(BushTransitionMaterial);
			}


			scene->SetAmbientLight(glm::vec3(0.2f));

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


		//result = system->playSound(sound4, 0, false, &channel);
		
		result = system->playSound(sound9, 0, false, &channel);
	
	

	bool isEscapePressed = false;
	

	float ProgressBarTime = 0; //will calculate the time from the beginning to the end of the level
	float ProgressBarTemp = 0; //temp value so we can calculate time elapsed from beginning to end of level
	float ProgressBarTempPaused = 0;
	float ProgressBarTempPaused2 = 0;
	float ProgressBarPaused = 0;

	PTime = 0;
	PTemp = 0;
	PTemp2 = 0;
	playerPlaying = false;

	///// Game loop /////
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		ImGuiHelper::StartFrame();

		//SDL_GL_Set

		/// test FMOD
		if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
		{
			scene->SetAmbientLight(glm::vec3(0.1f));
			std::cout << "J pressed" << std::endl;
		}
		if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
		{
			scene->SetAmbientLight(glm::vec3(0.2f));
			std::cout << "K pressed" << std::endl;
		}
		if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
		{
			scene->SetAmbientLight(glm::vec3(0.3f));
			std::cout << "L pressed" << std::endl;
		}


		


		/// with this change to the check, switching between scenes using scenePath no longer causes the game to crash since if the scene doesn't have a player it wont prompt commands
		if (scene->FindObjectByName("player") != NULL)
		{
			if (paused == true)
			{
				playerPlaying = false;
				scene->FindObjectByName("PanelPause")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 6, 6.5));
				scene->FindObjectByName("ButtonBack1")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 6.25, 6.0));
				scene->FindObjectByName("ButtonBack2")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 6.5, 5.0));
				scene->FindObjectByName("ButtonBack3")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 6.75, 4.0));
				scene->FindObjectByName("ResumeText")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 8, 6.1));
				scene->FindObjectByName("LSText")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 8, 5.4));
				scene->FindObjectByName("MainMenuText")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 8.2, 4.7));
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
				else if (index == 3)
				{
					scene->FindObjectByName("Filter")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 6.76, 4.0));
				}

			}

			if (playerLose == true)
			{
				AnimTime = 0;
				FPSIncrease = 0;
				playerPlaying = false;
				scene->FindObjectByName("PanelPause")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 6, 6.5));
				scene->FindObjectByName("ButtonBack1")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 6.25, 6.0));
				scene->FindObjectByName("ButtonBack2")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 6.5, 5.0));
				scene->FindObjectByName("ButtonBack3")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 6.75, 4.0));
				scene->FindObjectByName("ReplayText")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 8, 6.1));
				scene->FindObjectByName("MainMenuText")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 8.2, 4.7));
				scene->FindObjectByName("LSText")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 8, 5.4));
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
				else if (index == 3)
				{
					scene->FindObjectByName("Filter")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 6.76, 4.0));
				}
			}

			if (playerWin == true)
			{
				if (scoreWritten == false) {
					//writes time to text file
					timeToBeat.open("unsortedScores.txt", std::ios::app);
					timeToBeat << PTime << "\n";
					timeToBeat.close();
					std::cout << "yay it worked!";


					readScores(); //reads all of the scores before loading the scene
					quickSort(floatScores, 0, floatScores.size() - 1); //sorts our array from lowest to greatest

					timeToBeat.open("sortedScores.txt", std::ofstream::out | std::ofstream::trunc);
					timeToBeat.close();

					//writes sorted vector to text file
					for (int i = 0; i < floatScores.size(); i++) {
						timeToBeat.open("sortedScores.txt", std::ios::app);
						timeToBeat << floatScores[i] << "\n";
						timeToBeat.close();
						std::cout << "yay it worked2!";
					}
				}
				scoreWritten = true;
				PTime = 0;
				PTemp2 = 0;
				PTemp = 0;

				playerPlaying = false;
				scene->FindObjectByName("PanelPause")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 6, 6.5));
				scene->FindObjectByName("ButtonBack1")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 6.25, 6.0));
				scene->FindObjectByName("ButtonBack2")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 6.5, 5.0));
				scene->FindObjectByName("ButtonBack3")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 6.75, 4.0));
				scene->FindObjectByName("ReplayText")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 8, 6.1));
				scene->FindObjectByName("MainMenuText")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 8.2, 4.7));
				scene->FindObjectByName("LSText")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 8, 5.4));
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
				else if (index == 3)
				{
					scene->FindObjectByName("Filter")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 6.76, 4.0));
				}
			}
			else {
				scoreWritten = false;
			}

			if (paused != true && playerLose != true && playerWin != true)
			{
				playerPlaying = true;
				//originally these were all back at -15 but idk if that makes the game more jank cause of overlap so i tried to spread em out
				scene->FindObjectByName("PanelPause")->SetPostion(glm::vec3(scene->FindObjectByName("Main Camera")->GetPosition().x, scene->FindObjectByName("Main Camera")->GetPosition().y + 1, 6.5));
				scene->FindObjectByName("ButtonBack1")->SetPostion(glm::vec3(scene->FindObjectByName("Main Camera")->GetPosition().x, scene->FindObjectByName("Main Camera")->GetPosition().y + 2, 6));
				scene->FindObjectByName("ButtonBack2")->SetPostion(glm::vec3(scene->FindObjectByName("Main Camera")->GetPosition().x, scene->FindObjectByName("Main Camera")->GetPosition().y + 3, 5));
				scene->FindObjectByName("ButtonBack3")->SetPostion(glm::vec3(scene->FindObjectByName("Main Camera")->GetPosition().x, scene->FindObjectByName("Main Camera")->GetPosition().y + 4, 4));
				scene->FindObjectByName("ResumeText")->SetPostion(glm::vec3(scene->FindObjectByName("Main Camera")->GetPosition().x, scene->FindObjectByName("Main Camera")->GetPosition().y + 5, 6.1));
				scene->FindObjectByName("MainMenuText")->SetPostion(glm::vec3(scene->FindObjectByName("Main Camera")->GetPosition().x, scene->FindObjectByName("Main Camera")->GetPosition().y + 6, 4.7));
				scene->FindObjectByName("LSText")->SetPostion(glm::vec3(scene->FindObjectByName("Main Camera")->GetPosition().x, scene->FindObjectByName("Main Camera")->GetPosition().y + 6, 5.4));
				scene->FindObjectByName("PauseLogo")->SetPostion(glm::vec3(scene->FindObjectByName("Main Camera")->GetPosition().x, scene->FindObjectByName("Main Camera")->GetPosition().y + 7, 8));
				scene->FindObjectByName("Filter")->SetPostion(glm::vec3(scene->FindObjectByName("Main Camera")->GetPosition().x, scene->FindObjectByName("Main Camera")->GetPosition().y + 8, 8));
				scene->FindObjectByName("LoserLogo")->SetPostion(glm::vec3(scene->FindObjectByName("Main Camera")->GetPosition().x, scene->FindObjectByName("Main Camera")->GetPosition().y + 9, 8));
				scene->FindObjectByName("ReplayText")->SetPostion(glm::vec3(scene->FindObjectByName("Main Camera")->GetPosition().x, scene->FindObjectByName("Main Camera")->GetPosition().y + 10, 6.1));
				scene->FindObjectByName("WinnerLogo")->SetPostion(glm::vec3(scene->FindObjectByName("Main Camera")->GetPosition().x, scene->FindObjectByName("Main Camera")->GetPosition().y + 11, 6.1));

				if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && soundprompt == false)
				{
					result = system->playSound(sound6, 0, false, &channel);
					soundprompt = true;
				}

				if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && soundprompt == false)
				{
					result = system->playSound(sound5, 0, false, &channel);
					soundprompt = true;
				}

				if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_UP) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
				{
					soundprompt = false;
				}

			}

			if (paused == true || playerLose == true || playerWin == true)
			{
				playerPlaying = false;
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
			//std::cout << ProgressBarTime << "\n";

			scene->FindObjectByName("Main Camera")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 11.480, 6.290)); // makes the camera follow the player
			scene->FindObjectByName("Main Camera")->SetRotation(glm::vec3(84, 0, -180)); //angled view (stops camera from rotating)
			scene->FindObjectByName("player")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x, 0.f, scene->FindObjectByName("player")->GetPosition().z)); // makes the camera follow the player

			scene->FindObjectByName("ProgressBarGO")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x - 5, 1.620, 13)); //makes progress bar follow the player
			scene->FindObjectByName("ProgressBarProgress")->SetPostion(glm::vec3(scene->FindObjectByName("player")->GetPosition().x + 2 - ProgressBarTime, 1.7, 12.75)); //Makes Progress of progress bar follow the player


			//Stops the player from rotating
			scene->FindObjectByName("player")->SetRotation(glm::vec3(90.f, scene->FindObjectByName("player")->GetRotation().y, 90.f));

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

				result = system->playSound(sound10, 0, false, &channel);

				if (scenevalue == 1)
				{
					scene->FindObjectByName("player")->SetPostion(glm::vec3(-406.f, 0.f, scene->FindObjectByName("player")->GetPosition().z));
				}
				else if (scenevalue == 2)
				{
					scene->FindObjectByName("player")->SetPostion(glm::vec3(6.f, 0.f, scene->FindObjectByName("player")->GetPosition().z));
				}
				else if (scenevalue == 3)
				{
					scene->FindObjectByName("player")->SetPostion(glm::vec3(-806.f, 0.f, scene->FindObjectByName("player")->GetPosition().z));
				}
				else if (scenevalue == 4)
				{
					scene->FindObjectByName("player")->SetPostion(glm::vec3(-1206.f, 0.f, scene->FindObjectByName("player")->GetPosition().z));
				}
				else if (scenevalue == 5)
				{
					scene->FindObjectByName("player")->SetPostion(glm::vec3(-1606.f, 0.f, scene->FindObjectByName("player")->GetPosition().z));
				}
				else if (scenevalue == 6)
				{
					scene->FindObjectByName("player")->SetPostion(glm::vec3(-2006.f, 0.f, scene->FindObjectByName("player")->GetPosition().z));
				}

				std::cout << "colision detected";
				playerCollision.hitEntered = false;
				playerMove = false;
				playerLose = true;
			}
			//JumpBehaviour test;
			//test.Update();

			playerCollision.update(scene->FindObjectByName("player")->GetPosition()); // to update
			//scene->FindObjectByName("player")->Get<JumpBehaviour>()->getPlayerCoords(scene->FindObjectByName("player")->GetPosition()); //send the players coordinates to JumpBehavior so we know when the player is on the ground

			if (scenevalue == 1)
			{
				if (scene->FindObjectByName("player")->GetPosition().x < -800)
				{
					scene->FindObjectByName("player")->SetPostion(glm::vec3(-406.f, 0.f, scene->FindObjectByName("player")->GetPosition().z));
					playerMove = false;
					playerWin = true;
					result = system->playSound(sound11, 0, false, &channel);
				}
			}
			else if (scenevalue == 2)
			{
				if (scene->FindObjectByName("player")->GetPosition().x < -400)
				{
					scene->FindObjectByName("player")->SetPostion(glm::vec3(6.f, 0.f, scene->FindObjectByName("player")->GetPosition().z));
					playerMove = false;
					playerWin = true;
					result = system->playSound(sound11, 0, false, &channel);
				}
			}
			else if (scenevalue == 3)
			{
				if (scene->FindObjectByName("player")->GetPosition().x < -1200)
				{
					scene->FindObjectByName("player")->SetPostion(glm::vec3(-806.f, 0.f, scene->FindObjectByName("player")->GetPosition().z));
					playerMove = false;
					playerWin = true;
					result = system->playSound(sound11, 0, false, &channel);
				}
			}
			else if (scenevalue == 4)
			{
				if (scene->FindObjectByName("player")->GetPosition().x < -1600)
				{
					scene->FindObjectByName("player")->SetPostion(glm::vec3(-1206.f, 0.f, scene->FindObjectByName("player")->GetPosition().z));
					playerMove = false;
					playerWin = true;
					result = system->playSound(sound11, 0, false, &channel);
				}
			}
			else if (scenevalue == 5)
			{
				if (scene->FindObjectByName("player")->GetPosition().x < -2000)
				{
					scene->FindObjectByName("player")->SetPostion(glm::vec3(-1606.f, 0.f, scene->FindObjectByName("player")->GetPosition().z));
					playerMove = false;
					playerWin = true;
					result = system->playSound(sound11, 0, false, &channel);
				}
			}
			else if (scenevalue == 6)
			{
				if (scene->FindObjectByName("player")->GetPosition().x < -2400)
				{
					scene->FindObjectByName("player")->SetPostion(glm::vec3(-2006.f, 0.f, scene->FindObjectByName("player")->GetPosition().z));
					playerMove = false;
					playerWin = true;
					result = system->playSound(sound11, 0, false, &channel);
				}
			}


		}
		else
		{
			SceneChanger();

			if (((glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)) && soundprompt == false)
			{
				result = system->playSound(sound1, 0, false, &channel);
				soundprompt = true;
			}

			if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_UP) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_RELEASE)
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
		//std::cout << glfwGetTime() << std::endl;
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
			//if (glfwGetKey(window, GLFW_KEY_UP) && scene->IsPlaying == false) {
			//	scene->IsPlaying = true;
			//}
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