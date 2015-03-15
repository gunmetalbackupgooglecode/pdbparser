message(STATUS "Configuring for platform Linux/CLang.")


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
