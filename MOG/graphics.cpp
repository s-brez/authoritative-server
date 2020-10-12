#include "graphics.h"


namespace Graphics
{



// the return value indicates whether the calling layer should abort the vulkan call
static VkBool32 VKAPI_PTR 
vulkan_debug_callback(	VkDebugReportFlagsEXT flags,
						VkDebugReportObjectTypeEXT /*objType*/, uint64_t /*obj*/, 
						size_t /*location*/, int32_t /*code*/, 
						const char* layer_prefix, const char* msg, void* /*user_data*/) 
{
	log("[graphics::vulkan::%s] %s\n", layer_prefix, msg);
	// todo(jbr) define out in release
	if (flags & ~(VK_DEBUG_REPORT_INFORMATION_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT))
	{
		DebugBreak();
	}
    return VK_FALSE;
}

static void create_buffer(VkBuffer* out_buffer, VkDeviceMemory* out_buffer_memory, 
	VkPhysicalDevice physical_device, VkDevice device, 
	VkBufferUsageFlags buffer_usage_flags, uint32 size)
{
	VkBufferCreateInfo buffer_create_info = {};
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.size = size;
	buffer_create_info.usage = buffer_usage_flags;
	buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBuffer buffer;
	VkResult result = vkCreateBuffer(device, &buffer_create_info, 0, &buffer);
	assert(result == VK_SUCCESS);

	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);

	VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &physical_device_memory_properties);

	constexpr uint32 c_required_memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	uint32 chosen_memory_type_index = (uint32)-1;
	for (uint32 i = 0; i < physical_device_memory_properties.memoryTypeCount; ++i) 
	{
	    if ((memory_requirements.memoryTypeBits & (1 << i)) && 
	    	(physical_device_memory_properties.memoryTypes[i].propertyFlags & c_required_memory_properties) == c_required_memory_properties) 
	    {
	    	chosen_memory_type_index = i;
	    	break;
	    }
	} // todo(jbr) compression

	assert(chosen_memory_type_index != (uint32)-1);

	VkMemoryAllocateInfo memory_alloc_info = {};
	memory_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_alloc_info.allocationSize = memory_requirements.size;
	memory_alloc_info.memoryTypeIndex = chosen_memory_type_index;

	VkDeviceMemory buffer_memory;
	result = vkAllocateMemory(device, &memory_alloc_info, 0, &buffer_memory);
	assert(result == VK_SUCCESS);

	vkBindBufferMemory(device, buffer, buffer_memory, 0);

	*out_buffer = buffer;
	*out_buffer_memory = buffer_memory;
}

static void copy_to_buffer(VkDevice device, VkDeviceMemory buffer_memory, void* src, uint32 size)
{
	void* dst;
	vkMapMemory(device, buffer_memory, 0, size, 0, &dst);
	memcpy(dst, src, size);
	vkUnmapMemory(device, buffer_memory);
}

static void create_cube_face(Vertex* vertices, uint16 vertex_offset, 
	uint16* indices, uint32 index_offset, 
	Vec_3f center,
	Vec_3f right,
	Vec_3f up,
	Vec_3f colour)
{
	Vec_3f half_right = vec_3f_mul(right, 0.5f);
	Vec_3f half_up = vec_3f_mul(up, 0.5f);

	// top left
	vertices[vertex_offset].pos = vec_3f_add(vec_3f_sub(center, half_right), half_up);
	vertices[vertex_offset].colour = colour;
	// top right
	vertices[vertex_offset + 1].pos = vec_3f_add(vec_3f_add(center, half_right), half_up);
	vertices[vertex_offset + 1].colour = colour;
	// bottom right
	vertices[vertex_offset + 2].pos = vec_3f_sub(vec_3f_add(center, half_right), half_up);
	vertices[vertex_offset + 2].colour = colour;
	// bottom left
	vertices[vertex_offset + 3].pos = vec_3f_sub(vec_3f_sub(center, half_right), half_up);
	vertices[vertex_offset + 3].colour = colour;

	indices[index_offset] = vertex_offset;
	indices[index_offset + 1] = vertex_offset + 1;
	indices[index_offset + 2] = vertex_offset + 2;
	indices[index_offset + 3] = vertex_offset;
	indices[index_offset + 4] = vertex_offset + 2;
	indices[index_offset + 5] = vertex_offset + 3;
}

