#include "Scene.h"

#include <GLFW/glfw3.h>

#include "Utils/FileHelpers.h"
#include "Utils/GlmBulletConversions.h"

#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Physics/TriggerVolume.h"

#include "Graphics/DebugDraw.h"

namespace Gameplay {
	Scene::Scene() :
		Objects(std::vector<GameObject::Sptr>()),
		Lights(std::vector<Light>()),
		IsPlaying(false),
		MainCamera(nullptr),
		BaseShader(nullptr),
		_isAwake(false),
		_filePath(""),
		_ambientLight(glm::vec3(0.1f)),
		_gravity(glm::vec3(0.0f, 0.0f, -9.81f))
	{
		_InitPhysics();
	}

	Scene::~Scene() {
		Objects.clear();
		_CleanupPhysics();
	}

	void Scene::SetPhysicsDebugDrawMode(BulletDebugMode mode) {
		_bulletDebugDraw->setDebugMode((btIDebugDraw::DebugDrawModes)mode);
	}

	GameObject::Sptr Scene::CreateGameObject(const std::string& name)
	{
		GameObject::Sptr result(new GameObject());
		result->Name = name;
		result->_scene = this;
		Objects.push_back(result);
		return result;
	}

	GameObject::Sptr Scene::FindObjectByName(const std::string name) {
		auto it = std::find_if(Objects.begin(), Objects.end(), [&](const GameObject::Sptr& obj) {
			return obj->Name == name;
		});
		return it == Objects.end() ? nullptr : *it;
	}

	GameObject::Sptr Scene::FindObjectByGUID(Guid id) {
		auto it = std::find_if(Objects.begin(), Objects.end(), [&](const GameObject::Sptr& obj) {
			return obj->GUID == id;
		});
		return it == Objects.end() ? nullptr : *it;
	}

	void Scene::SetAmbientLight(const glm::vec3& value) {
		_ambientLight = value;
		BaseShader->SetUniform("u_AmbientCol", glm::vec3(0.1f));
	}

	const glm::vec3& Scene::GetAmbientLight() const { 
		return _ambientLight;
	}

	void Scene::Awake() {
		// Not a huge fan of this, but we need to get window size to notify our camera
		// of the current screen size
		int width, height;
		glfwGetWindowSize(Window, &width, &height);
		MainCamera->ResizeWindow(width, height);

		// Call awake on all gameobjects
		for (auto& obj : Objects) {
			obj->Awake();
		}
		// Set up our lighting 
		SetupShaderAndLights();

		_isAwake = true;
	}

	void Scene::DoPhysics(float dt) {
		if (IsPlaying) {
			ComponentManager::Each<Gameplay::Physics::RigidBody>([=](const std::shared_ptr<Gameplay::Physics::RigidBody>& body) {
				body->PhysicsPreStep(dt);
			});
			ComponentManager::Each<Gameplay::Physics::TriggerVolume>([=](const std::shared_ptr<Gameplay::Physics::TriggerVolume>& body) {
				body->PhysicsPreStep(dt);
			}); 

			_physicsWorld->stepSimulation(dt, 15);

			ComponentManager::Each<Gameplay::Physics::RigidBody>([=](const std::shared_ptr<Gameplay::Physics::RigidBody>& body) {
				body->PhysicsPostStep(dt);
			});
			ComponentManager::Each<Gameplay::Physics::TriggerVolume>([=](const std::shared_ptr<Gameplay::Physics::TriggerVolume>& body) {
				body->PhysicsPostStep(dt);
			});
			if (_bulletDebugDraw->getDebugMode() != btIDebugDraw::DBG_NoDebug) {
				_physicsWorld->debugDrawWorld();
				DebugDrawer::Get().FlushAll();
			}
		}
	}

	void Scene::Update(float dt) {
		if (IsPlaying) {
			for (auto& obj : Objects) {
				obj->Update(dt);
			}
		}
	}

	void Scene::SetShaderLight(int index, bool update /*= true*/) {
		std::stringstream stream;
		stream << "u_Lights[" << index << "]";
		std::string name = stream.str();

		Light& light = Lights[index];
	
		// Set the shader uniforms for the light
		BaseShader->SetUniform(name + ".Position", light.Position);
		BaseShader->SetUniform(name + ".Color", light.Color);
		BaseShader->SetUniform(name + ".Attenuation", 1.0f / (1.0f + light.Range));
	}

