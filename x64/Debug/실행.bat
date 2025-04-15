@echo off
:: 관리자 권한 확인 및 요청
>nul 2>&1 "%SYSTEMROOT%\system32\cacls.exe" "%SYSTEMROOT%\system32\config\system"
if '%errorlevel%' NEQ '0' (
    echo 관리자 권한 요청 중...
    goto UACPrompt
) else (
    goto gotAdmin
)

:UACPrompt
    echo Set UAC = CreateObject^("Shell.Application"^) > "%temp%\getadmin.vbs"
    echo UAC.ShellExecute "%~s0", "", "", "runas", 1 >> "%temp%\getadmin.vbs"
    "%temp%\getadmin.vbs"
    exit /B

:gotAdmin
    if exist "%temp%\getadmin.vbs" del "%temp%\getadmin.vbs"
    pushd "%CD%"
    CD /D "%~dp0"

:: 여기부터 실제 명령 시작
echo 관리자 권한으로 실행 중...
chcp 65001 > nul

set "pfxfile=MyCertificate.pfx"
set "password=123123"
set "wdkfolder=C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0"
set "debugfolder=%~dp0"

del "C:\Windows\INF\setupapi.dev.log"

"%wdkfolder%\x64\signtool.exe" sign /f "%debugfolder%\%pfxfile%" /p "%password%" /fd SHA256 /tr http://timestamp.digicert.com?td=sha256 /td SHA256 "%debugfolder%\hidriver\wudf.cat"
timeout /t 1 /nobreak
"%wdkfolder%\x64\signtool.exe" sign /f "%debugfolder%\%pfxfile%" /p "123123" /fd SHA256 /tr http://timestamp.digicert.com?td=sha256 /td SHA256 "%debugfolder%\hidriver\hidriver.sys"
timeout /t 1 /nobreak
"%wdkfolder%\x64\devcon.exe" install "%debugfolder%\hidriver\hidriver.inf" "root\hidriver"

echo.
if %ERRORLEVEL% EQU 0 (
    echo 드라이버 설치 성공!
) else (
    echo 드라이버 설치 실패! 오류 코드: %ERRORLEVEL%
)
pause