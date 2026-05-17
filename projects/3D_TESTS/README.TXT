Prueba 3D de cubo sin parpadeo
================================

Archivo principal:
  CUBE3D.CPP

Objetivo:
  Comparar varios metodos de refresco de lineas 3D en Turbo C++/BGI.

Modos incluidos:
  1. cleardevice completo
     Borra toda la pantalla y redibuja el cuadro completo.

  2. borrar solo lineas previas
     Borra el cubo anterior en negro, restaura el fondo de referencia y dibuja el cubo nuevo.

  3. XOR mover lineas
     Dibuja el cubo anterior otra vez en modo XOR para quitarlo, y luego dibuja el nuevo.

  4. doble pagina congelada
     Dibuja el siguiente cuadro en una pagina oculta y cambia la pagina visible cuando ya esta terminado.
     El programa usa VGA/VGAMED porque BGI documenta 2 paginas en ese modo.

Controles:
  1, 2, 3, 4  Cambiar modo.
  +, flecha derecha o flecha arriba  Menos delay, mas velocidad.
  -, flecha izquierda o flecha abajo Mas delay, menos velocidad.
  ESPACIO     Pausa.
  ESC         Salir.

Ejecucion desde Linux:
  ./tools/build-linux.sh projects/3D_TESTS/CUBE3D.CPP

Nota tecnica:
  En BGI no existe un objeto "linea" que se mueva como entidad persistente.
  Lo mas parecido es guardar las coordenadas del frame anterior y actualizar la pantalla
  con borrado incremental, XOR o doble buffer.
