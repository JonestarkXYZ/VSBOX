#!/usr/bin/env bash
set -euo pipefail

# Compila y ejecuta un archivo C/C++ de Turbo C usando DOSBox desde Linux/macOS.
# Uso: ./tools/build-linux.sh projects/3D/Show3D/Models3D.C

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if [ "$#" -ne 1 ]; then
    echo "Uso: $0 ruta/relativa/archivo.C"
    exit 2
fi

SOURCE_PATH="$1"

# Permite pasar rutas absolutas siempre que apunten dentro del proyecto.
if [[ "$SOURCE_PATH" = /* ]]; then
    case "$SOURCE_PATH" in
        "$ROOT_DIR"/*) SOURCE_PATH="${SOURCE_PATH#$ROOT_DIR/}" ;;
        *)
            echo "Error: el archivo debe estar dentro del proyecto: $ROOT_DIR"
            exit 2
            ;;
    esac
fi

SOURCE_PATH="${SOURCE_PATH#./}"
SOURCE_FULL_PATH="$ROOT_DIR/$SOURCE_PATH"

if [ ! -f "$SOURCE_FULL_PATH" ]; then
    echo "Error: no existe el archivo: $SOURCE_PATH"
    exit 2
fi

if ! command -v dosbox >/dev/null 2>&1; then
    echo "Error: dosbox no esta instalado o no esta en PATH."
    exit 127
fi

if [ ! -f "$ROOT_DIR/TURBOC3/BIN/TCC.EXE" ]; then
    echo "Error: falta TURBOC3/BIN/TCC.EXE."
    echo "Copia tu instalacion licenciada de Turbo C 3 en TURBOC3/."
    exit 2
fi

SOURCE_BASE="$(basename "$SOURCE_PATH")"
SOURCE_EXT=".${SOURCE_BASE##*.}"
STAGED_DIR="$ROOT_DIR/TURBOC3/BUILD"
STAGED_NAME="ACTIVE"
STAGED_EXT="$SOURCE_EXT"
STAGED_PATH="$STAGED_DIR/$STAGED_NAME$STAGED_EXT"
SELECTOR_SOURCE="$ROOT_DIR/projects/FSSELECT/FSSEL.C"
SELECTOR_EXE="$STAGED_DIR/FSSEL.EXE"
SELECTOR_CFG="$ROOT_DIR/TURBOC3/FULLSCR.CFG"
SELECTOR_BUILD_LOG="$ROOT_DIR/logs/FSSELECT_BUILD.TXT"

if [ ! -f "$SELECTOR_SOURCE" ]; then
    echo "Error: falta el selector de pantalla: $SELECTOR_SOURCE"
    exit 2
fi

# Instalar el BAT versionado dentro del toolchain local y preparar ruta corta.
mkdir -p "$STAGED_DIR"
mkdir -p "$ROOT_DIR/logs"
cp "$ROOT_DIR/tools/RUN.BAT" "$ROOT_DIR/TURBOC3/BIN/RUN.BAT"

# Turbo C/DOSBox trabaja de forma mas estable con nombres 8.3 sin espacios.
# El archivo original puede tener espacios; dentro de DOSBox se compila ACTIVE.C.
cp "$SOURCE_FULL_PATH" "$STAGED_PATH"

# El selector es una herramienta auxiliar: no usa RUN.BAT ni compila el archivo activo.
if [ ! -f "$SELECTOR_EXE" ] || [ "$SELECTOR_SOURCE" -nt "$SELECTOR_EXE" ]; then
    echo "DOSBox: compilando selector de pantalla"
    dosbox -noconsole \
        -c "mount c \"$ROOT_DIR\"" \
        -c "c:" \
        -c "cd TURBOC3\\BIN" \
        -c "TCC.EXE -IC:\\TURBOC3\\INCLUDE -LC:\\TURBOC3\\LIB -nC:\\TURBOC3\\BUILD C:\\projects\\FSSELECT\\FSSEL.C" \
        -c "exit" >"$SELECTOR_BUILD_LOG" 2>&1
fi

if [ ! -f "$SELECTOR_EXE" ]; then
    echo "Error: no se pudo compilar el selector de pantalla."
    echo "Log: $SELECTOR_BUILD_LOG"
    exit 1
fi

# Esta primera sesion solo pregunta si se quiere pantalla completa o ventana.
echo "DOSBox: seleccionando modo de pantalla"
dosbox -noconsole \
    -c "mount c \"$ROOT_DIR\"" \
    -c "c:" \
    -c "C:\\TURBOC3\\BUILD\\FSSEL.EXE" \
    -c "exit" >/dev/null 2>&1

# El selector guarda 1 para fullscreen y 0 para ventana. Si falta el archivo,
# se usa fullscreen para conservar el comportamiento anterior.
SCREEN_MODE="1"
if [ -f "$SELECTOR_CFG" ]; then
    read -r SCREEN_MODE < "$SELECTOR_CFG" || SCREEN_MODE="1"
    SCREEN_MODE="${SCREEN_MODE//$'\r'/}"
    SCREEN_MODE="${SCREEN_MODE:0:1}"
fi

DOSBOX_SCREEN_ARGS=()
if [ "$SCREEN_MODE" = "0" ]; then
    echo "DOSBox: modo ventana"
else
    DOSBOX_SCREEN_ARGS=(-fullscreen)
    echo "DOSBox: modo pantalla completa"
fi

# DOSBox monta el proyecto como C: y delega la compilacion real a RUN.BAT.
echo "DOSBox: compilando y ejecutando $SOURCE_PATH"
dosbox "${DOSBOX_SCREEN_ARGS[@]}" -noconsole \
    -c "mount c \"$ROOT_DIR\"" \
    -c "c:" \
    -c "cd TURBOC3\\BIN" \
    -c "RUN.BAT TURBOC3\\BUILD $STAGED_NAME $STAGED_EXT" >/dev/null 2>&1

echo "Log: $ROOT_DIR/logs/OUTPUT.TXT"
