#include "Gameplay/Components/JumpBehaviour.h"
#include <GLFW/glfw3.h>
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Utils/ImGuiHelper.h"

void JumpBehaviour::Awake()
{
	_body = GetComponent<Gameplay::Physics::RigidBody>();
	if (_body == nullptr) {
		IsEnabled = false;
	}
}

void JumpBehaviour::RenderImGui() {
	LABEL_LEFT(ImGui::DragFloat, "Impulse", &_impulse, 1.0f);
}

nlohmann::json JumpBehaviour::ToJson() const {
	return {
		{ "impulse", _impulse }
	};
}

JumpBehaviour::JumpBehaviour() :
	IComponent(),
	_impulse(1.f)
{ }

JumpBehaviour::JumpBehaviour(glm::vec3 position) :
	IComponent(),
	_impulse(1.f)
{
	//currentHeight = position.z;
}

JumpBehaviour::~JumpBehaviour() = default;

JumpBehaviour::Sptr JumpBehaviour::FromJson(const nlohmann::json & blob) {
	JumpBehaviour::Sptr result = std::make_shared<JumpBehaviour>();
	result->_impulse = blob["impulse"];
	return result;
}

void JumpBehaviour::getPlayerCoords(glm::vec3 position) {
	currentHeight = position.z;
}

void JumpBehaviour::Update(float deltaTime) {
	//_body->OnEnteredTrigger
	_body->SetLinearDamping(-10.f);
	float test = _body->GetMass();
	//std::cout << test;
	pressed = glfwGetKey(GetGameObject()->GetScene()->Window, GLFW_KEY_SPACE);
	UPpressed = glfwGetKey(GetGameObject()->GetScene()->Window, GLFW_KEY_UP);
	if (pressed) {
		if (_isPressed == false && currentHeight <= 0.602f) {
			_body->ApplyImpulse(glm::vec3(0.0f, 0.0f, _impulse));
			//_body->ApplyForce(glm::vec3(0.0f, 0.0f, -10.f));

			_body->ApplyForce(glm::vec3(0.0f, 0.0f, 1000.f));
		}
		_isPressed = pressed;
	}
	else {
		_isPressed = false;
	}

	if (UPpressed) {
		if (_isUPPressed == false && currentHeight <= 0.602f) {
			_body->ApplyImpulse(glm::vec3(0.0f, 0.0f, _impulse));
			//_body->ApplyForce(glm::vec3(0.0f, 0.0f, -10.f));

			_body->ApplyForce(glm::vec3(0.0f, 0.0f, 1200.f));
		}
		_isUPPressed = UPpressed;


		if (currentHeight > 10) {
			fly = true;
		}

		if (fly) {
			//_body->ApplyForce(glm::vec3(0.0f, 0.0f, 10.f));
			if (currentHeight > 8.f) {
				if (currentHeight < 10.f) {
					_body->ApplyImpulse(glm::vec3(0.0f, 0.0f, _impulse));
					_body->ApplyForce(glm::vec3(0.0f, 0.0f, 50.f));
				}
				else {
					//_body->ApplyImpulse(glm::vec3(0.0f, 0.0f, -_impulse));
					_body->ApplyForce(glm::vec3(0.0f, 0.0f, -50.f));
				}
			}
		}
	}
	else {
		fly = false;
		_isUPPressed = false;
	}
}
