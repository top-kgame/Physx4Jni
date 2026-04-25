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

# 兼容：如果当前构建类型对应的 PhysX 静态库缺失，则回落到 debug 目录
# （常见于只编译了 debug 版 PhysX，但 PhysxAPI 选择了 Release）
set(_PHYSX_CORE_LIB "${PHYSX_BUILD_DIR}/libPhysX_static_64.a")
if(NOT EXISTS "${_PHYSX_CORE_LIB}")
    set(_PHYSX_FALLBACK_DIR "${PHYSX_HOME}/bin/${PLATFORM_NAME}/debug")
    if(EXISTS "${_PHYSX_FALLBACK_DIR}/libPhysX_static_64.a")
        message(WARNING "PhysX libs not found in ${PHYSX_BUILD_DIR}; falling back to ${_PHYSX_FALLBACK_DIR}")
        set(PHYSX_BUILD_DIR "${_PHYSX_FALLBACK_DIR}")
        set(PHYSX_BUILD_TYPE "debug")
    else()
        message(FATAL_ERROR "PhysX core lib not found: ${_PHYSX_CORE_LIB}")
    endif()
endif()

# macOS：PhysX 静态库架构需与本项目一致（arm64 / x86_64）
# 如果 PhysX 库是 x86_64（常见于只编译了 Intel 版 PhysX），则强制本项目也编译为 x86_64 以便链接通过。
if(APPLE AND EXISTS "${_PHYSX_CORE_LIB}")
    execute_process(
            COMMAND lipo -info "${_PHYSX_CORE_LIB}"
            OUTPUT_VARIABLE _PHYSX_LIPO_OUT
            ERROR_VARIABLE _PHYSX_LIPO_ERR
            RESULT_VARIABLE _PHYSX_LIPO_CODE
    )
    if(_PHYSX_LIPO_CODE EQUAL 0)
        if(_PHYSX_LIPO_OUT MATCHES "x86_64" AND NOT _PHYSX_LIPO_OUT MATCHES "arm64")
            message(WARNING "PhysX libs are x86_64 only; forcing PhysxAPI to build as x86_64")
            set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "macOS architectures" FORCE)
        endif()
    endif()
endif()

message(STATUS "PhysxAPI: ${CMAKE_SYSTEM_NAME} | ${CMAKE_BUILD_TYPE} | out=${BUILD_OUTPUT_DIR} | PhysX=${PHYSX_BUILD_DIR}")

# 源文件组织
set(MAIN_SOURCES
        source/PhysxApi.cpp
        source/PhysxApi.h
        source/scene/Scene.cpp
        source/scene/Scene.h
        source/PhysxMath.h
        source/object/SceneObj.cpp
        source/object/SceneObj.h
        source/object/SceneRigidObj.cpp
        source/object/SceneRigidObj.h
        source/object/SceneKinematicObj.cpp
        source/object/SceneKinematicObj.h
        source/object/SceneCharacterObj.cpp
        source/object/SceneCharacterObj.h
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
        ${CMAKE_CURRENT_SOURCE_DIR}/source
)

# 将第三方 PhysX 头文件标记为 SYSTEM，避免 PhysX 自身 warning 刷屏（影响 Cursor Chat UI）
target_include_directories(PhysxAPI SYSTEM PRIVATE
        ${PHYSX_HOME}/include
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
            # PhysX headers trigger this on Clang; keep our warnings, silence 3rd-party noise
            -Wno-inconsistent-missing-override
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

# ---- Tests (CLion runnable) ----
add_executable(PhysxAPITest
        tests/TestMain.cpp
)
target_include_directories(PhysxAPITest PRIVATE
        ${PROJECT_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/source
)

target_include_directories(PhysxAPITest SYSTEM PRIVATE
        ${PHYSX_HOME}/include
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(PhysxAPITest PRIVATE
            -Wno-inconsistent-missing-override
    )
endif()
target_link_libraries(PhysxAPITest PRIVATE PhysxAPI)

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