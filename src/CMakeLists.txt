cmake_minimum_required(VERSION 3.18)
project(liteldacssdk  VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)

add_compile_definitions(LOG_USE_COLOR)

message(${CMAKE_INSTALL_PREFIX})
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/usr/local/ldacs-cauc" CACHE PATH "Default install path" FORCE)
endif()
message(${CMAKE_INSTALL_PREFIX})

AUX_SOURCE_DIRECTORY(. SRC_LIST) #把当前目录(.)下所有源代码文件和头文件加入变量SRC_LIST
add_library(${PROJECT_NAME} STATIC ${SRC_LIST})

target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include/sdk>
)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/global>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include/global>
)


#include(ExternalProject)
#set(BASE64_ROOT ${CMAKE_BINARY_DIR}/thirdparty/base64)
#ExternalProject_Add(base64_dep
#  PREFIX            ${BASE64_ROOT}
#  GIT_REPOSITORY    https://github.com/aklomp/base64.git
#  GIT_TAG           v0.5.1
#  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${BASE64_ROOT}
#  INSTALL_COMMAND   ${CMAKE_COMMAND} --build . --target install
#)
#
#set ( FOO_LIB          ${BASE64_ROOT}/lib/foo.lib)
#set ( FOO_INCLUDE_DIR  ${BASE64_ROOT}/include )
#
#ExternalProject_Get_property(base64_dep PREFIX)
#message( STATUS "PREFIX = ${PREFIX}" )

target_link_libraries(${PROJECT_NAME} LINK_PUBLIC pthread yaml base64 cjson)

find_package(Ldacscrypto REQUIRED)
if (Ldacscrypto_FOUND)
    include_directories(${Ldacscrypto_INCLUDE_DIRS})
    link_libraries(ldacscrypto::ldacscrypto)
endif ()

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES output_name "liteldacssdk")
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
INSTALL(TARGETS ${PROJECT_NAME} EXPORT LiteLdacsSdkTargets ARCHIVE DESTINATION lib LIBRARY DESTINATION lib )
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION include/sdk)
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/global/ DESTINATION include/global)

INSTALL(EXPORT LiteLdacsSdkTargets
        FILE LiteLdacsSdkTargets.cmake
        NAMESPACE liteldacssdk::
        DESTINATION /usr/local/lib/cmake/liteldacssdk
)

# 生成并安装 Config 文件
include(CMakePackageConfigHelpers)

# 生成 aaaConfigVersion.cmake
write_basic_package_version_file(
        ${CMAKE_SOURCE_DIR}/cmake/LiteLdacsSdkConfigVersion.cmake
        VERSION ${LiteLdacsSdk_VERSION}
        COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
        ${CMAKE_SOURCE_DIR}/cmake/LiteLdacsSdkConfig.cmake.in
        ${CMAKE_SOURCE_DIR}/cmake/LiteLdacsSdkConfig.cmake
        INSTALL_DESTINATION /usr/local/lib/cmake/liteldacssdk
)

# 安装 Config 和 Targets 文件
install(
        FILES
        ${CMAKE_SOURCE_DIR}/cmake/LiteLdacsSdkConfig.cmake
        ${CMAKE_SOURCE_DIR}/cmake/LiteLdacsSdkConfigVersion.cmake
        DESTINATION /usr/local/lib/cmake/liteldacssdk
)

set(SO_PATH /usr/local/ldacs-cauc/lib)
set(SO_CONFIG_DIR /etc/ld.so.conf.d)

INSTALL(CODE "execute_process(COMMAND sudo sh -c \"echo ${SO_PATH} > ${SO_CONFIGDIR}/ldacs-cauc.conf\" COMMAND sudo ldconfig )")
