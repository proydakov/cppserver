ADD_EXECUTABLE(asio_spawn_http_client
    asio_spawn_http_client.cpp
)

ADD_DEPENDENCIES(asio_spawn_http_client http)
TARGET_LINK_LIBRARIES(asio_spawn_http_client http)
TARGET_LINK_LIBRARIES(asio_spawn_http_client ${Boost_REGEX_LIBRARY})
TARGET_LINK_LIBRARIES(asio_spawn_http_client ${Boost_SYSTEM_LIBRARY})
TARGET_LINK_LIBRARIES(asio_spawn_http_client ${Boost_CONTEXT_LIBRARY})
TARGET_LINK_LIBRARIES(asio_spawn_http_client ${Boost_COROUTINE_LIBRARY})
TARGET_LINK_LIBRARIES(asio_spawn_http_client ${Boost_DATE_TIME_LIBRARY})

#==============================================================================

ADD_EXECUTABLE(asio_spawn_proxy_http_server
    asio_spawn_proxy_http_server.cpp
)

ADD_DEPENDENCIES(asio_spawn_proxy_http_server http)
TARGET_LINK_LIBRARIES(asio_spawn_proxy_http_server http)
TARGET_LINK_LIBRARIES(asio_spawn_proxy_http_server ${Boost_REGEX_LIBRARY})
TARGET_LINK_LIBRARIES(asio_spawn_proxy_http_server ${Boost_SYSTEM_LIBRARY})
TARGET_LINK_LIBRARIES(asio_spawn_proxy_http_server ${Boost_CONTEXT_LIBRARY})
TARGET_LINK_LIBRARIES(asio_spawn_proxy_http_server ${Boost_COROUTINE_LIBRARY})
TARGET_LINK_LIBRARIES(asio_spawn_proxy_http_server ${Boost_DATE_TIME_LIBRARY})

#==============================================================================

ADD_EXECUTABLE(asio-rapidjson-http-server
    asio_rapidjson_http_server.cpp
)

TARGET_LINK_LIBRARIES(asio-rapidjson-http-server ${Boost_REGEX_LIBRARY})
TARGET_LINK_LIBRARIES(asio-rapidjson-http-server ${Boost_SYSTEM_LIBRARY})
TARGET_LINK_LIBRARIES(asio-rapidjson-http-server ${Boost_DATE_TIME_LIBRARY})
