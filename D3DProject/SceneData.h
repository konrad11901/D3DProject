#pragma once

#include "vertex.h"

class SceneData {
public:
	SceneData(const std::string& source_path);

	const std::vector<vertex_t>& GetTriangleData();
private:
	std::vector<vertex_t> triangle_data;

	std::pair<std::size_t, std::size_t> GetIndexPair(std::ifstream& input_stream);
};
