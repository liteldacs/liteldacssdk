cmake_minimum_required(VERSION 3.18)
project(ldacsutils)

set(CMAKE_C_STANDARD 11)

add_compile_definitions(LOG_USE_COLOR)

AUX_SOURCE_DIRECTORY(. SRC_LIST) #把当前目录(.)下所有源代码文件和头文件加入变量SRC_LIST
add_library(${PROJECT_NAME} STATIC ${SRC_LIST})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/global)

target_link_libraries(${PROJECT_NAME} LINK_PUBLIC pthread)

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
INSTALL(TARGETS ${PROJECT_NAME} ARCHIVE DESTINATION lib/ldacs LIBRARY DESTINATION lib/ldacs RUNTIME DESTINATION bin)
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION include/ldacs/utils)
