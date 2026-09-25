FIND_LIB(libpqxx)

add_library(_postgresqlrepo STATIC
  ProductTypeRepository.cpp
)

target_include_directories(_postgresqlrepo
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/db>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/db/repository>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/db/repository/postgresql>

  $<INSTALL_INTERFACE:include>
  $<INSTALL_INTERFACE:include/db>
  $<INSTALL_INTERFACE:include/db/repository>
  $<INSTALL_INTERFACE:include/db/repository/postgresql>
)

target_link_libraries(_postgresqlrepo
  PUBLIC
  libpqxx::pqxx
)
