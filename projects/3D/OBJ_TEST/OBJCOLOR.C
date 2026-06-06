#include <graphics.h>
#include <conio.h>
#include <dos.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define OBJ_PATH "C:\\projects\\3D\\OBJ_TEST\\models\\model2.obj"
#define BGI_DIR "C:\\TURBOC3\\BGI"

#define MAX_LINE 180
/* Estos limites cargan mona.obj: 507 vertices y 968 triangulos generados. */
#define MAX_VERTICES 560
#define MAX_TRIANGLES 1000
#define MAX_EDGES 500
#define MAX_FACE_ITEMS 32

#define TARGET_SIZE 220.0
#define FOCAL_LENGTH 280.0
#define NEAR_CLIP 10.0
#define MOVE_STEP 10.0
#define ROT_STEP 0.050

#define KEY_ESC 27
#define KEY_EXTENDED_1 0
#define KEY_EXTENDED_2 224
#define KEY_LEFT 75
#define KEY_RIGHT 77
#define KEY_UP 72
#define KEY_DOWN 80

struct Vec3 {
    float x;
    float y;
    float z;
};

struct Triangle {
    int a;
    int b;
    int c;
};

struct Edge {
    int a;
    int b;
};

struct Model {
    struct Vec3 vertices[MAX_VERTICES];
    struct Triangle triangles[MAX_TRIANGLES];
    struct Edge edges[MAX_EDGES];
    int vertexCount;
    int triangleCount;
    int edgeCount;
    int ignoredCount;
};

struct Camera {
    float x;
    float y;
    float z;
    float yaw;
    float pitch;
    float roll;
};

struct RenderTriangle {
    int triangleIndex;
    int sx[3];
    int sy[3];
    int color;
    float depth;
};

struct Model model;
struct Camera camera;
struct RenderTriangle renderList[MAX_TRIANGLES];

int screenW;
int screenH;
int centerX;
int centerY;
int projectedX[MAX_VERTICES];
int projectedY[MAX_VERTICES];
int projectedOk[MAX_VERTICES];
float cameraX[MAX_VERTICES];
float cameraY[MAX_VERTICES];
float cameraZ[MAX_VERTICES];
int renderCount;
int visualPage;
int activePage;

int triangleColors[12] = {
    BLUE, GREEN, CYAN, RED, MAGENTA, BROWN,
    LIGHTBLUE, LIGHTGREEN, LIGHTCYAN, LIGHTRED, LIGHTMAGENTA, YELLOW
};

float absFloat(float value) {
    /* Turbo C trabaja bien con esta funcion simple sin depender de fabs. */
    if (value < 0.0) {
        return -value;
    }

    return value;
}

float maxFloat(float a, float b) {
    /* Devuelve el valor mayor evitando macros con efectos secundarios. */
    if (a > b) {
        return a;
    }

    return b;
}

void waitExit(void) {
    /* Mantiene visible el mensaje cuando DOSBox cierra el programa. */
    printf("\nPresiona una tecla para salir...");
    getch();
}

void clearModel(struct Model *m) {
    /* Limpia el modelo antes de cargar otro OBJ. */
    m->vertexCount = 0;
    m->triangleCount = 0;
    m->edgeCount = 0;
    m->ignoredCount = 0;
}

void stripComment(char *line) {
    int i;

    /* En OBJ todo lo que sigue despues de # es comentario. */
    for (i = 0; line[i] != '\0'; i++) {
        if (line[i] == '#') {
            line[i] = '\0';
            return;
        }
    }
}

char *trimStart(char *text) {
    /* Salta espacios iniciales antes de leer la palabra clave OBJ. */
    while (*text != '\0' && isspace((unsigned char)*text)) {
        text++;
    }

    return text;
}

void trimEnd(char *text) {
    int len;

    /* Quita espacios finales para detectar lineas vacias. */
    len = strlen(text);
    while (len > 0 && isspace((unsigned char)text[len - 1])) {
        text[len - 1] = '\0';
        len--;
    }
}

