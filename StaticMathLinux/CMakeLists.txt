cmake_minimum_required(VERSION 3.10)
project(MathLibrarySystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(Number STATIC
    NumberLibrary.cpp
    NumberLibrary.h
)
target_include_directories(Number PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(Vector SHARED
    VectorDLL.cpp
    VectorDLL.h
)
target_link_libraries(Vector PRIVATE Number)
target_include_directories(Vector PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(MathClient
    MathClient.cpp
)
target_link_libraries(MathClient PRIVATE Number Vector)
target_include_directories(MathClient PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS Number Vector MathClient
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES NumberLibrary.h VectorDLL.h DESTINATION include)

set_target_properties(Vector PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
