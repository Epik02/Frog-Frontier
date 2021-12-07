// GLM math library
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <GLM/gtx/common.hpp> // for fmod (floating modulus)
#include <iostream>

class CollisionRect
{
public:
	int id;
	int lastIdHit;
	bool hitEntered;
	double x;  //top left
	double y;  //top left
	float width; //width of 3d model
	float height; //height of 3d model
	CollisionRect();
	CollisionRect(glm::vec3 position, float modelwidth, float modelheight, int newId);

	void update(glm::vec3 position);

	bool valueInRange(double value, double min, float max);

	bool rectOverlap(CollisionRect  A, CollisionRect B);
};