int isDigitText(char value) {
    /* Valida digitos ASCII usados por indices OBJ. */
    return value >= '0' && value <= '9';
}

int skipOptionalObjIndex(char *token, int *pos) {
    int hasDigits;

    /* Lee una parte opcional de un indice tipo v/vt/vn. */
    if (token[*pos] == '-' || token[*pos] == '+') {
        (*pos)++;
    }

    hasDigits = 0;
    while (isDigitText(token[*pos])) {
        hasDigits = 1;
        (*pos)++;
    }

    return hasDigits;
}

int parseObjIndex(char *token, int *value) {
    int pos;
    int sign;
    int number;
    int slashCount;
    int hasOptional;

    /* Soporta indices de cara: 1, 1/2, 1/2/3 y 1//3. */
    pos = 0;
    sign = 1;
    number = 0;
    slashCount = 0;

    if (token[pos] == '-') {
        sign = -1;
        pos++;
    } else if (token[pos] == '+') {
        pos++;
    }

    if (!isDigitText(token[pos])) {
        return 0;
    }

    while (isDigitText(token[pos])) {
        number = (number * 10) + (token[pos] - '0');
        pos++;
    }

    *value = sign * number;

    while (token[pos] != '\0') {
        if (token[pos] != '/') {
            return 0;
        }

        slashCount++;
        if (slashCount > 2) {
            return 0;
        }
        pos++;

        if (token[pos] == '\0') {
            return 0;
        }

        if (token[pos] == '/') {
            if (slashCount != 1) {
                return 0;
            }
        } else {
            hasOptional = skipOptionalObjIndex(token, &pos);
            if (!hasOptional) {
                return 0;
            }
        }
    }

    return 1;
}

int convertObjIndex(int index, int vertexCount, int *zeroBased) {
    /* OBJ usa base 1; indices negativos son relativos al ultimo vertice leido. */
    if (index == 0 || vertexCount <= 0) {
        return 0;
    }

    if (index > 0) {
        if (index > vertexCount) {
            return 0;
        }

        *zeroBased = index - 1;
        return 1;
    }

    if (-index > vertexCount) {
        return 0;
    }

    *zeroBased = vertexCount + index;
    return 1;
}

int addVertex(struct Model *m, float x, float y, float z) {
    /* Guarda un vertice dentro del arreglo fijo del modelo. */
    if (m->vertexCount >= MAX_VERTICES) {
        return 0;
    }

    m->vertices[m->vertexCount].x = x;
    m->vertices[m->vertexCount].y = y;
    m->vertices[m->vertexCount].z = z;
    m->vertexCount++;
    return 1;
}

int addTriangle(struct Model *m, int a, int b, int c) {
    /* Guarda un triangulo ya convertido a indices base cero. */
    if (m->triangleCount >= MAX_TRIANGLES) {
        return 0;
    }

    m->triangles[m->triangleCount].a = a;
    m->triangles[m->triangleCount].b = b;
    m->triangles[m->triangleCount].c = c;
    m->triangleCount++;
    return 1;
}

int addEdge(struct Model *m, int a, int b) {
    /* Guarda lineas OBJ explicitas si el archivo las incluye. */
    if (m->edgeCount >= MAX_EDGES) {
        return 0;
    }

    m->edges[m->edgeCount].a = a;
    m->edges[m->edgeCount].b = b;
    m->edgeCount++;
    return 1;
}

int parseVertexLine(struct Model *m, char *args, int lineNo) {
    float x;
    float y;
    float z;
    float w;
    char extra[16];
    int readCount;

    /* Lee v x y z; acepta w opcional pero no lo usa. */
    if (args == NULL) {
        printf("ERROR linea %d: vertice sin coordenadas.\n", lineNo);
        return 0;
    }

    extra[0] = '\0';
    readCount = sscanf(args, " %f %f %f %f %15s", &x, &y, &z, &w, extra);
    if (readCount < 3 || readCount > 4) {
        printf("ERROR linea %d: formato de vertice invalido.\n", lineNo);
        return 0;
    }

    if (!addVertex(m, x, y, z)) {
        printf("ERROR linea %d: demasiados vertices. Limite: %d\n",
               lineNo, MAX_VERTICES);
        return 0;
    }

    return 1;
}

