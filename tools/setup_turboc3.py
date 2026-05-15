#!/usr/bin/env python3
"""
Instala un toolchain local compatible con Turbo C 3 desde un ZIP o carpeta.

El script no descarga automaticamente el ZIP para evitar redistribuir o
depender de mirrors externos. El usuario debe descargarlo por su cuenta y
ejecutar este instalador con la ruta local del ZIP o carpeta descomprimida.
"""

from __future__ import annotations

import argparse
import shutil
import sys
import tempfile
import zipfile
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parents[1]
DEST_DIR = PROJECT_ROOT / "TURBOC3"
RUN_BAT = PROJECT_ROOT / "tools" / "RUN.BAT"

REQUIRED_PATHS = (
    Path("BIN") / "TCC.EXE",
    Path("INCLUDE"),
    Path("LIB"),
    Path("BGI"),
)


def ask_yes_no(title: str, message: str) -> bool:
    """Pregunta confirmacion por GUI si es posible; si no, usa consola."""
    try:
        from tkinter import Tk, messagebox

        root = Tk()
        root.withdraw()
        root.attributes("-topmost", True)
        answer = messagebox.askyesno(title, message, parent=root)
        root.destroy()
        return bool(answer)
    except Exception:
        response = input(f"{message} [s/N]: ").strip().lower()
        return response in ("s", "si", "y", "yes")


def choose_source_gui() -> Path | None:
    """Abre un selector para elegir ZIP o carpeta del toolchain."""
    try:
        from tkinter import Tk, filedialog, messagebox
    except Exception as exc:
        print(f"Error: no se pudo cargar tkinter: {exc}", file=sys.stderr)
        return None

    root = Tk()
    root.withdraw()
    root.attributes("-topmost", True)

    messagebox.showinfo(
        "Instalar Turbo C 3",
        "Selecciona el ZIP Turbo.C.3.2.zip o la carpeta Turbo.C.3.2 ya descomprimida.",
        parent=root,
    )

    zip_path = filedialog.askopenfilename(
        parent=root,
        title="Seleccionar Turbo.C.3.2.zip",
        filetypes=(("Archivos ZIP", "*.zip"), ("Todos los archivos", "*.*")),
    )

    if zip_path:
        root.destroy()
        return Path(zip_path)

    directory_path = filedialog.askdirectory(
        parent=root,
        title="O selecciona la carpeta Turbo.C.3.2 descomprimida",
    )
    root.destroy()

    if directory_path:
        return Path(directory_path)

    return None


def is_toolchain_root(path: Path) -> bool:
    """Comprueba si una carpeta tiene la estructura minima esperada."""
    return all((path / required).exists() for required in REQUIRED_PATHS)


def find_toolchain_root(extracted_root: Path) -> Path:
    """Busca la carpeta que contiene BIN/TCC.EXE, INCLUDE, LIB y BGI."""
    candidates = [extracted_root]
    candidates.extend(path for path in extracted_root.rglob("*") if path.is_dir())

    for candidate in candidates:
        if is_toolchain_root(candidate):
            return candidate

    raise FileNotFoundError(
        "No se encontro una carpeta valida con BIN/TCC.EXE, INCLUDE, LIB y BGI."
    )


def copy_toolchain(source: Path, destination: Path, force: bool) -> None:
    """Copia el toolchain detectado hacia TURBOC3/."""
    if destination.exists():
        if not force:
            raise FileExistsError(
                f"Ya existe {destination}. Usa --force para reemplazarla."
            )
        shutil.rmtree(destination)

    shutil.copytree(source, destination)


def install_run_bat(destination: Path) -> None:
    """Instala el RUN.BAT versionado dentro de TURBOC3/BIN."""
    bin_dir = destination / "BIN"
    bin_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(RUN_BAT, bin_dir / "RUN.BAT")


def install_from_directory(directory_path: Path, force: bool) -> None:
    """Detecta el toolchain dentro de una carpeta ya descomprimida."""
    if not directory_path.is_dir():
        raise NotADirectoryError(f"No existe la carpeta: {directory_path}")

    toolchain_root = find_toolchain_root(directory_path)
    copy_toolchain(toolchain_root, DEST_DIR, force)
    install_run_bat(DEST_DIR)


def install_from_zip(zip_path: Path, force: bool) -> None:
    """Extrae el ZIP en temporal, detecta el toolchain y lo instala."""
    if not zip_path.is_file():
        raise FileNotFoundError(f"No existe el ZIP: {zip_path}")

    if not zipfile.is_zipfile(zip_path):
        raise ValueError(f"El archivo no parece ser un ZIP valido: {zip_path}")

    with tempfile.TemporaryDirectory(prefix="vsbox-turboc3-") as temp_name:
        temp_dir = Path(temp_name)

        with zipfile.ZipFile(zip_path, "r") as zip_file:
            zip_file.extractall(temp_dir)

        toolchain_root = find_toolchain_root(temp_dir)
        copy_toolchain(toolchain_root, DEST_DIR, force)
        install_run_bat(DEST_DIR)


def install_from_path(source_path: Path, force: bool) -> None:
    """Instala desde ZIP o carpeta, segun el tipo de ruta recibida."""
    if source_path.is_dir():
        install_from_directory(source_path, force)
    else:
        install_from_zip(source_path, force)

    print(f"Toolchain instalado en: {DEST_DIR}")
    print(f"RUN.BAT instalado en: {DEST_DIR / 'BIN' / 'RUN.BAT'}")


def parse_args() -> argparse.Namespace:
    """Define argumentos de consola."""
    parser = argparse.ArgumentParser(
        description="Instala TURBOC3/ desde un ZIP o carpeta descargada manualmente."
    )
    parser.add_argument(
        "source_path",
        nargs="?",
        type=Path,
        help="Ruta local del ZIP o carpeta, por ejemplo: ~/Descargas/Turbo.C.3.2.zip",
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="Reemplaza TURBOC3/ si ya existe.",
    )
    return parser.parse_args()


def main() -> int:
    """Punto de entrada del instalador."""
    args = parse_args()
    source_path = args.source_path
    force = args.force

    if source_path is None:
        source_path = choose_source_gui()
        if source_path is None:
            print("Instalacion cancelada.")
            return 1

        if DEST_DIR.exists():
            force = ask_yes_no(
                "Reemplazar TURBOC3",
                f"Ya existe {DEST_DIR}.\n\n¿Quieres reemplazarla?",
            )
            if not force:
                print("Instalacion cancelada: TURBOC3 ya existe.")
                return 1

    try:
        install_from_path(source_path.expanduser().resolve(), force)
    except Exception as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
