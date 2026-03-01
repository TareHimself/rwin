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

if(LINUX)
    find_package(wayland COMPONENTS wayland-client REQUIRED)
    find_package(wayland-protocols REQUIRED)
    find_package(xkbcommon REQUIRED)
    find_package(libdecor REQUIRED)
    include(FindPkgConfig)
    pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)
    find_program(WAYLAND_SCANNER_EXECUTABLE wayland-scanner)

    set(PROTOCOLS
            stable/xdg-shell/xdg-shell.xml
            unstable/xdg-decoration/xdg-decoration-unstable-v1.xml
            #    unstable/relative-pointer/relative-pointer-unstable-v1.xml
            #    unstable/pointer-constraints/pointer-constraints-unstable-v1.xml
            #    stable/viewporter/viewporter.xml
    )

    set(WAYLAND_GENERATED_HEADERS "")
    set(WAYLAND_GENERATED_SOURCES "")
    set(WAYLAND_GENERATED_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/external/wayland")
    file(MAKE_DIRECTORY ${WAYLAND_GENERATED_OUTPUT_DIR})
    foreach(xml IN LISTS PROTOCOLS)
        get_filename_component(name "${xml}" NAME_WE)
        string(REPLACE "-" "_" cname ${name})  # safer variable names
        set(header "${WAYLAND_GENERATED_OUTPUT_DIR}/${name}-client-protocol.h")
        set(source "${WAYLAND_GENERATED_OUTPUT_DIR}/${name}-protocol.c")

        list(APPEND WAYLAND_GENERATED_HEADERS ${header})
        list(APPEND WAYLAND_GENERATED_SOURCES ${source})

        if(NOT EXISTS "${header}")
            message(STATUS "Generating ${name}")
            execute_process(COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header ${WAYLAND_PROTOCOLS_DIR}/${xml} ${header})
            if(NOT EXISTS "${header}")
                message(FATAL_ERROR "Failed to generate ${name}")
            endif()
        endif()

        if(NOT EXISTS "${source}")
            message(STATUS "Generating ${name}")
            execute_process(COMMAND ${WAYLAND_SCANNER_EXECUTABLE} private-code  ${WAYLAND_PROTOCOLS_DIR}/${xml} ${source})
            if(NOT EXISTS "${source}")
                message(FATAL_ERROR "Failed to generate ${name}")
            endif()
        endif()

        message(STATUS "Generated wayland protocol: ${name}")
    endforeach()

    target_sources(${PROJECT_NAME} PRIVATE ${WAYLAND_GENERATED_SOURCES})
    target_include_directories(${PROJECT_NAME} PRIVATE ${WAYLAND_GENERATED_OUTPUT_DIR})
    target_link_libraries(${PROJECT_NAME} wayland::wayland-client wayland-protocols::wayland-protocols libdecor::libdecor xkbcommon::xkbcommon)
#    set(LIBDECOR_RESOURCES $<IF:$<CONFIG:Debug>,${libdecor_RES_DIRS_DEBUG},${libdecor_RES_DIRS_RELEASE}>)
    set(LIBDECOR_RESOURCES $<IF:$<CONFIG:Debug>,"A","B">)
    set_property(TARGET rwin PROPERTY RESOURCE_DIRS
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/share/libdecor/plugins-1>$<BUILD_INTERFACE:$<IF:$<CONFIG:Debug>,${libdecor_RES_DIRS_DEBUG},${libdecor_RES_DIRS_RELEASE}>>
    )
    install(
        DIRECTORY LIBDECOR_RESOURCES
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/libdecor
    )

endif()

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