int readIndexList(char *args, int output[], int *count, int lineNo, int vertexCount) {
    char *token;
    int rawIndex;
    int zeroBased;

    /* Lee una lista de indices usada por f o l. */
    if (args == NULL) {
        printf("ERROR linea %d: lista de indices vacia.\n", lineNo);
        return 0;
    }

    *count = 0;
    token = strtok(args, " \t\r\n");
    while (token != NULL) {
        if (*count >= MAX_FACE_ITEMS) {
            printf("ERROR linea %d: demasiados indices.\n", lineNo);
            return 0;
        }

        if (!parseObjIndex(token, &rawIndex)) {
            printf("ERROR linea %d: indice OBJ invalido: %s\n", lineNo, token);
            return 0;
        }

        if (!convertObjIndex(rawIndex, vertexCount, &zeroBased)) {
            printf("ERROR linea %d: indice fuera de rango: %s\n", lineNo, token);
            return 0;
        }

        output[*count] = zeroBased;
        (*count)++;
        token = strtok(NULL, " \t\r\n");
    }

    return 1;
}

int parseFaceLine(struct Model *m, char *args, int lineNo) {
    int indices[MAX_FACE_ITEMS];
    int count;
    int i;

    /* Convierte una cara de N vertices en triangulos tipo abanico. */
    if (!readIndexList(args, indices, &count, lineNo, m->vertexCount)) {
        return 0;
    }

    if (count < 3) {
        printf("ERROR linea %d: una cara f necesita minimo 3 vertices.\n", lineNo);
        return 0;
    }

    for (i = 1; i < count - 1; i++) {
        if (!addTriangle(m, indices[0], indices[i], indices[i + 1])) {
            printf("ERROR linea %d: demasiados triangulos. Limite: %d\n",
                   lineNo, MAX_TRIANGLES);
            return 0;
        }
    }

    return 1;
}

int parseLineCommand(struct Model *m, char *args, int lineNo) {
    int indices[MAX_FACE_ITEMS];
    int count;
    int i;

    /* OBJ tambien puede traer l; Blender normalmente usa f para mallas. */
    if (!readIndexList(args, indices, &count, lineNo, m->vertexCount)) {
        return 0;
    }

    if (count < 2) {
        printf("ERROR linea %d: una linea l necesita minimo 2 vertices.\n", lineNo);
        return 0;
    }

    for (i = 0; i < count - 1; i++) {
        if (!addEdge(m, indices[i], indices[i + 1])) {
            printf("ERROR linea %d: demasiadas lineas.\n", lineNo);
            return 0;
        }
    }

    return 1;
}

int parseFloatLine(char *args, int minValues, int maxValues, int lineNo, char *label) {
    float a;
    float b;
    float c;
    float d;
    char extra[16];
    int readCount;

    /* Valida vt/vn aunque el render actual no usa texturas ni normales. */
    if (args == NULL) {
        printf("ERROR linea %d: %s sin valores.\n", lineNo, label);
        return 0;
    }

    extra[0] = '\0';
    readCount = sscanf(args, " %f %f %f %f %15s", &a, &b, &c, &d, extra);
    if (readCount < minValues || readCount > maxValues) {
        printf("ERROR linea %d: formato invalido en %s.\n", lineNo, label);
        return 0;
    }

    return 1;
}

