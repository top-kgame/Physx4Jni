cmake_minimum_required(VERSION 3.16)
project(PhysxAPI
        VERSION 1.0.0
        DESCRIPTION "PhysX JNI API Wrapper"
        LANGUAGES CXX
)

# C++ 标准（目标级设置更清晰，此处保留全局以兼容子项目）
set(CMAKE_CXX_STANDARD 20)
#必须使用指定的 C++ 标准
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#关闭编译器特有的 C++ 扩展
set(CMAKE_CXX_EXTENSIONS OFF)

# 默认构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

# 平台与输出目录
if(WIN32)
    set(PLATFORM_PREFIX "windows")
    set(PLATFORM_NAME "win.x86_64.vc143.mt")
    set(PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/windows)
elseif(UNIX AND NOT APPLE)
    set(PLATFORM_PREFIX "linux")
    set(PLATFORM_NAME "linux.x86_64")
    set(PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/linux)
elseif(APPLE)
    set(PLATFORM_PREFIX "mac")
    # Apple Silicon (M 系列) 为 arm64；Intel Mac 为 x86_64
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        set(PLATFORM_NAME "mac.aarch64")
    else()
        set(PLATFORM_NAME "mac.x86_64")
    endif()
    set(PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/mac)
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

# PhysX 构建类型：与 CMAKE_BUILD_TYPE 对齐
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(PHYSX_BUILD_TYPE "debug")
else()
    set(PHYSX_BUILD_TYPE "release")
endif()

# 统一输出目录
set(BUILD_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build/${PLATFORM_PREFIX}-${CMAKE_BUILD_TYPE}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BUILD_OUTPUT_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BUILD_OUTPUT_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BUILD_OUTPUT_DIR}/lib)
foreach(CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG_TYPE} CONFIG_TYPE_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${BUILD_OUTPUT_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${BUILD_OUTPUT_DIR}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${BUILD_OUTPUT_DIR}/lib)
endforeach()

# PhysX路径设置
set(PHYSX_HOME "${CMAKE_CURRENT_SOURCE_DIR}/../physx")
set(PHYSX_BUILD_DIR "${PHYSX_HOME}/bin/${PLATFORM_NAME}/${PHYSX_BUILD_TYPE}")

# 验证PhysX路径
if(NOT EXISTS "${PHYSX_HOME}/include")
    message(FATAL_ERROR "PhysX include directory not found: ${PHYSX_HOME}/include")
endif()

if(NOT EXISTS "${PHYSX_BUILD_DIR}")
    message(FATAL_ERROR "PhysX build directory not found: ${PHYSX_BUILD_DIR}")
endif()

message(STATUS "PhysxAPI: ${CMAKE_SYSTEM_NAME} | ${CMAKE_BUILD_TYPE} | out=${BUILD_OUTPUT_DIR} | PhysX=${PHYSX_BUILD_DIR}")

# 源文件组织
set(MAIN_SOURCES
        source/PhysxApi.cpp
        source/PhysxApi.h
        source/scene/Scene.cpp
        source/scene/Scene.h
        source/Math.h
        source/object/SceneObj.cpp
        source/object/SceneObj.h
        source/PhysxApiCommon.h
        source/scene/QueryResult.cpp
        source/scene/QueryResult.h
        source/object/SceneStaticObj.cpp
        source/object/SceneStaticObj.h
        source/PhysxApiAllocator.cpp
        source/PhysxApiAllocator.h
)

# 创建共享库
add_library(PhysxAPI SHARED
        ${MAIN_SOURCES}
)

# 设置库属性
set_target_properties(PhysxAPI PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        OUTPUT_NAME "PhysxAPI"
        CLEAN_DIRECT_OUTPUT 1
)

# 包含目录设置
target_include_directories(PhysxAPI
        PRIVATE
        ${PROJECT_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include  # 添加根include目录
        ${PHYSX_HOME}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/source
)

# 编译定义
target_compile_definitions(PhysxAPI
        PRIVATE
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
        $<$<CONFIG:Debug>:PX_SUPPORT_PVD>
)

# 编译器特定设置
if(MSVC)
    # 使用静态运行时库
    target_compile_options(PhysxAPI
            PRIVATE
            $<$<CONFIG:Debug>:/MTd>
            $<$<CONFIG:Release>:/MT>
    )

    # 禁用一些MSVC警告
    target_compile_options(PhysxAPI PRIVATE
            /W3
            /wd4251  # 需要dll接口的类成员
            /wd4275  # 非dll接口的类
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(PhysxAPI PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endif()

# PhysX库文件
set(PHYSX_LIBS
        PhysXCharacterKinematic_static_64
        PhysXCooking_static_64
        PhysXExtensions_static_64
        PhysX_static_64
        PhysXPvdSDK_static_64
        PhysXCommon_static_64
        PhysXFoundation_static_64
)

# 平台特定库
if(UNIX AND NOT APPLE)
    list(APPEND PHYSX_LIBS pthread dl)
endif()

# 链接库
target_link_libraries(PhysxAPI
        PRIVATE
        ${PHYSX_LIBS}
)

# 设置库搜索路径
target_link_directories(PhysxAPI PRIVATE ${PHYSX_BUILD_DIR})

# 安装规则（当前安装到构建目录，如需安装到系统请改为 CMAKE_INSTALL_PREFIX 子路径）
set(INSTALL_LIB_DIR ${BUILD_OUTPUT_DIR}/lib)
set(INSTALL_BIN_DIR ${BUILD_OUTPUT_DIR}/bin)
install(TARGETS PhysxAPI
        LIBRARY DESTINATION ${INSTALL_LIB_DIR}
        ARCHIVE DESTINATION ${INSTALL_LIB_DIR}
        RUNTIME DESTINATION ${INSTALL_BIN_DIR}
)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/source/
        DESTINATION ${BUILD_OUTPUT_DIR}/include/PhysxAPI
        FILES_MATCHING PATTERN "*.h"
)

# 复制PhysX DLL到输出目录（Windows）
if(WIN32)
    add_custom_command(TARGET PhysxAPI POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            #        "${PHYSX_BUILD_DIR}/PhysXDevice64.dll"
            #        "${PHYSX_BUILD_DIR}/PhysXGpu_64.dll"
            "${PHYSX_BUILD_DIR}/PVDRuntime_64.dll"
            $<TARGET_FILE_DIR:PhysxAPI>
            COMMENT "Copying PhysX DLLs to output directory"
    )
endif()