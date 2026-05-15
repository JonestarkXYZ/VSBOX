Snake 2D clasico para Turbo C/C++ y DOSBox.

Archivo principal:
  SNAKE.CPP

Controles:
  Menu inicial:
    Flechas o WASD  - cambiar modo
    1 o C           - iniciar modo clasico
    2 o I           - iniciar modo infinito
    Enter/Espacio   - iniciar el modo seleccionado

  Durante la partida:
    Flechas o WASD  - mover la serpiente
    P o Espacio     - pausar / continuar
    R               - reiniciar partida
    ESC             - salir

Modos:
  Clasico:
    La serpiente muere al chocar contra una pared o contra su cuerpo.

  Infinito:
    Al salir por una pared aparece por el lado contrario.
    La serpiente sigue muriendo si choca contra su propio cuerpo.

Como ejecutar:
  1. Abrir projects/Snake/SNAKE.CPP en VSCode.
  2. Presionar Ctrl + Shift + B.

Notas:
  - Usa graphics.h, conio.h y dos.h, igual que otros ejemplos del repositorio.
  - Toda la logica del juego esta en SNAKE.CPP: tablero, comida, crecimiento,
    colisiones, modos de pared, puntuacion, pausa, reinicio y deteccion de victoria.
  - La velocidad inicia en nivel 10 de 20 y sube 1 nivel por cada fruta comida.
  - La fruta se dibuja como pixel art programado dentro de una celda de la grilla.
