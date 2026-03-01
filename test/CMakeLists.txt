cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

project(rwin-test LANGUAGES C CXX VERSION 1.0.0 DESCRIPTION "C++ library for management of windows")

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/_lib)

# Now link against the library target (e.g., MyLib)
add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/main.cpp)
find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE rwin::rwin Vulkan::Vulkan)
if(UNIX) # or if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    get_target_property(RWIN_RES_DIRS rwin RESOURCE_DIRS)
    message(STATUS ${RWIN_RES_DIRS})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${RWIN_RES_DIRS}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>libdecor/plugins-1"
            VERBATIM
    )
endif()
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMAND_EXPAND_LISTS
    )
endif()