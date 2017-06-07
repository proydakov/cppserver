find_package(Threads REQUIRED)

# static http server

add_executable(poco-static-http-server
    src/static_http_server.cpp
)

target_link_libraries(poco-static-http-server
    PocoUtil
    PocoNet
    PocoXML
    PocoFoundation
    PocoJSON
    ${CMAKE_THREAD_LIBS_INIT}
)

# proxy http server

add_executable(poco-proxy-http-server
    src/proxy_http_server.cpp
)

target_link_libraries(poco-proxy-http-server
    PocoUtil
    PocoNet
    PocoXML
    PocoFoundation
    PocoJSON
    ${CMAKE_THREAD_LIBS_INIT}
)

# cache http server

add_executable(poco-cache-http-server
    src/cache_http_server.cpp
)

target_link_libraries(poco-cache-http-server
    PocoUtil
    PocoNet
    PocoXML
    PocoFoundation
    PocoJSON
    ${CMAKE_THREAD_LIBS_INIT}
)

# rapidjson http server

add_executable(poco-rapidjson-http-server
    src/rapidjson_http_server.cpp
)

target_link_libraries(poco-rapidjson-http-server
    PocoUtil
    PocoNet
    PocoXML
    PocoFoundation
    PocoJSON
    ${CMAKE_THREAD_LIBS_INIT}
)
