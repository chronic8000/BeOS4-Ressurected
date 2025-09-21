@echo off
REM **************************************************************************
REM
REM M.BAT - Simplistic interface to nmake
REM
REM Copyright (c) 1996-1997 Crystal Semiconductor Corp.
REM
REM **************************************************************************

if NOT "%WATCOM%" == "" goto set
echo %0: WATCOM Environment Variable is not set.
goto exit
:set
set MAKE=%WATCOM%\binw\wmake
if EXIST %MAKE%.exe goto exists
echo %0: WATCOM is set to %WATCOM% but %MAKE% does not exist.
goto exit
:exists

goto doit



set MAKEOPTS=/NOLOGO
set ____OLD_DEBUG=%DEBUG%
set ____OLD_SYMBOLS=%SYMBOLS%

if "%1"=="-"    goto doit
if "%1"=="e"    goto Normal
if "%1"=="E"    goto Normal
if "%1"=="a"    goto NormalAll
if "%1"=="A"    goto NormalAll
if "%1"=="p"    goto Prod
if "%1"=="P"    goto Prod
if "%1"=="ap"   goto ProdAll
if "%1"=="AP"   goto ProdAll
if "%1"=="r"    goto Rel
if "%1"=="R"    goto Rel
if "%1"=="ar"   goto RelAll
if "%1"=="AR"   goto RelAll
if "%1"=="c"    goto Clean
if "%1"=="C"    goto Clean
if "%1"=="?"    goto Help
if not "%1"=="" goto Error
goto Normal

:NormalAll
set MAKEOPTS=%MAKEOPTS% -a
goto Normal

:Normal
set DEBUG=
set SYMBOLS=1
goto doit

:RelAll
set RELEASE=1
goto ProdAll

:Rel
set RELEASE=1
goto Prod

:ProdAll
set MAKEOPTS=%MAKEOPTS% -a
goto Prod

:Prod
set DEBUG=
set SYMBOLS=
goto doit

:Clean
set MAKEOPTS=%MAKEOPTS% clean
goto doit

:doit
REM if "%SILENT%"=="1" goto :silent
%MAKE% 
REM %MAKEOPTS% %2 %3 %4 %5 %6 %7 %8 %9 
goto end

:silent
@echo "Silent make..."
%MAKE% %MAKEOPTS% %2 %3 %4 %5 %6 %7 %8 %9 
goto end

:Error
echo %0: Unrecognized option "%1" supplied.
goto end

:Help
echo %0 {option {nmake options}}
echo.
echo     Options:
echo         [none] - normal build
echo         e      - normal build
echo         p      - production build
echo         r      - release build
echo         a      - rebuild everything
echo         ap     - production rebuild everything
echo         ar     - release rebuild everthing
echo         c      - clean build products from system
echo.
echo     Note:  SYMBOLS and DEBUG are individually settable for normal
echo            builds by setting them in the environment.

:end
set MAKEOPTS=
set RELEASE=
set DEBUG=%____OLD_DEBUG%
set SYMBOLS=%____OLD_SYMBOLS%
set ____OLD_DEBUG=
set ____OLD_SYMBOLS=
:exit
echo.
