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
#define MAX_NORMALS 560
#define MAX_TRIANGLES 1000
#define MAX_EDGES 500
#define MAX_FACE_ITEMS 32

#define TARGET_SIZE 220.0
#define FOCAL_LENGTH 280.0
#define NEAR_CLIP 10.0
#define MOVE_STEP 10.0
#define ROT_STEP 0.050
#define LIGHT_X -0.35
#define LIGHT_Y 0.55
#define LIGHT_Z -0.76
#define AMBIENT_LIGHT 0.18
#define DIFFUSE_LIGHT 0.82
#define NORMAL_SCALE 100.0

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

struct PackedNormal {
    signed char x;
    signed char y;
    signed char z;
};

struct Triangle {
    int a;
    int b;
    int c;
    int normal;
};

struct Edge {
    int a;
    int b;
};

struct Model {
    struct Vec3 vertices[MAX_VERTICES];
    struct PackedNormal normals[MAX_NORMALS];
    struct Triangle triangles[MAX_TRIANGLES];
    struct Edge edges[MAX_EDGES];
    int vertexCount;
    int normalCount;
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
char projectedOk[MAX_VERTICES];
float cameraX[MAX_VERTICES];
float cameraY[MAX_VERTICES];
float cameraZ[MAX_VERTICES];
int renderCount;
int visualPage;
int activePage;
int showEdges = 0;
int cullBackfaces = 1;
int backfaceCulledCount;
int offscreenCulledCount;
int nearCulledCount;

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

float clampFloat(float value, float minValue, float maxValue) {
    /* Mantiene la iluminacion dentro del rango usable de la paleta. */
    if (value < minValue) {
        return minValue;
    }

    if (value > maxValue) {
        return maxValue;
    }

    return value;
}

int packNormalComponent(float value) {
    int packed;

    /* Guarda normales en 1 byte por componente para ahorrar memoria DOS. */
    value = clampFloat(value, -1.0, 1.0);
    if (value >= 0.0) {
        packed = (int)((value * NORMAL_SCALE) + 0.5);
    } else {
        packed = (int)((value * NORMAL_SCALE) - 0.5);
    }

    return packed;
}

void waitExit(void) {
    /* Mantiene visible el mensaje cuando DOSBox cierra el programa. */
    printf("\nPresiona una tecla para salir...");
    getch();
}

void clearModel(struct Model *m) {
    /* Limpia el modelo antes de cargar otro OBJ. */
    m->vertexCount = 0;
    m->normalCount = 0;
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

int parseSignedObjNumber(char *token, int *pos, int *value) {
    int sign;
    int number;

    /* Lee un numero entero OBJ con signo opcional. */
    sign = 1;
    number = 0;

    if (token[*pos] == '-') {
        sign = -1;
        (*pos)++;
    } else if (token[*pos] == '+') {
        (*pos)++;
    }

    if (!isDigitText(token[*pos])) {
        return 0;
    }

    while (isDigitText(token[*pos])) {
        number = (number * 10) + (token[*pos] - '0');
        (*pos)++;
    }

    *value = sign * number;
    return 1;
}

int parseObjFaceItem(char *token, int *vertexIndex, int *normalIndex, int *hasNormal) {
    int pos;
    int ignoredTexture;

    /* Soporta indices OBJ: v, v/vt, v//vn y v/vt/vn. */
    pos = 0;
    *normalIndex = 0;
    *hasNormal = 0;

    if (!parseSignedObjNumber(token, &pos, vertexIndex)) {
        return 0;
    }

    if (token[pos] == '\0') {
        return 1;
    }

    if (token[pos] != '/') {
        return 0;
    }
    pos++;

    if (token[pos] == '\0') {
        return 0;
    }

    if (token[pos] != '/' && token[pos] != '\0') {
        if (!parseSignedObjNumber(token, &pos, &ignoredTexture)) {
            return 0;
        }
    }

    if (token[pos] == '\0') {
        return 1;
    }

    if (token[pos] != '/') {
        return 0;
    }
    pos++;

    if (!parseSignedObjNumber(token, &pos, normalIndex)) {
        return 0;
    }

    if (token[pos] != '\0') {
        return 0;
    }

    *hasNormal = 1;
    return 1;
}

int parseObjIndex(char *token, int *value) {
    int normalIndex;
    int hasNormal;

    /* Lee un indice de vertice y descarta vt/vn cuando se usa en lineas l. */
    return parseObjFaceItem(token, value, &normalIndex, &hasNormal);
}

int convertObjIndex(int index, int itemCount, int *zeroBased) {
    /* OBJ usa base 1; indices negativos son relativos al ultimo item leido. */
    if (index == 0 || itemCount <= 0) {
        return 0;
    }

    if (index > 0) {
        if (index > itemCount) {
            return 0;
        }

        *zeroBased = index - 1;
        return 1;
    }

    if (-index > itemCount) {
        return 0;
    }

    *zeroBased = itemCount + index;
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

int addNormal(struct Model *m, float x, float y, float z) {
    float length;
    int px;
    int py;
    int pz;

    /* Guarda una normal OBJ normalizada y compactada para culling e iluminacion. */
    if (m->normalCount >= MAX_NORMALS) {
        return 0;
    }

    length = sqrt((x * x) + (y * y) + (z * z));
    if (length < 0.001) {
        return 0;
    }

    px = packNormalComponent(x / length);
    py = packNormalComponent(y / length);
    pz = packNormalComponent(z / length);

    m->normals[m->normalCount].x = (signed char)px;
    m->normals[m->normalCount].y = (signed char)py;
    m->normals[m->normalCount].z = (signed char)pz;
    m->normalCount++;
    return 1;
}

int addTriangle(struct Model *m, int a, int b, int c, int normal) {
    /* Guarda un triangulo ya convertido a indices base cero. */
    if (m->triangleCount >= MAX_TRIANGLES) {
        return 0;
    }

    m->triangles[m->triangleCount].a = a;
    m->triangles[m->triangleCount].b = b;
    m->triangles[m->triangleCount].c = c;
    m->triangles[m->triangleCount].normal = normal;
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

int parseNormalLine(struct Model *m, char *args, int lineNo) {
    float x;
    float y;
    float z;
    char extra[16];
    int readCount;

    /* Lee vn x y z; esta normal indica hacia donde mira la superficie. */
    if (args == NULL) {
        printf("ERROR linea %d: normal vn sin coordenadas.\n", lineNo);
        return 0;
    }

    extra[0] = '\0';
    readCount = sscanf(args, " %f %f %f %15s", &x, &y, &z, extra);
    if (readCount != 3) {
        printf("ERROR linea %d: formato de normal vn invalido.\n", lineNo);
        return 0;
    }

    if (!addNormal(m, x, y, z)) {
        printf("ERROR linea %d: demasiadas normales o normal nula. Limite: %d\n",
               lineNo, MAX_NORMALS);
        return 0;
    }

    return 1;
}

int readFaceItemList(char *args, int vertices[], int normals[],
                     int normalPresent[], int *count, int lineNo,
                     int vertexCount, int normalCount) {
    char *token;
    int rawVertex;
    int rawNormal;
    int zeroVertex;
    int zeroNormal;
    int hasNormal;

    /* Lee una lista de indices f conservando el indice vn si existe. */
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

        if (!parseObjFaceItem(token, &rawVertex, &rawNormal, &hasNormal)) {
            printf("ERROR linea %d: indice OBJ invalido: %s\n", lineNo, token);
            return 0;
        }

        if (!convertObjIndex(rawVertex, vertexCount, &zeroVertex)) {
            printf("ERROR linea %d: indice de vertice fuera de rango: %s\n",
                   lineNo, token);
            return 0;
        }

        zeroNormal = -1;
        if (hasNormal) {
            if (!convertObjIndex(rawNormal, normalCount, &zeroNormal)) {
                printf("ERROR linea %d: indice de normal fuera de rango: %s\n",
                       lineNo, token);
                return 0;
            }
        }

        vertices[*count] = zeroVertex;
        normals[*count] = zeroNormal;
        normalPresent[*count] = hasNormal;
        (*count)++;
        token = strtok(NULL, " \t\r\n");
    }

    return 1;
}

int readIndexList(char *args, int output[], int *count, int lineNo, int vertexCount) {
    char *token;
    int rawIndex;
    int zeroBased;

    /* Lee una lista de indices usada por l. */
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
    int normalIndices[MAX_FACE_ITEMS];
    int normalPresent[MAX_FACE_ITEMS];
    int count;
    int i;
    int triNormal;

    /* Convierte una cara de N vertices en triangulos tipo abanico. */
    if (!readFaceItemList(args, indices, normalIndices, normalPresent, &count,
                          lineNo, m->vertexCount, m->normalCount)) {
        return 0;
    }

    if (count < 3) {
        printf("ERROR linea %d: una cara f necesita minimo 3 vertices.\n", lineNo);
        return 0;
    }

    for (i = 1; i < count - 1; i++) {
        /*
           Se guarda solo una normal por triangulo para ahorrar memoria en Turbo C.
           Si el OBJ trae normales por vertice, la primera sirve como referencia
           de direccion; si no existe, el render calcula la normal geometrica.
        */
        triNormal = -1;
        if (normalPresent[0]) {
            triNormal = normalIndices[0];
        }

        if (!addTriangle(m, indices[0], indices[i], indices[i + 1], triNormal)) {
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

    /* Valida vt aunque el render actual no usa texturas. */
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
        return parseNormalLine(m, args, lineNo);
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

void normalToCamera(struct Vec3 normal, float *x, float *y, float *z) {
    /* Las normales solo rotan con la camara; no se trasladan como vertices. */
    *x = normal.x;
    *y = normal.y;
    *z = normal.z;

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

int getObjNormalCamera(struct Model *m, struct Triangle *tri,
                       float *nx, float *ny, float *nz) {
    struct Vec3 normal;

    /* Usa una normal vn del OBJ cuando la cara la trae. */
    if (tri->normal < 0) {
        return 0;
    }

    if (tri->normal >= m->normalCount) {
        return 0;
    }

    normal.x = ((float)m->normals[tri->normal].x) / NORMAL_SCALE;
    normal.y = ((float)m->normals[tri->normal].y) / NORMAL_SCALE;
    normal.z = ((float)m->normals[tri->normal].z) / NORMAL_SCALE;
    normalToCamera(normal, nx, ny, nz);
    return 1;
}

int getGeometryNormalCamera(struct Triangle *tri, float *nx, float *ny, float *nz) {
    float ux;
    float uy;
    float uz;
    float vx;
    float vy;
    float vz;
    float length;

    /* Calcula la normal desde el orden de vertices cuando el OBJ no trae vn. */
    ux = cameraX[tri->b] - cameraX[tri->a];
    uy = cameraY[tri->b] - cameraY[tri->a];
    uz = cameraZ[tri->b] - cameraZ[tri->a];
    vx = cameraX[tri->c] - cameraX[tri->a];
    vy = cameraY[tri->c] - cameraY[tri->a];
    vz = cameraZ[tri->c] - cameraZ[tri->a];

    *nx = (uy * vz) - (uz * vy);
    *ny = (uz * vx) - (ux * vz);
    *nz = (ux * vy) - (uy * vx);

    length = sqrt((*nx * *nx) + (*ny * *ny) + (*nz * *nz));
    if (length < 0.001) {
        return 0;
    }

    *nx = *nx / length;
    *ny = *ny / length;
    *nz = *nz / length;
    return 1;
}

int getTriangleNormalCamera(struct Model *m, struct Triangle *tri,
                            float *nx, float *ny, float *nz) {
    /* Prioriza la normal vn del OBJ; si no existe, usa la geometria. */
    if (getObjNormalCamera(m, tri, nx, ny, nz)) {
        return 1;
    }

    return getGeometryNormalCamera(tri, nx, ny, nz);
}

int triangleFacesCamera(struct Model *m, struct Triangle *tri) {
    float nx;
    float ny;
    float nz;

    /*
       En esta camara se mira hacia +Z; una cara visible tiene normal con Z
       negativa porque apunta de la superficie hacia la camara.
    */
    if (!getTriangleNormalCamera(m, tri, &nx, &ny, &nz)) {
        return 0;
    }

    return nz < -0.01;
}

int grayColorFromTriangle(struct Model *m, struct Triangle *tri) {
    float nx;
    float ny;
    float nz;
    float dot;
    float intensity;
    int color;

    /* Calcula la iluminacion usando normal OBJ o normal geometrica. */
    if (!getTriangleNormalCamera(m, tri, &nx, &ny, &nz)) {
        return DARKGRAY;
    }

    /*
       La fuente de luz esta fija respecto a la camara: arriba, izquierda y al
       frente. Las caras que miran hacia esa direccion quedan mas claras.
    */
    dot = (nx * LIGHT_X) + (ny * LIGHT_Y) + (nz * LIGHT_Z);
    if (dot < 0.0) {
        dot = 0.0;
    }

    intensity = AMBIENT_LIGHT + (DIFFUSE_LIGHT * dot);
    intensity = clampFloat(intensity, 0.0, 1.0);

    /*
       Se usan solo colores BGI garantizados como grises. No dependemos de que
       DOSBox/Turbo C acepte una paleta VGA de 16 grises.
    */
    if (intensity < 0.34) {
        color = DARKGRAY;
    } else if (intensity < 0.72) {
        color = LIGHTGRAY;
    } else {
        color = WHITE;
    }

    return color;
}

int triangleOutsideScreen(int x0, int y0, int x1, int y1, int x2, int y2) {
    int minX;
    int maxX;
    int minY;
    int maxY;

    /*
       Descarta solo triangulos completamente fuera de la vista.
       Se usa caja envolvente para conservar triangulos grandes que cruzan
       la pantalla aunque sus vertices queden fuera del area visible.
    */
    minX = x0;
    if (x1 < minX) minX = x1;
    if (x2 < minX) minX = x2;

    maxX = x0;
    if (x1 > maxX) maxX = x1;
    if (x2 > maxX) maxX = x2;

    minY = y0;
    if (y1 < minY) minY = y1;
    if (y2 < minY) minY = y2;

    maxY = y0;
    if (y1 > maxY) maxY = y1;
    if (y2 > maxY) maxY = y2;

    if (maxX < 0) return 1;
    if (minX > screenW) return 1;
    if (maxY < 0) return 1;
    if (minY > screenH) return 1;

    return 0;
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
        nearCulledCount++;
        return;
    }

    if (triangleOutsideScreen(projectedX[tri->a], projectedY[tri->a],
                              projectedX[tri->b], projectedY[tri->b],
                              projectedX[tri->c], projectedY[tri->c])) {
        offscreenCulledCount++;
        return;
    }

    if (!triangleFacesCamera(m, tri)) {
        backfaceCulledCount++;
        if (cullBackfaces) {
            return;
        }
    }

    item = &renderList[renderCount];
    item->triangleIndex = triangleIndex;
    /*
       Para el orden painter usamos el vertice mas cercano del triangulo.
       Esto reduce pisadas visibles en esquinas frente/lateral frente al promedio.
    */
    item->depth = cameraZ[tri->a];
    if (cameraZ[tri->b] < item->depth) item->depth = cameraZ[tri->b];
    if (cameraZ[tri->c] < item->depth) item->depth = cameraZ[tri->c];
    item->color = grayColorFromTriangle(m, tri);
    renderCount++;
}

void buildRenderList(struct Model *m) {
    int i;

    /* Genera solo los triangulos proyectados y orientados hacia la camara. */
    renderCount = 0;
    backfaceCulledCount = 0;
    offscreenCulledCount = 0;
    nearCulledCount = 0;
    for (i = 0; i < m->triangleCount; i++) {
        addRenderTriangle(m, i);
    }
}

void sortRenderList(void) {
    int i;
    int j;
    struct RenderTriangle temp;

    /* Orden painter: primero los mas lejos, al final los mas cercanos. */
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

void drawFilledTriangle(struct Model *m, struct RenderTriangle *item) {
    struct Triangle *tri;
    int poly[6];

    /* Pinta el triangulo usando coordenadas proyectadas ya calculadas. */
    tri = &m->triangles[item->triangleIndex];
    poly[0] = projectedX[tri->a];
    poly[1] = projectedY[tri->a];
    poly[2] = projectedX[tri->b];
    poly[3] = projectedY[tri->b];
    poly[4] = projectedX[tri->c];
    poly[5] = projectedY[tri->c];

    setfillstyle(SOLID_FILL, item->color);
    fillpoly(3, poly);

    if (showEdges) {
        setcolor(BLACK);
        line(poly[0], poly[1], poly[2], poly[3]);
        line(poly[2], poly[3], poly[4], poly[5]);
        line(poly[4], poly[5], poly[0], poly[1]);
    }
}

void drawExplicitEdges(struct Model *m) {
    int i;
    int a;
    int b;

    /* Si el OBJ trae l, se dibujan encima de las caras coloreadas. */
    if (!showEdges || m->edgeCount <= 0) {
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
        drawFilledTriangle(m, &renderList[i]);
    }

    drawExplicitEdges(m);
}

void drawHud(void) {
    char line1[120];
    char line2[120];
    char line3[120];
    char line4[120];
    char edgeText[4];
    char cullText[4];

    /* El HUD muestra posicion y orientacion actual de la camara. */
    setfillstyle(SOLID_FILL, BLACK);
    bar(0, 0, screenW, 72);

    strcpy(edgeText, showEdges ? "ON" : "OFF");
    strcpy(cullText, cullBackfaces ? "ON" : "OFF");
    sprintf(line1, "OBJGRAY  cam X:%6.1f Y:%6.1f Z:%6.1f",
            camera.x, camera.y, camera.z);
    sprintf(line2, "yaw:%5.2f pitch:%5.2f roll:%5.2f  visibles:%d/%d",
            camera.yaw, camera.pitch, camera.roll,
            renderCount, model.triangleCount);
    sprintf(line3, "desc atras:%d fuera:%d cerca:%d  VN:%d bordes:%s cull:%s",
            backfaceCulledCount, offscreenCulledCount, nearCulledCount,
            model.normalCount, edgeText, cullText);
    sprintf(line4, "W/S A/D Q/E mover  Flechas rotan  Z/X roll  L bordes  B atras  R reset");

    setcolor(LIGHTGRAY);
    outtextxy(8, 8, line1);
    outtextxy(8, 22, line2);
    outtextxy(8, 36, line3);
    outtextxy(8, 50, line4);
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
        } else if (key == 'l' || key == 'L') {
            /* Permite comparar caras limpias contra triangulacion con bordes. */
            showEdges = !showEdges;
            changed = 1;
        } else if (key == 'b' || key == 'B') {
            /* Permite comparar el render con y sin descarte de caras traseras. */
            cullBackfaces = !cullBackfaces;
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
