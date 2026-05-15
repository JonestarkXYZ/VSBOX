@ECHO OFF
CLS

IF NOT EXIST C:\TURBOC3\EXECUTE MD C:\TURBOC3\EXECUTE
IF NOT EXIST C:\logs MD C:\logs

REM Validar parametros recibidos desde DOSBox/VSCode:
REM %1 = carpeta relativa, %2 = nombre sin extension, %3 = extension.
IF "%1"=="" GOTO USAGE
IF "%2"=="" GOTO USAGE
IF "%3"=="" GOTO USAGE

REM Limpiar solo la salida del archivo actual para no borrar otros builds.
IF EXIST C:\TURBOC3\EXECUTE\%2%.EXE DEL C:\TURBOC3\EXECUTE\%2%.EXE >NUL
IF EXIST C:\TURBOC3\EXECUTE\%2%.OBJ DEL C:\TURBOC3\EXECUTE\%2%.OBJ >NUL

ECHO =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= INICIO =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= > C:\logs\OUTPUT.TXT
ECHO. >> C:\logs\OUTPUT.TXT

REM DOSDATE.C es codigo propio versionado en tools; se compila temporalmente.
IF EXIST C:\logs\DATETIME.TXT DEL C:\logs\DATETIME.TXT >NUL
IF EXIST C:\tools\DOSDATE.C TCC.EXE -IC:\TURBOC3\INCLUDE -LC:\TURBOC3\LIB -nC:\TURBOC3\BUILD C:\tools\DOSDATE.C >NUL
IF EXIST C:\TURBOC3\BUILD\DOSDATE.EXE C:\TURBOC3\BUILD\DOSDATE.EXE
IF EXIST C:\logs\DATETIME.TXT TYPE C:\logs\DATETIME.TXT >> C:\logs\OUTPUT.TXT
IF NOT EXIST C:\logs\DATETIME.TXT ECHO FECHA/HORA ............: no registrada por RUN.BAT >> C:\logs\OUTPUT.TXT
ECHO. >> C:\logs\OUTPUT.TXT

ECHO FILE PATH .............: C:\%1 >> C:\logs\OUTPUT.TXT
ECHO FILE NAME .............: %2 >> C:\logs\OUTPUT.TXT
ECHO FILE EXTENSION ........: %3 >> C:\logs\OUTPUT.TXT
ECHO FILE FULLPATH .........: C:\%1\%2%3 >> C:\logs\OUTPUT.TXT
ECHO. >> C:\logs\OUTPUT.TXT

ECHO =============================== COMPILANDO =============================== >> C:\logs\OUTPUT.TXT

REM Compilar usando INCLUDE, LIB y salida en EXECUTE.
TCC.EXE -IC:\TURBOC3\INCLUDE -LC:\TURBOC3\LIB -nC:\TURBOC3\EXECUTE C:\%1\%2%3 GRAPHICS.LIB >> C:\logs\OUTPUT.TXT

REM Verificar si el EXE fue creado
IF NOT EXIST C:\TURBOC3\EXECUTE\%2%.EXE GOTO ERROR

ECHO Archivo compilado .EXE guardado en C:\TURBOC3\EXECUTE\%2%.EXE >> C:\logs\OUTPUT.TXT
ECHO. >> C:\logs\OUTPUT.TXT

REM Ejecutar desde C:\TURBOC3 para que rutas relativas como "BGI" funcionen.
CD C:\TURBOC3
CLS
C:\TURBOC3\EXECUTE\%2%.EXE
GOTO END

:USAGE
  ECHO Uso: RUN.BAT CARPETA_RELATIVA NOMBRE EXTENSION > C:\logs\OUTPUT.TXT
  ECHO Ejemplo: RUN.BAT projects\Show3D 3D5 .C >> C:\logs\OUTPUT.TXT
  GOTO END

:ERROR
  ECHO =================================== ERROR =================================== >> C:\logs\OUTPUT.TXT
  ECHO. >> C:\logs\OUTPUT.TXT
  ECHO --- NO SE PUDO COMPILAR EL ARCHIVO --- >> C:\logs\OUTPUT.TXT
  ECHO. >> C:\logs\OUTPUT.TXT
  GOTO END

:END
  ECHO =================================== END =================================== >> C:\logs\OUTPUT.TXT
  ECHO. >> C:\logs\OUTPUT.TXT
  ECHO. >> C:\logs\OUTPUT.TXT
  ECHO. >> C:\logs\OUTPUT.TXT
  ECHO. >> C:\logs\OUTPUT.TXT

  REM Agregar log al final
  TYPE C:\logs\OUTPUT.TXT >> C:\logs\LOG.TXT
  EXIT
