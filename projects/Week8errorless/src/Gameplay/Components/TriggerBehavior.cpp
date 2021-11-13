#include "Gameplay/Components/TriggerBehavior.h"
#include "Gameplay/Components/ComponentManager.h"
#include "Gameplay/GameObject.h"

TriggerBehavior::TriggerBehavior() :
	IComponent(),
	_renderer(nullptr),
	EnterMaterial(nullptr),
	ExitMaterial(nullptr)
{ }
TriggerBehavior::~TriggerBehavior() = default;

void TriggerBehavior::OnEnteredTrigger(const Gameplay::Physics::TriggerVolume::Sptr & trigger) {
	if (_renderer && EnterMaterial) {
		_renderer->SetMaterial(EnterMaterial);
	}
	LOG_INFO("Entered trigger: {}", trigger->GetGameObject()->Name);
	hit = true;
}

void TriggerBehavior::OnLeavingTrigger(const Gameplay::Physics::TriggerVolume::Sptr & trigger) {
	if (_renderer && ExitMaterial) {
		_renderer->SetMaterial(ExitMaterial);
	}
	LOG_INFO("Left trigger: {}", trigger->GetGameObject()->Name);
	hit = false;
}

void TriggerBehavior::Awake() {
	_renderer = GetComponent<RenderComponent>();
}

void TriggerBehavior::RenderImGui() { }

nlohmann::json TriggerBehavior::ToJson() const {
	return {
		{ "enter_material", EnterMaterial != nullptr ? EnterMaterial->GetGUID().str() : "null" },
		{ "exit_material", ExitMaterial != nullptr ? ExitMaterial->GetGUID().str() : "null" }
	};
}

TriggerBehavior::Sptr TriggerBehavior::FromJson(const nlohmann::json & blob) {
	TriggerBehavior::Sptr result = std::make_shared<TriggerBehavior>();
	result->EnterMaterial = ResourceManager::Get<Gameplay::Material>(Guid(blob["enter_material"]));
	result->ExitMaterial = ResourceManager::Get<Gameplay::Material>(Guid(blob["exit_material"]));
	return result;
}
