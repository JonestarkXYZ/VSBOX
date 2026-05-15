@ECHO OFF
SETLOCAL EnableExtensions EnableDelayedExpansion

REM Compila y ejecuta un archivo C/C++ de Turbo C usando DOSBox desde Windows.
REM Uso: tools\build-windows.bat projects\Show3D\3D.C

IF "%~1"=="" (
    ECHO Uso: %~nx0 ruta\relativa\archivo.C
    EXIT /B 2
)

SET "ROOT_DIR=%~dp0.."
SET "SOURCE_INPUT=%~1"

REM Resolver la raiz real del proyecto para montar una ruta estable en DOSBox.
FOR /F "usebackq delims=" %%R IN (`powershell -NoProfile -ExecutionPolicy Bypass -Command "(Resolve-Path $env:ROOT_DIR).Path"`) DO (
    SET "ROOT_DIR=%%R"
)

WHERE dosbox >NUL 2>NUL
IF ERRORLEVEL 1 (
    ECHO Error: dosbox.exe no esta instalado o no esta en PATH.
    EXIT /B 127
)

IF NOT EXIST "%ROOT_DIR%\TURBOC3\BIN\TCC.EXE" (
    ECHO Error: falta TURBOC3\BIN\TCC.EXE.
    ECHO Copia tu instalacion licenciada de Turbo C 3 en TURBOC3\.
    EXIT /B 2
)

REM Si la ruta recibida es relativa, buscarla primero dentro de la raiz del proyecto.
IF NOT EXIST "%SOURCE_INPUT%" IF EXIST "%ROOT_DIR%\%SOURCE_INPUT%" (
    SET "SOURCE_INPUT=%ROOT_DIR%\%SOURCE_INPUT%"
)

IF NOT EXIST "%SOURCE_INPUT%" (
    ECHO Error: no existe el archivo: %SOURCE_INPUT%
    EXIT /B 2
)

REM Calcular ruta relativa al proyecto sin depender de la carpeta actual.
FOR /F "usebackq delims=" %%R IN (`powershell -NoProfile -ExecutionPolicy Bypass -Command "$root=(Resolve-Path $env:ROOT_DIR).Path; $file=(Resolve-Path $env:SOURCE_INPUT).Path; if(-not $file.StartsWith($root,[StringComparison]::OrdinalIgnoreCase)){exit 3}; $uriRoot=New-Object System.Uri(($root.TrimEnd('\')+'\')); $uriFile=New-Object System.Uri($file); [Uri]::UnescapeDataString($uriRoot.MakeRelativeUri($uriFile).ToString()).Replace('/','\')"`) DO (
    SET "REL_PATH=%%R"
)

IF NOT DEFINED REL_PATH (
    ECHO Error: no se pudo calcular la ruta relativa del archivo.
    EXIT /B 2
)

FOR %%F IN ("!REL_PATH!") DO (
    SET "SOURCE_EXT=%%~xF"
)

SET "STAGED_DIR=%ROOT_DIR%\TURBOC3\BUILD"
SET "STAGED_NAME=ACTIVE"
SET "STAGED_EXT=!SOURCE_EXT!"
SET "SELECTOR_SOURCE=%ROOT_DIR%\projects\FSSELECT\FSSEL.C"
SET "SELECTOR_EXE=%STAGED_DIR%\FSSEL.EXE"
SET "SELECTOR_CFG=%ROOT_DIR%\TURBOC3\FULLSCR.CFG"
SET "SELECTOR_BUILD_LOG=%ROOT_DIR%\logs\FSSELECT_BUILD.TXT"

IF NOT EXIST "%SELECTOR_SOURCE%" (
    ECHO Error: falta el selector de pantalla: %SELECTOR_SOURCE%
    EXIT /B 2
)

