#include <graphics.h>
#include <conio.h>
#include <dos.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// Proyecto de pruebas: cubo 3D con varios metodos de refresco para comparar parpadeo.
#define BGI_DIR "C:\\TURBOC3\\BGI"

#define CUBE_VERTICES 8
#define CUBE_EDGES 12

#define MODE_CLEAR 1
#define MODE_INCREMENTAL 2
#define MODE_XOR 3
#define MODE_PAGE_FLIP 4

#define KEY_ESC 27
#define KEY_SPACE 32
#define KEY_EXTENDED_1 0
#define KEY_EXTENDED_2 224
#define KEY_LEFT 75
#define KEY_RIGHT 77
#define KEY_UP 72
#define KEY_DOWN 80

#define CAMERA_DISTANCE 420.0
#define FOCAL_LENGTH 260.0
#define CUBE_SIZE 115.0

struct Vec3 {
    float x;
    float y;
    float z;
};

struct Point2 {
    int x;
    int y;
};

struct CubeFrame {
    struct Point2 p[CUBE_VERTICES];
    float z[CUBE_VERTICES];
};

struct AxisFrame {
    struct Point2 p[4];
};

// Vertices fijos del cubo. Las rotaciones se calculan sobre una copia por cuadro.
struct Vec3 cube[CUBE_VERTICES] = {
    {-CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE},
    { CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE},
    { CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE},
    {-CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE},
    {-CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE},
    { CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE},
    { CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE},
    {-CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE}
};

// Cada arista guarda los dos indices de vertices que conecta.
int edges[CUBE_EDGES][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0},
    {4, 5}, {5, 6}, {6, 7}, {7, 4},
    {0, 4}, {1, 5}, {2, 6}, {3, 7}
};

int screenW = 639;
int screenH = 349;
int centerX = 319;
int centerY = 190;
int currentMode = MODE_INCREMENTAL;
int oldFrameValid = 0;
int visualPage = 0;
int activePage = 1;
int frameDelay = 22;
int oldAxisValid = 0;

struct CubeFrame currentFrame;
struct CubeFrame oldFrame;
struct AxisFrame currentAxis;
struct AxisFrame oldAxis;

char *modeName(int mode) {
    // Nombre corto para que quepa en la barra superior de 640 px.
    if (mode == MODE_CLEAR) return "cleardevice completo";
    if (mode == MODE_INCREMENTAL) return "borrar solo lineas previas";
    if (mode == MODE_XOR) return "XOR mover lineas";
    if (mode == MODE_PAGE_FLIP) return "doble pagina congelada";
    return "desconocido";
}

void rotatePair(float *a, float *b, float angle) {
    float oldA;
    float oldB;
    float c;
    float s;

    // Rota dos componentes y permite construir rotaciones X/Y/Z sin matrices.
    oldA = *a;
    oldB = *b;
    c = cos(angle);
    s = sin(angle);

    *a = (oldA * c) - (oldB * s);
    *b = (oldA * s) + (oldB * c);
}

void rotatePoint(struct Vec3 *p, float ax, float ay, float az) {
    // Rotacion acumulada en tres ejes para que se perciba volumen.
    rotatePair(&p->y, &p->z, ax);
    rotatePair(&p->z, &p->x, ay);
    rotatePair(&p->x, &p->y, az);
}

void projectPoint(struct Vec3 p, struct Point2 *out) {
    float denom;
    float scale;

    // Proyeccion perspectiva simple: mas Z reduce el tamano visible.
    denom = p.z + CAMERA_DISTANCE;
    if (denom < 45.0) {
        denom = 45.0;
    }

    scale = FOCAL_LENGTH / denom;
    out->x = centerX + (int)(p.x * scale);
    out->y = centerY - (int)(p.y * scale);
}

void buildCubeFrame(float angle, struct CubeFrame *frame) {
    int i;
    struct Vec3 p;

    // Solo cambia la proyeccion calculada; los vertices base no se modifican.
    for (i = 0; i < CUBE_VERTICES; i++) {
        p = cube[i];
        rotatePoint(&p, angle * 0.73, angle * 1.05, angle * 0.41);
        projectPoint(p, &frame->p[i]);
        frame->z[i] = p.z;
    }
}

