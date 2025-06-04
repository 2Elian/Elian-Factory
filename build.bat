@echo off
chcp 65001 > nul
title ElianFactory 构建工具

REM 构建ElianFactory工具
echo === 开始构建 ===

REM 1. 进入web-vue目录并构建前端
echo 正在构建前端...
cd /d "%~dp0web-vue" || (
    echo 错误：无法进入web-vue目录
    pause
    exit /b 1
)

call npm.cmd run build
if %errorlevel% neq 0 (
    echo 前端构建失败！
    pause
    exit /b %errorlevel%
)
echo 前端构建成功！

REM 2. 返回上级目录
cd /d "%~dp0" || (
    echo 错误：无法返回上级目录
    pause
    exit /b 1
)

REM 3. 编译C++服务器
echo 正在编译C++服务器...
g++ llm_trainer_server.cpp -o llm_trainer_server.exe -std=c++17 -lws2_32
if %errorlevel% neq 0 (
    echo C++编译失败！
    pause
    exit /b %errorlevel%
)
echo C++服务器编译成功！

REM 4. 创建启动脚本
(
echo @echo off
echo chcp 65001 > nul
echo title ElianFactory 服务端
echo echo 正在启动ElianFactory服务端...
echo llm_trainer_server.exe
echo pause
) > start_server.bat

echo === 所有操作已完成 ===
echo 请双击 start_server.bat 开始ElianFactory之旅！
echo.
echo 注意：如果系统提示缺少dll文件，请安装Visual C++运行库
echo 下载地址：https://aka.ms/vs/17/release/vc_redist.x64.exe
pause