REM Instalar el BAT versionado dentro del toolchain local y preparar ruta corta.
IF NOT EXIST "%STAGED_DIR%" MKDIR "%STAGED_DIR%"
IF NOT EXIST "%ROOT_DIR%\logs" MKDIR "%ROOT_DIR%\logs"
COPY /Y "%ROOT_DIR%\tools\RUN.BAT" "%ROOT_DIR%\TURBOC3\BIN\RUN.BAT" >NUL
IF ERRORLEVEL 1 (
    ECHO Error: no se pudo copiar tools\RUN.BAT a TURBOC3\BIN.
    EXIT /B 1
)

REM Turbo C/DOSBox trabaja de forma mas estable con nombres 8.3 sin espacios.
REM El archivo original puede tener espacios; dentro de DOSBox se compila ACTIVE.C.
COPY /Y "%SOURCE_INPUT%" "%STAGED_DIR%\!STAGED_NAME!!STAGED_EXT!" >NUL
IF ERRORLEVEL 1 (
    ECHO Error: no se pudo copiar el archivo a TURBOC3\BUILD.
    EXIT /B 1
)

REM El selector es una herramienta auxiliar: no usa RUN.BAT ni compila el archivo activo.
SET "BUILD_SELECTOR=0"
IF NOT EXIST "%SELECTOR_EXE%" SET "BUILD_SELECTOR=1"
IF EXIST "%SELECTOR_EXE%" (
    FOR /F "usebackq delims=" %%B IN (`powershell -NoProfile -ExecutionPolicy Bypass -Command "if ((Get-Item $env:SELECTOR_SOURCE).LastWriteTime -gt (Get-Item $env:SELECTOR_EXE).LastWriteTime) { '1' } else { '0' }"`) DO (
        SET "BUILD_SELECTOR=%%B"
    )
)

IF "!BUILD_SELECTOR!"=="1" (
    ECHO DOSBox: compilando selector de pantalla
    dosbox -noconsole ^
        -c "mount c \"%ROOT_DIR%\"" ^
        -c "c:" ^
        -c "cd TURBOC3\BIN" ^
        -c "TCC.EXE -IC:\TURBOC3\INCLUDE -LC:\TURBOC3\LIB -nC:\TURBOC3\BUILD C:\projects\FSSELECT\FSSEL.C" ^
        -c "exit" >"%SELECTOR_BUILD_LOG%" 2>&1
)

IF NOT EXIST "%SELECTOR_EXE%" (
    ECHO Error: no se pudo compilar el selector de pantalla.
    ECHO Log: %SELECTOR_BUILD_LOG%
    EXIT /B 1
)

REM Esta primera sesion solo pregunta si se quiere pantalla completa o ventana.
ECHO DOSBox: seleccionando modo de pantalla
dosbox -noconsole ^
    -c "mount c \"%ROOT_DIR%\"" ^
    -c "c:" ^
    -c "C:\TURBOC3\BUILD\FSSEL.EXE" ^
    -c "exit" >NUL 2>NUL

REM El selector guarda 1 para fullscreen y 0 para ventana. Si falta el archivo,
REM se usa fullscreen para conservar el comportamiento anterior.
SET "SCREEN_MODE=1"
IF EXIST "%SELECTOR_CFG%" SET /P SCREEN_MODE=<"%SELECTOR_CFG%"
SET "SCREEN_MODE=!SCREEN_MODE:~0,1!"
SET "DOSBOX_SCREEN_ARG=-fullscreen"
IF "!SCREEN_MODE!"=="0" (
    SET "DOSBOX_SCREEN_ARG="
    ECHO DOSBox: modo ventana
) ELSE (
    ECHO DOSBox: modo pantalla completa
)

REM DOSBox monta el proyecto como C: y delega la compilacion real a RUN.BAT.
ECHO DOSBox: compilando y ejecutando !REL_PATH!
dosbox !DOSBOX_SCREEN_ARG! -noconsole ^
    -c "mount c \"%ROOT_DIR%\"" ^
    -c "c:" ^
    -c "cd TURBOC3\BIN" ^
    -c "RUN.BAT TURBOC3\BUILD !STAGED_NAME! !STAGED_EXT!" >NUL 2>NUL

ECHO Log: %ROOT_DIR%\logs\OUTPUT.TXT
