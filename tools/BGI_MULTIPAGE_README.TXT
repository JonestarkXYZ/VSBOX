Uso de doble pagina BGI para animaciones
========================================

Objetivo
--------

Cuando un programa redibuja frames directamente sobre la pantalla visible, se
puede ver el proceso de pintado: primero el borrado, luego lineas, caras o
sprites apareciendo por partes. Esto pasa tanto en 2D como en 3D.

La solucion practica en Turbo C/BGI es usar dos paginas graficas:

  1. Una pagina visible, que el usuario esta viendo.
  2. Una pagina oculta, donde se dibuja el siguiente frame completo.
  3. Al terminar el frame, se cambia la pagina visible con setvisualpage().

Esta tecnica tambien se conoce como page flipping o doble buffer por paginas.

Modo grafico recomendado
------------------------

Para BGI en Turbo C, usa:

  gd = VGA;
  gm = VGAMED;
  initgraph(&gd, &gm, "BGI");

VGAMED trabaja a 640x350 con 16 colores y permite dos paginas. La ventaja es que
el cambio de frame se ve completo. La desventaja es que no usa los 480 pixeles
verticales del modo VGAHI.

Plantilla minima
----------------

Incluye estas variables globales:

  int visualPage = 0;
  int activePage = 1;

Inicializa las paginas despues de initgraph():

  void initDoubleBufferPages() {
      // Limpia ambas paginas y deja lista la pagina oculta para dibujar.
      setwritemode(COPY_PUT);
      setactivepage(0);
      cleardevice();
      setactivepage(1);
      cleardevice();
      setvisualpage(0);
      setactivepage(1);
      visualPage = 0;
      activePage = 1;
  }

  void flipDoubleBufferPage() {
      // Muestra el frame completo que ya fue dibujado en la pagina oculta.
      setvisualpage(activePage);
      visualPage = activePage;
      activePage = 1 - activePage;
  }

Uso dentro del loop principal:

  setactivepage(activePage);
  setwritemode(COPY_PUT);
  cleardevice();

  // Dibujar aqui el frame completo:
  // - fondo
  // - sprites
  // - lineas 3D
  // - triangulos
  // - HUD o texto

  flipDoubleBufferPage();

Al salir del programa, deja BGI en una pagina conocida:

  setactivepage(0);
  setvisualpage(0);
  closegraph();

Ejemplo completo de estructura
------------------------------

  int main() {
      int gd = VGA;
      int gm = VGAMED;
      int salir = 0;

      initgraph(&gd, &gm, "BGI");
      initDoubleBufferPages();

      while (!salir) {
          // Leer teclado, mouse o actualizar logica.

          setactivepage(activePage);
          setwritemode(COPY_PUT);
          cleardevice();

          // Dibujar el frame completo aqui.

          flipDoubleBufferPage();
          delay(16);
      }

      setactivepage(0);
      setvisualpage(0);
      closegraph();
      return 0;
  }

Reglas importantes
------------------

- No dibujes partes del frame en la pagina visible.
- No llames setvisualpage() hasta que el frame este completo.
- Dibuja tambien el HUD/texto en la pagina oculta antes del flip.
- Usa setwritemode(COPY_PUT) antes de dibujar frames normales.
- Evita mezclar esta tecnica con borrado incremental en la pagina visible.
- Si cambias de escena o modelo, limpia ambas paginas para evitar residuos.

Coordenadas
-----------

VGAMED usa 640x350. Si antes usabas centro Y = 240 para VGAHI 640x480, cambia
el centro vertical aproximadamente a:

  centerY = 175;

O mejor, usa:

  centerX = getmaxx() / 2;
  centerY = getmaxy() / 2;

Limitaciones
------------

- No es z-buffer. Solo evita que se vea el pintado parcial.
- Para 3D con caras rellenas, el orden de dibujo sigue siendo importante.
- En BGI, no todos los modos tienen dos paginas. VGAMED es una opcion estable.
- En DOSBox puede influir la configuracion de ciclos, pero eso no reemplaza el
  doble buffer si el programa pinta directamente en la pantalla visible.

Ejemplos en este proyecto
-------------------------

Estos archivos usan esta tecnica:

  projects/3D/Cubo/LINEAS.CPP
  projects/3D/Cubo/COLOR.CPP
  projects/3D/Cubo/MOUSE.CPP
  projects/3D/Show3D/Models3D.C
  projects/3D/IA_EXAMPLES/NAVE.CPP
  projects/3D/OBJ_TEST/OBJVIEW.C
  projects/3D/OBJ_TEST/OBJCOLOR.C

Para comparar metodos de refresco:

  projects/3D/3D_TESTS/CUBE3D.CPP

Ese ejemplo incluye modos con cleardevice, borrado incremental, XOR y doble
pagina.
