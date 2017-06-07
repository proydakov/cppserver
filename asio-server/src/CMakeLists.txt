ADD_EXECUTABLE(asio_speed_client
    asio_speed_client.cpp
)

TARGET_LINK_LIBRARIES(asio_speed_client ${Boost_REGEX_LIBRARY})
TARGET_LINK_LIBRARIES(asio_speed_client ${Boost_SYSTEM_LIBRARY})
TARGET_LINK_LIBRARIES(asio_speed_client ${Boost_CONTEXT_LIBRARY})
TARGET_LINK_LIBRARIES(asio_speed_client ${Boost_COROUTINE_LIBRARY})
TARGET_LINK_LIBRARIES(asio_speed_client ${Boost_DATE_TIME_LIBRARY})

#==============================================================================

ADD_EXECUTABLE(asio_speed_server
    asio_speed_server.cpp
)

TARGET_LINK_LIBRARIES(asio_speed_server ${Boost_REGEX_LIBRARY})
TARGET_LINK_LIBRARIES(asio_speed_server ${Boost_SYSTEM_LIBRARY})
TARGET_LINK_LIBRARIES(asio_speed_server ${Boost_CONTEXT_LIBRARY})
TARGET_LINK_LIBRARIES(asio_speed_server ${Boost_COROUTINE_LIBRARY})
TARGET_LINK_LIBRARIES(asio_speed_server ${Boost_DATE_TIME_LIBRARY})
