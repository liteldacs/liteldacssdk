cmake_minimum_required(VERSION 3.18)
project(ldacsutils  VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)

add_compile_definitions(LOG_USE_COLOR)

AUX_SOURCE_DIRECTORY(. SRC_LIST) #把当前目录(.)下所有源代码文件和头文件加入变量SRC_LIST
add_library(${PROJECT_NAME} STATIC ${SRC_LIST})

target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include/ldacs/utils>
)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/global>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include/ldacs/global>
)

target_link_libraries(${PROJECT_NAME} LINK_PUBLIC pthread )

#cmake_host_system_information(RESULT PRETTY_NAME QUERY DISTRIB_PRETTY_NAME)
#IF (${PRETTY_NAME} MATCHES "CentOS")
#    target_link_libraries(${PROJECT_NAME} LINK_PUBLIC km_src gmssl)
#    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_CRYCARD)
#ELSEIF (${PRETTY_NAME} MATCHES "Ubuntu")
#    target_link_libraries(${PROJECT_NAME} LINK_PUBLIC km_src gmssl)
#    target_compile_definitions(${PROJECT_NAME} PRIVATE UNUSE_CRYCARD)
#ENDIF ()

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES output_name "ldacs_util")
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
INSTALL(TARGETS ${PROJECT_NAME} EXPORT LdacsutilsTargets ARCHIVE DESTINATION lib/ldacs LIBRARY DESTINATION lib/ldacs RUNTIME DESTINATION bin)
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION include/ldacs/utils)
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/global/ DESTINATION include/ldacs/global)

INSTALL(EXPORT LdacsutilsTargets
        FILE LdacsutilsTargets.cmake
        NAMESPACE ldacsutils::
        DESTINATION lib/cmake/ldacsutils
)

# 生成并安装 Config 文件
include(CMakePackageConfigHelpers)

# 生成 aaaConfigVersion.cmake
write_basic_package_version_file(
        ${CMAKE_SOURCE_DIR}/LdacsutilsConfigVersion.cmake
        VERSION ${Ldacsutils_VERSION}
        COMPATIBILITY SameMajorVersion
)

# 生成 aaaConfig.cmake
configure_package_config_file(
        ${CMAKE_SOURCE_DIR}/cmake/LdacsutilsConfig.cmake.in
        ${CMAKE_SOURCE_DIR}/LdacsutilsConfig.cmake
        INSTALL_DESTINATION lib/cmake/ldacsutils
)

# 安装 Config 和 Targets 文件
install(
        FILES
        ${CMAKE_SOURCE_DIR}/LdacsutilsConfig.cmake
        ${CMAKE_SOURCE_DIR}/LdacsutilsConfigVersion.cmake
        DESTINATION lib/cmake/ldacsutils
)

message("${CMAKE_INSTALL_PREFIX}")

