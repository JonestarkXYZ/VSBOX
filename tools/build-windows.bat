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
    SET "SOURCE_NAME=%%~nF"
    SET "SOURCE_EXT=%%~xF"
)

REM Instalar el BAT versionado dentro del toolchain local y preparar ruta corta.
IF NOT EXIST "%ROOT_DIR%\TURBOC3\BUILD" MKDIR "%ROOT_DIR%\TURBOC3\BUILD"
COPY /Y "%ROOT_DIR%\tools\RUN.BAT" "%ROOT_DIR%\TURBOC3\BIN\RUN.BAT" >NUL
IF ERRORLEVEL 1 (
    ECHO Error: no se pudo copiar tools\RUN.BAT a TURBOC3\BIN.
    EXIT /B 1
)
COPY /Y "%SOURCE_INPUT%" "%ROOT_DIR%\TURBOC3\BUILD\!SOURCE_NAME!!SOURCE_EXT!" >NUL
IF ERRORLEVEL 1 (
    ECHO Error: no se pudo copiar el archivo a TURBOC3\BUILD.
    EXIT /B 1
)

REM DOSBox monta el proyecto como C: y delega la compilacion real a RUN.BAT.
ECHO DOSBox: compilando y ejecutando !REL_PATH!
dosbox -noconsole ^
    -c "mount c \"%ROOT_DIR%\"" ^
    -c "c:" ^
    -c "cd TURBOC3\BIN" ^
    -c "RUN.BAT TURBOC3\BUILD !SOURCE_NAME! !SOURCE_EXT!" >NUL 2>NUL

ECHO Log: %ROOT_DIR%\logs\OUTPUT.TXT
