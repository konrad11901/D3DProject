#include "pch.h"
#include "SceneData.h"

struct obj_vertex {
	FLOAT position[3];
};
struct obj_texture {
	FLOAT position[2];
};

SceneData::SceneData(const std::string& scene_path) {
	std::ifstream scene_stream(scene_path);

	std::vector<obj_vertex> vertices;
	std::vector<obj_texture> texture_positions;

	std::string type;
	while (scene_stream >> type) {
		if (type == "v") {
			FLOAT x, y, z;
			scene_stream >> x >> y >> z;
			vertices.push_back({ x, y, z });
		}
		else if (type == "vt") {
			FLOAT u, v;
			scene_stream >> u >> v;
			texture_positions.push_back({ u, v });
		}
		else if (type == "s") {
			bool smooth_shading;
			scene_stream >> smooth_shading;
			// Ignore
		}
		else if (type == "usemtl") {
			std::string material;
			scene_stream >> material;
			// Ignore
		}
		else if (type == "f") {
			for (std::size_t i = 0; i < 3; i++) {
				auto index_pair = GetIndexPair(scene_stream);
				FLOAT* vertex_position = vertices[index_pair.first - 1].position;
				FLOAT* texture_position = texture_positions[index_pair.second - 1].position;
				triangle_data.push_back({ { vertex_position[0], vertex_position[1], vertex_position[2] },
					{ 1.0f, 1.0f, 1.0f, 1.0f }, { texture_position[0], texture_position[1] } });
			}
		}
	}
}

const std::vector<vertex_t>& SceneData::GetTriangleData() {
	return triangle_data;
}

std::pair<std::size_t, std::size_t> SceneData::GetIndexPair(std::ifstream& input_stream) {
	std::size_t vertex_index, texture_index;
	input_stream >> vertex_index;
	input_stream.ignore(1); // Ignore '/' between the indices.
	input_stream >> texture_index;
	return { vertex_index, texture_index };
}
