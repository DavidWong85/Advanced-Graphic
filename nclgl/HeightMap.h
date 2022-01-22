#pragma once

#include <string>
#include "Mesh.h"

class HeightMap : public Mesh {
protected:
	Vector3 heightmapSize;

public:
	HeightMap(const std::string& name);
	~HeightMap(void) {};

	Vector3 GetHeightmapSize() const { return heightmapSize; }
};