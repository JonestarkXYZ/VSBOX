#include <graphics.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define MAX_VERTICES 180
#define MAX_ARISTAS 260
#define TOTAL_MODELOS 12
#define MODEL_DIR "C:\\projects\\Show3D\\models\\"
#define BGI_DIR "C:\\TURBOC3\\BGI"
#define VIEW_DISTANCE 420.0
#define FOCAL_LENGTH 300.0
#define TARGET_SIZE 280.0
#define ESC_KEY 27
#define AXIS_POINTS 4
#define KEY_EXTENDED_1 0
#define KEY_EXTENDED_2 224
#define KEY_LEFT 75
#define KEY_RIGHT 77
#define NAV_NONE 0
#define NAV_NEXT 1
#define NAV_PREV -1
#define NAV_EXIT 2
#define FRAME_DELAY_MS 35
#define KEY_NAV_DELAY_MS 220

struct Vec3 {
    float x;
    float y;
    float z;
};

struct ModeloInfo {
    char *archivo;
    char *nombre;
    int color;
};

struct Vec3 vertices[MAX_VERTICES];
int aristas[MAX_ARISTAS][2];
int px[MAX_VERTICES];
int py[MAX_VERTICES];
float pz[MAX_VERTICES];
int oldPx[MAX_VERTICES];
int oldPy[MAX_VERTICES];
float oldPz[MAX_VERTICES];
int axisPx[AXIS_POINTS];
int axisPy[AXIS_POINTS];
int oldAxisPx[AXIS_POINTS];
int oldAxisPy[AXIS_POINTS];
int numVertices = 0;
int numAristas = 0;
int screenW = 639;
int screenH = 479;
int hasPreviousFrame = 0;
float angulo = 0.0;

struct ModeloInfo modelos[TOTAL_MODELOS] = {
    {"model1.txt", "Cubo", LIGHTCYAN},
    {"model2.txt", "Piramide cuadrada", LIGHTRED},
    {"model3.txt", "Casa", LIGHTGREEN},
    {"model4.txt", "Prisma hexagonal", YELLOW},
    {"model5.txt", "Poligono extruido", LIGHTMAGENTA},
    {"model6.txt", "Tetraedro", CYAN},
    {"model7.txt", "Octaedro", LIGHTBLUE},
    {"model8.txt", "Prisma triangular", GREEN},
    {"model9.txt", "Cilindro", WHITE},
    {"model10.txt", "Cono", RED},
    {"model11.txt", "Esfera de alambre", LIGHTGRAY},
    {"model12.txt", "Casa voxel tipo Minecraft", YELLOW}
};

float absFloat(float value) {
    if (value < 0.0) {
        return -value;
    }
    return value;
}

float maxFloat(float a, float b) {
    if (a > b) {
        return a;
    }
    return b;
}

void rotatePair(float *a, float *b, float angle) {
    float cosA;
    float sinA;
    float temp;

    cosA = cos(angle);
    sinA = sin(angle);
    temp = (*a * cosA) - (*b * sinA);
    *b = (*a * sinA) + (*b * cosA);
    *a = temp;
}

void rotatePoint(struct Vec3 *point, float ax, float ay, float az) {
    // Rotacion en X, Y y Z usando pares de coordenadas.
    rotatePair(&point->y, &point->z, ax);
    rotatePair(&point->z, &point->x, ay);
    rotatePair(&point->x, &point->y, az);
}

void projectPoint(struct Vec3 point, int *sx, int *sy) {
    float denom;
    float scale;
    int centerX;
    int centerY;

    denom = point.z + VIEW_DISTANCE;
    if (denom < 1.0) {
        denom = 1.0;
    }

    // Proyeccion en perspectiva: mas lejos en Z se dibuja mas pequeno.
    scale = FOCAL_LENGTH / denom;
    centerX = screenW / 2;
    centerY = (screenH / 2) + 18;
    *sx = centerX + (int)(point.x * scale);
    *sy = centerY - (int)(point.y * scale);
}

