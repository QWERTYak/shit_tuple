set(CMAKE_CXX_COMPILER "/usr/bin/clang")
set(CMAKE_CXX_COMPILER_LINKER  "/usr/bin/lld")

cmake_minimum_required(VERSION 3.31)
project(shit_tuple LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 26)

add_executable(tuple_impl main.cpp)
target_compile_options(tuple_impl PUBLIC -nostdinc++ -I/usr/include/c++/v1)
target_link_options(tuple_impl PUBLIC -stdlib=libc++ -rtlib=compiler-rt -unwindlib=libgcc  -shared-libgcc)
target_link_libraries(tuple_impl PUBLIC /usr/lib/libc++.a /usr/lib/libc++abi.a /usr/lib/libc++experimental.a /usr/lib/libc.so)