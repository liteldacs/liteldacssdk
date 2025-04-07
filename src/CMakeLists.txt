cmake_minimum_required(VERSION 3.18)
project(ldacsutils  VERSION 1.0.0 LANGUAGES C)

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
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include/utils>
)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/global>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include/global>
)

target_link_libraries(${PROJECT_NAME} LINK_PUBLIC pthread yaml base64 cjson)

find_package(Ldacscrypto REQUIRED)
if (Ldacscrypto_FOUND)
    include_directories(${Ldacscrypto_INCLUDE_DIRS})
    link_libraries(ldacscrypto::ldacscrypto)
endif ()

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES output_name "ldacsutils")
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
INSTALL(TARGETS ${PROJECT_NAME} EXPORT LdacsutilsTargets ARCHIVE DESTINATION lib LIBRARY DESTINATION lib )
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION include/utils)
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/global/ DESTINATION include/global)

INSTALL(EXPORT LdacsutilsTargets
        FILE LdacsutilsTargets.cmake
        NAMESPACE ldacsutils::
        DESTINATION /usr/local/lib/cmake/ldacsutils
)

# 生成并安装 Config 文件
include(CMakePackageConfigHelpers)

# 生成 aaaConfigVersion.cmake
write_basic_package_version_file(
        ${CMAKE_SOURCE_DIR}/cmake/LdacsutilsConfigVersion.cmake
        VERSION ${Ldacsutils_VERSION}
        COMPATIBILITY SameMajorVersion
)

# 生成 aaaConfig.cmake
configure_package_config_file(
        ${CMAKE_SOURCE_DIR}/cmake/LdacsutilsConfig.cmake.in
        ${CMAKE_SOURCE_DIR}/cmake/LdacsutilsConfig.cmake
        INSTALL_DESTINATION /usr/local/lib/cmake/ldacsutils
)

# 安装 Config 和 Targets 文件
install(
        FILES
        ${CMAKE_SOURCE_DIR}/cmake/LdacsutilsConfig.cmake
        ${CMAKE_SOURCE_DIR}/cmake/LdacsutilsConfigVersion.cmake
        DESTINATION /usr/local/lib/cmake/ldacsutils
)

set(SO_PATH /usr/local/ldacs-cauc/lib)
set(SO_CONFIG_DIR /etc/ld.so.conf.d)

INSTALL(CODE "execute_process(COMMAND sudo sh -c \"echo ${SO_PATH} > ${SO_CONFIG_DIR}/ldacs-cauc.conf\" COMMAND sudo ldconfig )")
