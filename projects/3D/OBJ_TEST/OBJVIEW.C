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
#define MAX_VERTICES 600
#define MAX_TRIANGLES 1400
#define MAX_EDGES 1000
#define MAX_FACE_ITEMS 32

#define TARGET_SIZE 220.0
#define FOCAL_LENGTH 280.0
#define NEAR_CLIP 18.0
#define MOVE_STEP 18.0
#define ROT_STEP 0.055

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

struct Model model;
struct Camera camera;

int screenW;
int screenH;
int centerX;
int centerY;
int projectedX[MAX_VERTICES];
int projectedY[MAX_VERTICES];
int projectedOk[MAX_VERTICES];
int oldProjectedX[MAX_VERTICES];
int oldProjectedY[MAX_VERTICES];
int oldProjectedOk[MAX_VERTICES];
int oldFrameValid;
int visualPage;
int activePage;

float absFloat(float value) {
    /* Turbo C no necesita fabs para este uso simple. */
    if (value < 0.0) {
        return -value;
    }

    return value;
}

float maxFloat(float a, float b) {
    /* Devuelve el mayor valor sin macros para evitar dobles evaluaciones. */
    if (a > b) {
        return a;
    }

    return b;
}

void waitExit(void) {
    /* Mantiene visible el mensaje de error en DOSBox. */
    printf("\nPresiona una tecla para salir...");
    getch();
}

void clearModel(struct Model *m) {
    /* Reinicia contadores antes de cargar un archivo OBJ nuevo. */
    m->vertexCount = 0;
    m->triangleCount = 0;
    m->edgeCount = 0;
    m->ignoredCount = 0;
}

void stripComment(char *line) {
    int i;

    /* OBJ usa # para comentarios; tambien puede aparecer al final de una linea. */
    for (i = 0; line[i] != '\0'; i++) {
        if (line[i] == '#') {
            line[i] = '\0';
            return;
        }
    }
}

char *trimStart(char *text) {
    /* Avanza hasta el primer caracter visible. */
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
    /* Los indices OBJ usan digitos ASCII. */
    return value >= '0' && value <= '9';
}

int skipOptionalObjIndex(char *token, int *pos) {
    int hasDigits;

    /* Lee la parte opcional de v/vt/vn dentro de un token de cara. */
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

    /* Soporta 1, 1/2, 1/2/3 y 1//3; solo guarda el indice de vertice. */
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
    /* OBJ empieza en 1; indices negativos son relativos al ultimo vertice leido. */
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
    /* Guarda un vertice del OBJ si hay espacio en el arreglo fijo. */
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
    /* Cada cara OBJ se convierte en triangulos para dibujar wireframe triangular. */
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
    /* Soporte opcional para lineas OBJ l, si algun exportador las genera. */
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

    /* v requiere x y z; w opcional se acepta pero no se usa para dibujar. */
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
        printf("ERROR linea %d: demasiados vertices.\n", lineNo);
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

    /* Una cara n-gon se triangula en abanico: 0,i,i+1. */
    if (!readIndexList(args, indices, &count, lineNo, m->vertexCount)) {
        return 0;
    }

    if (count < 3) {
        printf("ERROR linea %d: una cara f necesita minimo 3 vertices.\n", lineNo);
        return 0;
    }

    for (i = 1; i < count - 1; i++) {
        if (!addTriangle(m, indices[0], indices[i], indices[i + 1])) {
            printf("ERROR linea %d: demasiados triangulos.\n", lineNo);
            return 0;
        }
    }

    return 1;
}

int parseLineCommand(struct Model *m, char *args, int lineNo) {
    int indices[MAX_FACE_ITEMS];
    int count;
    int i;

    /* l conecta vertices consecutivos; Blender normalmente no la usa en mallas. */
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

    /* Valida vt/vn de forma basica aunque el visor no los use todavia. */
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

    /* Divide cada linea OBJ en palabra clave y argumentos. */
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
        /* Son datos utiles para materiales/grupos, pero no para lineas 3D aun. */
        m->ignoredCount++;
        return 1;
    }

    /* Palabras desconocidas no detienen el visor; solo se informan. */
    printf("AVISO linea %d: palabra OBJ ignorada: %s\n", lineNo, keyword);
    m->ignoredCount++;
    return 1;
}

int loadObj(struct Model *m, char *path) {
    FILE *file;
    char line[MAX_LINE];
    int lineNo;

    /* Carga el archivo OBJ completo antes de iniciar el modo grafico. */
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

    /* Centra y escala el modelo para que cualquier OBJ pequeno sea visible. */
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
    /* La camara mira hacia +Z, con el modelo centrado en el origen. */
    camera.x = 0.0;
    camera.y = 0.0;
    camera.z = -430.0;
    camera.yaw = 0.0;
    camera.pitch = 0.0;
    camera.roll = 0.0;
}

void rotatePair(float *a, float *b, float angle) {
    float oldA;
    float oldB;
    float c;
    float s;

    /* Rota dos componentes y permite formar yaw, pitch y roll. */
    oldA = *a;
    oldB = *b;
    c = cos(angle);
    s = sin(angle);

    *a = oldA * c - oldB * s;
    *b = oldA * s + oldB * c;
}

