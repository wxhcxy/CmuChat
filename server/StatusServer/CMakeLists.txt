cmake_minimum_required(VERSION 3.27)

project(StatusServer LANGUAGES CXX)

# 手动指定 jsoncpp 的包含目录和库路径
set(JSONCPP_INCLUDE_DIR "/usr/include/jsoncpp")
set(JSONCPP_LIBRARIES "/usr/lib")

# 包含 jsoncpp 头文件
include_directories(${JSONCPP_INCLUDE_DIR})

# 手动指定 hiredis 的包含目录和库路径
set(HIREDIS_INCLUDE_DIR "/usr/include/hiredis")
set(HIREDIS_LIBRARIES "/usr/lib")
# 包含 hiredis 头文件
include_directories(${HIREDIS_INCLUDE_DIR})

# 手动指定 MySQL 路径（示例路径，根据实际安装位置调整）
set(MySQL_INCLUDE_DIRS "/usr/include/mysql")
set(MySQL_LIBRARIES "/usr/lib/libmysqlclient.so")

# 手动指定 MySQL++ 路径
set(MYSQL++_INCLUDE_DIRS "/usr/include/mysql++")
set(MYSQL++_LIBRARIES "/usr/lib/libmysqlpp.so")

# 查找 Boost 库
find_package(Boost REQUIRED COMPONENTS filesystem system)

if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  add_definitions(-DBOOST_ALL_NO_LIB)
  link_directories(${Boost_LIBRARY_DIRS})

else()
  message(FATAL_ERROR "Boost not found")
endif()

# linux里cmake使用grpc
find_package(Threads REQUIRED)

set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_REFLECTION gRPC::grpc++_reflection)

# Find gRPC installation Looks for gRPCConfig.cmake file installed by gRPC's
# cmake installation.
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

set(_GRPC_GRPCPP gRPC::grpc++)

# 添加可执行文件和源文件
file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB PBSOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

add_executable(
  StatusServer
  ${SOURCES}
  ${PBSOURCES}
  StatusServer.cpp
  config.ini
  RedisMgr.cpp
  StatusServiceImpl.cpp
  StatusServiceImpl.h
  const.h
  RedisMgr.h
  ChatGrpcClient.cpp
  ChatGrpcClient.h
  message.pb.h
  message.pb.cc
  message.grpc.pb.h
  message.grpc.pb.cc
  message.proto
  Singleton.h
  MysqlMgr.h
  MysqlMgr.cpp
  ConfigMgr.h
  ConfigMgr.cpp
  AsioIOServicePool.h
  AsioIOServicePool.cpp
  MysqlDao.h
  MysqlDao.cpp)

target_include_directories(StatusServer PRIVATE ${MySQL_INCLUDE_DIRS}
                                                ${MYSQL++_INCLUDE_DIRS})

target_link_libraries(
  StatusServer
  PRIVATE ${MySQL_LIBRARIES}
          ${MYSQL++_LIBRARIES}
          ${Boost_LIBRARIES}
          ${_REFLECTION}
          ${_GRPC_GRPCPP}
          ${_PROTOBUF_LIBPROTOBUF}
          hiredis
          pthread
          dl
          jsoncpp)

target_compile_features(StatusServer PRIVATE cxx_std_20)

# include(GNUInstallDirs) install(TARGETS StatusServer LIBRARY DESTINATION
# ${CMAKE_INSTALL_LIBDIR} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )

# target_sources( StatusServer PRIVATE main.cpp
#
# PUBLIC
#
# )

# target_compile_options( StatusServer PRIVATE -fmodules-ts )

# target_include_directories( StatusServer PRIVATE /usr/include )

# target_link_directories( StatusServer PRIVATE /usr/lib )

# target_link_libraries( StatusServer PRIVATE m )

# set_target_properties( StatusServer PROPERTIES CXX_STANDARD 20 CXX_EXTENSIONS
# OFF etc.. ) 设置指定的C++编译器版本是必须的，如果不设置，或者为OFF，则指定版本不可用时，会使用上一版本。
# set_target_properties(StatusServer PROPERTIES  CXX_STANDARD_REQUIRED on)

# --------------------------------------------------------------------------
# Adding build-requirements 源文件列表 target_sources( <target> PRIVATE
# <source-file>... )
#
# target_precompile_headers( <target> PRIVATE <header-file>... )
#
# 预处理头文件搜索路径列表，等价参数：-I target_include_directories( <target> PRIVATE
# <include-search-dir>... )
#
# 预编译宏 自动在指定的宏前添加，等价参数：-D target_compile_definitions( <target> PRIVATE
# <macro-definitions>... )
#
# 编译选项和特性 target_compile_options( <target> PRIVATE <compiler-option>... )
# target_compile_features( <target> PRIVATE <feature>... )
#
# 链接库 target_link_libraries( <target> PRIVATE <dependency>... )
#
# 链接选项 target_link_options( <target> PRIVATE <linker-option>... )
#
# 链接库搜索路径，等价参数：-L target_link_directories( <target> PRIVATE
# <linker-search-dir>... )

# execute_process(COMMAND ln -s /opt/gcc/gcm.cache  ./)

# Modern Cmake建议围绕Target和Property进行项目配置，竭力避免出现下面变量variable的set定义。
# Variable横行是典型CMake2.8时期的风格, Modern CMake则遵循了OO方法。其中，target表示待构建的对象，
# target相关的语句用于定义来约束link、compile等相关属性的scope。 set(CMAKE_CXX_STANDARD 20)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 完整的project命令 project( StatusServer LANGUAGES CXX ) DESCRIPTION "Description of
# project" HOMEPAGE_URL "http://www.abc.com" VERSION 0.1.0 )
# execute_process(COMMAND g++ -std=c++20 -xc++-system-header -fmodules-ts #
# 如何要使用modules,该命令应该放在project命令之后 iostream string )