void normalizeModel(void) {
    int i;
    float minX;
    float maxX;
    float minY;
    float maxY;
    float minZ;
    float maxZ;
    float centerX;
    float centerY;
    float centerZ;
    float sizeX;
    float sizeY;
    float sizeZ;
    float maxSize;
    float scale;

    if (numVertices <= 0) {
        return;
    }

    minX = vertices[0].x;
    maxX = vertices[0].x;
    minY = vertices[0].y;
    maxY = vertices[0].y;
    minZ = vertices[0].z;
    maxZ = vertices[0].z;

    for (i = 1; i < numVertices; i++) {
        if (vertices[i].x < minX) minX = vertices[i].x;
        if (vertices[i].x > maxX) maxX = vertices[i].x;
        if (vertices[i].y < minY) minY = vertices[i].y;
        if (vertices[i].y > maxY) maxY = vertices[i].y;
        if (vertices[i].z < minZ) minZ = vertices[i].z;
        if (vertices[i].z > maxZ) maxZ = vertices[i].z;
    }

    centerX = (minX + maxX) / 2.0;
    centerY = (minY + maxY) / 2.0;
    centerZ = (minZ + maxZ) / 2.0;
    sizeX = absFloat(maxX - minX);
    sizeY = absFloat(maxY - minY);
    sizeZ = absFloat(maxZ - minZ);
    maxSize = maxFloat(sizeX, maxFloat(sizeY, sizeZ));

    if (maxSize < 1.0) {
        return;
    }

    // Todos los modelos quedan centrados y con tamano parecido en pantalla.
    scale = TARGET_SIZE / maxSize;
    for (i = 0; i < numVertices; i++) {
        vertices[i].x = (vertices[i].x - centerX) * scale;
        vertices[i].y = (vertices[i].y - centerY) * scale;
        vertices[i].z = (vertices[i].z - centerZ) * scale;
    }
}

int failLoad(FILE *archivo) {
    if (archivo != NULL) {
        fclose(archivo);
    }
    numVertices = 0;
    numAristas = 0;
    return 0;
}

int loadModel(char *path) {
    FILE *archivo;
    char palabra[24];
    int i;
    int cantidad;
    int a;
    int b;
    float x;
    float y;
    float z;
    int tieneVertices;
    int tieneAristas;

    archivo = fopen(path, "r");
    if (archivo == NULL) {
        return 0;
    }

    numVertices = 0;
    numAristas = 0;
    tieneVertices = 0;
    tieneAristas = 0;

    while (fscanf(archivo, "%23s", palabra) == 1) {
        if (strcmp(palabra, "VERTICES") == 0) {
            if (fscanf(archivo, "%d", &cantidad) != 1) {
                return failLoad(archivo);
            }
            if (cantidad < 1 || cantidad > MAX_VERTICES) {
                return failLoad(archivo);
            }

            numVertices = cantidad;
            for (i = 0; i < numVertices; i++) {
                if (fscanf(archivo, "%f %f %f", &x, &y, &z) != 3) {
                    return failLoad(archivo);
                }
                vertices[i].x = x;
                vertices[i].y = y;
                vertices[i].z = z;
            }
            tieneVertices = 1;
        } else if (strcmp(palabra, "ARISTAS") == 0) {
            if (fscanf(archivo, "%d", &cantidad) != 1) {
                return failLoad(archivo);
            }
            if (cantidad < 1 || cantidad > MAX_ARISTAS) {
                return failLoad(archivo);
            }

            numAristas = cantidad;
            for (i = 0; i < numAristas; i++) {
                if (fscanf(archivo, "%d %d", &a, &b) != 2) {
                    return failLoad(archivo);
                }
                if (a < 0 || b < 0 || a >= numVertices || b >= numVertices) {
                    return failLoad(archivo);
                }
                aristas[i][0] = a;
                aristas[i][1] = b;
            }
            tieneAristas = 1;
        }
    }

    fclose(archivo);

    if (!tieneVertices || !tieneAristas) {
        numVertices = 0;
        numAristas = 0;
        return 0;
    }

    normalizeModel();
    return 1;
}

void transformModel(float angle) {
    int i;
    struct Vec3 point;

    for (i = 0; i < numVertices; i++) {
        point = vertices[i];

        // Velocidades distintas por eje para que se perciba mejor el volumen.
        rotatePoint(&point, angle * 0.75, angle * 1.10, angle * 0.45);
        projectPoint(point, &px[i], &py[i]);
        pz[i] = point.z;
    }
}

int depthLevel(float zValue) {
    if (zValue > 65.0) {
        return 0;
    }
    if (zValue < -65.0) {
        return 2;
    }
    return 1;
}

