Proyectos 3D
============

Todos los ejemplos graficos principales de esta carpeta usan doble pagina BGI
cuando redibujan frames completos:

  setactivepage(pagina_oculta)
  dibujar frame completo
  setvisualpage(pagina_oculta)

Esto evita que se vea el borrado o pintado parcial entre frames. Para tener dos
paginas reales en BGI se usa VGA/VGAMED, con resolucion 640x350 y 16 colores.

Archivos graficos cubiertos:
  3D_TESTS/CUBE3D.CPP
  Cubo/LINEAS.CPP
  Cubo/COLOR.CPP
  Cubo/MOUSE.CPP
  IA_EXAMPLES/NAVE.CPP
  OBJ_TEST/OBJVIEW.C
  OBJ_TEST/OBJCOLOR.C
  Show3D/Models3D.C

Los archivos de tests de consola no usan multipagina porque no dibujan frames.
