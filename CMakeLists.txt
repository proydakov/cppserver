#==============================================================================
# Copyright (c) 2017 Evgeny Proydakov <e.proydakov@gmail.com>
#==============================================================================

CMAKE_MINIMUM_REQUIRED(VERSION 2.8.0)

PROJECT(CPP-SERVER)
MESSAGE(STATUS "CREATE PROJECT I++")

STRING(REGEX MATCH "Linux" LINUX ${CMAKE_SYSTEM_NAME})

IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Debug)
    #SET(CMAKE_BUILD_TYPE Release)
ENDIF()

IF(LINUX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -pthread")
ENDIF()

IF(APPLE)
    SET(CMAKE_MACOSX_RPATH 1)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -stdlib=libc++")
    SET(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.llvm.clang.1_0")
    SET(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++11")
    SET(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY           "libc++")
    SET(CMAKE_MACOSX_RPATH 1)
ENDIF()

IF(MSVC)
    ADD_DEFINITIONS(-D_WIN32_WINNT=0x0501)

    SET(Boost_USE_STATIC_LIBS    ON)
    SET(Boost_USE_MULTITHREADED  ON)
ENDIF()

SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

SET(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

#==============================================================================

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/rapidjson/include)

ADD_SUBDIRECTORY(asio-server)
ADD_SUBDIRECTORY(asio-http-server)

IF(LINUX)
    ADD_SUBDIRECTORY(poco-http-server)
ENDIF()

#==============================================================================
#---------------------------------- info --------------------------------------
#==============================================================================

SET(BUILD_INFO_BAR "=========================================================")
SET(NOOP_STRING "")

MESSAGE(STATUS ${BUILD_INFO_BAR})
MESSAGE(STATUS "Summary of the build:")
MESSAGE(STATUS ${BUILD_INFO_BAR})
MESSAGE(STATUS "Build type : ${CMAKE_BUILD_TYPE}")
MESSAGE(STATUS ${BUILD_INFO_BAR})
MESSAGE(STATUS ${NOOP_STRING})

#==============================================================================
