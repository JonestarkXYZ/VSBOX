Proyecto Musica8
================

Objetivo
--------

Reproducir musica 8-bit escrita a mano dentro de DOSBox usando Turbo C 3 y
Sound Blaster Direct DAC. El programa lee MUSIC.TXT, mezcla las pistas en
tiempo real y muestra una vista grafica tipo DAW para la pista Frecuencia.

Archivos
--------

- MUSIC8.C
  Reproductor, parser, tabla de frecuencias, instrumentos y vista grafica.

- MUSIC.TXT
  Cancion editable en texto plano.

- DEMO.TXT
  Ejemplo con Piano, Bajo y Bateria.

Formato de MUSIC.TXT
--------------------

Cada pista usa:

Instrumento[nota,duracion,nota,duracion]

Ejemplo:

Piano[C4,4,D4,4,E4,4,MUTE,4]
Bajo[C3,2,G2,2]
Bateria[KICK,4,HAT,8,SNARE,4,HAT,8]
Frecuencia[C4,4,MUTE,4,E4,4,G4,4]

Todas las pistas inician al mismo tiempo. Si una pista termina antes que las
demas, queda en silencio.

Notas
-----

Notas soportadas:

C
C#
D
D#
E
F
F#
G
G#
A
A#
B

Se recomienda indicar octava:

C4, C#4, D4, A5, DS6

Tambien se aceptan alias sin simbolo #:

CS = C#
DS = D#
FS = F#
GS = G#
AS = A#

MUTE es silencio.

Tambien puedes escribir frecuencias directas en Hz:

Frecuencia[440,4,0,4,660,4,880,4]

En una pista Frecuencia puedes mezclar notas y Hz:

Frecuencia[D7,4,MUTE,8,3136,8,A7,4]

Loop desde MUSIC.TXT
-------------------

Dentro de una pista puedes escribir:

LOOP,

Ese token no lleva duracion y no genera sonido. Marca desde donde vuelve a
reproducirse la pista cuando la cancion llega al final.

Ejemplo:

Frecuencia[
    C6,8,D6,8,E6,8,
    LOOP,
    G6,8,A6,8,B6,8
]

En ese ejemplo, la primera vuelta reproduce C6, D6, E6, G6, A6, B6. Cuando
termina, vuelve a G6. Si una pista no tiene LOOP propio pero otra pista si lo
activa, esa pista vuelve desde el inicio.

Duracion y tempo
----------------

La duracion usa denominadores de notacion musical. El BPM mide negras por
minuto, por eso `4` equivale a un beat del tempo:

1  = redonda, cuatro beats
2  = blanca, dos beats
4  = negra, un beat
8  = corchea, medio beat
16 = semicorchea, un cuarto de beat

Ejemplo a 60 BPM:

4  = 1000 ms
8  = 500 ms
16 = 250 ms

El tempo se cambia en MUSIC8.C:

#define TEMPO_BPM 60

La constante que define que la negra es el beat es:

#define BEAT_NOTE_DIVISOR 4UL

La salida de audio se cambia en MUSIC8.C:

#define SAMPLE_RATE 11025UL

Con Sound Blaster Direct DAC no siempre conviene subir este valor. Si DOSBox no
alcanza a enviar las muestras a tiempo, las notas pueden sonar graves o la
musica puede sentirse lenta.

La velocidad se puede ajustar sin cambiar la afinacion:

#define TEMPO_SPEED_PERCENT 100UL

Usa 125 para 25% mas rapido o 80 para 20% mas lento.

La afinacion se puede corregir sin tocar MUSIC.TXT:

#define PITCH_CORRECT_PCT 100UL

Si todo suena grave, prueba 125, 150 o 200. Si todo suena agudo, baja el valor.

El bucle global se cambia en MUSIC8.C:

#define LOOP_SONG 0

Usa 1 para repetir en bucle y 0 para reproducir una sola vez. Si MUSIC.TXT
tiene un token LOOP, el bucle queda activo aunque LOOP_SONG sea 0.

Vista grafica tipo DAW
----------------------

Al iniciar, MUSIC8.C muestra un piano-roll grafico con BGI para la primera
pista Frecuencia encontrada:

- Cada fila representa un semitono.
- El eje horizontal representa el tiempo.
- Cada rectangulo representa una nota sostenida.
- Los silencios dejan espacio vacio.
- La linea amarilla marca LOOP si existe.

La escala horizontal se comprime para entrar en pantalla. Si la cancion es
larga, las notas se ven como bloques o pixeles pequenos, pero no como ASCII.

Instrumentos
------------

- Piano
  Onda cuadrada con decaimiento corto.

- Bajo
  Onda de pulso mas fuerte y sostenida.

- Bateria
  KICK, SNARE, HAT y tambien notas normales como toms afinados.

- Lead o Synth
  Onda cuadrada sostenida para melodias principales.

- Frecuencia
  Onda cuadrada suavizada usando notas musicales o Hz escritos en el archivo.

Compilar y ejecutar
-------------------

Desde Linux:

./tools/build-linux.sh "projects/Sonido/Musica8/MUSIC8.C"

Para validar el archivo sin abrir la vista grafica ni reproducir audio:

MUSIC8.EXE CHECK

Para reproducir el ejemplo anterior:

MUSIC8.EXE DEMO.TXT

Para validar otro archivo:

MUSIC8.EXE DEMO.TXT CHECK

El programa busca MUSIC.TXT en estas rutas:

- MUSIC.TXT
- C:\TURBOC3\MUSIC.TXT
- C:\PROJECTS\SONIDO\MUSICA8\MUSIC.TXT

Con la tarea normal de VSCode debe encontrar el archivo del proyecto por la
tercera ruta.

Notas tecnicas
--------------

La reproduccion actual usa mezcla en tiempo real. No depende de archivos de
audio pre-renderizados ni de cache temporal.

Si una cancion con muchas pistas se siente lenta, baja SAMPLE_RATE o simplifica
los instrumentos de percusion. El modo Direct DAC obliga a enviar cada muestra
al DSP en tiempo real, por eso el costo de CPU afecta el tempo percibido.