int edgeColor(struct CubeFrame *frame, int edgeIndex) {
    int a;
    int b;
    float avgZ;

    // Color por profundidad: lejos oscuro, medio cian, cerca blanco.
    a = edges[edgeIndex][0];
    b = edges[edgeIndex][1];
    avgZ = (frame->z[a] + frame->z[b]) / 2.0;

    if (avgZ > 65.0) {
        return DARKGRAY;
    }

    if (avgZ < -65.0) {
        return WHITE;
    }

    return LIGHTCYAN;
}

void drawCubeEdges(struct CubeFrame *frame, int erase) {
    int pass;
    int i;
    int a;
    int b;
    int color;
    float avgZ;

    // Se dibuja de atras hacia adelante para mejorar la lectura 3D.
    for (pass = 0; pass < 3; pass++) {
        for (i = 0; i < CUBE_EDGES; i++) {
            a = edges[i][0];
            b = edges[i][1];
            avgZ = (frame->z[a] + frame->z[b]) / 2.0;

            if ((pass == 0 && avgZ > 65.0) ||
                (pass == 1 && avgZ <= 65.0 && avgZ >= -65.0) ||
                (pass == 2 && avgZ < -65.0)) {
                color = erase ? BLACK : edgeColor(frame, i);
                setcolor(color);
                line(frame->p[a].x, frame->p[a].y, frame->p[b].x, frame->p[b].y);
            }
        }
    }
}

void drawCubeVertices(struct CubeFrame *frame, int erase) {
    int i;

    // Puntos pequenos en los vertices para notar mejor si quedan residuos.
    setcolor(erase ? BLACK : YELLOW);
    for (i = 0; i < CUBE_VERTICES; i++) {
        circle(frame->p[i].x, frame->p[i].y, 2);
    }
}

void drawCube(struct CubeFrame *frame, int erase) {
    // El orden evita que los puntos borren segmentos recien dibujados.
    drawCubeEdges(frame, erase);
    drawCubeVertices(frame, erase);
}

void copyFrame(struct CubeFrame *dst, struct CubeFrame *src) {
    int i;

    // Copia manual para mantener compatibilidad simple con Turbo C++.
    for (i = 0; i < CUBE_VERTICES; i++) {
        dst->p[i] = src->p[i];
        dst->z[i] = src->z[i];
    }
}

void copyAxis(struct AxisFrame *dst, struct AxisFrame *src) {
    int i;

    // Copia los cuatro puntos: origen, X, Y y Z.
    for (i = 0; i < 4; i++) {
        dst->p[i] = src->p[i];
    }
}

void drawHud(int paused) {
    char line1[90];
    char line2[90];

    // La franja superior se limpia para que el texto no deje rastros.
    setfillstyle(SOLID_FILL, BLACK);
    bar(0, 0, screenW, 33);

    sprintf(line1, "CUBO 3D - Modo %d: %s", currentMode, modeName(currentMode));
    sprintf(line2, "1-4 cambia  +/- velocidad  ESP pausa  ESC salir  delay:%dms", frameDelay);

    setcolor(LIGHTGRAY);
    outtextxy(8, 7, line1);
    setcolor(paused ? YELLOW : DARKGRAY);
    outtextxy(8, 20, paused ? "PAUSA" : line2);
    setcolor(DARKGRAY);
    line(0, 34, screenW, 34);
}

void drawGroundGrid(void) {
    int i;
    int y;
    int x1;
    int x2;

    // Rejilla de referencia 2D: ayuda a ver que metodos borran o conservan fondo.
    setcolor(DARKGRAY);
    line(0, centerY + 72, screenW, centerY + 72);

    for (i = -5; i <= 5; i++) {
        x1 = centerX + (i * 19);
        x2 = centerX + (i * 62);
        line(x1, centerY + 20, x2, screenH - 18);
    }

    for (i = 0; i < 7; i++) {
        y = centerY + 35 + (i * 18);
        line(55, y, screenW - 55, y);
    }
}

void computeAxis(float angle, struct AxisFrame *axis) {
    struct Vec3 origin;
    struct Vec3 axisX;
    struct Vec3 axisY;
    struct Vec3 axisZ;

    // Calcula ejes 3D del mismo sistema del cubo para leer la orientacion.
    origin.x = 0.0;
    origin.y = 0.0;
    origin.z = 0.0;
    axisX.x = 150.0;
    axisX.y = 0.0;
    axisX.z = 0.0;
    axisY.x = 0.0;
    axisY.y = 150.0;
    axisY.z = 0.0;
    axisZ.x = 0.0;
    axisZ.y = 0.0;
    axisZ.z = 150.0;

    rotatePoint(&axisX, angle * 0.73, angle * 1.05, angle * 0.41);
    rotatePoint(&axisY, angle * 0.73, angle * 1.05, angle * 0.41);
    rotatePoint(&axisZ, angle * 0.73, angle * 1.05, angle * 0.41);

    projectPoint(origin, &axis->p[0]);
    projectPoint(axisX, &axis->p[1]);
    projectPoint(axisY, &axis->p[2]);
    projectPoint(axisZ, &axis->p[3]);
}