int parseObjLine(struct Model *m, char *line, int lineNo) {
    char *clean;
    char *keyword;
    char *args;

    /* Divide cada linea en palabra clave OBJ y argumentos. */
    stripComment(line);
    clean = trimStart(line);
    trimEnd(clean);

    if (clean[0] == '\0') {
        return 1;
    }

    keyword = strtok(clean, " \t\r\n");
    args = strtok(NULL, "\n");

    if (strcmp(keyword, "v") == 0) {
        return parseVertexLine(m, args, lineNo);
    }

    if (strcmp(keyword, "f") == 0) {
        return parseFaceLine(m, args, lineNo);
    }

    if (strcmp(keyword, "l") == 0) {
        return parseLineCommand(m, args, lineNo);
    }

    if (strcmp(keyword, "vn") == 0) {
        m->ignoredCount++;
        return parseFloatLine(args, 3, 3, lineNo, "normal vn");
    }

    if (strcmp(keyword, "vt") == 0) {
        m->ignoredCount++;
        return parseFloatLine(args, 1, 3, lineNo, "textura vt");
    }

    if (strcmp(keyword, "o") == 0 ||
        strcmp(keyword, "g") == 0 ||
        strcmp(keyword, "s") == 0 ||
        strcmp(keyword, "usemtl") == 0 ||
        strcmp(keyword, "mtllib") == 0) {
        /* Datos reconocidos pero no necesarios para pintar triangulos planos. */
        m->ignoredCount++;
        return 1;
    }

    printf("AVISO linea %d: palabra OBJ ignorada: %s\n", lineNo, keyword);
    m->ignoredCount++;
    return 1;
}

int loadObj(struct Model *m, char *path) {
    FILE *file;
    char line[MAX_LINE];
    int lineNo;

    /* Carga el OBJ en memoria antes de abrir graphics.h. */
    clearModel(m);
    file = fopen(path, "r");
    if (file == NULL) {
        printf("ERROR: no se pudo abrir:\n%s\n", path);
        return 0;
    }

    lineNo = 0;
    while (fgets(line, MAX_LINE, file) != NULL) {
        lineNo++;
        if (!parseObjLine(m, line, lineNo)) {
            fclose(file);
            return 0;
        }
    }

    fclose(file);

    if (m->vertexCount < 1) {
        printf("ERROR: el OBJ no tiene vertices v.\n");
        return 0;
    }

    if (m->triangleCount < 1 && m->edgeCount < 1) {
        printf("ERROR: el OBJ no tiene caras f ni lineas l.\n");
        return 0;
    }

    return 1;
}

void normalizeModel(struct Model *m) {
    int i;
    float minX;
    float maxX;
    float minY;
    float maxY;
    float minZ;
    float maxZ;
    float centerModelX;
    float centerModelY;
    float centerModelZ;
    float sizeX;
    float sizeY;
    float sizeZ;
    float maxSize;
    float scale;

    /* Centra el OBJ y le da un tamano visible dentro de 640x480. */
    if (m->vertexCount < 1) {
        return;
    }

    minX = m->vertices[0].x;
    maxX = m->vertices[0].x;
    minY = m->vertices[0].y;
    maxY = m->vertices[0].y;
    minZ = m->vertices[0].z;
    maxZ = m->vertices[0].z;

    for (i = 1; i < m->vertexCount; i++) {
        if (m->vertices[i].x < minX) minX = m->vertices[i].x;
        if (m->vertices[i].x > maxX) maxX = m->vertices[i].x;
        if (m->vertices[i].y < minY) minY = m->vertices[i].y;
        if (m->vertices[i].y > maxY) maxY = m->vertices[i].y;
        if (m->vertices[i].z < minZ) minZ = m->vertices[i].z;
        if (m->vertices[i].z > maxZ) maxZ = m->vertices[i].z;
    }

    centerModelX = (minX + maxX) / 2.0;
    centerModelY = (minY + maxY) / 2.0;
    centerModelZ = (minZ + maxZ) / 2.0;
    sizeX = absFloat(maxX - minX);
    sizeY = absFloat(maxY - minY);
    sizeZ = absFloat(maxZ - minZ);
    maxSize = maxFloat(sizeX, maxFloat(sizeY, sizeZ));

    if (maxSize < 0.001) {
        maxSize = 1.0;
    }

    scale = TARGET_SIZE / maxSize;
    for (i = 0; i < m->vertexCount; i++) {
        m->vertices[i].x = (m->vertices[i].x - centerModelX) * scale;
        m->vertices[i].y = (m->vertices[i].y - centerModelY) * scale;
        m->vertices[i].z = (m->vertices[i].z - centerModelZ) * scale;
    }
}

