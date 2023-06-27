#include <cstdio>

#include <volk.h>
#include <VkBootstrap.h>
#include <vk_mem_alloc.h>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

vkb::Instance g_instance;
vkb::PhysicalDevice g_physicalDevice;
vkb::Device g_device;
vkb::Swapchain g_swapchain;
std::vector<VkImage> g_swapchainImages;
std::vector<VkImageView> g_swapchainImageViews;
VmaAllocator g_allocator;
VkSurfaceKHR g_surface;
VkCommandPool g_commandPool;
VkFence g_fence;

GLFWwindow* g_window;

VkImage g_image;
VmaAllocation g_imageAllocation;

void init_image();

void render();

void init_vulkan();
void init_allocator();

int main() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	g_window = glfwCreateWindow(640, 480, "RenderDoc Bug", nullptr, nullptr);

	init_vulkan();
	init_allocator();

	init_image();

	while (!glfwWindowShouldClose(g_window)) {
		glfwPollEvents();
		render();
		glfwSwapBuffers(g_window);
	}

	// DEINIT
	vkDeviceWaitIdle(g_device);

	vmaDestroyImage(g_allocator, g_image, g_imageAllocation);

	for (auto& imageView : g_swapchainImageViews) {
		vkDestroyImageView(g_device, imageView, nullptr);
	}

	vkDestroyFence(g_device, g_fence, nullptr);
	vkDestroyCommandPool(g_device, g_commandPool, nullptr);
	vkDestroySwapchainKHR(g_device, g_swapchain, nullptr);
	vkDestroySurfaceKHR(g_instance, g_surface, nullptr);
	glfwDestroyWindow(g_window);
	vmaDestroyAllocator(g_allocator);
	vkDestroyDevice(g_device, nullptr);
	vkb::destroy_debug_utils_messenger(g_instance, g_instance.debug_messenger);
	vkDestroyInstance(g_instance, nullptr);
	glfwTerminate();
}

void init_image() {
	VkImageCreateInfo imageInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = VK_FORMAT_R8_UINT,
		.extent = {1, 1, 1},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
	};

	VmaAllocationCreateInfo allocInfo{
		.usage = VMA_MEMORY_USAGE_GPU_ONLY,
	};

	vmaCreateImage(g_allocator, &imageInfo, &allocInfo, &g_image, &g_imageAllocation, nullptr);
}

void render() {
	vkWaitForFences(g_device, 1, &g_fence, VK_TRUE, UINT64_MAX);
	vkResetFences(g_device, 1, &g_fence);

	uint32_t imageIndex{};
	vkAcquireNextImageKHR(g_device, g_swapchain, UINT64_MAX, VK_NULL_HANDLE, VK_NULL_HANDLE, &imageIndex);

	vkResetCommandPool(g_device, g_commandPool, 0);

	VkCommandBufferAllocateInfo cmdAllocInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = g_commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};
	VkCommandBuffer cmd;
	vkAllocateCommandBuffers(g_device, &cmdAllocInfo, &cmd);

	VkCommandBufferBeginInfo beginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	vkBeginCommandBuffer(cmd, &beginInfo);

	VkImageMemoryBarrier barrier{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = g_swapchainImages[imageIndex],
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		}
	};
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			0, 0, nullptr, 0, nullptr, 1, &barrier);

	vkEndCommandBuffer(cmd);

	VkSwapchainKHR swapchain = g_swapchain;
	VkPresentInfoKHR presentInfo{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.swapchainCount = 1,
		.pSwapchains = &swapchain,
		.pImageIndices = &imageIndex,
	};

	VkSubmitInfo submitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmd,
	};

	vkQueueSubmit(g_device.get_queue(vkb::QueueType::graphics).value(), 1, &submitInfo, g_fence);
	vkQueuePresentKHR(g_device.get_queue(vkb::QueueType::present).value(), &presentInfo);
}

void init_vulkan() {
	volkInitialize();

	g_instance = vkb::InstanceBuilder()
		.set_app_name("RenderDoc Bug")
		.request_validation_layers()
		.require_api_version(1, 2, 0)
		.use_default_debug_messenger()
		.build()
		.value();

	volkLoadInstance(g_instance);

	glfwCreateWindowSurface(g_instance, g_window, nullptr, &g_surface);

	VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddress{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
		.bufferDeviceAddress = VK_TRUE,
		.bufferDeviceAddressCaptureReplay = VK_TRUE,
	};

	g_physicalDevice = vkb::PhysicalDeviceSelector{g_instance, g_surface}
		.set_minimum_version(1, 2)
		.add_required_extension_features(bufferDeviceAddress)
		.select()
		.value();

	g_device = vkb::DeviceBuilder{g_physicalDevice}
		.build()
		.value();

	volkLoadDevice(g_device);

	g_swapchain = vkb::SwapchainBuilder{g_device, g_surface}
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.build()
		.value();

	g_swapchainImages = g_swapchain.get_images().value();
	g_swapchainImageViews = g_swapchain.get_image_views().value();

	VkCommandPoolCreateInfo commandPoolCreateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
		.queueFamilyIndex = g_device.get_queue_index(vkb::QueueType::graphics).value(),
	};
	vkCreateCommandPool(g_device, &commandPoolCreateInfo, nullptr, &g_commandPool);

	VkFenceCreateInfo fenceCreateInfo{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT,
	};
	vkCreateFence(g_device, &fenceCreateInfo, nullptr, &g_fence);
}

void init_allocator() {
	VmaVulkanFunctions vkFns{};
	vkFns.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vkFns.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
	vkFns.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
	vkFns.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
	vkFns.vkAllocateMemory = vkAllocateMemory;
	vkFns.vkFreeMemory = vkFreeMemory;
	vkFns.vkMapMemory = vkMapMemory;
	vkFns.vkUnmapMemory = vkUnmapMemory;
	vkFns.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
	vkFns.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
	vkFns.vkBindBufferMemory = vkBindBufferMemory;
	vkFns.vkBindImageMemory = vkBindImageMemory;
	vkFns.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
	vkFns.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
	vkFns.vkCreateBuffer = vkCreateBuffer;
	vkFns.vkDestroyBuffer = vkDestroyBuffer;
	vkFns.vkCreateImage = vkCreateImage;
	vkFns.vkDestroyImage = vkDestroyImage;
	vkFns.vkCmdCopyBuffer = vkCmdCopyBuffer;
	vkFns.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2;
	vkFns.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2;
	vkFns.vkBindBufferMemory2KHR = vkBindBufferMemory2;
	vkFns.vkBindImageMemory2KHR = vkBindImageMemory2;
	vkFns.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2;

	VmaAllocatorCreateInfo allocatorInfo{};
	allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	allocatorInfo.physicalDevice = g_physicalDevice;
	allocatorInfo.device = g_device;
	allocatorInfo.instance = g_instance;
	allocatorInfo.pVulkanFunctions = &vkFns;
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
	vmaCreateAllocator(&allocatorInfo, &g_allocator);
}