void drawAxisFrame(struct AxisFrame *axis, int erase) {
    int colorX;
    int colorY;
    int colorZ;

    // Al borrar tambien se borran las letras de cada eje.
    colorX = erase ? BLACK : RED;
    colorY = erase ? BLACK : GREEN;
    colorZ = erase ? BLACK : CYAN;

    setcolor(colorX);
    line(axis->p[0].x, axis->p[0].y, axis->p[1].x, axis->p[1].y);
    outtextxy(axis->p[1].x + 4, axis->p[1].y, "X");

    setcolor(colorY);
    line(axis->p[0].x, axis->p[0].y, axis->p[2].x, axis->p[2].y);
    outtextxy(axis->p[2].x + 4, axis->p[2].y, "Y");

    setcolor(colorZ);
    line(axis->p[0].x, axis->p[0].y, axis->p[3].x, axis->p[3].y);
    outtextxy(axis->p[3].x + 4, axis->p[3].y, "Z");
}

void drawAxis(float angle) {
    // Version directa para los modos que redibujan el cuadro completo.
    computeAxis(angle, &currentAxis);
    drawAxisFrame(&currentAxis, 0);
}

void drawStaticScene(float angle, int paused) {
    // Escena comun para todos los metodos de refresco.
    drawHud(paused);
    drawGroundGrid();
    drawAxis(angle);
}

void resetSinglePage(int paused) {
    // Deja BGI en una sola pagina visible para los modos 1, 2 y 3.
    setwritemode(COPY_PUT);
    setactivepage(0);
    setvisualpage(0);
    cleardevice();
    oldFrameValid = 0;
    oldAxisValid = 0;
    visualPage = 0;
    activePage = 1;
    drawStaticScene(0.0, paused);
    copyAxis(&oldAxis, &currentAxis);
    oldAxisValid = 1;
}

void resetDoublePage(void) {
    // Limpia ambas paginas para evitar basura al entrar al modo de doble buffer.
    setwritemode(COPY_PUT);
    setactivepage(0);
    cleardevice();
    setactivepage(1);
    cleardevice();
    setvisualpage(0);
    setactivepage(1);
    visualPage = 0;
    activePage = 1;
    oldFrameValid = 0;
    oldAxisValid = 0;
}

void prepareMode(int mode, int paused) {
    // Al cambiar de metodo se reinicia la pagina para que no queden residuos.
    currentMode = mode;
    if (currentMode == MODE_PAGE_FLIP) {
        resetDoublePage();
    } else {
        resetSinglePage(paused);
    }
}

void drawClearMode(float angle, int paused) {
    // Metodo base: limpia toda la pantalla, por eso suele parpadear mas.
    setactivepage(0);
    setvisualpage(0);
    setwritemode(COPY_PUT);
    cleardevice();
    drawStaticScene(angle, paused);
    buildCubeFrame(angle, &currentFrame);
    drawCube(&currentFrame, 0);
}

void drawIncrementalMode(float angle, int paused) {
    // Borra solo el cubo anterior y vuelve a dibujar el fondo dañado por el borrado.
    setactivepage(0);
    setvisualpage(0);
    setwritemode(COPY_PUT);

    if (oldFrameValid) {
        drawCube(&oldFrame, 1);
    }

    if (oldAxisValid) {
        drawAxisFrame(&oldAxis, 1);
    }

    drawHud(paused);
    drawGroundGrid();
    computeAxis(angle, &currentAxis);
    drawAxisFrame(&currentAxis, 0);
    buildCubeFrame(angle, &currentFrame);
    drawCube(&currentFrame, 0);
    copyFrame(&oldFrame, &currentFrame);
    copyAxis(&oldAxis, &currentAxis);
    oldFrameValid = 1;
    oldAxisValid = 1;
}

