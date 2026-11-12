@echo off
REM ============================================================
REM  SDCC 一键编译脚本（替代 Keil C51，无 2KB 代码限制）
REM  用法：双击本文件，或在命令行运行 build_sdcc.bat
REM  输出：firmware\build_sdcc\51car.hex （用 STC-ISP 烧录）
REM ============================================================
setlocal

REM ---- 1. 找到 SDCC ----
set SDCC_BIN=
where sdcc >nul 2>&1 && set SDCC_BIN=
if not defined SDCC_BIN (
    if exist "C:\Program Files\SDCC\bin\sdcc.exe" set SDCC_BIN=C:\Program Files\SDCC\bin
)
if not defined SDCC_BIN (
    echo [ERROR] 找不到 SDCC。请先安装 SDCC（winget install SDCC.SDCC）
    pause
    exit /b 1
)
set PATH=%SDCC_BIN%;%PATH%

REM ---- 2. 切到脚本所在目录 ----
cd /d "%~dp0"

REM ---- 3. 清理旧产物 ----
if exist build_sdcc rmdir /s /q build_sdcc
mkdir build_sdcc

REM ---- 4. 逐个编译 .c -> .rel ----
set SRCS=app\app.c app\pid.c common\tick.c common\delay.c drivers\motor\motor.c drivers\motor\encoder.c drivers\tracking\tracking.c main\main.c
set FAIL=0
for %%f in (%SRCS%) do (
    echo [CC] %%f
    sdcc -mmcs51 --model-small -Iconfig -Iinclude -c %%f -o build_sdcc\
    if errorlevel 1 set FAIL=1
)
if %FAIL%==1 (
    echo.
    echo [ERROR] 编译失败，请查看上面的错误信息
    pause
    exit /b 1
)

REM ---- 5. 链接所有 .rel ----
echo [LINK]
sdcc -mmcs51 --model-small build_sdcc\app.rel build_sdcc\delay.rel build_sdcc\encoder.rel build_sdcc\main.rel build_sdcc\motor.rel build_sdcc\pid.rel build_sdcc\tick.rel build_sdcc\tracking.rel -o build_sdcc\
if errorlevel 1 (
    echo [ERROR] 链接失败
    pause
    exit /b 1
)

REM ---- 6. 输出烧录用 hex ----
if exist build_sdcc\app.ihx (
    copy /y build_sdcc\app.ihx build_sdcc\51car.hex >nul
    echo.
    echo [OK] 编译成功: build_sdcc\51car.hex
    for %%A in (build_sdcc\51car.hex) do echo      大小: %%~zA 字节
) else (
    echo [ERROR] 未生成 ihx
    pause
    exit /b 1
)

pause
endlocal
