FIND_LIB(Boost)
FIND_LIB(OpenSSL)

add_library(_client STATIC
    RecipesClient.cpp
)

target_include_directories(_client
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/client>
    $<INSTALL_INTERFACE:include>
    $<INSTALL_INTERFACE:include/client>
)

target_link_libraries(_client PUBLIC
    _api
    Boost::boost
    Boost::cobalt
    Boost::cobalt_io_ssl
    OpenSSL::SSL
    OpenSSL::Crypto
)