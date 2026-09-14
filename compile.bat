@echo off
chcp 65001 >nul
title 编译雷霆战机

echo ========================================
echo   雷霆战机 - Thunder Fighter
echo   编译脚本
echo ========================================
echo.

:: 检查是否有 g++ (MinGW)
where g++ >nul 2>nul
if %errorlevel% equ 0 (
    echo [使用 MinGW g++ 编译]
    g++ -o ThunderFighter.exe main.cpp -lgdi32 -lwinmm -static -O2
    if %errorlevel% equ 0 (
        echo.
        echo ✓ 编译成功! 生成 ThunderFighter.exe
    ) else (
        echo.
        echo ✗ 编译失败
    )
    goto :end
)

:: 检查是否有 cl (MSVC)
where cl >nul 2>nul
if %errorlevel% equ 0 (
    echo [使用 MSVC cl 编译]
    cl main.cpp user32.lib gdi32.lib winmm.lib /Fe:ThunderFighter.exe /O2
    if %errorlevel% equ 0 (
        echo.
        echo ✓ 编译成功! 生成 ThunderFighter.exe
    ) else (
        echo.
        echo ✗ 编译失败
    )
    goto :end
)

echo [错误] 未找到编译器!
echo 请安装 MinGW (g++) 或 Visual Studio (MSVC)
echo.
echo MinGW 下载: https://www.mingw-w64.org/
echo.
pause
exit /b 1

:end
echo.
echo 按任意键运行游戏...
pause >nul
if exist ThunderFighter.exe (
    start ThunderFighter.exe
)