	void Scene::SetupShaderAndLights() {
		BaseShader->SetUniform("u_NumLights", (int)Lights.size());
		for (int ix = 0; ix < Lights.size(); ix++) {
			SetShaderLight(ix, true);
		}
	}

	btDynamicsWorld* Scene::GetPhysicsWorld() const {
		return _physicsWorld;
	}

	Scene::Sptr Scene::FromJson(const nlohmann::json& data)
	{
		Scene::Sptr result = std::make_shared<Scene>();
		result->BaseShader = ResourceManager::Get<Shader>(Guid(data["default_shader"]));

		// Make sure the scene has objects, then load them all in!
		LOG_ASSERT(data["objects"].is_array(), "Objects not present in scene!");
		for (auto& object : data["objects"]) {
			result->Objects.push_back(GameObject::FromJson(object, result.get()));
		}

		// Make sure the scene has lights, then load all
		LOG_ASSERT(data["lights"].is_array(), "Lights not present in scene!");
		for (auto& light : data["lights"]) {
			result->Lights.push_back(Light::FromJson(light));
		}

		// Create and load camera config
		result->MainCamera = ComponentManager::GetComponentByGUID<Camera>(Guid(data["main_camera"]));
	
		return result;
	}

	nlohmann::json Scene::ToJson() const
	{
		nlohmann::json blob;
		// Save the default shader (really need a material class)
		blob["default_shader"] = BaseShader->GetGUID().str();

		// Save renderables
		std::vector<nlohmann::json> objects;
		objects.resize(Objects.size());
		for (int ix = 0; ix < Objects.size(); ix++) {
			objects[ix] = Objects[ix]->ToJson();
		}
		blob["objects"] = objects;

		// Save lights
		std::vector<nlohmann::json> lights;
		lights.resize(Lights.size());
		for (int ix = 0; ix < Lights.size(); ix++) {
			lights[ix] = Lights[ix].ToJson();
		}
		blob["lights"] = lights;

		// Save camera info
		blob["main_camera"] = MainCamera != nullptr ? MainCamera->GetGUID().str() : "null";

		return blob;
	}

	void Scene::Save(const std::string& path) {
		_filePath = path;
		// Save data to file
		FileHelpers::WriteContentsToFile(path, ToJson().dump(1, '\t'));
		LOG_INFO("Saved scene to \"{}\"", path);
	}

	Scene::Sptr Scene::Load(const std::string& path)
	{
		LOG_INFO("Loading scene from \"{}\"", path);
		std::string content = FileHelpers::ReadFile(path);
		nlohmann::json blob = nlohmann::json::parse(content);
		Scene::Sptr result = FromJson(blob);
		result->_filePath = path;
		return result;
	}

	int Scene::NumObjects() const {
		return Objects.size();
	}

	GameObject::Sptr Scene::GetObjectByIndex(int index) const {
		return Objects[index];
	}

	void Scene::_InitPhysics() {
		_collisionConfig = new btDefaultCollisionConfiguration();
		_collisionDispatcher = new btCollisionDispatcher(_collisionConfig);
		_broadphaseInterface = new btDbvtBroadphase();
		_ghostCallback = new btGhostPairCallback();
		_broadphaseInterface->getOverlappingPairCache()->setInternalGhostPairCallback(_ghostCallback);
		_constraintSolver = new btSequentialImpulseConstraintSolver();
		_physicsWorld = new btDiscreteDynamicsWorld(
			_collisionDispatcher,
			_broadphaseInterface,
			_constraintSolver,
			_collisionConfig
		);
		_physicsWorld->setGravity(ToBt(_gravity));
		// TODO bullet debug drawing
		_bulletDebugDraw = new BulletDebugDraw();
		_physicsWorld->setDebugDrawer(_bulletDebugDraw);
		_bulletDebugDraw->setDebugMode(btIDebugDraw::DBG_NoDebug);
	}

	void Scene::_CleanupPhysics() {
		delete _physicsWorld;
		delete _constraintSolver;
		delete _broadphaseInterface;
		delete _ghostCallback;
		delete _collisionDispatcher;
		delete _collisionConfig;
	}

	void Scene::DrawAllGameObjectGUIs()
	{
		for (auto& object : Objects) {
			object->DrawImGui();
		}
	}
}