void drawXorMode(float angle, int paused) {
    // XOR permite "mover" lineas: dibujar la misma linea otra vez la borra.
    setactivepage(0);
    setvisualpage(0);
    drawHud(paused);
    setwritemode(XOR_PUT);

    if (oldFrameValid) {
        drawCube(&oldFrame, 0);
    }

    buildCubeFrame(angle, &currentFrame);
    drawCube(&currentFrame, 0);
    copyFrame(&oldFrame, &currentFrame);
    oldFrameValid = 1;
    setwritemode(COPY_PUT);
}

void drawPageFlipMode(float angle, int paused) {
    // La pagina visible queda congelada mientras se borra y redibuja la pagina oculta.
    setactivepage(activePage);
    setwritemode(COPY_PUT);
    cleardevice();
    drawStaticScene(angle, paused);
    buildCubeFrame(angle, &currentFrame);
    drawCube(&currentFrame, 0);

    // El cambio visual ocurre cuando el cuadro completo ya esta terminado.
    setvisualpage(activePage);
    visualPage = activePage;
    activePage = 1 - activePage;
}

void adjustDelay(int delta) {
    // Limita el delay para poder comparar fluidez sin bloquear la demo.
    frameDelay += delta;
    if (frameDelay < 0) {
        frameDelay = 0;
    }
    if (frameDelay > 120) {
        frameDelay = 120;
    }
}

int handleInput(int *paused) {
    int key;
    int scan;
    int changedMode;

    changedMode = 0;

    while (kbhit()) {
        key = getch();

        if (key == KEY_ESC) {
            return 1;
        }

        if (key == KEY_SPACE) {
            *paused = !*paused;
            changedMode = 1;
        } else if (key == '1') {
            prepareMode(MODE_CLEAR, *paused);
            changedMode = 1;
        } else if (key == '2') {
            prepareMode(MODE_INCREMENTAL, *paused);
            changedMode = 1;
        } else if (key == '3') {
            prepareMode(MODE_XOR, *paused);
            changedMode = 1;
        } else if (key == '4') {
            prepareMode(MODE_PAGE_FLIP, *paused);
            changedMode = 1;
        } else if (key == '+' || key == '=') {
            adjustDelay(-3);
            changedMode = 1;
        } else if (key == '-' || key == '_') {
            adjustDelay(3);
            changedMode = 1;
        } else if (key == KEY_EXTENDED_1 || key == KEY_EXTENDED_2) {
            scan = getch();
            if (scan == KEY_RIGHT || scan == KEY_UP) {
                adjustDelay(-3);
                changedMode = 1;
            } else if (scan == KEY_LEFT || scan == KEY_DOWN) {
                adjustDelay(3);
                changedMode = 1;
            }
        }
    }

    if (changedMode && currentMode != MODE_PAGE_FLIP) {
        drawHud(*paused);
    }

    return 0;
}

void drawFrame(float angle, int paused) {
    // Enruta cada cuadro al metodo activo para comparar resultados en caliente.
    if (currentMode == MODE_CLEAR) {
        drawClearMode(angle, paused);
    } else if (currentMode == MODE_INCREMENTAL) {
        drawIncrementalMode(angle, paused);
    } else if (currentMode == MODE_XOR) {
        drawXorMode(angle, paused);
    } else {
        drawPageFlipMode(angle, paused);
    }
}

int main(void) {
    int gd;
    int gm;
    int graphError;
    int done;
    int paused;
    float angle;

    // VGAMED = 640x350 con 2 paginas en BGI; esto permite probar page flipping.
    gd = VGA;
    gm = VGAMED;
    initgraph(&gd, &gm, BGI_DIR);
    graphError = graphresult();
    if (graphError != grOk) {
        printf("Error grafico: %s\n", grapherrormsg(graphError));
        printf("No se pudo iniciar VGA/VGAMED con BGI en %s\n", BGI_DIR);
        printf("Presiona una tecla para salir...");
        getch();
        return 1;
    }

    screenW = getmaxx();
    screenH = getmaxy();
    centerX = screenW / 2;
    centerY = (screenH / 2) + 18;

    setbkcolor(BLACK);
    setlinestyle(SOLID_LINE, 0, NORM_WIDTH);

    done = 0;
    paused = 0;
    angle = 0.0;
    prepareMode(MODE_INCREMENTAL, paused);

    while (!done) {
        done = handleInput(&paused);

        if (!paused) {
            angle += 0.040;
        }

        drawFrame(angle, paused);
        delay(frameDelay);
    }

    // Restaura la pagina visible antes de cerrar para no dejar DOSBox en pagina oculta.
    setwritemode(COPY_PUT);
    setactivepage(0);
    setvisualpage(0);
    closegraph();
    return 0;
}
