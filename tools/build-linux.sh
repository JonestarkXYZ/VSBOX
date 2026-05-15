#!/usr/bin/env bash
set -euo pipefail

# Compila y ejecuta un archivo C/C++ de Turbo C usando DOSBox desde Linux/macOS.
# Uso: ./tools/build-linux.sh test/Show3D/3D.C

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
SOURCE_NAME="${SOURCE_BASE%.*}"
SOURCE_EXT=".${SOURCE_BASE##*.}"
STAGED_DIR="$ROOT_DIR/TURBOC3/BUILD"
STAGED_PATH="$STAGED_DIR/$SOURCE_NAME$SOURCE_EXT"

# Instalar el BAT versionado dentro del toolchain local y preparar ruta corta.
mkdir -p "$STAGED_DIR"
cp "$ROOT_DIR/tools/RUN.BAT" "$ROOT_DIR/TURBOC3/BIN/RUN.BAT"
cp "$SOURCE_FULL_PATH" "$STAGED_PATH"

# DOSBox monta el proyecto como C: y delega la compilacion real a RUN.BAT.
echo "DOSBox: compilando y ejecutando $SOURCE_PATH"
dosbox -noconsole \
    -c "mount c \"$ROOT_DIR\"" \
    -c "c:" \
    -c "cd TURBOC3\\BIN" \
    -c "RUN.BAT TURBOC3\\BUILD $SOURCE_NAME $SOURCE_EXT" >/dev/null 2>&1

echo "Log: $ROOT_DIR/logs/OUTPUT.TXT"
