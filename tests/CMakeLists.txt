cmake_minimum_required(VERSION 3.20)
project(ldacs_test C)

set(CMAKE_C_STANDARD 11)
set(tests
        json
        aqueue
        #        stateMachine
#        multitimer
        #        crc
#                buffer
        #        pbs
        #        pqueue
#        base64
        #        mqueue
#        queue
        #        map
        #        libevent
#        globaltimer
        #        list
        #        file
        #        http
        #        http_sse
        #        sqlite
        #        crypto
        sql
        ui
#        event_server
#        event_client
#        bitset
#        drr
#        rbuffer
##        rkeygen
#        test
#        mps
#        ntp
#        timerfd
#        hashmap
#        ipv6addr
#        coredump
#        env
#        str
#        yaml
#        ipv6pkt
)

#cmake_host_system_information(RESULT PRETTY_NAME QUERY DISTRIB_PRETTY_NAME)
#IF (${PRETTY_NAME} MATCHES "Ubuntu")
#    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -g")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=leak -g")
#ENDIF ()
INCLUDE_DIRECTORIES(/usr/local/include/ldacs)

find_package(Ldacscrypto REQUIRED)
if (Ldacscrypto_FOUND)
    include_directories(${Ldacscrypto_INCLUDE_DIRS})
    link_libraries(ldacscrypto::ldacscrypto)
endif ()

enable_testing()
foreach (name ${tests})
    add_test(NAME ${name} COMMAND ${name}test)

    add_executable(${name}test ${name}_test.c)

    if(${name} STREQUAL "ui")
        message("String is ui")
        # 添加GTK相关配置
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
        include_directories(${GTK3_INCLUDE_DIRS})
        link_libraries(${GTK3_LIBRARIES})
    endif()

    target_link_libraries(${name}test LINK_PUBLIC liteldacssdk yaml cjson base64 event event_pthreads )
    target_include_directories(${name}test PUBLIC  ${CMAKE_SOURCE_DIR}/include)

    find_package(SQLite3 REQUIRED)
    if (SQLite3_FOUND)
        include_directories(${SQLite3_INCLUDE_DIRS})
        target_link_libraries(${name}test LINK_PUBLIC ${SQLite3_LIBRARIES})
    endif (SQLite3_FOUND)
endforeach ()

