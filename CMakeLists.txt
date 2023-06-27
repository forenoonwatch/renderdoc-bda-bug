cmake_minimum_required(VERSION 3.14)

project(
	RDBug VERSION 1.0
	LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

add_executable(${PROJECT_NAME} "")

set_target_properties(${PROJECT_NAME} PROPERTIES
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
set(GLFW_INSTALL OFF CACHE BOOL "")

set(VOLK_STATIC_DEFINES ON CACHE BOOL "")
set(VMA_STATIC_VULKAN_FUNCTIONS OFF CACHE BOOL "")

find_package(Vulkan REQUIRED)

add_subdirectory(glfw)
add_subdirectory(vk-bootstrap)
add_subdirectory(volk)

target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
target_link_libraries(${PROJECT_NAME} PRIVATE vk-bootstrap)
target_link_libraries(${PROJECT_NAME} PRIVATE volk)

target_sources(${PROJECT_NAME} PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/vma.cpp"
)

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/VulkanMemoryAllocator/include")

