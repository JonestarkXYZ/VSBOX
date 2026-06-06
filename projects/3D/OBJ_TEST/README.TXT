Prueba de lectura OBJ
=====================

Archivos:
  OBJREAD.C
  OBJVIEW.C
  OBJCOLOR.C
  OBJGRAY.C
  modelo.obj
  model2.obj

Objetivo:
  Abrir C:\projects\3D\OBJ_TEST\modelo.obj y validar sintaxis OBJ basica.

Soportado por ahora:
  v  x y z
  f  indices de vertices
  l  indices de vertices
  #  comentarios

Tambien reconoce o, g, s, usemtl, mtllib, vn y vt. OBJGRAY.C usa vn cuando
esta disponible para saber hacia donde mira cada cara.

Ejecucion desde Linux:
  ./tools/build-linux.sh projects/3D/OBJ_TEST/OBJREAD.C

Visor 3D por lineas:
  ./tools/build-linux.sh projects/3D/OBJ_TEST/OBJVIEW.C

Visor 3D con triangulos coloreados:
  ./tools/build-linux.sh projects/3D/OBJ_TEST/OBJCOLOR.C

Visor 3D en escala de grises con luz:
  ./tools/build-linux.sh projects/3D/OBJ_TEST/OBJGRAY.C

Nota:
  Esta prueba no renderiza 3D todavia. Solo comprueba que el archivo se abre y
  que las listas principales del OBJ se pueden leer sin romper la sintaxis.

Nota del visor:
  OBJVIEW.C carga model2.obj, convierte las caras f en triangulos y dibuja sus
  aristas. Si el OBJ trae lineas l tambien las dibuja, pero Blender normalmente
  exporta mallas con caras f. Usa doble pagina BGI para ocultar el redibujado de
  cada frame.

Nota del visor coloreado:
  OBJCOLOR.C usa la misma camara, descarta triangulos que miran hacia atras y
  pinta de mas lejos a mas cercano para que los triangulos cercanos cubran a los
  lejanos. Tambien usa doble pagina BGI con VGA/VGAMED: primero dibuja el frame
  completo en una pagina oculta y luego cambia la pagina visible, evitando que se
  vea el pintado triangulo por triangulo.

Nota del visor en grises:
  OBJGRAY.C usa solo DARKGRAY, LIGHTGRAY y WHITE para evitar tonos de color.
  Cada triangulo usa la normal vn del OBJ si existe; si no existe, calcula una
  normal desde el orden de vertices. Esa normal se usa para descartar caras que
  miran hacia atras y para iluminar segun una luz fija arriba/izquierda/frente
  de la camara. La tecla L activa o desactiva los bordes negros de triangulacion.
  La tecla B activa o desactiva el descarte de caras traseras para comparar. El
  HUD muestra cuantas caras se descartan por estar atras, fuera de pantalla o
  cerca de la camara. El orden de pintado usa el vertice mas cercano de cada
  triangulo para reducir pisadas en esquinas. Tambien descarta triangulos cuya
  caja completa queda fuera de la pantalla. Esto evita dibujar caras fuera de la
  vista, pero no reemplaza un clipping geometrico completo ni un z-buffer real.
