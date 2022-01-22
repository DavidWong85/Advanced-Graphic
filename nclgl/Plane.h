#pragma once
#include "vector3.h"

class Plane {
protected:
	Vector3 normal;
	float distance;

public:
	Plane(void) {};
	Plane(const Vector3& normal, float distance, bool normalise = false);
	~Plane(void) {};

	void SetNormal(const Vector3& normal) { this->normal = normal; }
	Vector3 GetNormal() const { return normal; }

	void SetDistance(float dist) { distance = dist; }
	float GetDistance() const { return distance; }

	bool SphereInPlane(const Vector3& position, float radius) const;
};