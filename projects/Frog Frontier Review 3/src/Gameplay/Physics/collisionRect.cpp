#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <GLM/gtx/common.hpp> // for fmod (floating modulus)
#include "Gameplay/Physics/CollisionRect.h"
#include <iostream>
#pragma once


using namespace std;


int id;
int lastIdHit;
bool hitEntered;
double x;  //top left
double y;  //top left
double width; //width of 3d model
double height; //height of 3d model
CollisionRect::CollisionRect() {
	id = 0;
	lastIdHit = -1;
	hitEntered = false;
	x = 0.f;
	y = 0.f;
	width = 0;
	height = 0;
}
CollisionRect::CollisionRect(glm::vec3 position, double modelwidth, double modelheight, int newId)
{
	id = newId;
	lastIdHit = -1;
	hitEntered = false;
	x = position.x;
	y = position.z; //uses z because the z axis controls going up and down
	width = modelwidth;
	height = modelheight;
}

//call this if gameobject moves
void CollisionRect::update(glm::vec3 position)
{
	x = position.x;
	y = position.z; //uses z because the z axis controls going up and down
}

bool CollisionRect::valueInRange(double value, double min, double max)
{
	return (value >= min) && (value <= max);
}

bool CollisionRect::rectOverlap(CollisionRect A, CollisionRect B)
{
	bool xOverlap = valueInRange(A.x, B.x, B.x + B.width) ||
		valueInRange(B.x, A.x, A.x + A.width);

	bool yOverlap = valueInRange(A.y, B.y, B.y + B.height) ||
		valueInRange(B.y, A.y, A.y + A.height);

	bool xOverlap2 = valueInRange(A.x, B.x, B.x + 1.f) ||
		valueInRange(B.x, A.x, A.x + 1.f);

	bool yOverlap2 = valueInRange(A.y, B.y, B.y + 1.f) ||
		valueInRange(B.y, A.y, A.y + 1.f);

	if (xOverlap2 && yOverlap2) {
		if (lastIdHit != B.id) {
			lastIdHit = B.id;
			hitEntered = true;
			cout << "working" << "/n";
		}
	}
	//cout << A.x << "\n" << B.x << "\n" << " " << "\n";
	if (A.x + A.width > B.x && A.x < B.x + B.width && A.y + A.height < B.y && A.y > B.y + B.height) {
		hitEntered = true;
	}

	return xOverlap2 && yOverlap2;
}