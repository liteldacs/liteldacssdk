cmake_minimum_required(VERSION 3.18)
project(ldacs_test C)

set(CMAKE_C_STANDARD 11)


set(tests
        json
        #        stateMachine
#        multitimer
        #        crc
        #        buffer
        #        pbs
        #        pqueue
        base64
        #        mqueue
        queue
        #        map
        #        libevent
        globaltimer
        #        list
        #        file
        #        http
        #        http_sse
        #        sqlite
        #        crypto
        sql
        bitset
        drr
        rbuffer
#        rkeygen
)

cmake_host_system_information(RESULT PRETTY_NAME QUERY DISTRIB_PRETTY_NAME)
IF (${PRETTY_NAME} MATCHES "Ubuntu")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -g")
    #    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=leak -g")
ENDIF ()
INCLUDE_DIRECTORIES(/usr/local/include/ldacs)
enable_testing()
foreach (name ${tests})
    add_test(NAME ${name} COMMAND ${name}test)
    add_executable(${name}test ${name}_test.c)
    target_link_libraries(${name}test LINK_PUBLIC ldacsutils yaml cjson base64 statemachine event event_pthreads)
    target_include_directories(${name}test PUBLIC  ${CMAKE_SOURCE_DIR}/include)

    find_package(SQLite3 REQUIRED)
    if (SQLite3_FOUND)
        include_directories(${SQLite3_INCLUDE_DIRS})
        target_link_libraries(${name}test LINK_PUBLIC ${SQLite3_LIBRARIES})
    endif (SQLite3_FOUND)

#    cmake_host_system_information(RESULT PRETTY_NAME QUERY DISTRIB_PRETTY_NAME)
#    IF (${PRETTY_NAME} MATCHES "Ubuntu")
#        target_link_libraries(${name}test LINK_PUBLIC gmssl)
#        target_compile_definitions(${name}test PRIVATE UNUSE_CRYCARD)
#    ELSEIF (${PRETTY_NAME} MATCHES "CentOS")
#        target_link_libraries(${name}test LINK_PUBLIC km_src uuid)
#        target_compile_definitions(${name}test PRIVATE USE_CRYCARD)
#    ENDIF ()
endforeach ()

