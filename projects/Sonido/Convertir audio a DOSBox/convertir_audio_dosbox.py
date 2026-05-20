#!/usr/bin/env python3
"""
Convierte audio moderno a un formato simple para DOSBox + Sound Blaster.

Salida principal:
  - SOUND.RAW: PCM unsigned 8-bit, mono, frecuencia configurable, sin cabecera.
  - SOUND.WAV: la misma senal con cabecera WAV para revisar en el sistema host.

El reproductor PLAYSB.C lee SOUND.RAW desde C:\\TURBOC3 cuando se ejecuta con
las tareas normales del repositorio.
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import time
import wave
from datetime import datetime
from pathlib import Path
from typing import Iterable, List, Optional


DEFAULT_RATE = 16000
DEFAULT_NAME = "SOUND"
AUDIO_PATTERNS = "*.wav *.mp3 *.ogg *.flac *.aac *.m4a *.aiff *.aif"
RATE_CONFIG_RELATIVE = Path("TURBOC3") / "SNDHZ.CFG"
SUPPORTED_RATES = (8000, 11025, 16000, 22050)
CONVERT_LOG_NAME = "CONVERT_LOG.TXT"


class AudioFormatError(Exception):
    """Error controlado para formatos de audio no compatibles."""


def get_repo_root() -> Path:
    """Calcula la raiz del repositorio desde la ubicacion de este script."""
    return Path(__file__).resolve().parents[3]


def is_supported_rate(rate: int) -> bool:
    """Mantiene el conversor sincronizado con las opciones de SELHZ.C."""
    return rate in SUPPORTED_RATES


def read_selected_rate_config(repo_root: Path) -> Optional[int]:
    """Lee TURBOC3/SNDHZ.CFG, que es escrito por SELHZ.C."""
    config_path = repo_root / RATE_CONFIG_RELATIVE

    if not config_path.exists():
        return None

    try:
        first_token = config_path.read_text(encoding="ascii").split()[0]
        rate = int(first_token)
    except (OSError, IndexError, ValueError):
        return None

    if not is_supported_rate(rate):
        return None

    return rate


def normalize_dos_stem(name: str) -> str:
    """Normaliza el nombre base a 8 caracteres seguros para DOS 8.3."""
    allowed = []

    for char in name.upper():
        if char.isalnum() or char == "_":
            allowed.append(char)

    stem = "".join(allowed)[:8]
    return stem or DEFAULT_NAME


def has_graphical_session() -> bool:
    """Detecta si vale la pena intentar un selector grafico del sistema."""
    if sys.platform.startswith("linux"):
        return bool(os.environ.get("DISPLAY") or os.environ.get("WAYLAND_DISPLAY"))

    return True


def run_picker_command(command: List[str]) -> Optional[Path]:
    """Ejecuta un selector externo y devuelve la ruta seleccionada."""
    try:
        result = subprocess.run(
            command,
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
    except OSError:
        return None

    if result.returncode != 0:
        return None

    selected = result.stdout.strip()
    if not selected:
        return None

    return Path(selected.splitlines()[-1]).expanduser()


def select_audio_file_with_system_picker() -> Optional[Path]:
    """Usa el selector nativo disponible en el sistema operativo."""
    if not has_graphical_session():
        return None

    if sys.platform.startswith("linux"):
        zenity = shutil.which("zenity")
        if zenity is not None:
            selected = run_picker_command([
                zenity,
                "--file-selection",
                "--title=Selecciona un audio para convertir a DOSBox",
                "--file-filter=Audio | {0}".format(AUDIO_PATTERNS),
                "--file-filter=Todos los archivos | *",
            ])
            if selected is not None:
                return selected

        kdialog = shutil.which("kdialog")
        if kdialog is not None:
            selected = run_picker_command([
                kdialog,
                "--getopenfilename",
                str(Path.home()),
                "{0}|Audio".format(AUDIO_PATTERNS),
            ])
            if selected is not None:
                return selected

    if sys.platform == "darwin":
        osascript = shutil.which("osascript")
        if osascript is not None:
            return run_picker_command([
                osascript,
                "-e",
                'POSIX path of (choose file with prompt "Selecciona un audio para convertir a DOSBox")',
            ])

    if os.name == "nt":
        powershell = shutil.which("powershell") or shutil.which("pwsh")
        if powershell is not None:
            script = (
                "Add-Type -AssemblyName System.Windows.Forms;"
                "$d=New-Object System.Windows.Forms.OpenFileDialog;"
                "$d.Title='Selecciona un audio para convertir a DOSBox';"
                "$d.Filter='Audio|*.wav;*.mp3;*.ogg;*.flac;*.aac;*.m4a;*.aiff;*.aif|Todos los archivos|*.*';"
                "if($d.ShowDialog() -eq 'OK'){Write-Output $d.FileName}"
            )
            return run_picker_command([
                powershell,
                "-NoProfile",
                "-ExecutionPolicy",
                "Bypass",
                "-Command",
                script,
            ])

    return None


def select_audio_file() -> Path:
    """Abre selector del sistema si es posible; si no, usa respaldos."""
    selected = select_audio_file_with_system_picker()
    if selected is not None:
        return selected

    try:
        import tkinter as tk
        from tkinter import filedialog

        root = tk.Tk()
        root.withdraw()
        root.update()
        selected = filedialog.askopenfilename(
            title="Selecciona un audio para convertir a DOSBox",
            filetypes=[
                ("Audio", AUDIO_PATTERNS),
                ("Todos los archivos", "*.*"),
            ],
        )
        root.destroy()

        if selected:
            return Path(selected).expanduser()
    except Exception as exc:
        print("Selector grafico no disponible:", exc)

    typed = input("Ruta del audio a convertir: ").strip().strip('"')
    if not typed:
        raise AudioFormatError("No se selecciono ningun audio.")

    return Path(typed).expanduser()


def run_command(command: List[str]) -> None:
    """Ejecuta un comando externo mostrando un error corto si falla."""
    try:
        subprocess.run(command, check=True)
    except subprocess.CalledProcessError as exc:
        raise AudioFormatError("ffmpeg no pudo convertir el archivo.") from exc


def delete_if_exists(path: Path, log_lines: List[str]) -> None:
    """Borra salidas viejas para que cada conversion empiece desde cero."""
    try:
        if path.exists():
            path.unlink()
            log_lines.append("Borrado previo: {0}".format(path))
    except OSError as exc:
        raise AudioFormatError("No se pudo borrar salida vieja: {0}".format(path)) from exc


def verify_wav_file(wav_path: Path, expected_rate: int) -> int:
    """Verifica que SOUND.WAV realmente quedo en mono/u8/Hz esperados."""
    with wave.open(str(wav_path), "rb") as source:
        channels = source.getnchannels()
        sample_width = source.getsampwidth()
        frame_rate = source.getframerate()
        frame_count = source.getnframes()

    if channels != 1:
        raise AudioFormatError("Verificacion WAV fallo: canales={0}".format(channels))
    if sample_width != 1:
        raise AudioFormatError("Verificacion WAV fallo: bytes={0}".format(sample_width))
    if frame_rate != expected_rate:
        raise AudioFormatError("Verificacion WAV fallo: Hz={0}".format(frame_rate))

    return frame_count


def write_conversion_log(log_path: Path, log_lines: List[str]) -> None:
    """Guarda un log completo de la ultima conversion."""
    log_path.write_text("\n".join(log_lines) + "\n", encoding="utf-8")


def extract_raw_from_wav(wav_path: Path, raw_path: Path, rate: int) -> bytes:
    """Extrae bytes PCM unsigned 8-bit mono desde un WAV ya normalizado."""
    chunks = []

    with wave.open(str(wav_path), "rb") as source:
        if source.getnchannels() != 1:
            raise AudioFormatError("El WAV normalizado no quedo en mono.")
        if source.getsampwidth() != 1:
            raise AudioFormatError("El WAV normalizado no quedo en 8 bits.")
        if source.getframerate() != rate:
            raise AudioFormatError("El WAV normalizado no quedo a la tasa pedida.")

        with raw_path.open("wb") as output:
            while True:
                data = source.readframes(4096)
                if not data:
                    break
                output.write(data)
                chunks.append(data)

    return b"".join(chunks)


def convert_with_ffmpeg(input_path: Path, wav_path: Path, raw_path: Path, rate: int) -> bytes:
    """Usa ffmpeg para aceptar formatos como MP3, OGG, FLAC o M4A."""
    ffmpeg = shutil.which("ffmpeg")
    if ffmpeg is None:
        raise AudioFormatError("ffmpeg no esta instalado.")

    command = [
        ffmpeg,
        "-y",
        "-hide_banner",
        "-loglevel",
        "error",
        "-i",
        str(input_path),
        "-map",
        "0:a:0",
        "-ac",
        "1",
        "-ar",
        str(rate),
        "-acodec",
        "pcm_u8",
        str(wav_path),
    ]
    run_command(command)
    return extract_raw_from_wav(wav_path, raw_path, rate)


def decode_24bit_signed(data: bytes, offset: int) -> int:
    """Decodifica una muestra PCM signed little-endian de 24 bits."""
    value = data[offset] | (data[offset + 1] << 8) | (data[offset + 2] << 16)
    if value & 0x800000:
        value -= 0x1000000
    return value


def decode_pcm_frames(data: bytes, channels: int, sample_width: int) -> List[float]:
    """Convierte PCM WAV a muestras flotantes mono en rango -1.0..1.0."""
    if sample_width not in (1, 2, 3, 4):
        raise AudioFormatError("Ancho de muestra WAV no compatible.")

    frame_size = channels * sample_width
    total_frames = len(data) // frame_size
    samples: List[float] = []

    for frame_index in range(total_frames):
        frame_offset = frame_index * frame_size
        mixed = 0.0

        for channel in range(channels):
            offset = frame_offset + channel * sample_width

            if sample_width == 1:
                value = data[offset] - 128
                mixed += value / 128.0
            elif sample_width == 2:
                value = int.from_bytes(data[offset:offset + 2], "little", signed=True)
                mixed += value / 32768.0
            elif sample_width == 3:
                value = decode_24bit_signed(data, offset)
                mixed += value / 8388608.0
            else:
                value = int.from_bytes(data[offset:offset + 4], "little", signed=True)
                mixed += value / 2147483648.0

        mixed /= float(channels)
        if mixed < -1.0:
            mixed = -1.0
        elif mixed > 1.0:
            mixed = 1.0

        samples.append(mixed)

    return samples


def resample_to_u8(samples: List[float], source_rate: int, target_rate: int) -> bytes:
    """Remuestrea por interpolacion lineal y entrega PCM unsigned 8-bit."""
    if not samples:
        raise AudioFormatError("El audio WAV no contiene muestras.")

    if source_rate <= 0 or target_rate <= 0:
        raise AudioFormatError("Tasa de muestreo invalida.")

    output_count = max(1, int(round(len(samples) * float(target_rate) / float(source_rate))))
    output = bytearray()

    for out_index in range(output_count):
        position = out_index * float(source_rate) / float(target_rate)
        index = int(position)
        fraction = position - index

        if index >= len(samples) - 1:
            value = samples[-1]
        else:
            value = samples[index] * (1.0 - fraction) + samples[index + 1] * fraction

        pcm = int(round((value + 1.0) * 127.5))
        if pcm < 0:
            pcm = 0
        elif pcm > 255:
            pcm = 255

        output.append(pcm)

    return bytes(output)


def write_u8_wav(wav_path: Path, raw_data: bytes, rate: int) -> None:
    """Escribe WAV mono unsigned 8-bit para revisar la conversion en el host."""
    with wave.open(str(wav_path), "wb") as output:
        output.setnchannels(1)
        output.setsampwidth(1)
        output.setframerate(rate)
        output.writeframes(raw_data)


def convert_wav_with_stdlib(input_path: Path, wav_path: Path, raw_path: Path, rate: int) -> bytes:
    """Convierte WAV PCM sin depender de librerias externas."""
    with wave.open(str(input_path), "rb") as source:
        if source.getcomptype() != "NONE":
            raise AudioFormatError("Solo WAV PCM sin compresion es compatible sin ffmpeg.")

        channels = source.getnchannels()
        sample_width = source.getsampwidth()
        source_rate = source.getframerate()
        data = source.readframes(source.getnframes())

    samples = decode_pcm_frames(data, channels, sample_width)
    raw_data = resample_to_u8(samples, source_rate, rate)

    raw_path.write_bytes(raw_data)
    write_u8_wav(wav_path, raw_data, rate)
    return raw_data


def get_turboc3_dir(repo_root: Path) -> Optional[Path]:
    """Devuelve TURBOC3/ si existe en este proyecto."""
    turboc3_dir = repo_root / "TURBOC3"
    if not turboc3_dir.exists():
        return None

    return turboc3_dir


def sync_runtime_files(
    raw_path: Path,
    metadata_path: Path,
    repo_root: Path,
    rate: int,
) -> Optional[Path]:
    """Copia RAW, metadata y Hz al entorno que ve DOSBox como C:\\TURBOC3."""
    turboc3_dir = get_turboc3_dir(repo_root)
    if turboc3_dir is None:
        return None

    raw_target = turboc3_dir / "SOUND.RAW"
    metadata_target = turboc3_dir / "SOUND.TXT"
    rate_target = turboc3_dir / "SNDHZ.CFG"

    shutil.copyfile(raw_path, raw_target)
    shutil.copyfile(metadata_path, metadata_target)
    rate_target.write_text("{0}\n".format(rate), encoding="ascii")

    return raw_target


def write_metadata(
    metadata_path: Path,
    source_path: Path,
    raw_path: Path,
    wav_path: Path,
    runtime_path: Optional[Path],
    runtime_note: str,
    rate: int,
    raw_data: bytes,
    used_backend: str,
) -> None:
    """Guarda informacion corta para saber como se genero el audio."""
    duration = len(raw_data) / float(rate)
    lines = [
        "Audio convertido para DOSBox + Sound Blaster",
        "",
        "Fuente: {0}".format(source_path),
        "Backend: {0}".format(used_backend),
        "Formato RAW: unsigned 8-bit, mono, {0} Hz".format(rate),
        "Duracion aproximada: {0:.2f} s".format(duration),
        "RAW: {0}".format(raw_path),
        "WAV de revision: {0}".format(wav_path),
    ]

    if runtime_path is not None:
        lines.append("Copia para DOSBox: {0}".format(runtime_path))
    else:
        lines.append("Copia para DOSBox: {0}".format(runtime_note))

    metadata_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def convert_audio(input_path: Path, output_stem: str, rate: int, copy_runtime: bool) -> None:
    """Coordina la conversion y deja los archivos en Reproducir sonido."""
    started = time.perf_counter()
    repo_root = get_repo_root()
    playback_dir = repo_root / "projects" / "Sonido" / "Reproducir sonido"
    playback_dir.mkdir(parents=True, exist_ok=True)

    stem = normalize_dos_stem(output_stem)
    raw_path = playback_dir / "{0}.RAW".format(stem)
    wav_path = playback_dir / "{0}.WAV".format(stem)
    metadata_path = playback_dir / "{0}.TXT".format(stem)
    log_path = playback_dir / CONVERT_LOG_NAME

    log_lines = [
        "VSBOX conversion log",
        "Inicio: {0}".format(datetime.now().isoformat(timespec="seconds")),
        "Fuente recibida: {0}".format(input_path),
        "Fuente absoluta: {0}".format(input_path.resolve() if input_path.exists() else input_path),
        "Variable rate usada por Python: {0} Hz".format(rate),
        "Nombre DOS base: {0}".format(stem),
    ]

    if not input_path.exists():
        raise AudioFormatError("No existe el archivo: {0}".format(input_path))

    log_lines.append("Tamano fuente: {0} bytes".format(input_path.stat().st_size))

    delete_if_exists(raw_path, log_lines)
    delete_if_exists(wav_path, log_lines)
    delete_if_exists(metadata_path, log_lines)
    delete_if_exists(log_path, log_lines)

    if copy_runtime:
        turboc3_dir = get_turboc3_dir(repo_root)
        if turboc3_dir is not None:
            delete_if_exists(turboc3_dir / "SOUND.RAW", log_lines)
            delete_if_exists(turboc3_dir / "SOUND.TXT", log_lines)

    used_backend = "ffmpeg"
    try:
        raw_data = convert_with_ffmpeg(input_path, wav_path, raw_path, rate)
    except AudioFormatError as ffmpeg_error:
        if input_path.suffix.lower() != ".wav":
            raise AudioFormatError(
                "{0} Para MP3/OGG/FLAC/M4A instala ffmpeg.".format(ffmpeg_error)
            ) from ffmpeg_error

        used_backend = "python wave"
        raw_data = convert_wav_with_stdlib(input_path, wav_path, raw_path, rate)

    wav_frames = verify_wav_file(wav_path, rate)
    raw_size = raw_path.stat().st_size
    wav_size = wav_path.stat().st_size
    expected_duration = raw_size / float(rate)

    if raw_size != len(raw_data):
        raise AudioFormatError("RAW inconsistente: archivo y memoria no coinciden.")
    if wav_frames != raw_size:
        raise AudioFormatError(
            "WAV/RAW no coinciden: frames={0}, raw_bytes={1}.".format(
                wav_frames, raw_size
            )
        )

    log_lines.extend([
        "Backend: {0}".format(used_backend),
        "WAV verificado: mono, unsigned 8-bit, {0} Hz".format(rate),
        "Frames WAV: {0}".format(wav_frames),
        "Bytes RAW: {0}".format(raw_size),
        "Bytes WAV: {0}".format(wav_size),
        "Duracion esperada: {0:.3f} s".format(expected_duration),
    ])

    runtime_path = None
    runtime_note = "desactivada por --no-turboc3-copy"
    if copy_runtime:
        turboc3_dir = get_turboc3_dir(repo_root)
        if turboc3_dir is not None:
            runtime_path = turboc3_dir / "SOUND.RAW"
        else:
            runtime_note = "no creada porque TURBOC3/ no existe"

    write_metadata(
        metadata_path=metadata_path,
        source_path=input_path,
        raw_path=raw_path,
        wav_path=wav_path,
        runtime_path=runtime_path,
        runtime_note=runtime_note,
        rate=rate,
        raw_data=raw_data,
        used_backend=used_backend,
    )

    if copy_runtime:
        runtime_path = sync_runtime_files(
            raw_path=raw_path,
            metadata_path=metadata_path,
            repo_root=repo_root,
            rate=rate,
        )
        if runtime_path is not None:
            log_lines.append("Runtime RAW: {0}".format(runtime_path))
            log_lines.append("Runtime RAW bytes: {0}".format(runtime_path.stat().st_size))
            log_lines.append("Runtime Hz cfg: {0}".format(repo_root / RATE_CONFIG_RELATIVE))
            log_lines.append("Runtime metadata: {0}".format(repo_root / "TURBOC3" / "SOUND.TXT"))

    elapsed = time.perf_counter() - started
    log_lines.append("Tiempo total conversion: {0:.3f} s".format(elapsed))
    log_lines.append("Fin: {0}".format(datetime.now().isoformat(timespec="seconds")))
    write_conversion_log(log_path, log_lines)

    if copy_runtime and get_turboc3_dir(repo_root) is not None:
        shutil.copyfile(log_path, repo_root / "TURBOC3" / CONVERT_LOG_NAME)

    print("Conversion completada.")
    print("Variable rate usada por Python:", rate, "Hz")
    print("Fuente usada:", input_path)
    print("Bytes RAW:", raw_size)
    print("Duracion esperada:", "{0:.3f} s".format(expected_duration))
    print("Tiempo conversion:", "{0:.3f} s".format(elapsed))
    print("RAW para PLAYSB.C:", raw_path)
    print("WAV de revision:", wav_path)
    print("Datos:", metadata_path)
    print("Log conversion:", log_path)

    if runtime_path is not None:
        print("Copia lista para DOSBox:", runtime_path)
        print("Hz sincronizados en:", repo_root / RATE_CONFIG_RELATIVE)
        print("Metadata para PLAYSB.C:", repo_root / "TURBOC3" / "SOUND.TXT")
    elif copy_runtime:
        print("Aviso: no se copio a TURBOC3/SOUND.RAW porque TURBOC3/ no existe.")
    else:
        print("Copia a TURBOC3/SOUND.RAW desactivada por opcion.")


def parse_args(argv: Iterable[str]) -> argparse.Namespace:
    """Define interfaz por consola sin impedir el selector grafico."""
    parser = argparse.ArgumentParser(
        description="Convierte audio a RAW unsigned 8-bit mono para DOSBox."
    )
    parser.add_argument(
        "audio",
        nargs="?",
        help="Ruta del audio. Si se omite, se abre selector grafico o prompt.",
    )
    parser.add_argument(
        "--rate",
        type=int,
        default=None,
        help=(
            "Tasa de salida: 8000, 11025, 16000 o 22050. "
            "Si se omite, usa TURBOC3/SNDHZ.CFG o 16000."
        ),
    )
    parser.add_argument(
        "--name",
        default=DEFAULT_NAME,
        help="Nombre base DOS 8.3 para la salida. Por defecto: SOUND.",
    )
    parser.add_argument(
        "--no-turboc3-copy",
        action="store_true",
        help="No copiar el RAW a TURBOC3/SOUND.RAW.",
    )
    return parser.parse_args(list(argv))


def main(argv: Iterable[str]) -> int:
    """Punto de entrada del conversor."""
    args = parse_args(argv)

    try:
        repo_root = get_repo_root()
        selected_rate = read_selected_rate_config(repo_root)
        output_rate = args.rate if args.rate is not None else selected_rate or DEFAULT_RATE

        if not is_supported_rate(output_rate):
            raise AudioFormatError(
                "Frecuencia no compatible. Usa una de estas: {0}.".format(
                    ", ".join(str(rate) for rate in SUPPORTED_RATES)
                )
            )

        input_path = Path(args.audio).expanduser() if args.audio else select_audio_file()
        convert_audio(
            input_path=input_path,
            output_stem=args.name,
            rate=output_rate,
            copy_runtime=not args.no_turboc3_copy,
        )
        return 0
    except (AudioFormatError, OSError, wave.Error) as exc:
        print("Error:", exc, file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
