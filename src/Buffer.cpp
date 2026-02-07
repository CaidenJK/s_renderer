#include "Buffer.h"

#include "Device.h"

#define ERROR_VOLATILE(x) x; if (getAlertSeverity() == FATAL) { return; }

namespace Render 
{
	Buffer::Buffer()
	{
	}

	Buffer::~Buffer() 
	{
		destroy();
	}

	void Buffer::init(size_t deviceUUID)
	{
		device = Request<Device>(deviceUUID, "self");
	}

	void Buffer::destroy()
	{
		if (device) {
			if (buffer != VK_NULL_HANDLE) {
				vkDestroyBuffer((*device).getDevice(), buffer, nullptr);
				buffer = VK_NULL_HANDLE;
				vkFreeMemory((*device).getDevice(), bufferMemory, nullptr);
				bufferMemory = VK_NULL_HANDLE;
			}

			if (indexBuffer != VK_NULL_HANDLE) {
				vkDestroyBuffer((*device).getDevice(), indexBuffer, nullptr);
				indexBuffer = VK_NULL_HANDLE;
				vkFreeMemory((*device).getDevice(), indexBufferMemory, nullptr);
				indexBufferMemory = VK_NULL_HANDLE;
			}

			if (stagingBufferVertex != VK_NULL_HANDLE) {
				vkDestroyBuffer((*device).getDevice(), stagingBufferVertex, nullptr);
				stagingBufferVertex = VK_NULL_HANDLE;
			}
			if (stagingBufferMemoryVertex != VK_NULL_HANDLE) {
				vkFreeMemory((*device).getDevice(), stagingBufferMemoryVertex, nullptr);
				stagingBufferMemoryVertex = VK_NULL_HANDLE;
			}
			if (stagingBufferIndex != VK_NULL_HANDLE) {
				vkDestroyBuffer((*device).getDevice(), stagingBufferIndex, nullptr);
				stagingBufferIndex = VK_NULL_HANDLE;
			}
			if (stagingBufferMemoryIndex != VK_NULL_HANDLE) {
				vkFreeMemory((*device).getDevice(), stagingBufferMemoryIndex, nullptr);
				stagingBufferMemoryIndex = VK_NULL_HANDLE;
			}
		}
		isReady = false;
	}

	uint32_t Buffer::bind(VkCommandBuffer commandBuffer)
	{
		VkBuffer buffers[] = { buffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		return getNumberSubBuffers();
	}

	void Buffer::recordSubBuffer(VkCommandBuffer commandBuffer, uint32_t index)
	{
		vkCmdDrawIndexed(commandBuffer, sizes[1][index], 1, offsets[1][index], offsets[0][index], 0);
	}

	void Buffer::loadData(VertexBufferData& data) 
	{
		bufferData[data.getID()] = data;
		isReady = false;
	}

	void Buffer::removeData(size_t id)
	{
		bufferData.erase(id);
		isReady = false;
	}

	void Buffer::finalize()
	{
		vertices.clear();
		indices.clear();
		offsets[0].clear();
		offsets[1].clear();
		sizes[0].clear();
		sizes[1].clear();

		for(auto& data : bufferData) {
			offsets[0].push_back(vertices.size());
			sizes[0].push_back(data.second.getVertices().size());
			vertices.insert(vertices.end(), data.second.getVertices().begin(), data.second.getVertices().end());

			offsets[1].push_back(indices.size());
			sizes[1].push_back(data.second.getIndices().size());
			indices.insert(indices.end(), data.second.getIndices().begin(), data.second.getIndices().end());
		}

		loadBufferToMemory();
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