@echo off
chcp 65001 > nul
title ElianFactory 构建工具

REM 构建ElianFactory工具
echo === 开始构建 ===
echo 正在编译C++服务器...
g++ llm_trainer_server.cpp -o llm_trainer_server.exe -std=c++17 -lws2_32
if %errorlevel% neq 0 (
    echo C++编译失败！
    pause
    exit /b %errorlevel%
)
echo C++服务器编译成功！

echo === 所有操作已完成 ===
echo 请双击 start_server.bat 开始ElianFactory之旅！
echo.
echo 注意：如果系统提示缺少dll文件，请安装Visual C++运行库
echo 下载地址：https://aka.ms/vs/17/release/vc_redist.x64.exe
pause