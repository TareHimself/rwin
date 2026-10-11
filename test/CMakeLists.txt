cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

project(rwin-test LANGUAGES C CXX VERSION 1.0.0 DESCRIPTION "C++ library for management of windows")

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/_lib)

# Now link against the library target (e.g., MyLib)
if(APPLE)
    add_executable(${PROJECT_NAME}  MACOSX_BUNDLE  ${CMAKE_CURRENT_LIST_DIR}/main.cpp)
    set_target_properties(${PROJECT_NAME} PROPERTIES
            MACOSX_BUNDLE_BUNDLE_IDENTIFIER "com.example.${PROJECT_NAME}"
            MACOSX_BUNDLE_BUNDLE_NAME "${PROJECT_NAME}"
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.example.${PROJECT_NAME}"
    )
else()
    add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/main.cpp)
endif()
find_package(Vulkan REQUIRED)

include(FetchContent)
FetchContent_Declare(imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.91.9
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)
add_library(imgui_vulkan STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)
target_include_directories(imgui_vulkan PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui_vulkan PUBLIC Vulkan::Vulkan)

target_link_libraries(${PROJECT_NAME} PRIVATE rwin::rwin Vulkan::Vulkan imgui_vulkan)
if(LINUX AND NOT RWIN_PLATFORM_COMPAT)
    get_target_property(RWIN_RES_DIRS rwin RESOURCE_DIRS)
    message(STATUS ${RWIN_RES_DIRS})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${RWIN_RES_DIRS}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/libdecor/plugins-1"
            VERBATIM
    )
endif()
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMAND_EXPAND_LISTS
    )
endif()