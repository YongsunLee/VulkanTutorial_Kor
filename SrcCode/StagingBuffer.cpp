#ifdef _M_X64
	#pragma comment(lib, "vulkan-1.lib")
	#pragma comment(lib, "glfw3.lib")
#else
	#pragma comment(lib, "vulkan-1.lib")
	#pragma comment(lib, "glfw3.lib")
#endif

const int MAX_FRAMES_IN_FLIGHT = 2;

//#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <optional>
#include <set>
#include <cstdint>
#include <algorithm>
#include <fstream>

#include <glm.hpp>
#include <array>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;


	bool isComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;			// 기본 Surface 기능
	std::vector<VkSurfaceFormatKHR> formats;			// Surface 형식(format)
	std::vector<VkPresentModeKHR> presentModes;		// 사용 가능한 presentation 모드
};

struct Vertex {
	glm::vec2 pos;
	glm::vec3 color;

	// GPU 메모리에 업로드 된 후 버텍스 쉐이더에 전달하는 방법을 알리는 것
	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription = {};

		bindingDescription.binding = 0;											// 
		bindingDescription.stride = sizeof(Vertex);								// 한 항목에서 다음 항목까지의 바이트수
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;				// 각 정점 다음에 다음 데이터 입력으로 이동

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;									// vertex Shader Location 0번째 in
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;				// attribute의 데이터 유형 현재 vec2라는 의미
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;			// vec3
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}
};

