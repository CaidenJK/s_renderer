#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>

#include <vector>
#include <array>

#include <StarryManager.h>

// Helpful debug colors
#define RED_COLOR glm::vec3(1.0f, 0.0f, 0.0f)
#define GREEN_COLOR glm::vec3(0.0f, 1.0f, 0.0f)
#define BLUE_COLOR glm::vec3(0.0f, 0.0f, 1.0f)
#define CYAN_COLOR glm::vec3(0.0f, 1.0f, 1.0f)
#define MAGENTA_COLOR glm::vec3(1.0f, 0.0f, 1.0f)
#define YELLOW_COLOR glm::vec3(1.0f, 1.0f, 0.0f)
#define WHITE_COLOR glm::vec3(1.0f, 1.0f, 1.0f)
#define BLACK_COLOR glm::vec3(0.1f, 0.1f, 0.1f)

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

	class Device;

	class Buffer : public Manager::StarryAsset {
		public:
			Buffer();
			~Buffer();

			void init(uint64_t deviceUUID);
			void destroy();

			void add(Buffer& other);

			void loadData(const std::vector<Vertex>& verticiesInput, const std::vector<uint32_t>& indiciesInput);

			size_t getNumVertices() { return vertices.empty() ? 0 : vertices.size(); }
			size_t getNumIndices() { return indices.empty() ? 0 : indices.size(); }
			VkBuffer& getBuffer() { return buffer; }
			VkBuffer& getIndexBuffer() { return indexBuffer; }

			ASSET_NAME("Vertex Buffer")

		private:
			bool isReady = false;

			void loadBufferToMemory();

			void createBuffer();
			void createIndexBuffer();

			void fillBufferData(VkDeviceMemory& bufferMemory);
			void fillIndexBufferData(VkDeviceMemory& bufferMemory);

			std::vector<Vertex> vertices = {};
			std::vector<uint32_t> indices = {};

			VkBuffer stagingBufferVertex = VK_NULL_HANDLE;
			VkDeviceMemory stagingBufferMemoryVertex = VK_NULL_HANDLE;
			VkBuffer stagingBufferIndex = VK_NULL_HANDLE;
			VkDeviceMemory stagingBufferMemoryIndex = VK_NULL_HANDLE;

			VkBuffer buffer = VK_NULL_HANDLE;
			VkDeviceMemory bufferMemory = VK_NULL_HANDLE;

			VkBuffer indexBuffer = VK_NULL_HANDLE;
			VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

			VkDeviceSize bufferSizeVertex = 0;
			VkDeviceSize bufferSizeIndex = 0;

			Manager::ResourceHandle<Device> device;
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

/*

	float: VK_FORMAT_R32_SFLOAT
	vec2: VK_FORMAT_R32G32_SFLOAT
	vec3: VK_FORMAT_R32G32B32_SFLOAT
	vec4: VK_FORMAT_R32G32B32A32_SFLOAT

*/
