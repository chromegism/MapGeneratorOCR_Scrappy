#pragma once

#include <vulkan/vulkan.h>

#include "Device.h"

class Buffer {
	const LogicalDevice* device_ = nullptr;			// non owning
	VkBuffer handle_ = VK_NULL_HANDLE;				// owning
	VkDeviceMemory memory_ = VK_NULL_HANDLE;		// owning

	VkDeviceSize size_ = 0;							// trivial
	VkBufferUsageFlags usageFlags_ = 0;				// trivial
	VkMemoryPropertyFlags memoryPropertyFlags_ = 0;	// trivial

	bool isMapped_ = false;							// trivial

	void exchangeHandles(Buffer& other) noexcept {
		device_ = std::exchange(other.device_, nullptr);
		handle_ = std::exchange(other.handle_, VK_NULL_HANDLE);
		memory_ = std::exchange(other.memory_, VK_NULL_HANDLE);
		size_ = other.size_;
		usageFlags_ = other.usageFlags_;
		memoryPropertyFlags_ = other.memoryPropertyFlags_;
		isMapped_ = other.isMapped_;
	}
	void clearHandles() noexcept {
		device_ = nullptr;
		handle_ = VK_NULL_HANDLE;
		memory_ = VK_NULL_HANDLE;
		size_ = 0;
		usageFlags_ = 0;
		memoryPropertyFlags_ = 0;
		isMapped_ = false;
	}

	void genBuffer(VkDeviceSize size);

	Buffer(const LogicalDevice& device, VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags) :
		device_(&device), size_(size), usageFlags_(usageFlags), memoryPropertyFlags_(memoryPropertyFlags)
	{
		genBuffer(size);
	}

public:
	Buffer() noexcept = default;
	Buffer(const Buffer&) noexcept = delete; // move only
	Buffer(Buffer&& other) noexcept {
		exchangeHandles(other);
	}
	Buffer& operator=(Buffer&& other) noexcept {
		if (this != &other) {
			destroy();

			exchangeHandles(other);
		}
		return *this;
	}

	const LogicalDevice& device() const noexcept { return *device_; }
	VkDevice deviceHandle() const noexcept { return device_->handle(); }
	VkBuffer handle() const noexcept { return handle_; }
	VkDeviceMemory memory() const noexcept { return memory_; }
	VkDeviceSize size() const noexcept { return size_; }
	VkBufferUsageFlags usageFlags() const noexcept { return usageFlags_; }
	VkMemoryPropertyFlags memoryPropertyFlags() const noexcept { return memoryPropertyFlags_; }
	bool isValid() const noexcept { return handle_ != VK_NULL_HANDLE; }
	bool isMapped() const noexcept { return isMapped_; }

	void destroy() noexcept {
		if (isMapped()) {
			unmapMemory();
		}
		if (isValid()) {
			vkDestroyBuffer(device_->handle(), handle_, nullptr);
			vkFreeMemory(device_->handle(), memory_, nullptr);
		}
		clearHandles();
	}

	void copyBuffer(const Buffer& other, VkCommandBuffer commandBuffer);

	template<NonPointer T>
	T* mapMemory(VkDeviceSize offset, VkDeviceSize size) noexcept {
		if (isMapped_) {
			unmapMemory();
			std::cerr << "Attempted to map buffer memory twice! Unmapping...\n";
		}
		void* data;
		vkMapMemory(device_->handle(), memory_, 0, size, 0, &data);
		isMapped_ = true;
		return static_cast<T*>(data);
	}

	template<NonPointer T>
	T* mapMemory(VkDeviceSize offset) noexcept  {
		return mapMemory<T>(offset, size_);
	}

	template<NonPointer T>
	T* mapMemory() noexcept  {
		return mapMemory<T>(0, size_);
	}

	void unmapMemory() noexcept  {
		isMapped_ = false;
		vkUnmapMemory(device_->handle(), memory_);
	}

	~Buffer() noexcept { destroy(); }

	static Buffer createStaging(const LogicalDevice& device, VkDeviceSize size) {
		return Buffer(device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}
	static Buffer createVertex(const LogicalDevice& device, VkDeviceSize size) {
		return Buffer(device, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}
	static Buffer createIndex(const LogicalDevice& device, VkDeviceSize size) {
		return Buffer(device, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}
	static Buffer createUniform(const LogicalDevice& device, VkDeviceSize size) {
		return Buffer(device, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}
};