void resetCamera(void) {
    /* La camara queda delante del modelo y girada para mostrar volumen inicial. */
    camera.x = 0.0;
    camera.y = 0.0;
    camera.z = -430.0;
    camera.yaw = 0.34;
    camera.pitch = -0.18;
    camera.roll = 0.0;
}

void rotatePair(float *a, float *b, float angle) {
    float oldA;
    float oldB;
    float c;
    float s;

    /* Rota dos componentes; se usa para yaw, pitch y roll de camara. */
    oldA = *a;
    oldB = *b;
    c = cos(angle);
    s = sin(angle);

    *a = oldA * c - oldB * s;
    *b = oldA * s + oldB * c;
}

void worldToCamera(struct Vec3 point, float *x, float *y, float *z) {
    /* Convierte un punto del mundo al sistema local de la camara. */
    *x = point.x - camera.x;
    *y = point.y - camera.y;
    *z = point.z - camera.z;

    rotatePair(x, z, camera.yaw);
    rotatePair(y, z, camera.pitch);
    rotatePair(x, y, camera.roll);
}

int projectCameraPoint(float x, float y, float z, int *screenX, int *screenY) {
    float factor;

    /* Proyeccion perspectiva simple; no recorta parcialmente triangulos. */
    if (z <= NEAR_CLIP) {
        return 0;
    }

    factor = FOCAL_LENGTH / z;
    *screenX = centerX + (int)(x * factor);
    *screenY = centerY - (int)(y * factor);
    return 1;
}

void projectAllVertices(struct Model *m) {
    int i;

    /* Guarda coordenadas de camara y pantalla para cada vertice. */
    for (i = 0; i < m->vertexCount; i++) {
        worldToCamera(m->vertices[i], &cameraX[i], &cameraY[i], &cameraZ[i]);
        projectedOk[i] = projectCameraPoint(cameraX[i], cameraY[i], cameraZ[i],
                                            &projectedX[i], &projectedY[i]);
    }
}

int triangleFacesCamera(struct Triangle *tri) {
    float ux;
    float uy;
    float vx;
    float vy;
    float normalZ;

    /* Backface culling en espacio de camara: normal Z negativa mira a la camara. */
    ux = cameraX[tri->b] - cameraX[tri->a];
    uy = cameraY[tri->b] - cameraY[tri->a];
    vx = cameraX[tri->c] - cameraX[tri->a];
    vy = cameraY[tri->c] - cameraY[tri->a];
    normalZ = (ux * vy) - (uy * vx);

    return normalZ < -0.01;
}

void addRenderTriangle(struct Model *m, int triangleIndex) {
    struct Triangle *tri;
    struct RenderTriangle *item;

    /* Prepara un triangulo visible para ordenarlo y pintarlo despues. */
    if (renderCount >= MAX_TRIANGLES) {
        return;
    }

    tri = &m->triangles[triangleIndex];
    if (!projectedOk[tri->a] || !projectedOk[tri->b] || !projectedOk[tri->c]) {
        return;
    }

    if (!triangleFacesCamera(tri)) {
        return;
    }

    item = &renderList[renderCount];
    item->triangleIndex = triangleIndex;
    item->sx[0] = projectedX[tri->a];
    item->sy[0] = projectedY[tri->a];
    item->sx[1] = projectedX[tri->b];
    item->sy[1] = projectedY[tri->b];
    item->sx[2] = projectedX[tri->c];
    item->sy[2] = projectedY[tri->c];
    item->depth = (cameraZ[tri->a] + cameraZ[tri->b] + cameraZ[tri->c]) / 3.0;
    item->color = triangleColors[triangleIndex % 12];
    renderCount++;
}

