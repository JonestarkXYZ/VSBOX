# VSBOX

Entorno de trabajo para compilar y ejecutar programas C/C++ de estilo Turbo C dentro de DOSBox desde VSCode.

El proyecto monta la carpeta actual como unidad `C:` en DOSBox, compila el archivo abierto con `TCC.EXE` y ejecuta el `.EXE` generado automáticamente.

## Requisitos

- VSCode.
- Extensión de Python para VSCode, recomendada para ejecutar el instalador gráfico del toolchain.
- DOSBox instalado en el sistema.
- ZIP o carpeta local de Turbo C 3 compatible.

## Instalar DOSBox

### Linux

```bash
sudo apt install dosbox
```

### Windows

Instala DOSBox y asegúrate de que `dosbox.exe` esté disponible en el `PATH`.

## Preparar Turbo C 3

El toolchain `TURBOC3/` no se incluye en el repositorio. Debe instalarse localmente después de clonar o descargar el proyecto.

Una fuente usada en pruebas es:

```text
https://sourceforge.net/projects/turbocpp.mirror/files/v3.2/Turbo.C.3.2.zip/download
```

Esa descarga es un mirror/reempaquetado externo, no una fuente oficial de Borland/Embarcadero. El uso del toolchain queda sujeto a la licencia correspondiente.

### Instalación desde VSCode

1. Descarga `Turbo.C.3.2.zip`.
2. Abre esta carpeta del proyecto en VSCode.
3. Abre el archivo:

```text
tools/setup_turboc3.py
```

4. Ejecuta el archivo con `Run Python File`.
5. Selecciona el ZIP descargado o la carpeta `Turbo.C.3.2` ya descomprimida.
6. El instalador detecta la carpeta correcta y la copia al proyecto como:

```text
TURBOC3/
```

La carpeta válida debe contener:

```text
BIN/TCC.EXE
INCLUDE/
LIB/
BGI/
```

Para el ZIP de SourceForge indicado, la carpeta interna correcta es:

```text
Turbo.C.3.2/WinRoot/TURBOC3
```

No es necesario copiarla manualmente si se usa `tools/setup_turboc3.py`.

### Instalación por consola

Linux:

```bash
python3 tools/setup_turboc3.py ~/Descargas/Turbo.C.3.2.zip
```

Windows:

```bat
py tools\setup_turboc3.py %USERPROFILE%\Downloads\Turbo.C.3.2.zip
```

Si ya existe `TURBOC3/` y debe reemplazarse:

```bash
python3 tools/setup_turboc3.py ~/Descargas/Turbo.C.3.2.zip --force
```

## Ejecutar un archivo C/C++

1. Abre en VSCode el archivo `.C` o `.CPP` que quieras ejecutar.
2. Presiona:

```text
Ctrl + Shift + B
```

VSCode ejecuta la tarea de compilación, abre DOSBox, compila el archivo activo y ejecuta el programa automáticamente.

La salida de compilación queda en:

```text
logs/OUTPUT.TXT
```

El historial de compilaciones queda en:

```text
logs/LOG.TXT
```

## Ejecutar por consola

Linux/macOS:

```bash
./tools/build-linux.sh projects/Show3D/3D.C
```

Windows:

```bat
tools\build-windows.bat projects\Show3D\3D.C
```

## Estructura

- `src/`: código principal del proyecto.
- `projects/`: ejemplos y pruebas C/C++.
- `models/`: modelos de vértices/aristas usados por ejemplos 3D.
- `logs/`: logs generados por compilación y ejecución.
- `tools/setup_turboc3.py`: instalador local del toolchain desde ZIP o carpeta.
- `tools/RUN.BAT`: script DOS usado dentro de DOSBox.
- `tools/DOSDATE.C`: auxiliar propio para fecha/hora en logs.
- `tools/build-linux.sh`: lanzador para Linux/macOS.
- `tools/build-windows.bat`: lanzador para Windows.
- `.vscode/tasks.json`: tarea de VSCode para `Ctrl + Shift + B`.

## Funcionamiento interno

1. El script de build copia el archivo activo a `TURBOC3/BUILD`.
2. Se copia `tools/RUN.BAT` a `TURBOC3/BIN/RUN.BAT`.
3. DOSBox monta la raíz del proyecto como `C:`.
4. `RUN.BAT` compila con `TCC.EXE`.
5. El ejecutable se guarda en `TURBOC3/EXECUTE`.
6. El programa se ejecuta desde `C:\TURBOC3` para que rutas relativas como `BGI` funcionen correctamente.

## Licencia

El código propio del proyecto está bajo la licencia indicada en `LICENSE`.

El toolchain `TURBOC3/` no forma parte de la licencia del proyecto. No se redistribuyen binarios, headers, librerías ni documentación de Turbo C, Turbo C++, Borland o Embarcadero.

## Notas para repositorios públicos

- No subir `TURBOC3/`.
- No subir `Turbo.C.3.2/`.
- No subir archivos `.zip` del toolchain.
- No subir salidas generadas como `.EXE`, `.OBJ`, `logs/OUTPUT.TXT` o `logs/LOG.TXT`.
