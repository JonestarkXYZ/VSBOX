Proyecto Musica8
================

Objetivo
--------

Reproducir musica 8-bit escrita a mano dentro de DOSBox usando Turbo C 3 y
Sound Blaster Direct DAC.

Archivos
--------

- MUSIC8.C
  Reproductor, parser, tabla de frecuencias e instrumentos.

- MUSIC.TXT
  Cancion editable en texto plano. Ahora contiene una pista Frecuencia con
  notas musicales directas.

- DEMO.TXT
  Ejemplo anterior con Piano, Bajo y Bateria.

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

La pista Frecuencia usa el tono directo de la nota o del numero escrito. Para
evitar el sonido roto de la onda cuadrada en notas altas, el programa la
reproduce con onda cuadrada suavizada y una entrada/salida corta contra clicks.

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

La duracion divide un pulso del tempo:

1  = pulso completo
2  = medio pulso
3  = un tercio de pulso
4  = un cuarto de pulso
8  = un octavo de pulso
16 = un dieciseisavo de pulso

El tempo se cambia en MUSIC8.C:

#define TEMPO_BPM 120

La salida de audio se cambia en MUSIC8.C:

#define SAMPLE_RATE 11025UL

Con Sound Blaster Direct DAC no siempre conviene subir este valor. Si DOSBox no
alcanza a enviar las muestras a tiempo, las notas suenan mas graves y la musica
se vuelve lenta.

La velocidad se puede ajustar sin cambiar la afinacion:

#define TEMPO_SPEED_PERCENT 100UL

Usa 125 para 25% mas rapido o 80 para 20% mas lento.

La afinacion se puede corregir sin tocar MUSIC.TXT:

#define PITCH_CORRECT_PCT 100UL

Si todo suena grave, prueba 125, 150 o 200. Si todo suena agudo, baja el valor.

El bucle se cambia en MUSIC8.C:

#define LOOP_SONG 0

Usa 1 para repetir en bucle y 0 para reproducir una sola vez.

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

Para validar el archivo sin reproducir audio, ejecuta el EXE con CHECK dentro
de DOSBox:

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

El audio se mezcla en tiempo real a 11025 Hz, unsigned 8-bit mono. Direct DAC
es simple y facil de depurar, pero consume CPU porque se escribe muestra por
muestra.

Si necesitas probar mas calidad, sube SAMPLE_RATE a 16000UL. Si al hacerlo las
notas bajan de tono, vuelve a 11025UL o sube PITCH_CORRECT_PCT.