void buildRenderList(struct Model *m) {
    int i;

    /* Genera solo los triangulos proyectados y orientados hacia la camara. */
    renderCount = 0;
    for (i = 0; i < m->triangleCount; i++) {
        addRenderTriangle(m, i);
    }
}

void sortRenderList(void) {
    int i;
    int j;
    struct RenderTriangle temp;

    /* Orden painter: primero los mas lejos, al final los cercanos. */
    for (i = 1; i < renderCount; i++) {
        temp = renderList[i];
        j = i - 1;

        while (j >= 0 && renderList[j].depth < temp.depth) {
            renderList[j + 1] = renderList[j];
            j--;
        }

        renderList[j + 1] = temp;
    }
}

void drawFilledTriangle(struct RenderTriangle *item) {
    int poly[6];

    /* Pinta el triangulo y luego marca su borde para ver la triangulacion. */
    poly[0] = item->sx[0];
    poly[1] = item->sy[0];
    poly[2] = item->sx[1];
    poly[3] = item->sy[1];
    poly[4] = item->sx[2];
    poly[5] = item->sy[2];

    setfillstyle(SOLID_FILL, item->color);
    fillpoly(3, poly);

    setcolor(BLACK);
    line(poly[0], poly[1], poly[2], poly[3]);
    line(poly[2], poly[3], poly[4], poly[5]);
    line(poly[4], poly[5], poly[0], poly[1]);
}

void drawExplicitEdges(struct Model *m) {
    int i;
    int a;
    int b;

    /* Si el OBJ trae l, se dibujan encima de las caras coloreadas. */
    if (m->edgeCount <= 0) {
        return;
    }

    setcolor(WHITE);
    for (i = 0; i < m->edgeCount; i++) {
        a = m->edges[i].a;
        b = m->edges[i].b;

        if (projectedOk[a] && projectedOk[b]) {
            line(projectedX[a], projectedY[a], projectedX[b], projectedY[b]);
        }
    }
}

void drawSceneTriangles(struct Model *m) {
    int i;

    /* Pinta de atras hacia adelante para que los cercanos tapen a los lejanos. */
    projectAllVertices(m);
    buildRenderList(m);
    sortRenderList();

    for (i = 0; i < renderCount; i++) {
        drawFilledTriangle(&renderList[i]);
    }

    drawExplicitEdges(m);
}

void drawHud(void) {
    char line1[120];
    char line2[120];
    char line3[120];

    /* El HUD muestra posicion y orientacion actual de la camara. */
    setfillstyle(SOLID_FILL, BLACK);
    bar(0, 0, screenW, 58);

    sprintf(line1, "OBJCOLOR  cam X:%6.1f Y:%6.1f Z:%6.1f",
            camera.x, camera.y, camera.z);
    sprintf(line2, "yaw:%5.2f pitch:%5.2f roll:%5.2f  visibles:%d/%d",
            camera.yaw, camera.pitch, camera.roll, renderCount, model.triangleCount);
    sprintf(line3, "W/S avance  A/D lateral  Q/E vertical  Flechas rotan  Z/X roll  R reset");

    setcolor(LIGHTGRAY);
    outtextxy(8, 8, line1);
    outtextxy(8, 22, line2);
    outtextxy(8, 36, line3);
}

