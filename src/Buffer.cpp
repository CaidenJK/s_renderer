#include "Buffer.h"

#include "Device.h"

#define ERROR_VOLATILE(x) x; if (getAlertSeverity() == FATAL) { return; }

namespace Render 
{
	bool Vertex::operator==(const Vertex& other) const
	{
		return position == other.position && color == other.color && texCoord == other.texCoord;
	}

	VkVertexInputBindingDescription Vertex::getBindingDescriptions()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}
	std::array<VkVertexInputAttributeDescription, 4> Vertex::getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, position);

		attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, normal);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, color);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
	}

	Buffer::Buffer()
	{
	}

	Buffer::~Buffer() 
	{
		destroy();
	}

	void Buffer::init(uint64_t deviceUUID)
	{
		device = Request<Device>(deviceUUID, "self");

		loadBufferToMemory();
	}

	void Buffer::destroy()
	{
		if (device) {
			if (buffer != VK_NULL_HANDLE) {
				vkDestroyBuffer((*device).getDevice(), buffer, nullptr);
				vkFreeMemory((*device).getDevice(), bufferMemory, nullptr);
			}

			if (indexBuffer != VK_NULL_HANDLE) {
				vkDestroyBuffer((*device).getDevice(), indexBuffer, nullptr);
				vkFreeMemory((*device).getDevice(), indexBufferMemory, nullptr);
			}

			if (stagingBufferVertex != VK_NULL_HANDLE) {
				vkDestroyBuffer((*device).getDevice(), stagingBufferVertex, nullptr);
			}
			if (stagingBufferMemoryVertex != VK_NULL_HANDLE) {
				vkFreeMemory((*device).getDevice(), stagingBufferMemoryVertex, nullptr);
			}
			if (stagingBufferIndex != VK_NULL_HANDLE) {
				vkDestroyBuffer((*device).getDevice(), stagingBufferIndex, nullptr);
			}
			if (stagingBufferMemoryIndex != VK_NULL_HANDLE) {
				vkFreeMemory((*device).getDevice(), stagingBufferMemoryIndex, nullptr);
			}
		}
		isReady = false;
	}

	void Buffer::add(Buffer& other)
	{
		isReady = false;

		destroy();

		vertices.insert(vertices.end(), other.vertices.begin(), other.vertices.end());
		indices.insert(indices.end(), other.indices.begin(), other.indices.end());
	}

	void Buffer::loadData(const std::vector<Vertex>& verticiesInput, const std::vector<uint32_t>& indiciesInput) 
	{
		vertices = verticiesInput;
		indices = indiciesInput;
		isReady = false;
	}

	void Buffer::createBuffer() 
	{
		if (vertices.empty()) {
			Alert("No vertex data loaded into Buffer!", FATAL);
			return;
		}

		bufferSizeVertex = sizeof(vertices[0]) * vertices.size();

		if (device.wait() != Manager::State::YES) {
			Alert("Device not avalible!", FATAL);
			return;
		}

		(*device).createBuffer(bufferSizeVertex, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBufferVertex, stagingBufferMemoryVertex);
		fillBufferData(stagingBufferMemoryVertex);

		if (buffer == VK_NULL_HANDLE) {
			(*device).createBuffer(bufferSizeVertex, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, bufferMemory);
		}
	}

	void Buffer::createIndexBuffer() 
	{
		if (indices.empty()) {
			Alert("No index data loaded into Buffer!", FATAL);
			return;
		}
		bufferSizeIndex = sizeof(indices[0]) * indices.size();

		if (device.wait() != Manager::State::YES) {
			Alert("Device not avalible!", FATAL);
			return;
		}

		(*device).createBuffer(bufferSizeIndex, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBufferIndex, stagingBufferMemoryIndex);
		fillIndexBufferData(stagingBufferMemoryIndex);

		if (indexBuffer == VK_NULL_HANDLE) {
			(*device).createBuffer(bufferSizeIndex, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
		}
	}

	void Buffer::loadBufferToMemory()
	{
		createBuffer();
		createIndexBuffer();

		if (stagingBufferVertex == VK_NULL_HANDLE ||
			stagingBufferIndex == VK_NULL_HANDLE ||
			stagingBufferMemoryVertex == VK_NULL_HANDLE ||
			stagingBufferMemoryIndex == VK_NULL_HANDLE ||
			buffer == VK_NULL_HANDLE ||
			indexBuffer == VK_NULL_HANDLE) {
				Alert("Load Buffer called before all buffers were created!", CRITICAL);
				return;
		}

		(*device).copyBuffer(stagingBufferVertex, buffer, bufferSizeVertex);
		(*device).copyBuffer(stagingBufferIndex, indexBuffer, bufferSizeIndex);
		
		if (device.wait() != Manager::State::YES) {
			Alert("Device died before it was ready to be used.", FATAL);
			return;
		}
		vkDestroyBuffer((*device).getDevice(), stagingBufferVertex, nullptr);
		vkFreeMemory((*device).getDevice(), stagingBufferMemoryVertex, nullptr);
		stagingBufferVertex = VK_NULL_HANDLE;
		stagingBufferMemoryVertex = VK_NULL_HANDLE;

		vkDestroyBuffer((*device).getDevice(), stagingBufferIndex, nullptr);
		vkFreeMemory((*device).getDevice(), stagingBufferMemoryIndex, nullptr);
		stagingBufferIndex = VK_NULL_HANDLE;
		stagingBufferMemoryIndex = VK_NULL_HANDLE;

		isReady = true;
	}

	void Buffer::fillBufferData(VkDeviceMemory& bufferMemory)
	{
		if (bufferMemory == VK_NULL_HANDLE) {
			Alert("Vertex buffer not created before filling data!", FATAL);
			return;
		}
		if (vertices.empty()) {
			Alert("No vertex data loaded into Buffer!", FATAL);
			return;
		}

		void* data;

		if (device.wait() != Manager::State::YES) {
			Alert("Device died before it was ready to be used.", FATAL);
			return;
		}
		vkMapMemory((*device).getDevice(), bufferMemory, 0, bufferSizeVertex, 0, &data);
		memcpy(data, vertices.data(), (size_t)bufferSizeVertex);
		vkUnmapMemory((*device).getDevice(), bufferMemory);
	}

	void Buffer::fillIndexBufferData(VkDeviceMemory& bufferMemory) 
	{
		if (bufferMemory == VK_NULL_HANDLE) {
			Alert("Vertex buffer not created before filling data!", FATAL);
			return;
		}
		if (indices.empty()) {
			Alert("No vertex data loaded into Buffer!", FATAL);
			return;
		}

		void* data;

		if (device.wait() != Manager::State::YES) {
			Alert("Device died before it was ready to be used.", FATAL);
			return;
		}
		vkMapMemory((*device).getDevice(), bufferMemory, 0, bufferSizeIndex, 0, &data);
		memcpy(data, indices.data(), (size_t)bufferSizeIndex);
		vkUnmapMemory((*device).getDevice(), bufferMemory);
	}
}
// Possibly sendData() with asset handler