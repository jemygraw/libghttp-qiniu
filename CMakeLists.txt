cmake_minimum_required(VERSION 3.3)
project(libghttp_qiniu)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# compile & link
include_directories(ghttp-qiniu)
include_directories(/usr/local/lib)

# link library
link_libraries(ghttp)

set(SOURCE_FILES
        ghttp-qiniu/ghttp-qiniu.c
        main.c

)

add_executable(libghttp_qiniu ${SOURCE_FILES})