void initDoubleBufferPages(void) {
    /* VGAMED ofrece dos paginas en BGI: una visible y otra oculta para dibujar. */
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

void flipDoubleBufferPage(void) {
    /* El cuadro se muestra solo cuando la pagina oculta ya fue dibujada completa. */
    setvisualpage(activePage);
    visualPage = activePage;
    activePage = 1 - activePage;
}

void drawFrame(void) {
    /* Dibuja en la pagina oculta; el usuario no ve el proceso de pintado. */
    setactivepage(activePage);
    setwritemode(COPY_PUT);
    cleardevice();
    drawSceneTriangles(&model);
    drawHud();
    flipDoubleBufferPage();
}

void moveForward(float amount) {
    float cp;

    /* Mueve la camara segun su eje frontal actual. */
    cp = cos(camera.pitch);
    camera.x += sin(camera.yaw) * cp * amount;
    camera.y += sin(camera.pitch) * amount;
    camera.z += cos(camera.yaw) * cp * amount;
}

void moveRight(float amount) {
    /* Mueve la camara lateralmente segun su yaw actual. */
    camera.x += cos(camera.yaw) * amount;
    camera.z -= sin(camera.yaw) * amount;
}

int handleInput(void) {
    int changed;
    int key;
    int scan;

    /* Mantiene los mismos controles usados por OBJVIEW.C. */
    changed = 0;

    while (kbhit()) {
        key = getch();

        if (key == KEY_ESC) {
            return -1;
        }

        if (key == KEY_EXTENDED_1 || key == KEY_EXTENDED_2) {
            scan = getch();
            if (scan == KEY_LEFT) {
                camera.yaw -= ROT_STEP;
                changed = 1;
            } else if (scan == KEY_RIGHT) {
                camera.yaw += ROT_STEP;
                changed = 1;
            } else if (scan == KEY_UP) {
                camera.pitch += ROT_STEP;
                changed = 1;
            } else if (scan == KEY_DOWN) {
                camera.pitch -= ROT_STEP;
                changed = 1;
            }
        } else if (key == 'w' || key == 'W') {
            moveForward(MOVE_STEP);
            changed = 1;
        } else if (key == 's' || key == 'S') {
            moveForward(-MOVE_STEP);
            changed = 1;
        } else if (key == 'a' || key == 'A') {
            moveRight(-MOVE_STEP);
            changed = 1;
        } else if (key == 'd' || key == 'D') {
            moveRight(MOVE_STEP);
            changed = 1;
        } else if (key == 'q' || key == 'Q') {
            camera.y -= MOVE_STEP;
            changed = 1;
        } else if (key == 'e' || key == 'E') {
            camera.y += MOVE_STEP;
            changed = 1;
        } else if (key == 'z' || key == 'Z') {
            camera.roll -= ROT_STEP;
            changed = 1;
        } else if (key == 'x' || key == 'X') {
            camera.roll += ROT_STEP;
            changed = 1;
        } else if (key == 'r' || key == 'R') {
            resetCamera();
            changed = 1;
        }
    }

    return changed;
}

int main(void) {
    int gd;
    int gm;
    int graphError;
    int action;

    if (!loadObj(&model, OBJ_PATH)) {
        waitExit();
        return 1;
    }

    normalizeModel(&model);
    resetCamera();

    /*
       VGAMED baja de 640x480 a 640x350, pero permite doble pagina real.
       Con VGAHI normalmente solo hay una pagina y se ve el dibujo progresivo.
    */
    gd = VGA;
    gm = VGAMED;
    initgraph(&gd, &gm, BGI_DIR);
    graphError = graphresult();
    if (graphError != grOk) {
        printf("Error grafico: %s\n", grapherrormsg(graphError));
        waitExit();
        return 1;
    }

    screenW = getmaxx();
    screenH = getmaxy();
    centerX = screenW / 2;
    centerY = (screenH / 2) + 28;

    setbkcolor(BLACK);
    initDoubleBufferPages();
    drawFrame();

    while (1) {
        action = handleInput();
        if (action < 0) {
            break;
        }

        if (action > 0) {
            drawFrame();
        }

        delay(16);
    }

    setactivepage(0);
    setvisualpage(0);
    closegraph();
    return 0;
}
