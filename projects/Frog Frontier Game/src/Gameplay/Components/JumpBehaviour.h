#pragma once
#include "IComponent.h"
#include "Gameplay/Physics/RigidBody.h"

/// <summary>
/// A simple behaviour that applies an impulse along the Z axis to the 
/// rigidbody of the parent when the space key is pressed
/// </summary>
class JumpBehaviour : public Gameplay::IComponent {
public:
	float currentHeight;
	bool pressed, UPpressed;
	typedef std::shared_ptr<JumpBehaviour> Sptr;

	JumpBehaviour();
	JumpBehaviour(glm::vec3 position);
	virtual ~JumpBehaviour();

	virtual void Awake() override;
	virtual void Update(float deltaTime) override;
	virtual void getPlayerCoords(glm::vec3 position);

public:
	virtual void RenderImGui() override;
	MAKE_TYPENAME(JumpBehaviour);
	virtual nlohmann::json ToJson() const override;
	static JumpBehaviour::Sptr FromJson(const nlohmann::json& blob);

protected:
	float _impulse;

	bool _isPressed = false;
	bool _isUPPressed = false;
	bool fly = false;
	Gameplay::Physics::RigidBody::Sptr _body;


};