int projectVertex(struct Vec3 point, int *screenX, int *screenY) {
    float x;
    float y;
    float z;
    float factor;

    /* Pasa de mundo a camara: trasladar y aplicar rotacion inversa de vista. */
    x = point.x - camera.x;
    y = point.y - camera.y;
    z = point.z - camera.z;

    rotatePair(&x, &z, camera.yaw);
    rotatePair(&y, &z, camera.pitch);
    rotatePair(&x, &y, camera.roll);

    if (z <= NEAR_CLIP) {
        return 0;
    }

    factor = FOCAL_LENGTH / z;
    *screenX = centerX + (int)(x * factor);
    *screenY = centerY - (int)(y * factor);
    return 1;
}

void projectAllVertices(struct Model *m, int sx[], int sy[], int ok[]) {
    int i;

    /* Proyecta vertices una vez por cuadro; las lineas reutilizan estos puntos. */
    for (i = 0; i < m->vertexCount; i++) {
        ok[i] = projectVertex(m->vertices[i], &sx[i], &sy[i]);
    }
}

void drawTriangleLines(struct Triangle *tri, int sx[], int sy[], int ok[]) {
    int a;
    int b;
    int c;

    /* Dibuja las tres aristas del triangulo si sus vertices son visibles. */
    a = tri->a;
    b = tri->b;
    c = tri->c;

    if (!ok[a] || !ok[b] || !ok[c]) {
        return;
    }

    line(sx[a], sy[a], sx[b], sy[b]);
    line(sx[b], sy[b], sx[c], sy[c]);
    line(sx[c], sy[c], sx[a], sy[a]);
}

void drawExplicitEdge(struct Edge *edge, int sx[], int sy[], int ok[]) {
    int a;
    int b;

    /* Dibuja lineas l del OBJ cuando existen. */
    a = edge->a;
    b = edge->b;

    if (!ok[a] || !ok[b]) {
        return;
    }

    line(sx[a], sy[a], sx[b], sy[b]);
}

void drawModelLines(struct Model *m, int sx[], int sy[], int ok[], int color) {
    int i;

    /* Wireframe triangular: cada cara f ya fue triangulada al cargar. */
    setcolor(color);
    for (i = 0; i < m->triangleCount; i++) {
        drawTriangleLines(&m->triangles[i], sx, sy, ok);
    }

    if (m->edgeCount > 0) {
        setcolor(color == BLACK ? BLACK : YELLOW);
        for (i = 0; i < m->edgeCount; i++) {
            drawExplicitEdge(&m->edges[i], sx, sy, ok);
        }
    }
}

void copyProjection(void) {
    int i;

    /* Guarda el frame dibujado para borrarlo con lineas negras en el siguiente. */
    for (i = 0; i < model.vertexCount; i++) {
        oldProjectedX[i] = projectedX[i];
        oldProjectedY[i] = projectedY[i];
        oldProjectedOk[i] = projectedOk[i];
    }

    oldFrameValid = 1;
}

void drawHud(void) {
    char line1[120];
    char line2[120];
    char line3[120];

    /* La barra del HUD se repinta completa para no dejar texto viejo. */
    setfillstyle(SOLID_FILL, BLACK);
    bar(0, 0, screenW, 44);

    sprintf(line1, "OBJVIEW: %d vertices  %d triangulos  %d lineas",
            model.vertexCount, model.triangleCount, model.edgeCount);
    sprintf(line2, "W/S adelante-atras  A/D lateral  Q/E subir-bajar  Flechas rotan");
    sprintf(line3, "Z/X roll eje propio  R reset  ESC salir");

    setcolor(LIGHTGRAY);
    outtextxy(8, 8, line1);
    outtextxy(8, 21, line2);
    outtextxy(8, 34, line3);
}

void initDoubleBufferPages(void) {
    /* Dibuja en una pagina oculta y muestra el frame completo al final. */
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
}

void flipDoubleBufferPage(void) {
    /* Evita que el wireframe se vea linea por linea durante el redibujado. */
    setvisualpage(activePage);
    visualPage = activePage;
    activePage = 1 - activePage;
}

void drawFrame(int forceClear) {
    /* Con doble pagina se redibuja completo en la pagina oculta. */
    if (forceClear) {
        oldFrameValid = 0;
    }

    setactivepage(activePage);
    setwritemode(COPY_PUT);
    cleardevice();
    projectAllVertices(&model, projectedX, projectedY, projectedOk);
    drawModelLines(&model, projectedX, projectedY, projectedOk, LIGHTCYAN);
    drawHud();
    copyProjection();
    flipDoubleBufferPage();
}

void moveForward(float amount) {
    float cp;

    /* Movimiento relativo hacia donde apunta la camara. */
    cp = cos(camera.pitch);
    camera.x += sin(camera.yaw) * cp * amount;
    camera.y += sin(camera.pitch) * amount;
    camera.z += cos(camera.yaw) * cp * amount;
}

void moveRight(float amount) {
    /* Movimiento lateral relativo al yaw de la camara. */
    camera.x += cos(camera.yaw) * amount;
    camera.z -= sin(camera.yaw) * amount;
}

int handleInput(void) {
    int changed;
    int key;
    int scan;

    /* Lee todas las teclas pendientes para que la camara responda fluida. */
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
       VGAMED permite dos paginas BGI. Asi el usuario ve solo el frame final,
       no el proceso de limpiar y redibujar lineas.
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
    centerY = (screenH / 2) + 22;
    oldFrameValid = 0;

    setbkcolor(BLACK);
    initDoubleBufferPages();
    drawFrame(1);

    while (1) {
        action = handleInput();
        if (action < 0) {
            break;
        }

        if (action > 0) {
            drawFrame(0);
        }

        delay(16);
    }

    setactivepage(0);
    setvisualpage(0);
    closegraph();
    return 0;
}
