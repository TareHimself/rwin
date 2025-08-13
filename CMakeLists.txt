cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

set(RWIN_VERSION "1.0.0" CACHE STRING "rwin version")
project(rwin LANGUAGES C CXX VERSION ${RWIN_VERSION} DESCRIPTION "C++ library for management of windows")

file(GLOB_RECURSE SOURCE_FILES "${CMAKE_CURRENT_LIST_DIR}/lib/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/lib/*.h")

add_library(${PROJECT_NAME} ${SOURCE_FILES})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE RWIN_DX_PRODUCER)
endif()

find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)
include(GNUInstallDirs)


# Install headers
install(
    DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install the shared/static library
install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}    # .so/.dylib
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}    # .lib (static or import)
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}    # .dll
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install the CMake config package files
install(
    EXPORT ${PROJECT_NAME}-targets
    FILE ${PROJECT_NAME}Config.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)


