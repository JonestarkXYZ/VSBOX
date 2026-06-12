Proyecto Sonido para VSBOX
==========================

Objetivo
--------

Probar sonido dentro de DOSBox y convertir audio moderno a un formato facil
de reproducir desde Turbo C 3.

Estructura
----------

- Convertir audio a DOSBox/
  Script Python para seleccionar o indicar un audio y convertirlo.

- Reproducir sonido/
  Programa C para DOSBox que reproduce SOUND.RAW usando Sound Blaster.

- Musica8/
  Reproductor C para musica 8-bit escrita a mano con MUSIC.TXT.

Formato usado
-------------

El formato de reproduccion es RAW PCM unsigned 8-bit, mono. No tiene
cabecera. Es simple de leer desde C y compatible con la salida Direct DAC
de Sound Blaster.

La frecuencia puede ser 8000, 11025, 16000 o 22050 Hz. El valor por defecto
es 16000 Hz.

El selector SELHZ.C guarda la frecuencia en:

- TURBOC3/SNDHZ.CFG

PLAYSB.C lee ese archivo al iniciar. El conversor Python tambien lo lee si
existe, para generar SOUND.RAW con la misma frecuencia.

Ademas, el conversor copia SOUND.TXT a TURBOC3/ y sincroniza SNDHZ.CFG con
la frecuencia usada al convertir.

Uso del conversor
-----------------

Desde Linux:

python3 "projects/Sonido/Convertir audio a DOSBox/convertir_audio_dosbox.py"

Tambien se puede pasar la ruta directamente:

python3 "projects/Sonido/Convertir audio a DOSBox/convertir_audio_dosbox.py" audio.wav

Si quieres forzar una frecuencia sin usar SELHZ.C:

python3 "projects/Sonido/Convertir audio a DOSBox/convertir_audio_dosbox.py" audio.wav --rate 22050

Si ffmpeg esta instalado, acepta formatos como MP3, OGG, FLAC, M4A y WAV.
Sin ffmpeg, el respaldo solo convierte WAV PCM sin compresion.

El conversor genera:

- projects/Sonido/Reproducir sonido/SOUND.RAW
- projects/Sonido/Reproducir sonido/SOUND.WAV
- projects/Sonido/Reproducir sonido/SOUND.TXT

Si existe TURBOC3/, tambien copia:

- TURBOC3/SOUND.RAW
- TURBOC3/SOUND.TXT
- TURBOC3/SNDHZ.CFG

Esa copia es la que abre PLAYSB.C cuando se ejecuta con las tareas normales
del repositorio, porque RUN.BAT ejecuta los programas desde C:\TURBOC3.

Para diagnostico, SELHZ.C escribe TURBOC3/SELHZ.TXT y PLAYSB.C escribe
TURBOC3/PLAYHZ.TXT con la frecuencia que uso cada programa.

Uso del reproductor
-------------------

Primero puedes seleccionar Hz:

./tools/build-linux.sh "projects/Sonido/Reproducir sonido/SELHZ.C"

Luego convierte el audio para que SOUND.RAW tenga esos mismos Hz.

Compilar y ejecutar:

./tools/build-linux.sh "projects/Sonido/Reproducir sonido/PLAYSB.C"

Si SOUND.RAW no existe, PLAYSB.C reproduce un tono generado en tiempo real.
Eso permite probar si Sound Blaster responde en DOSBox antes de convertir
un audio real.

Notas tecnicas
--------------

DOSBox suele usar Sound Blaster 16 por defecto con puerto 220, IRQ 7 y DMA 1.
PLAYSB.C no usa DMA; escribe muestras 8-bit en modo Direct DAC para mantener
el ejemplo corto y facil de depurar.
