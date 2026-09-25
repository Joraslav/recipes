FIND_LIB(SQLiteCpp)

add_library(_sqliterepo STATIC
  ProductTypeRepository.cpp
  InventoryItemRepository.cpp
  RecipeRepository.cpp
)

target_include_directories(_sqliterepo
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/db>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/db/repository>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/db/repository/sqlite>

  $<INSTALL_INTERFACE:include>
  $<INSTALL_INTERFACE:include/db>
  $<INSTALL_INTERFACE:include/db/repository>
  $<INSTALL_INTERFACE:include/db/repository/sqlite>
)

target_link_libraries(_sqliterepo
  PUBLIC
  SQLiteCpp
)
