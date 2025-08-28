@echo off
chcp 65001 >nul
title SVG注册工具启动器

echo ========================================
echo        SVG注册工具启动器
echo ========================================
echo.
echo 请选择要启动的工具：
echo.
echo 1. SVG图标注册管理工具
echo 2. SVG工具打包器
echo 3. 退出
echo.
set /p choice="请输入选择 (1-3): "

if "%choice%"=="1" goto svg_tool
if "%choice%"=="2" goto package_tool
if "%choice%"=="3" goto exit
echo 无效选择，请重新运行程序
pause
goto exit

:svg_tool
echo.
echo 正在启动SVG图标注册管理工具...
echo.
REM 检查Python是否安装
python --version >nul 2>&1
if errorlevel 1 (
    echo 错误: 未找到Python，请先安装Python 3.7+
    echo 下载地址: https://www.python.org/downloads/
    pause
    exit /b 1
)

REM 检查psutil是否安装
python -c "import psutil" >nul 2>&1
if errorlevel 1 (
    echo 正在安装psutil...
    python -m pip install psutil
    if errorlevel 1 (
        echo 错误: psutil安装失败
        pause
        exit /b 1
    )
)

echo 启动SVG工具...
python SVG_Icon_Manager.py
exit

:package_tool
echo.
echo 正在启动SVG工具打包器...
echo.
REM 检查Python是否安装
python --version >nul 2>&1
if errorlevel 1 (
    echo 错误: 未找到Python，请先安装Python 3.7+
    echo 下载地址: https://www.python.org/downloads/
    pause
    exit /b 1
)

REM 检查psutil是否安装
python -c "import psutil" >nul 2>&1
if errorlevel 1 (
    echo 正在安装psutil...
    python -m pip install psutil
    if errorlevel 1 (
        echo 错误: psutil安装失败
        pause
        exit /b 1
    )
)

echo 启动打包工具...
python PackageTool.py
exit

:exit
echo.
echo 程序已退出
