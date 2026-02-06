#pragma once

#include <StarryManager.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>

#include <vector>
#include <array>

namespace Render
{
    struct Vertex {
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec3 color;
		glm::vec2 texCoord;

		bool operator==(const Vertex& other) const;

		static VkVertexInputBindingDescription getBindingDescriptions();
		static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions();
	};

    class VertexBufferData
    {
        public:
            VertexBufferData();
            ~VertexBufferData();

            std::vector<Vertex>& getVertices() { return vertices; }
            std::vector<uint32_t>& getIndices() { return indices; }

            void setVertices(std::vector<Vertex>& data) { vertices = data; }
            void setIndices(std::vector<uint32_t>& data) { indices = data; }

            size_t getID() { return id; }

            bool isEmpty() { return vertices.empty() || indices.empty(); }

        private:
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;

            size_t id;

            static std::mt19937_64 randomGen;
    };
}

namespace std
{
	template<> struct std::hash<Render::Vertex>
	{
		size_t operator()(Render::Vertex const& vertex) const {
			return ((((hash<glm::vec3>()(vertex.position) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.normal) << 1);
		}
	};
}