int depthColor(int baseColor, int level) {
    // Nivel 0: lejos, nivel 1: medio, nivel 2: cerca.
    if (level == 0) {
        return DARKGRAY;
    }
    if (level == 2) {
        return WHITE;
    }
    return baseColor;
}

void drawEdgesFrom(int baseColor, int erase, int drawPx[], int drawPy[], float drawPz[]) {
    int pass;
    int i;
    int a;
    int b;
    int level;
    float zAvg;

    // Se dibuja de atras hacia adelante para reforzar la profundidad.
    for (pass = 0; pass < 3; pass++) {
        for (i = 0; i < numAristas; i++) {
            a = aristas[i][0];
            b = aristas[i][1];
            zAvg = (drawPz[a] + drawPz[b]) / 2.0;
            level = depthLevel(zAvg);

            if (level == pass) {
                if (erase) {
                    setcolor(BLACK);
                } else {
                    setcolor(depthColor(baseColor, level));
                }
                line(drawPx[a], drawPy[a], drawPx[b], drawPy[b]);
            }
        }
    }
}

void drawEdges(int baseColor) {
    drawEdgesFrom(baseColor, 0, px, py, pz);
}

void drawVerticesFrom(int baseColor, int erase, int drawPx[], int drawPy[], float drawPz[]) {
    int i;
    int level;

    for (i = 0; i < numVertices; i++) {
        level = depthLevel(drawPz[i]);
        if (erase) {
            setcolor(BLACK);
        } else {
            setcolor(depthColor(baseColor, level));
        }
        circle(drawPx[i], drawPy[i], 2);
    }
}

void drawVertices(int baseColor) {
    drawVerticesFrom(baseColor, 0, px, py, pz);
}

void computeAxis(float angle, int drawPx[], int drawPy[]) {
    struct Vec3 origin;
    struct Vec3 axisX;
    struct Vec3 axisY;
    struct Vec3 axisZ;

    origin.x = 0.0;
    origin.y = 0.0;
    origin.z = 0.0;
    axisX.x = 155.0;
    axisX.y = 0.0;
    axisX.z = 0.0;
    axisY.x = 0.0;
    axisY.y = 155.0;
    axisY.z = 0.0;
    axisZ.x = 0.0;
    axisZ.y = 0.0;
    axisZ.z = 155.0;

    rotatePoint(&axisX, angle * 0.75, angle * 1.10, angle * 0.45);
    rotatePoint(&axisY, angle * 0.75, angle * 1.10, angle * 0.45);
    rotatePoint(&axisZ, angle * 0.75, angle * 1.10, angle * 0.45);

    // Indice 0: origen; 1, 2 y 3: extremos X, Y y Z.
    projectPoint(origin, &drawPx[0], &drawPy[0]);
    projectPoint(axisX, &drawPx[1], &drawPy[1]);
    projectPoint(axisY, &drawPx[2], &drawPy[2]);
    projectPoint(axisZ, &drawPx[3], &drawPy[3]);
}

void drawAxisFrom(int drawPx[], int drawPy[], int erase) {
    int colorX;
    int colorY;
    int colorZ;

    if (erase) {
        colorX = BLACK;
        colorY = BLACK;
        colorZ = BLACK;
    } else {
        colorX = RED;
        colorY = GREEN;
        colorZ = CYAN;
    }

    setcolor(colorX);
    line(drawPx[0], drawPy[0], drawPx[1], drawPy[1]);
    outtextxy(drawPx[1] + 4, drawPy[1], "X");

    setcolor(colorY);
    line(drawPx[0], drawPy[0], drawPx[2], drawPy[2]);
    outtextxy(drawPx[2] + 4, drawPy[2], "Y");

    setcolor(colorZ);
    line(drawPx[0], drawPy[0], drawPx[3], drawPy[3]);
    outtextxy(drawPx[3] + 4, drawPy[3], "Z");
}

void drawAxis(float angle) {
    computeAxis(angle, axisPx, axisPy);
    drawAxisFrom(axisPx, axisPy, 0);
}

void saveFrame(void) {
    int i;

    // Guarda coordenadas proyectadas para borrar este frame en la vuelta siguiente.
    for (i = 0; i < numVertices; i++) {
        oldPx[i] = px[i];
        oldPy[i] = py[i];
        oldPz[i] = pz[i];
    }

    for (i = 0; i < AXIS_POINTS; i++) {
        oldAxisPx[i] = axisPx[i];
        oldAxisPy[i] = axisPy[i];
    }

    hasPreviousFrame = 1;
}

