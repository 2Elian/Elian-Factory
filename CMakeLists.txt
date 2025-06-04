cmake_minimum_required(VERSION 3.10)
project(LLMTrainerApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找Threads库
find_package(Threads REQUIRED)

# 创建可执行文件
add_executable(llm_trainer_server llm_trainer_server.cpp)

# 链接库
target_link_libraries(llm_trainer_server PRIVATE Threads::Threads)

# 如果在Windows平台，添加Winsock库
if(WIN32)
    target_link_libraries(llm_trainer_server PRIVATE wsock32 ws2_32)
endif()

# 启用警告
if (MSVC)
    target_compile_options(llm_trainer_server PRIVATE /W4)
else()
    target_compile_options(llm_trainer_server PRIVATE -Wall -Wextra)
endif()

# 安装目标
install(TARGETS llm_trainer_server DESTINATION bin)

# 创建web目录
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/web)

# 复制web文件
file(GLOB WEB_FILES "${CMAKE_CURRENT_SOURCE_DIR}/web/*")
foreach(WEB_FILE ${WEB_FILES})
    file(COPY ${WEB_FILE} DESTINATION ${CMAKE_BINARY_DIR}/web)
endforeach() 