// file Load
static std::vector<char> readFile(const std::string& filename)
{
	// ate 파일의 끝에서 읽기 시작
	// binary 파일을 이진 파일로 읽습니다.
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	// 끝에서 부터 읽는 이유 (파일크기 결정 및 버퍼 할당)
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	// 다시 처음으로 돌아가서 읽는다.
	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

// debug Messenger Addr 생성
VkResult CreateDebugUtilsMessengerEXT(
	VkInstance instance, 
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator, 
	VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	
	if (func != nullptr){
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

// debug Messenger 제거
void DestroyDebugUtilsMessengerEXT(
	VkInstance instance, 
	VkDebugUtilsMessengerEXT debugMessenger, 
	const VkAllocationCallbacks* pAllocatior)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

	if (func != nullptr){
		func(instance, debugMessenger, pAllocatior);
	}
}

class HellorTriangleApplication {
public:
	void run()
	{
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:
	void initWindow()
	{
		// window init
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", nullptr, nullptr);

		glfwSetWindowUserPointer(window, this);

		// Frame Buffer Resize
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	}


	
	void initVulkan()
	{
		// Instance 생성
		createInstance();

		// DebugMessenger
		setupDebugMessenger();

		// Surface
		createSurface();

		// Physical Device (그래픽카드)
		pickPhysicalDevice();

		// Logical Device
		createLogicalDevice();

		// Swap Chain
		createSwapChain();

		// Image View
		createImageViews();

		// Render pass
		createRenderPass();

		// Graphics PipeLine
		createGraphicsPipeline();

		// Frame Buffer
		createFrameBuffers();

		// Command Pool
		createCommandPool();

		// Vertex Buffer
		createVertexBuffer();

		// Command Buffer
		createCommandBuffers();

		// Semaphores
		createSyncObjects();
	}

	

	void createSurface()
	{
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}
	}

	void createLogicalDevice()
	{
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQeueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQeueueFamilies){
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures = {};

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		createInfo.pEnabledFeatures = &deviceFeatures;

		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();
		
		// 유효성 검사 계층
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
			throw std::runtime_error("failed to create logical device!");
		}

		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
	}

	void pickPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		if (deviceCount == 0){
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		for (const auto& device : devices) {
			if (isDeviceSuitable(device)) {
				physicalDevice = device;
				break;
			}
		}

		if (physicalDevice == VK_NULL_HANDLE) {
			throw std::runtime_error("failed to find a suitable GPU!");
		}
	}

	bool isDeviceSuitable(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices = findQueueFamilies(device);

		// SwapChain
		bool extensionSupported = checkDeviceExtensionSupport(device);

		bool swapChainAdequate = false;
		if (extensionSupported) {
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		return indices.isComplete() && extensionSupported && swapChainAdequate;
	}

	bool checkDeviceExtensionSupport(VkPhysicalDevice device) 
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const auto& extension : availableExtensions){
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	void mainLoop()
	{
		// 오류가 발생하거나 창이 닫힐때까지 루프
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
			drawFrame();
		}

		vkDeviceWaitIdle(device);
	}

	void drawFrame()
	{
		// 스왑체인에서 이미지 획득
		// 프레임 버퍼에서 attachments로 사용하여 command buffer 실행
		// 프레젠테이션을 위해 이미지 SwapChain으로 반환

		vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
		vkResetFences(device,1, &inFlightFences[currentFrame]);

		uint32_t imageIndex;
		// 논리장치, 스왑체인, 시간초과 (나노초 단위) UINT64_MAX : 시간 초과 비활성화
		// 프레젠테이션 엔진이 이미지사용을 마치면 신호를 받을 동기화 개체, 이미지 인덱스를 출력하는 변수(인덱스를 이용하여 올바른 명령버퍼를 선택한다.)
		VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
		
		// VK_ERROR_OUT_OF_DATE_KHR : 스왑체인이 표면과 호환되지 않아 더 이상 렌더링에 사용할 수 없는경우 recreateSwapChain 함수를 호출한다.
		// VK_SUBOPTIMAL_KHR : 스왑체인을 사용하여 surface에 성공적으로 표시할 수 있으나 surface 속성이 더이상 정확하게 일치하지 않음
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return;
		} else if ( result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
			vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
		}

		imagesInFlight[imageIndex] = inFlightFences[currentFrame];

		// 큐 제출 및 동기화
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame] };									// 대기할 세마포어
		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};		// 대기할 파이프라인의 단계 (Color Attachment 에 기록하는 그래픽 파이프라인 단계를 지정)
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

		VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		vkResetFences(device, 1, &inFlightFences[currentFrame]);

		// graphicsQueue에 Command buffer에 Presentation
		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = {swapChain};
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;

		//presentInfo.pResults = nullptr;															// 프레젠테이션에 성공한 경우 모든 개별 스왑체인을 확인하기 위해 VkResult 값의 배열을 사용가능

		result = vkQueuePresentKHR(presentQueue, &presentInfo);											// 이미지를 스왑체인에 제시하라는 요청을 제출한다.

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
			framebufferResized = false;
			recreateSwapChain();
		} else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}

		//vkQueueWaitIdle(presentQueue);

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void cleanup()
	{
		// 스왑체인 삭제
		cleanupSwapChain();

		// Vertex Buffer 삭제
		vkDestroyBuffer(device, vertexBuffer, nullptr);

		// vertex Buffer에 할당된 Mem
		vkFreeMemory(device, vertexBufferMemory, nullptr);

		// 세마포어 삭제
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(device, inFlightFences[i], nullptr);
		}
		
		// 커맨드 풀 삭제
		vkDestroyCommandPool(device, commandPool, nullptr);

		// vk Device 제거
		vkDestroyDevice(device, nullptr);

		// vk Debug Util Messenger EXT 파괴
		if (enableValidationLayers) {
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}

		// Surface 파괴
		vkDestroySurfaceKHR(instance, surface, nullptr);

		// vk instance 파괴
		vkDestroyInstance(instance, nullptr);
		
		// 리소스 파괴 및 GLFW 종료
		glfwDestroyWindow(window);

		glfwTerminate();
	}

	void createInstance()
	{
		if (enableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error("validation layers requested, but not available!");
		}

		// application Info
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		// 전역 확장 및 유효성 검사 계층을 알려주는 구조체
		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		auto extensions = getRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		// 유효성 검사 계층
		if (enableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else {
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		// 오류 확인
		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}
	}

	// 기본 Debug Messenger Info
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
	}

	void setupDebugMessenger()
	{
		if (!enableValidationLayers) return;

		// Vk Debug Messenger Info
		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		populateDebugMessengerCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug messenger!");
		}
	}

	std::vector<const char*> getRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (enableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	// 지원하는 유효성 레이어 체크
	bool checkValidationLayerSupport()
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : validationLayers) {
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0){
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				return false;
			}
		}

		return true;
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT  messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData
	){
		// pMessage 디버그 메세지 (null 종료 문자열)
		std::cerr << "validation Layer : " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}

	// 큐 패밀리
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

			if (presentSupport) {
				indices.presentFamily = i;
			}

			if (indices.isComplete()) {
				break;
			}
			i++;
		}

		return indices;
	}

	// 스왑체인 쿼리
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
		SwapChainSupportDetails details;

		// 기본 Surface 기능
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		// Surface Format
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		// 프레젠테이션 모드
		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	// 스왑체인 포멧
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{ 
		//VkSurfaceFormatKHR
		//Color Space 포함

		// SRGB가 사용가능한 경우 SRGB를 사용함.
		// why) 더욱 정확한 색상을 인식하기 때문이며 나중에 사용할 텍스처와 같이 이미지 표준 색상 공간과 거의 같기 때문
		for (const auto& availableFormat : availableFormats){
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	//스왑체인 모드
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) 
	{
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	// 스왑체인 capabilities
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilites)
	{
		if (capabilites.currentExtent.width != UINT32_MAX) {
			return capabilites.currentExtent;
		}
		else {
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);
			VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

			actualExtent.width = std::max(capabilites.minImageExtent.width, std::min(capabilites.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(capabilites.minImageExtent.height, std::min(capabilites.maxImageExtent.height, actualExtent.height));

			return actualExtent;
		}
	}

	void createSwapChain()
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		//스왑체인에 몇개의 이미지를 가지고 있는가?
		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		// SwapChain createInfo
		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;												//이미지가 구성되는 레이어의 양 (3D 응용프로그램이 아니라면 항상1)
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;					//Swap Chain에서 이미지를 사용할 작업 종류

		// 여러 큐 패밀리에서 사용될 스왑체인 이미지를 처리하는 방법을 지정
		// 그래픽스 큐 패밀리가 프레젠테이션 큐 패밀리와 다른 경우	(대부분의 하드웨어는 같다.)
		// 그래픽스 대기열에서 그린후 프리젠테이션에 제출
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;					// 한번에 하나의 큐 패밀리만 이미지를 소유 (소유권을 명시적으로 전송해야함)
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;					// 명시적 소유권 이전 없이 여러 큐 패밀리에서 사용
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;		// 이미지에 현재 Transform 적용
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;					// 알파 채널을 윈도우 시스템의 다른 윈도우와 블렌딩하는데 사용해야하는지의 여부지정
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;													// 다른 윈도우창에 가려진 부분의 픽셀을 신경쓰지 않음

		createInfo.oldSwapchain = VK_NULL_HANDLE;										// 새로 스왑체인을 작성해야 할때(윈도우창의 크기가 조절되거나 할때) 이젠 스왑체인에 대한 참조 처리

		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
			throw std::runtime_error("failed to create swap chain!");
		}

		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}

	// Image View
	void createImageViews()
	{
		swapChainImageViews.resize(swapChainImages.size());

		for (size_t i = 0; i < swapChainImages.size(); ++i) {
			// ImageView CreateInfo
			VkImageViewCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapChainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = swapChainImageFormat;
			// component
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			// subresourceRange
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;							// 0이 가장 해상도가 높은것
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;							// 3D응용 프로그램을 작업하는 경우 (ex)VR)
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create image views!");
			}
		}
	}

	// 랜더 패스
	void createRenderPass()
	{
		// Render Pass Color Attachment
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = swapChainImageFormat;								// 스왑체인 이미지의 Format과 일치해야함
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;							// 멀티 샘플링을 사용하는 경우 변경됨
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;						// 시작시 상수값을 지운다.
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;						// 렌더링 된 내용이 메모리에 저장되어 나중에 읽을 수 있다.
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;			
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;					// 랜더링 패스가 시작되기전에 이미지의 레이아웃
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;				// 스왑 체인에 표시될 이미지

		// Sub Pass
		// 서브패스의 attachment Ref
		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// Sub Pass
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		// 서브 패스 종속성
		VkSubpassDependency dependency = {};
		/* VK_SUBPASS_EXTERNAL 사용하지 않는 경우 dstSubpass는 srcSubpass 보다 항상 높아야 한다. */
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;									//어디에 지정되느냐에 따라 렌더패스 전/후에 대한 암시적 서브 패스를 의미
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstStageMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;


		if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render Pass!");
		}
	}

	// 그래픽스 파이프라인
	void createGraphicsPipeline()
	{
		auto vertexShaderCode = readFile("Shader/vert.spv");
		auto fragmentShaderCode = readFile("Shader/frag.spv");

		VkShaderModule vertexShaderModule = createShaderModule(vertexShaderCode);
		VkShaderModule fragmentShaderModule = createShaderModule(fragmentShaderCode);

		VkPipelineShaderStageCreateInfo vertexShaderStageInfo = {};
		vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexShaderStageInfo.module = vertexShaderModule;
		vertexShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragmentShaderStageInfo = {};
		fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentShaderStageInfo.module = fragmentShaderModule;
		fragmentShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = {vertexShaderStageInfo , fragmentShaderStageInfo};

		// Fixed Functions
		// Vertex Input
		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		
		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();

		vertexInputInfo.vertexBindingDescriptionCount = 1;										// 데이터 간 각겨과 데이터가 버텍스당 또는 인스턴스
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());									// 버텍스 셰이더에 전달될 속성의 유형
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		// Input assembly
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;							// 모든 삼각형의 두번째 점과 세번째 정점이 다음 삼각형의 두 정점으로 사용된다.
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		// Viewports and scissors
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapChainExtent.width;
		viewport.height = (float)swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = {0, 0};
		scissor.extent = swapChainExtent;

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		//Rasterizer
		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;												// Shadow Map과 같은 경우에는 VK_TRUE (GPU 기능 활성화 필요)
		rasterizer.rasterizerDiscardEnable = VK_FALSE;										// VK_TRUE시 지오메트리가 통과하지 못함
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;										// 다각형의 영역을 프래그먼트로 채움 (채우기 이외의 기능은 GPU 기능 활성 필요)
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;										// 컬링 유형 결정
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;										// 와인딩 오더
		rasterizer.depthBiasEnable = VK_FALSE;												// 일정한 값 또는 프래그먼트의 기울기를 기준으로 Depth값을 변경할 수 있다.
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;

		// Multisampling
		VkPipelineMultisampleStateCreateInfo multisampling = {};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;										// GPU 기능 활성 필요
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f;
		multisampling.pSampleMask = nullptr;
		multisampling.alphaToCoverageEnable = VK_FALSE;
		multisampling.alphaToOneEnable = VK_FALSE;

		// Depth and Stencil Testing
		

		// Color Blending
		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_TRUE;											// 알파 블렌딩
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		// Dynamic State
		VkDynamicState dynamicStates[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH
		};

		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = 2;
		dynamicState.pDynamicStates = dynamicStates;

		// Pipeline Layout
		// 세이더에 uniform values (ex) sampler)
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pSetLayouts = nullptr;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}

		// 그래픽스 파이프라인 설정
		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = nullptr;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create grahics pipeline!");
		}

		// Destroy
		vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
		vkDestroyShaderModule(device, vertexShaderModule, nullptr);
	}

	// ShaderModule
	VkShaderModule createShaderModule(const std::vector<char>& code)
	{
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}

		return shaderModule;
	}

	void createFrameBuffers()
	{
		swapChainFramebuffers.resize(swapChainImageViews.size());

		for (size_t i = 0; i < swapChainImageViews.size(); ++i) {
			VkImageView attachments[] = {
				swapChainImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;													// swapChain Image는 단일 이미지이기 때문에 레이어수는 1이다.

			if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create framebuffer!");
			}
		}
	}

	void createCommandPool()
	{
		QueueFamilyIndices QueueFamilyIndices = findQueueFamilies(physicalDevice);

		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = QueueFamilyIndices.graphicsFamily.value();			// 단일 유형의 큐에 제출된 명령버퍼만 할당 가능 (현재 그리기 명령 기록)
		poolInfo.flags = 0;																// 명령버퍼를 어떻게 기록할지 2가지 opt 제공
		
		if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool!");
		}
	}

	void createVertexBuffer()
	{
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemroy;

		// Staging buffer
		// 정점 데이터를 device local buffer로 옮기기 위해 사용한다.
		createBuffer(bufferSize, 
					 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,															// 메모리 전송작업에서 버퍼를 소스로 사용
					 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					 stagingBuffer,
					 stagingBufferMemroy
			);

		// 정점 데이터를 스테이징 버퍼에 복사
		void* data;
		vkMapMemory(device, stagingBufferMemroy, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), (size_t)bufferSize);
		vkUnmapMemory(device, stagingBufferMemroy);

		// vertex buffer 생성
		createBuffer(bufferSize,
					 VK_BUFFER_USAGE_TRANSFER_DST_BIT |	VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,						// 메모리 전송작업에서 버퍼를 대상으로 사용
					 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,														// Device Local 메모리 유형으로 지정							
					 vertexBuffer,
					 vertexBufferMemory
		);

		// Device Local 메모리 유형은 VkMapMemory를 사용할 수 없다.
		copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
		
		// staging 버퍼 삭제 , 스테이징 버퍼 메모리삭제
		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemroy, nullptr);
	}

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory) 
	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create vertex buffer!");
		}

		// 버텍스 버퍼 Memory
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate vertex buffer memory!");
		}

		vkBindBufferMemory(device, buffer, bufferMemory, 0);
	}

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		// 메모리 전송작업은 명령 버퍼를 사용하여 실행된다.
		// 임시 명령 버퍼를 할당해야 함 (메모리 할당 최적화를 위해 단기 버퍼에 별도의 명령 풀을 만들 수 있다.)
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;			//명령 버퍼를 한번만 사용하고 완료될때 까지 기다린다.
		
		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		// 복사할 영역의 구조체
		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = size;
		
		// 소스, 대상, 복사할 영역의 배열
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		// 복사 명령후 기록 중지
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);											// Fence를 사용하면 여러 전송을 동시 예약할 수 있다.

		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) 
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		// 메모리 유형 
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
			if (typeFilter & (1<<i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}

	void createCommandBuffers()
	{
		commandBuffers.resize(swapChainFramebuffers.size());

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;								// 실행을 위해 큐에 제출할 수 있지만 다른 명령버퍼에서 호출할 수 없음
		allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

		if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}

		for (size_t i = 0; i < commandBuffers.size(); ++i) {
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;			
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;														// command buffer 사용 방법 (3가지 opt)
			beginInfo.pInheritanceInfo = nullptr;										// 보조  명령 버퍼에만 관련됨
			
			if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
				throw std::runtime_error("failed to begin recording command buffer!");
			}

			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = renderPass;
			renderPassInfo.framebuffer = swapChainFramebuffers[i];
			renderPassInfo.renderArea.offset = { 0,0 };
			renderPassInfo.renderArea.extent = swapChainExtent;
			VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			// VK_SUBPASS_CONTENTS_INLINE : 렌더 패스 명령은 기본 명령 버퍼 자체에 포함되며 보조 명령버퍼는 실행되지 않는다.
			vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			// 두번째 파라메터 : 그래픽 or 컴퓨터 파이프라인인지 지정하는 변수
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			// Vertex Buffer Binding (Rendering)
			VkBuffer vertexBuffers[] = {vertexBuffer};
			VkDeviceSize offsets[] = {0};
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

			// 명령버퍼, 버텍스 갯수, 인스턴스 렌더링(사용하지 않는 경우 1), 버텍스 버퍼 오프셋, 인스턴스 렌더링 오프셋
			vkCmdDraw(commandBuffers[i], static_cast<uint32_t>(vertices.size()), 1, 0, 0);

			vkCmdEndRenderPass(commandBuffers[i]);

			if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to record command buffer!");
			}
		}
	}

	void createSyncObjects()
	{
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
		imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i])					 != VK_SUCCESS) {
				throw std::runtime_error("failed to craete semaphores!");
			}
		}
	}

	// 스왑체인의 정리
	void cleanupSwapChain()
	{
		// 프레임 버퍼 삭제
		for (auto framebuffer : swapChainFramebuffers) {
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		}

		// 커맨드 버퍼 초기화(Free)
		vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

		// 그래픽스 파이프라인 삭제
		vkDestroyPipeline(device, graphicsPipeline, nullptr);

		// PipeLine Layout 삭제
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

		// Render Pass 삭제
		vkDestroyRenderPass(device, renderPass, nullptr);

		// Image View 삭제
		for (auto imageView : swapChainImageViews) {
			vkDestroyImageView(device, imageView, nullptr);
		}

		// 스왑체인 삭제
		vkDestroySwapchainKHR(device, swapChain, nullptr);
	}

	// 스왑체인의 크기 변경 등에 의해 새로 만들어저야 하는 객체
	void recreateSwapChain()
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(device);

		cleanupSwapChain();

		createSwapChain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createFrameBuffers();
		createCommandBuffers();
	}

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		auto app = reinterpret_cast<HellorTriangleApplication*>(glfwGetWindowUserPointer(window));
		app->framebufferResized = true;
	}

private:
	GLFWwindow* window;
	
	// Instance
	VkInstance instance;
	
	// Debug Messenger
	VkDebugUtilsMessengerEXT debugMessenger;
	
	// device
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;

	// Surface
	VkSurfaceKHR surface;

	// Queue
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	// SwapChain
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	// ImageView
	std::vector<VkImageView> swapChainImageViews;

	// RenderPass
	VkRenderPass renderPass;

	// PipeLine
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	// FrameBuffer
	std::vector<VkFramebuffer> swapChainFramebuffers;

	// Command Pool
	VkCommandPool commandPool;

	// Command Buffer
	std::vector<VkCommandBuffer> commandBuffers;

	// Semaphores & Fence
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;

	// frame
	size_t currentFrame = 0;

	// resize
	bool framebufferResized = false;

	const std::vector<Vertex> vertices = {
		{ { 0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{ { 0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
		{ {-0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}}
	};

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
};

int main()
{
	HellorTriangleApplication app;

	try {
		app.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}