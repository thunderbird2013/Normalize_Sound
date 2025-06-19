@echo off
setlocal

:: === KONFIGURATION ===
set EXE_NAME=normalize
set BUILD_DIR=build
set COMPILER=clang++.exe
set STRIP=strip.exe

:: === TOOLCHAIN-PFAD setzen ===
echo [INFO] Setze PATH für clang++, mingw32-make, strip...
set PATH=C:\msys64\clang64\bin;%PATH%

:: === VORHERIGES BUILD VERZEICHNIS LÖSCHEN ===
echo [INFO] Entferne vorheriges Build-Verzeichnis (%BUILD_DIR%)...
if exist %BUILD_DIR% (
    rmdir /s /q %BUILD_DIR%
)

:: === BUILD-VERZEICHNIS ERSTELLEN ===
echo [INFO] Erstelle neues Build-Verzeichnis...
mkdir %BUILD_DIR%
cd %BUILD_DIR%

:: === CMAKE AUSFÜHREN ===
echo [INFO] Konfiguriere Projekt mit CMake...
cmake -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=%COMPILER% ..
if errorlevel 1 (
    echo [ERROR] CMake-Konfiguration fehlgeschlagen!
    goto fail
)

:: === BUILD STARTEN ===
echo [INFO] Kompiliere mit mingw32-make...
mingw32-make.exe
if errorlevel 1 (
    echo [ERROR] Kompilierung fehlgeschlagen!
    goto fail
)

:: === EXE INS ROOT KOPIEREN ===
echo [INFO] Kopiere EXE nach Projekt-Root...
copy %EXE_NAME%.exe ..\%EXE_NAME%.exe >nul

:: === DATEIGRÖSSE VOR STRIP ANZEIGEN ===
for %%F in (..\%EXE_NAME%.exe) do (
    set SIZE_BEFORE=%%~zF
)
echo [INFO] Größe vor Strip: %SIZE_BEFORE% Bytes

:: === STRIP AUSFÜHREN ===
echo [INFO] Entferne Debug-Symbole mit strip.exe...
%STRIP% ..\%EXE_NAME%.exe

:: === DATEIGRÖSSE NACH STRIP ANZEIGEN ===
for %%F in (..\%EXE_NAME%.exe) do (
    set SIZE_AFTER=%%~zF
)
echo [INFO] Größe nach Strip: %SIZE_AFTER% Bytes

:: === AUFRÄUMEN ===
cd ..
echo [INFO] Lösche Build-Verzeichnis...
rmdir /s /q %BUILD_DIR%

:: === ERFOLGSMELDUNG ===
echo.
echo ✅ Build abgeschlossen. Executable: %EXE_NAME%.exe
echo ✅ Größe reduziert von %SIZE_BEFORE% auf %SIZE_AFTER% Bytes
pause
exit /b 0

:fail
echo.
echo ❌ Build abgebrochen wegen eines Fehlers.
pause
exit /b 1