void erasePreviousFrame(int color) {
    if (!hasPreviousFrame) {
        return;
    }

    // Borrado incremental: evita el parpadeo de cleardevice() en cada frame.
    drawVerticesFrom(color, 1, oldPx, oldPy, oldPz);
    drawEdgesFrom(color, 1, oldPx, oldPy, oldPz);
    drawAxisFrom(oldAxisPx, oldAxisPy, 1);
}

void flushKeyboard(void) {
    // Limpia codigos pendientes para evitar que una pulsacion genere varios saltos.
    while (kbhit()) {
        getch();
    }
}

void debounceNavigationKey(void) {
    // Pausa corta despues de una flecha aceptada para filtrar repeticion del teclado.
    delay(KEY_NAV_DELAY_MS);
    flushKeyboard();
}

int readNavigationKey(void) {
    int key;
    int scanCode;

    if (!kbhit()) {
        return NAV_NONE;
    }

    key = getch();
    if (key == ESC_KEY) {
        debounceNavigationKey();
        return NAV_EXIT;
    }

    if (key == KEY_EXTENDED_1 || key == KEY_EXTENDED_2) {
        scanCode = getch();

        if (scanCode == KEY_RIGHT) {
            debounceNavigationKey();
            return NAV_NEXT;
        }

        if (scanCode == KEY_LEFT) {
            debounceNavigationKey();
            return NAV_PREV;
        }
    }

    // Cualquier otra tecla se consume pero no cambia el modelo.
    flushKeyboard();
    return NAV_NONE;
}

void drawHud(char *nombre, int indice) {
    char texto[120];

    setcolor(LIGHTGRAY);
    line(0, 31, screenW, 31);
    sprintf(texto, "%02d/%02d  %s   vertices:%d  aristas:%d",
            indice + 1, TOTAL_MODELOS, nombre, numVertices, numAristas);
    outtextxy(10, 10, texto);

    setcolor(DARKGRAY);
    outtextxy(10, screenH - 18, "Derecha: siguiente | Izquierda: anterior | ESC: salir");
}

int drawModel(char *nombre, int color, int indice) {
    int navAction;

    cleardevice();
    drawHud(nombre, indice);
    hasPreviousFrame = 0;

    while (1) {
        navAction = readNavigationKey();
        if (navAction != NAV_NONE) {
            return navAction;
        }

        erasePreviousFrame(color);
        transformModel(angulo);
        drawAxis(angulo);
        drawEdges(color);
        drawVertices(color);
        drawHud(nombre, indice);
        saveFrame();

        delay(FRAME_DELAY_MS);
        angulo += 0.035;
    }
}

int main(void) {
    int gd;
    int gm;
    int graphError;
    int modeloActual;
    int navAction;
    char path[90];

    gd = DETECT;
    gm = 0;
    initgraph(&gd, &gm, BGI_DIR);
    graphError = graphresult();
    if (graphError != grOk) {
        printf("Error grafico: %s\n", grapherrormsg(graphError));
        getch();
        return 1;
    }

    screenW = getmaxx();
    screenH = getmaxy();
    setbkcolor(BLACK);

    modeloActual = 0;
    while (1) {
        sprintf(path, "%s%s", MODEL_DIR, modelos[modeloActual].archivo);

        if (!loadModel(path)) {
            closegraph();
            printf("Error: no se pudo cargar el modelo:\n%s\n", path);
            printf("Revisa VERTICES, ARISTAS y limites maximos.\n");
            getch();
            return 1;
        }

        navAction = drawModel(modelos[modeloActual].nombre,
                              modelos[modeloActual].color,
                              modeloActual);

        if (navAction == NAV_EXIT) {
            break;
        }

        if (navAction == NAV_PREV) {
            modeloActual--;
            if (modeloActual < 0) {
                modeloActual = TOTAL_MODELOS - 1;
            }
        } else if (navAction == NAV_NEXT) {
            modeloActual++;
            if (modeloActual >= TOTAL_MODELOS) {
                modeloActual = 0;
            }
        }
    }

    cleardevice();
    setcolor(LIGHTGREEN);
    outtextxy(10, 10, "Fin del visor 3D. Presiona una tecla...");
    getch();
    closegraph();
    return 0;
}
