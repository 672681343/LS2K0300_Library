@echo off
chcp 65001 >nul
title Remote Control Tool

:MENU
cls
echo ========================================
echo    Remote Control Tool
echo ========================================
echo.
echo   [1] Run build.sh menu (Linux VM)
echo   [2] Run ./project (Car 192.168.1.104)
echo   [0] Exit
echo.
echo ========================================
set /p choice="Select [0-2]: "

if "%choice%"=="1" goto RUN_BUILD
if "%choice%"=="2" goto RUN_PROJECT
if "%choice%"=="0" goto EXIT
if "%choice%"=="" goto MENU
goto MENU

:RUN_BUILD
echo.
echo Connecting to VM to run build.sh...
echo.
ssh -t kaka@192.168.229.131 "cd /home/kaka/桌面/LS2K0300_Library/LS2K0300_Library/Seekfree_LS2K0300_Opensource_Library/project/user && ./build.sh"
echo.
echo Press any key to return...
pause > nul
goto MENU

:RUN_PROJECT
echo.
echo Connecting to Car to run ./project...
echo.
ssh -t root@192.168.1.104 "cd /home/root/ && ./project"
echo.
echo Press any key to return...
pause > nul
goto MENU

:EXIT
exit
