PROJECT(ToyHttpServer CXX)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

set(CXX_FLAGS
 -g
 -Wall
 -Wextra
 -Werror
 -Wconversion
 -Woverloaded-virtual
 -Wpointer-arith
 -Wshadow
 -Wwrite-strings
 -march=native
 -rdynamic
 -lpthread
 )
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -finline-limit=1000 -DNDEBUG")

include_directories(.)

#当前目录下所有源文件赋给DIR_SRCS
aux_source_directory(. ALL_OBJ)

#生成可执行文件
add_executable(ToyHttpServer ${ALL_OBJ}) 

# #设置执行文件输出目录
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)