void init(	State* out_state, 
			HWND window_handle, HINSTANCE instance, 
			uint32 window_width, uint32 window_height, 
			uint32 max_players,
			Linear_Allocator* allocator, Linear_Allocator* temp_allocator)
{
	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo instance_create_info = {};
	instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_create_info.pApplicationInfo = &app_info;
	#ifndef RELEASE
	instance_create_info.enabledLayerCount = 1;
	const char* validation_layers[] = {"VK_LAYER_LUNARG_standard_validation"};
	instance_create_info.ppEnabledLayerNames = validation_layers;
	#endif
	const char* enabled_extension_names[] = {VK_EXT_DEBUG_REPORT_EXTENSION_NAME, VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
	instance_create_info.enabledExtensionCount = sizeof(enabled_extension_names) / sizeof(enabled_extension_names[0]);
	instance_create_info.ppEnabledExtensionNames = enabled_extension_names;
	
	VkInstance vulkan_instance;
	VkResult result = vkCreateInstance(&instance_create_info, 0, &vulkan_instance);
	assert(result == VK_SUCCESS);

	VkDebugReportCallbackCreateInfoEXT debug_callback_create_info = {};
	debug_callback_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	debug_callback_create_info.flags = VK_DEBUG_REPORT_INFORMATION_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT;
	debug_callback_create_info.pfnCallback = vulkan_debug_callback;

	PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(vulkan_instance, "vkCreateDebugReportCallbackEXT");
	assert(vkCreateDebugReportCallbackEXT);

	VkDebugReportCallbackEXT debug_callback;
	result = vkCreateDebugReportCallbackEXT(vulkan_instance, &debug_callback_create_info, 0, &debug_callback);
	assert(result == VK_SUCCESS);

	VkWin32SurfaceCreateInfoKHR surface_create_info = {};
	surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surface_create_info.hwnd = window_handle;
	surface_create_info.hinstance = instance;

	PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(vulkan_instance, "vkCreateWin32SurfaceKHR");
	assert(vkCreateWin32SurfaceKHR);

	VkSurfaceKHR surface;
	result = vkCreateWin32SurfaceKHR(vulkan_instance, &surface_create_info, 0, &surface);
	assert(result == VK_SUCCESS);

	uint32 physical_device_count;
	result = vkEnumeratePhysicalDevices(vulkan_instance, &physical_device_count, 0);
	assert(result == VK_SUCCESS);
	assert(physical_device_count > 0);
	
	VkPhysicalDevice* physical_devices = (VkPhysicalDevice*)linear_allocator_alloc(temp_allocator, sizeof(VkPhysicalDevice) * physical_device_count);

	result = vkEnumeratePhysicalDevices(vulkan_instance, &physical_device_count, physical_devices);
	assert(result == VK_SUCCESS);

	VkPhysicalDevice chosen_physical_device = 0;
	VkPhysicalDeviceProperties chosen_physical_device_properties = {};
	VkSurfaceFormatKHR swapchain_surface_format = {};
	VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR ; // guaranteed to be supported
	uint32 swapchain_image_count = 0;
	for (uint32 i = 0; i < physical_device_count; ++i)
	{
		VkPhysicalDeviceProperties device_properties;
		//VkPhysicalDeviceFeatures device_features; TODO(jbr) pick best device based on type and features

		vkGetPhysicalDeviceProperties(physical_devices[i], &device_properties);
		//vkGetPhysicalDeviceFeatures(physical_devices[i], &device_features);

		// for now just try to pick a discrete gpu, otherwise anything
		if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			uint32 extension_count;
			result = vkEnumerateDeviceExtensionProperties(physical_devices[i], 0, &extension_count, 0);
			assert(result == VK_SUCCESS);

			VkExtensionProperties* device_extensions = (VkExtensionProperties*)linear_allocator_alloc(temp_allocator, sizeof(VkExtensionProperties) * extension_count);
			result = vkEnumerateDeviceExtensionProperties(physical_devices[i], 0, &extension_count, device_extensions);
			assert(result == VK_SUCCESS);

			for (uint32 j = 0; j < extension_count; ++j)
			{
				if (strcmp(device_extensions[j].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
				{
					chosen_physical_device = physical_devices[i];
					chosen_physical_device_properties = device_properties;
					break;
				}
			}

			if (chosen_physical_device)
			{
				uint32 format_count;
				uint32 present_mode_count;

				result = vkGetPhysicalDeviceSurfaceFormatsKHR(chosen_physical_device, surface, &format_count, 0);
				assert(result == VK_SUCCESS);

				result = vkGetPhysicalDeviceSurfacePresentModesKHR(chosen_physical_device, surface, &present_mode_count, 0);
				assert(result == VK_SUCCESS);

				if (format_count && present_mode_count) 
				{
					VkSurfaceFormatKHR* surface_formats = (VkSurfaceFormatKHR*)linear_allocator_alloc(temp_allocator, sizeof(VkSurfaceFormatKHR) * format_count);
				    result = vkGetPhysicalDeviceSurfaceFormatsKHR(chosen_physical_device, surface, &format_count, surface_formats);
				    assert(result == VK_SUCCESS);

				    if (format_count == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED)
				    {
				    	swapchain_surface_format.format = VK_FORMAT_R8G8B8A8_UNORM;
				    	swapchain_surface_format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
				    }
				    else
				    {
				    	// todo(jbr) choose best surface format
						swapchain_surface_format = surface_formats[0];
				    }

					VkPresentModeKHR* present_modes = (VkPresentModeKHR*)linear_allocator_alloc(temp_allocator, sizeof(VkPresentModeKHR) * present_mode_count);
				    result = vkGetPhysicalDeviceSurfacePresentModesKHR(chosen_physical_device, surface, &present_mode_count, present_modes);
				    assert(result == VK_SUCCESS);

				    for (uint32 j = 0; j < present_mode_count; ++j)
				    {
				    	if (present_modes[j] == VK_PRESENT_MODE_MAILBOX_KHR)
				    	{
				    		swapchain_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
				    	}
				    }

					VkSurfaceCapabilitiesKHR surface_capabilities = {};
					result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(chosen_physical_device, surface, &surface_capabilities);
					assert(result == VK_SUCCESS);
					
					if (surface_capabilities.currentExtent.width == 0xFFFFFFFF)
					{
						out_state->swapchain_extent = {window_width, window_height};
					}
					else
					{
						out_state->swapchain_extent = surface_capabilities.currentExtent;
					}

					if (surface_capabilities.maxImageCount == 0 || surface_capabilities.maxImageCount >= 3)
					{
						swapchain_image_count = 3;
					}
					else
					{
						swapchain_image_count = surface_capabilities.maxImageCount;
					}
				}
				else
				{
					// no formats, can't use this device
					chosen_physical_device = 0;
				}
			}
		}

		if (chosen_physical_device)
		{
			break;
		}
	}

	assert(chosen_physical_device);

	uint32 queue_family_count;
	vkGetPhysicalDeviceQueueFamilyProperties(chosen_physical_device, &queue_family_count, 0);
	assert(queue_family_count > 0);

	VkQueueFamilyProperties* queue_families = (VkQueueFamilyProperties*)linear_allocator_alloc(temp_allocator, sizeof(VkQueueFamilyProperties) * queue_family_count);

	vkGetPhysicalDeviceQueueFamilyProperties(chosen_physical_device, &queue_family_count, queue_families);

	uint32 graphics_queue_family_index = uint32(-1);
	uint32 present_queue_family_index = uint32(-1);
	for (uint32 i = 0; i < queue_family_count; ++i)
	{
		if (queue_families[i].queueCount > 0)
		{
			if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				graphics_queue_family_index = i;
			}

			VkBool32 present_support = false;
			result = vkGetPhysicalDeviceSurfaceSupportKHR(chosen_physical_device, i, surface, &present_support);
			assert(result == VK_SUCCESS);
			if (present_support)
			{
				present_queue_family_index = i;
			}

			if (graphics_queue_family_index != uint32(-1) && present_queue_family_index != uint32(-1))
			{
				break;
			}
		}
	}

	assert(graphics_queue_family_index != (uint32)-1);
	assert(present_queue_family_index != (uint32)-1);

	VkDeviceQueueCreateInfo device_queue_create_infos[2];
	device_queue_create_infos[0] = {};
	device_queue_create_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	device_queue_create_infos[0].queueFamilyIndex = graphics_queue_family_index;
	device_queue_create_infos[0].queueCount = 1;
	float32 queue_priority = 1.0f;
	device_queue_create_infos[0].pQueuePriorities = &queue_priority;

	uint32 queue_count = 1;
	if (graphics_queue_family_index != present_queue_family_index)
	{
		queue_count = 2;

		device_queue_create_infos[1] = {};
		device_queue_create_infos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		device_queue_create_infos[1].queueFamilyIndex = present_queue_family_index;
		device_queue_create_infos[1].queueCount = 1;
		device_queue_create_infos[1].pQueuePriorities = &queue_priority;
	}

	VkPhysicalDeviceFeatures device_features = {};
	device_features.depthClamp =VK_TRUE;

	VkDeviceCreateInfo device_create_info = {};
	device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.queueCreateInfoCount = queue_count;
	device_create_info.pQueueCreateInfos = device_queue_create_infos;
	device_create_info.pEnabledFeatures = &device_features;
	const char* enabled_device_extension_names[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
	device_create_info.enabledExtensionCount = sizeof(enabled_device_extension_names) / sizeof(enabled_device_extension_names[0]);
	device_create_info.ppEnabledExtensionNames = enabled_device_extension_names;

	result = vkCreateDevice(chosen_physical_device, &device_create_info, 0, &out_state->device);
	assert(result == VK_SUCCESS);
	
	vkGetDeviceQueue(out_state->device, graphics_queue_family_index, 0, &out_state->graphics_queue);

	if (graphics_queue_family_index != present_queue_family_index)
	{
		vkGetDeviceQueue(out_state->device, present_queue_family_index, 0, &out_state->present_queue);
	}
	else
	{
		out_state->present_queue = out_state->graphics_queue;
	}

	VkSwapchainCreateInfoKHR swapchain_create_info = {};
	swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_create_info.surface = surface;
	swapchain_create_info.minImageCount = swapchain_image_count;
	swapchain_create_info.imageFormat = swapchain_surface_format.format;
	swapchain_create_info.imageColorSpace = swapchain_surface_format.colorSpace;
	swapchain_create_info.imageExtent = out_state->swapchain_extent;
	swapchain_create_info.imageArrayLayers = 1;
	swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32 queue_family_indices[] = {graphics_queue_family_index, present_queue_family_index};
	swapchain_create_info.pQueueFamilyIndices = queue_family_indices;
	swapchain_create_info.queueFamilyIndexCount = queue_count;
	if (queue_count > 1) 
	{
	    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // todo(jbr) use exclusive and transfer owenership properly
	} 
	else 
	{
	    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	swapchain_create_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_create_info.presentMode = swapchain_present_mode;
	swapchain_create_info.clipped = VK_TRUE;

	result = vkCreateSwapchainKHR(out_state->device, &swapchain_create_info, 0, &out_state->swapchain);
	assert(result == VK_SUCCESS);

	// implementation can create more than the number requested, so get the image count again here
	result = vkGetSwapchainImagesKHR(out_state->device, out_state->swapchain, &swapchain_image_count, 0);
	assert(result == VK_SUCCESS);
	
	VkImage* swapchain_images = (VkImage*)linear_allocator_alloc(temp_allocator, sizeof(VkImage) * swapchain_image_count);
	result = vkGetSwapchainImagesKHR(out_state->device, out_state->swapchain, &swapchain_image_count, swapchain_images);
	assert(result == VK_SUCCESS);

	VkImageView* swapchain_image_views = (VkImageView*)linear_allocator_alloc(temp_allocator, sizeof(VkImageView) * swapchain_image_count);
	
	VkImageViewCreateInfo image_view_create_info = {};
	image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	image_view_create_info.format = swapchain_surface_format.format;
	image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	image_view_create_info.subresourceRange.baseMipLevel = 0;
	image_view_create_info.subresourceRange.levelCount = 1;
	image_view_create_info.subresourceRange.baseArrayLayer = 0;
	image_view_create_info.subresourceRange.layerCount = 1;
	for (uint32 i = 0; i <  swapchain_image_count; ++i)
	{
		image_view_create_info.image = swapchain_images[i];
	
		result = vkCreateImageView(out_state->device, &image_view_create_info, 0, &swapchain_image_views[i]);
		assert(result == VK_SUCCESS);
	}

	// depth buffer
	// first find the format to use
	VkFormat depth_buffer_formats[6] = // formats in order of preference 
	{
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_X8_D24_UNORM_PACK32,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM,
		VK_FORMAT_D16_UNORM_S8_UINT
	};
	VkFormat chosen_depth_buffer_format = VK_FORMAT_UNDEFINED;
	for (uint32 i = 0; i < 6; ++i)
	{
		VkFormatProperties format_properties;
		vkGetPhysicalDeviceFormatProperties(
			chosen_physical_device, 
			depth_buffer_formats[i], 
			&format_properties);
		if (format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			chosen_depth_buffer_format = depth_buffer_formats[i];
			break;
		}
	}
	assert(chosen_depth_buffer_format != VK_FORMAT_UNDEFINED);

	// create image
	VkImageCreateInfo depth_buffer_image_create_info = {};
	depth_buffer_image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	depth_buffer_image_create_info.imageType = VK_IMAGE_TYPE_2D;
	depth_buffer_image_create_info.format = chosen_depth_buffer_format;
	depth_buffer_image_create_info.extent.width = out_state->swapchain_extent.width;
	depth_buffer_image_create_info.extent.height = out_state->swapchain_extent.height;
	depth_buffer_image_create_info.extent.depth = 1;
	depth_buffer_image_create_info.mipLevels = 1;
	depth_buffer_image_create_info.arrayLayers = 1;
	depth_buffer_image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
	depth_buffer_image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depth_buffer_image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	depth_buffer_image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkImage depth_buffer_image;
	result = vkCreateImage(out_state->device, &depth_buffer_image_create_info, 0, &depth_buffer_image);
	assert(result == VK_SUCCESS);

	// allocate memory for depth image
	VkMemoryRequirements depth_buffer_image_memory_reqs;
	vkGetImageMemoryRequirements(out_state->device, depth_buffer_image, &depth_buffer_image_memory_reqs);

	VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
	vkGetPhysicalDeviceMemoryProperties(chosen_physical_device, &physical_device_memory_properties);

	uint32 chosen_memory_type_index = (uint32)-1;
	for (uint32 i = 0; i < physical_device_memory_properties.memoryTypeCount; ++i) 
	{
	    if ((depth_buffer_image_memory_reqs.memoryTypeBits & (1 << i)) && 
	    	(physical_device_memory_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) 
	    {
	    	chosen_memory_type_index = i;
	    	break;
	    }
	} // todo(jbr) compression

	assert(chosen_memory_type_index != (uint32)-1);

	VkMemoryAllocateInfo depth_buffer_mem_alloc = {};
	depth_buffer_mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	depth_buffer_mem_alloc.allocationSize = depth_buffer_image_memory_reqs.size;
	depth_buffer_mem_alloc.memoryTypeIndex = chosen_memory_type_index;
	VkDeviceMemory depth_buffer_mem;
	result = vkAllocateMemory(out_state->device, &depth_buffer_mem_alloc, 0, &depth_buffer_mem);
	assert(result == VK_SUCCESS);

	// bind memory to depth buffer image
	vkBindImageMemory(out_state->device, depth_buffer_image, depth_buffer_mem, 0);

	// craete depth buffer image view
	VkImageViewCreateInfo depth_buffer_image_view_info = {};
	depth_buffer_image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	depth_buffer_image_view_info.image = depth_buffer_image;
	depth_buffer_image_view_info.format = chosen_depth_buffer_format;
	depth_buffer_image_view_info.components.r = VK_COMPONENT_SWIZZLE_R;
	depth_buffer_image_view_info.components.g = VK_COMPONENT_SWIZZLE_G;
	depth_buffer_image_view_info.components.b = VK_COMPONENT_SWIZZLE_B;
	depth_buffer_image_view_info.components.a = VK_COMPONENT_SWIZZLE_A;
	depth_buffer_image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	depth_buffer_image_view_info.subresourceRange.baseMipLevel = 0;
	depth_buffer_image_view_info.subresourceRange.levelCount = 1;
	depth_buffer_image_view_info.subresourceRange.baseArrayLayer = 0;
	depth_buffer_image_view_info.subresourceRange.layerCount = 1;
	depth_buffer_image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;

	VkImageView depth_buffer_image_view;
	result = vkCreateImageView(out_state->device, &depth_buffer_image_view_info, 0, &depth_buffer_image_view);
	assert(result == VK_SUCCESS);

	// load shaders
	HANDLE file = CreateFileA("shaders/shader.vert", GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	assert(file != INVALID_HANDLE_VALUE);
	DWORD vert_shader_size = GetFileSize(file, 0);
	assert(vert_shader_size != INVALID_FILE_SIZE);
	uint8* vert_shader_bytes = linear_allocator_alloc(temp_allocator, vert_shader_size);
	bool32 read_success = ReadFile(file, vert_shader_bytes, vert_shader_size, 0, 0);
	assert(read_success);

	file = CreateFileA("shaders/shader.frag", GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	assert(file != INVALID_HANDLE_VALUE);
	DWORD frag_shader_size = GetFileSize(file, 0);
	assert(frag_shader_size != INVALID_FILE_SIZE);
	uint8* frag_shader_bytes = linear_allocator_alloc(temp_allocator, frag_shader_size);
	read_success = ReadFile(file, frag_shader_bytes, frag_shader_size, 0, 0);
	assert(read_success);

	VkShaderModuleCreateInfo shader_module_create_info = {};
	shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shader_module_create_info.codeSize = vert_shader_size;
	shader_module_create_info.pCode = (uint32_t*)vert_shader_bytes;
	VkShaderModule vert_shader_module;
	result = vkCreateShaderModule(out_state->device, &shader_module_create_info, 0, &vert_shader_module);
	assert(result == VK_SUCCESS);

	shader_module_create_info = {};
	shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shader_module_create_info.codeSize = frag_shader_size;
	shader_module_create_info.pCode = (uint32_t*)frag_shader_bytes;
	VkShaderModule frag_shader_module;
	result = vkCreateShaderModule(out_state->device, &shader_module_create_info, 0, &frag_shader_module);
	assert(result == VK_SUCCESS);

	VkPipelineShaderStageCreateInfo shader_stage_create_info[2];
	shader_stage_create_info[0] = {};
	shader_stage_create_info[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage_create_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shader_stage_create_info[0].module = vert_shader_module;
	shader_stage_create_info[0].pName = "main";
	
	shader_stage_create_info[1] = {};
	shader_stage_create_info[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage_create_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shader_stage_create_info[1].module = frag_shader_module;
	shader_stage_create_info[1].pName = "main";

	VkVertexInputBindingDescription vertex_input_binding_desc = {};
	vertex_input_binding_desc.binding = 0;
	vertex_input_binding_desc.stride = sizeof(Vertex);
	vertex_input_binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription vertex_input_attribute_desc[2];
	vertex_input_attribute_desc[0] = {};
	vertex_input_attribute_desc[0].binding = 0;
	vertex_input_attribute_desc[0].location = 0;
	vertex_input_attribute_desc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertex_input_attribute_desc[0].offset = 0;
	vertex_input_attribute_desc[1] = {};
	vertex_input_attribute_desc[1].binding = 0;
	vertex_input_attribute_desc[1].location = 1;
	vertex_input_attribute_desc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertex_input_attribute_desc[1].offset = 12;

	VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = 1;
	vertex_input_info.pVertexBindingDescriptions = &vertex_input_binding_desc;
	vertex_input_info.vertexAttributeDescriptionCount = 2;
	vertex_input_info.pVertexAttributeDescriptions = vertex_input_attribute_desc;

	VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info = {};
	input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)out_state->swapchain_extent.width;
	viewport.height = (float)out_state->swapchain_extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = {0, 0};
	scissor.extent = out_state->swapchain_extent;

	VkPipelineViewportStateCreateInfo viewport_create_info = {};
	viewport_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_create_info.viewportCount = 1;
	viewport_create_info.pViewports = &viewport;
	viewport_create_info.scissorCount = 1;
	viewport_create_info.pScissors = &scissor;

 	VkPipelineRasterizationStateCreateInfo rasteriser_create_info = {};
 	rasteriser_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
 	rasteriser_create_info.polygonMode = VK_POLYGON_MODE_FILL;
 	rasteriser_create_info.lineWidth = 1.0f;
 	rasteriser_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
 	rasteriser_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
 	rasteriser_create_info.depthClampEnable = VK_TRUE;

	VkPipelineMultisampleStateCreateInfo multisampling_create_info = {};
	multisampling_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colour_blend_attachment = {};
	colour_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
											VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colour_blend_attachment.blendEnable = VK_FALSE;
	colour_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
	colour_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
	colour_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colour_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colour_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colour_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

	VkPipelineColorBlendStateCreateInfo colour_blend_state_create_info = {};
	colour_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colour_blend_state_create_info.attachmentCount = 1;
	colour_blend_state_create_info.pAttachments = &colour_blend_attachment;
	colour_blend_state_create_info.logicOpEnable = VK_FALSE;
	colour_blend_state_create_info.logicOp = VK_LOGIC_OP_NO_OP;
	colour_blend_state_create_info.blendConstants[0] = 1.0f;
	colour_blend_state_create_info.blendConstants[1] = 1.0f;
	colour_blend_state_create_info.blendConstants[2] = 1.0f;
	colour_blend_state_create_info.blendConstants[3] = 1.0f;

	VkPipelineDepthStencilStateCreateInfo depth_state_create_info;
	depth_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_state_create_info.pNext = NULL;
	depth_state_create_info.flags = 0;
	depth_state_create_info.depthTestEnable = VK_TRUE;
	depth_state_create_info.depthWriteEnable = VK_TRUE;
	depth_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depth_state_create_info.depthBoundsTestEnable = VK_FALSE;
	depth_state_create_info.minDepthBounds = 0;
	depth_state_create_info.maxDepthBounds = 0;
	depth_state_create_info.stencilTestEnable = VK_FALSE;
	depth_state_create_info.back.failOp = VK_STENCIL_OP_KEEP;
	depth_state_create_info.back.passOp = VK_STENCIL_OP_KEEP;
	depth_state_create_info.back.compareOp = VK_COMPARE_OP_ALWAYS;
	depth_state_create_info.back.compareMask = 0;
	depth_state_create_info.back.reference = 0;
	depth_state_create_info.back.depthFailOp = VK_STENCIL_OP_KEEP;
	depth_state_create_info.back.writeMask = 0;
	depth_state_create_info.front = depth_state_create_info.back;

	VkDescriptorSetLayoutBinding descriptor_set_layout_binding = {};
    descriptor_set_layout_binding.binding = 0;
    descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descriptor_set_layout_binding.descriptorCount = 1;
    descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	descriptor_set_layout_binding.pImmutableSamplers = 0;

	VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {};
	descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptor_set_layout_create_info.bindingCount = 1;
	descriptor_set_layout_create_info.pBindings = &descriptor_set_layout_binding;

	VkDescriptorSetLayout descriptor_set_layout;
	result = vkCreateDescriptorSetLayout(out_state->device, &descriptor_set_layout_create_info, 0, &descriptor_set_layout);
	assert(result == VK_SUCCESS);

	VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
	pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_create_info.setLayoutCount = 1;
	pipeline_layout_create_info.pSetLayouts = &descriptor_set_layout;

	result = vkCreatePipelineLayout(out_state->device, &pipeline_layout_create_info, 0, &out_state->pipeline_layout);
	assert(result == VK_SUCCESS);

	VkAttachmentDescription attachments[2];
	attachments[0] = {};
    attachments[0].format = swapchain_surface_format.format;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	attachments[1] = {};
	attachments[1].format = chosen_depth_buffer_format;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colour_attachment_ref = {};
	colour_attachment_ref.attachment = 0;
	colour_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_attachment_ref = {};
	depth_attachment_ref.attachment = 1;
	depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass_desc = {};
	subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass_desc.colorAttachmentCount = 1;
	subpass_desc.pColorAttachments = &colour_attachment_ref;
	subpass_desc.pDepthStencilAttachment = &depth_attachment_ref;

	VkRenderPassCreateInfo render_pass_create_info = {};
	render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_create_info.attachmentCount = 2;
	render_pass_create_info.pAttachments = attachments;
	render_pass_create_info.subpassCount = 1;
	render_pass_create_info.pSubpasses = &subpass_desc;

	result = vkCreateRenderPass(out_state->device, &render_pass_create_info, 0, &out_state->render_pass);
	assert(result == VK_SUCCESS);

	VkGraphicsPipelineCreateInfo pipeline_create_info = {};
	pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_create_info.stageCount = 2;
	pipeline_create_info.pStages = shader_stage_create_info;
	pipeline_create_info.pVertexInputState = &vertex_input_info;
	pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
	pipeline_create_info.pViewportState = &viewport_create_info;
	pipeline_create_info.pRasterizationState = &rasteriser_create_info;
	pipeline_create_info.pMultisampleState = &multisampling_create_info;
	pipeline_create_info.pColorBlendState = &colour_blend_state_create_info;
	pipeline_create_info.pDepthStencilState = &depth_state_create_info;
	pipeline_create_info.layout = out_state->pipeline_layout;
	pipeline_create_info.renderPass = out_state->render_pass;
	pipeline_create_info.subpass = 0;
	
	result = vkCreateGraphicsPipelines(out_state->device, 0, 1, &pipeline_create_info, 0, &out_state->graphics_pipeline);
	assert(result == VK_SUCCESS);
	
	out_state->swapchain_framebuffers = (VkFramebuffer*)linear_allocator_alloc(allocator, sizeof(VkFramebuffer) * swapchain_image_count);
	VkImageView framebuffer_attachments[2];
	framebuffer_attachments[1] = depth_buffer_image_view;
	VkFramebufferCreateInfo framebuffer_create_info = {};
	framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebuffer_create_info.renderPass = out_state->render_pass;
	framebuffer_create_info.attachmentCount = 2;
	framebuffer_create_info.pAttachments = framebuffer_attachments;
	framebuffer_create_info.width = out_state->swapchain_extent.width;
	framebuffer_create_info.height = out_state->swapchain_extent.height;
	framebuffer_create_info.layers = 1;
	for (uint32 i = 0; i < swapchain_image_count; ++i)
	{
		framebuffer_attachments[0] = swapchain_image_views[i];
		result = vkCreateFramebuffer(out_state->device, &framebuffer_create_info, 0, &out_state->swapchain_framebuffers[i]);
		assert(result == VK_SUCCESS);
	}

	out_state->num_matrix_buffer_padding_bytes = (uint32)((((sizeof(Matrix_4x4) / chosen_physical_device_properties.limits.minUniformBufferOffsetAlignment) + 1) * chosen_physical_device_properties.limits.minUniformBufferOffsetAlignment) - sizeof(Matrix_4x4));
	const uint32 c_matrix_buffer_size = (sizeof(Matrix_4x4) + out_state->num_matrix_buffer_padding_bytes) * (max_players + 1);
	create_buffer(	&out_state->matrix_buffer, 
					&out_state->matrix_buffer_memory,
					chosen_physical_device, 
					out_state->device, 
					VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
					c_matrix_buffer_size);

	// Create descriptor set to store the projection matrix
	VkDescriptorPoolSize descriptor_pool_size = {};
	descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	descriptor_pool_size.descriptorCount = 1;

	VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
	descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptor_pool_create_info.flags = 0;
	descriptor_pool_create_info.maxSets = 1;
	descriptor_pool_create_info.pPoolSizes = &descriptor_pool_size;
	descriptor_pool_create_info.poolSizeCount = 1;

	VkDescriptorPool descriptor_pool;
	result = vkCreateDescriptorPool(out_state->device, &descriptor_pool_create_info, 0, &descriptor_pool);
	assert(result == VK_SUCCESS);

	VkDescriptorSetAllocateInfo descriptor_set_alloc_info = {};
	descriptor_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptor_set_alloc_info.descriptorPool = descriptor_pool;
	descriptor_set_alloc_info.descriptorSetCount = 1;
	descriptor_set_alloc_info.pSetLayouts = &descriptor_set_layout;

	result = vkAllocateDescriptorSets(out_state->device,
									&descriptor_set_alloc_info,
									&out_state->descriptor_set);
	assert(result == VK_SUCCESS);

	VkDescriptorBufferInfo buffer_info = {};
	buffer_info.buffer = out_state->matrix_buffer;
	buffer_info.offset = 0;
	buffer_info.range = VK_WHOLE_SIZE;

	VkWriteDescriptorSet write_descriptor_set = {};
	write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_set.dstSet = out_state->descriptor_set;
	write_descriptor_set.dstBinding = 0;
	write_descriptor_set.dstArrayElement = 0;
	write_descriptor_set.descriptorCount = 1;
	write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	write_descriptor_set.pImageInfo = 0;
	write_descriptor_set.pBufferInfo = &buffer_info;
	write_descriptor_set.pTexelBufferView = 0;

	uint32 descriptor_write_count = 1;
	const VkWriteDescriptorSet* descriptor_writes = &write_descriptor_set;
	uint32 descriptor_copy_count = 0;
	const VkCopyDescriptorSet* descriptor_copies = 0;
	vkUpdateDescriptorSets(	out_state->device,
							descriptor_write_count,
							descriptor_writes,
							descriptor_copy_count,
							descriptor_copies);

	// create a command pool per swapchain image, so they can be reset per swapchain image
	out_state->command_pools = (VkCommandPool*)linear_allocator_alloc(allocator, sizeof(VkCommandPool) * swapchain_image_count);
	out_state->command_buffers = (VkCommandBuffer*)linear_allocator_alloc(allocator, sizeof(VkCommandBuffer) * swapchain_image_count);
	out_state->command_buffers_in_use = (bool32*)linear_allocator_alloc(allocator, sizeof(bool32) * swapchain_image_count);
	out_state->fences = (VkFence*)linear_allocator_alloc(allocator, sizeof(VkFence) * swapchain_image_count);

	VkCommandPoolCreateInfo command_pool_create_info = {};
	command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_create_info.queueFamilyIndex = graphics_queue_family_index;

	VkCommandBufferAllocateInfo command_buffer_alloc_info = {};
	command_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_buffer_alloc_info.commandBufferCount = 1;
	VkFenceCreateInfo fence_create_info = {};
	fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	for (uint32 i = 0; i < swapchain_image_count; ++i)
	{
		// create the pool
		result = vkCreateCommandPool(out_state->device, &command_pool_create_info, 0, &out_state->command_pools[i]);
		assert(result == VK_SUCCESS);

		// create the single command buffer that will get reset each frame
		command_buffer_alloc_info.commandPool = out_state->command_pools[i];
		result = vkAllocateCommandBuffers(out_state->device, &command_buffer_alloc_info, &out_state->command_buffers[i]);
		assert(result == VK_SUCCESS);

		out_state->command_buffers_in_use[i] = 0;

		result = vkCreateFence(out_state->device, &fence_create_info, 0, &out_state->fences[i]);
		assert(result == VK_SUCCESS);
	}

	VkSemaphoreCreateInfo semaphore_create_info = {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
	result = vkCreateSemaphore(out_state->device, &semaphore_create_info, 0, &out_state->image_available_semaphore);
	assert(result == VK_SUCCESS);

	result = vkCreateSemaphore(out_state->device, &semaphore_create_info, 0, &out_state->render_finished_semaphore);
	assert(result == VK_SUCCESS);

	// Create cube mesh
	constexpr uint32 c_num_vertices = 24;
	constexpr uint32 c_num_indices = 36;
	constexpr uint32 c_cube_vertex_buffer_size = c_num_vertices * sizeof(Vertex);
	constexpr uint32 c_cube_index_buffer_size = c_num_indices * sizeof(uint16);
	Vertex* vertices = (Vertex*)linear_allocator_alloc(temp_allocator, c_cube_vertex_buffer_size);
	uint16* indices = (uint16*)linear_allocator_alloc(temp_allocator, c_cube_index_buffer_size);

	// front face
	constexpr float32 c_size = 1.0f;
	Vec_3f center = vec_3f(0.0f, -0.5f, 0.0f);
	Vec_3f colour = vec_3f(1.0f, 0.0f, 0.0f);
	Vec_3f right = vec_3f(c_size, 0.0f, 0.0f);
	Vec_3f up = vec_3f(0.0f, 0.0f, c_size);
	create_cube_face(vertices, 0, indices, 0, center, right, up, colour);
	// back face
	center = vec_3f(0.0f, 0.5f, 0.0f);
	colour = vec_3f(0.0f, 1.0f, 0.0f);
	right = vec_3f(-c_size, 0.0f, 0.0f);
	create_cube_face(vertices, 4, indices, 6, center, right, up, colour);
	// left face
	center = vec_3f(-0.5f, 0.0f, 0.0f);
	colour = vec_3f(0.0f, 0.0f, 1.0f);
	right = vec_3f(0.0f, -c_size, 0.0f);
	create_cube_face(vertices, 8, indices, 12, center, right, up, colour);
	// right face
	center = vec_3f(0.5f, 0.0f, 0.0f);
	colour = vec_3f(1.0f, 1.0f, 0.0f);
	right = vec_3f(0.0f, c_size, 0.0f);
	create_cube_face(vertices, 12, indices, 18, center, right, up, colour);
	// bottom face
	center = vec_3f(0.0f, 0.0f, -0.5f);
	colour = vec_3f(0.0f, 1.0f, 1.0f);
	right = vec_3f(c_size, 0.0f, 0.0f);
	up = vec_3f(0.0f, -c_size, 0.0f);
	create_cube_face(vertices, 16, indices, 24, center, right, up, colour);
	// top face
	center = vec_3f(0.0f, 0.0f, 0.5f);
	colour = vec_3f(1.0f, 0.0f, 1.0f);
	right = vec_3f(-c_size, 0.0f, 0.0f);
	create_cube_face(vertices, 20, indices, 30, center, right, up, colour);

	// Create vertex buffer
	VkDeviceMemory cube_vertex_buffer_memory;
	create_buffer(&out_state->cube_vertex_buffer, &cube_vertex_buffer_memory, 
		chosen_physical_device, out_state->device, 
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, c_cube_vertex_buffer_size);

	copy_to_buffer(out_state->device, cube_vertex_buffer_memory, (void*)vertices, c_cube_vertex_buffer_size);

	// Create index buffer
	VkDeviceMemory cube_index_buffer_memory;
	create_buffer(&out_state->cube_index_buffer, &cube_index_buffer_memory,
		chosen_physical_device, out_state->device,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT, c_cube_index_buffer_size);

	copy_to_buffer(out_state->device, cube_index_buffer_memory, (void*)indices, c_cube_index_buffer_size);

	out_state->cube_num_indices = c_num_indices;

	// Create scenery, just need something to give a sense of movement
	constexpr uint32 c_floor_tiles_count = 50;
	constexpr uint32 c_floor_tiles_total = c_floor_tiles_count * c_floor_tiles_count;
	constexpr float32 c_floor_tile_size = 1.0f;
	constexpr float32 c_floor_tile_spacing = 0.5f;
	constexpr uint32 c_num_scenery_vertices = c_floor_tiles_total * 4;
	constexpr uint32 c_num_scenery_indices = c_floor_tiles_total * 6;
	constexpr uint32 c_scenery_vertex_buffer_size = c_num_scenery_vertices * sizeof(Vertex);
	constexpr uint32 c_scenery_index_buffer_size = c_num_scenery_indices * sizeof(uint16);
	vertices = (Vertex*)linear_allocator_alloc(temp_allocator, c_scenery_vertex_buffer_size);
	indices = (uint16*)linear_allocator_alloc(temp_allocator, c_scenery_index_buffer_size);
	up = vec_3f(0.0f, 1.0f, 0.0f);
	right = vec_3f(1.0f, 0.0f, 0.0f);
	colour = vec_3f(1.0f, 1.0f, 1.0f);
	uint16 vertex_offset = 0;
	uint32 index_offset = 0;
	for (uint32 x = 0; x < c_floor_tiles_count; ++x)
	{
		for (uint32 y = 0; y < c_floor_tiles_count; ++y)
		{
			center.x = (x - ((c_floor_tiles_count - 1) / 2.0f)) * (c_floor_tile_size + c_floor_tile_spacing);
			center.y = (y - ((c_floor_tiles_count - 1) / 2.0f)) * (c_floor_tile_size + c_floor_tile_spacing);
			center.z = -0.5f;
			create_cube_face(vertices, vertex_offset, indices, index_offset, center, right, up, colour);
			vertex_offset += 4;
			index_offset += 6;
		}
	}
	
	VkDeviceMemory scenery_vertex_buffer_memory;
	VkDeviceMemory scenery_index_buffer_memory;
	
	create_buffer(&out_state->scenery_vertex_buffer, &scenery_vertex_buffer_memory, 
		chosen_physical_device, out_state->device, 
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, c_scenery_vertex_buffer_size);

	copy_to_buffer(out_state->device, scenery_vertex_buffer_memory, (void*)vertices, c_scenery_vertex_buffer_size);
	
	create_buffer(&out_state->scenery_index_buffer, &scenery_index_buffer_memory,
		chosen_physical_device, out_state->device,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT, c_scenery_index_buffer_size);

	copy_to_buffer(out_state->device, scenery_index_buffer_memory, (void*)indices, c_scenery_index_buffer_size);

	out_state->scenery_num_indices = c_num_scenery_indices;
}

void update_and_draw(State* state, Matrix_4x4* matrices, uint32 num_players)
{
	if (!num_players)
	{
		return;
	}

	// copy matrix uniform data to buffer
	const uint32 c_matrix_data_size = (num_players + 1) * sizeof(matrices[0]);
	uint8* dst;
	vkMapMemory(state->device, state->matrix_buffer_memory, 0, c_matrix_data_size, 0, (void**)&dst);
	for (uint32 i = 0; i < (num_players + 1); ++i)
	{
		uint32 offset = i * (sizeof(matrices[0]) + state->num_matrix_buffer_padding_bytes);
		memcpy(&dst[offset], &matrices[i], sizeof(matrices[0]));
	}
	vkUnmapMemory(state->device, state->matrix_buffer_memory);

	// get the swapchain image to use
	uint32 image_index;
    VkResult result = vkAcquireNextImageKHR(state->device, state->swapchain, (uint64)-1, state->image_available_semaphore, 0, &image_index);
    assert(result == VK_SUCCESS);

	// wait for it...
	VkFence fence = state->fences[image_index];
	if (state->command_buffers_in_use[image_index])
	{
		vkWaitForFences(state->device, 1, &fence, true, (uint64)-1);
		vkResetFences(state->device, 1, &fence);

		vkResetCommandPool(state->device, state->command_pools[image_index], 0);
	}
	else
	{
		state->command_buffers_in_use[image_index] = 1;
	}

    // write command buffer
    VkCommandBufferBeginInfo command_buffer_begin_info = {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    vkBeginCommandBuffer(state->command_buffers[image_index], &command_buffer_begin_info);

    VkClearValue clear_colours[2];
    clear_colours[0].color = {0.0f, 0.0f, 0.0f, 1.0f}; // colour
    clear_colours[1].depthStencil.depth = 1.0f;
    clear_colours[1].depthStencil.stencil = 0;
    VkRenderPassBeginInfo render_pass_begin_info = {};
	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin_info.renderPass = state->render_pass;
	render_pass_begin_info.framebuffer = state->swapchain_framebuffers[image_index];
	render_pass_begin_info.renderArea.offset = {0, 0};
	render_pass_begin_info.renderArea.extent = state->swapchain_extent;
	render_pass_begin_info.clearValueCount = 2;
	render_pass_begin_info.pClearValues = clear_colours;
	vkCmdBeginRenderPass(state->command_buffers[image_index], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(state->command_buffers[image_index], VK_PIPELINE_BIND_POINT_GRAPHICS, state->graphics_pipeline);

	// draw scenery
	// todo(jbr) this is all just awful, use push constants or something
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(state->command_buffers[image_index], 0, 1, &state->scenery_vertex_buffer, &offset);
	vkCmdBindIndexBuffer(state->command_buffers[image_index], state->scenery_index_buffer, 0, VK_INDEX_TYPE_UINT16);
	VkPipelineBindPoint pipeline_bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
	uint32 first_set = 0;
	uint32 descriptor_set_count = 1;
	uint32 dynamic_offset_count = 1;
	uint32 dynamic_offset = 0;
	vkCmdBindDescriptorSets(state->command_buffers[image_index],
							pipeline_bind_point,
							state->pipeline_layout,
							first_set,
							descriptor_set_count,
							&state->descriptor_set,
							dynamic_offset_count,
							&dynamic_offset);
	vkCmdDrawIndexed(state->command_buffers[image_index], state->scenery_num_indices, 1, 0, 0, 0);

	// draw players
	vkCmdBindVertexBuffers(state->command_buffers[image_index], 0, 1, &state->cube_vertex_buffer, &offset);
	vkCmdBindIndexBuffer(state->command_buffers[image_index], state->cube_index_buffer, 0, VK_INDEX_TYPE_UINT16);

	for (uint32 i = 0; i < num_players; ++i)
	{
		first_set = 0;
		descriptor_set_count = 1;
		dynamic_offset_count = 1;
		dynamic_offset = (i + 1) * (sizeof(matrices[0]) + state->num_matrix_buffer_padding_bytes);
		vkCmdBindDescriptorSets(state->command_buffers[image_index],
								pipeline_bind_point,
								state->pipeline_layout,
								first_set,
								descriptor_set_count,
								&state->descriptor_set,
								dynamic_offset_count,
								&dynamic_offset);

		vkCmdDrawIndexed(state->command_buffers[image_index], state->cube_num_indices, 1, 0, 0, 0);
	}

	vkCmdEndRenderPass(state->command_buffers[image_index]);

	result = vkEndCommandBuffer(state->command_buffers[image_index]);
	assert(result == VK_SUCCESS);


    VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &state->image_available_semaphore;
	VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submit_info.pWaitDstStageMask = &wait_stage;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &state->command_buffers[image_index];
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &state->render_finished_semaphore;
	result = vkQueueSubmit(state->graphics_queue, 1, &submit_info, fence);
	assert(result == VK_SUCCESS);


	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &state->render_finished_semaphore;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &state->swapchain;
	present_info.pImageIndices = &image_index;
	result = vkQueuePresentKHR(state->present_queue, &present_info);
	assert(result == VK_SUCCESS);
}

} // namespace Graphics