#pragma once
#include "IComponent.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Gameplay/Components/RenderComponent.h"
#include "Gameplay/Physics/TriggerVolume.h"

/// <summary>
/// Provides an example behaviour that uses some of the trigger interface to change the material
/// of the game object the component is attached to when entering or leaving a trigger
/// </summary>
class TriggerBehavior : public Gameplay::IComponent {

public:
	typedef std::shared_ptr<TriggerBehavior> Sptr;
	TriggerBehavior();
	virtual ~TriggerBehavior();

	Gameplay::Material::Sptr        EnterMaterial;
	Gameplay::Material::Sptr        ExitMaterial;
	bool hit = false;

	// Inherited from IComponent

	virtual void OnEnteredTrigger(const std::shared_ptr<Gameplay::Physics::TriggerVolume>& trigger) override;
	virtual void OnLeavingTrigger(const std::shared_ptr<Gameplay::Physics::TriggerVolume>& trigger) override;
	virtual void Awake() override;
	virtual void RenderImGui() override;
	virtual nlohmann::json ToJson() const override;
	static TriggerBehavior::Sptr FromJson(const nlohmann::json& blob);
	MAKE_TYPENAME(TriggerBehavior);

protected:

	RenderComponent::Sptr _renderer;
};