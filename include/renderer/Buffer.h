#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>

#include <vector>
#include <array>
#include <map>

#include <StarryManager.h>

#include "VertexBufferData.h"

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
	class Device;

	class Buffer : public Manager::StarryAsset {
		public:
			Buffer();
			~Buffer();

			void init(size_t deviceUUID);
			void destroy();

			void loadData(VertexBufferData& data);
			void removeData(size_t id);

			void finalize();

			size_t getNumVertices() { return vertices.empty() ? 0 : vertices.size(); }
			size_t getNumIndices() { return indices.empty() ? 0 : indices.size(); }

			uint32_t bind(VkCommandBuffer commandBuffer);
			
			uint32_t getNumberSubBuffers() { return offsets[0].size(); }
			void recordSubBuffer(VkCommandBuffer commandBuffer, uint32_t index);

			virtual ASSET_NAME("Buffer")

		private:
			bool isReady = false;

			void loadBufferToMemory();

			void createBuffer();
			void createIndexBuffer();

			void fillBufferData(VkDeviceMemory& bufferMemory);
			void fillIndexBufferData(VkDeviceMemory& bufferMemory);

			std::map<size_t, VertexBufferData> bufferData;
			
			std::vector<Vertex> vertices;
			std::vector<uint32_t> indices;
			std::array<std::vector<uint32_t>, 2> offsets;
			std::array<std::vector<uint32_t>, 2> sizes;

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
/*

	float: VK_FORMAT_R32_SFLOAT
	vec2: VK_FORMAT_R32G32_SFLOAT
	vec3: VK_FORMAT_R32G32B32_SFLOAT
	vec4: VK_FORMAT_R32G32B32A32_SFLOAT

*/
