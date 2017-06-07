FIND_PACKAGE(Boost COMPONENTS date_time regex system coroutine context REQUIRED)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})

#==============================================================================

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

ADD_SUBDIRECTORY(src)
