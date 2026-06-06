#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <conio.h>

#define OBJ_PATH "C:\\projects\\3D\\OBJ_TEST\\models\\model2.obj"
#define MAX_LINE 180
#define MAX_ITEMS_PER_LINE 32

struct ObjStats {
    int vertices;
    int faces;
    int lines;
    int ignored;
    int warnings;
    float minX;
    float maxX;
    float minY;
    float maxY;
    float minZ;
    float maxZ;
};

void waitExit(void) {
    /* Mantiene visible el resultado cuando el programa se ejecuta desde DOSBox. */
    printf("\nPresiona una tecla para salir...");
    getch();
}

void initStats(struct ObjStats *stats) {
    /* Los limites se inicializan cuando se lee el primer vertice real. */
    stats->vertices = 0;
    stats->faces = 0;
    stats->lines = 0;
    stats->ignored = 0;
    stats->warnings = 0;
    stats->minX = 0.0;
    stats->maxX = 0.0;
    stats->minY = 0.0;
    stats->maxY = 0.0;
    stats->minZ = 0.0;
    stats->maxZ = 0.0;
}

void stripComment(char *line) {
    int i;

    /* OBJ permite comentarios con #; para esta prueba tambien se aceptan al final. */
    for (i = 0; line[i] != '\0'; i++) {
        if (line[i] == '#') {
            line[i] = '\0';
            return;
        }
    }
}

char *trimStart(char *text) {
    /* Avanza hasta el primer caracter que no sea espacio. */
    while (*text != '\0' && isspace((unsigned char)*text)) {
        text++;
    }

    return text;
}

void trimEnd(char *text) {
    int len;

    /* Elimina espacios finales para que las lineas vacias queden claras. */
    len = strlen(text);
    while (len > 0 && isspace((unsigned char)text[len - 1])) {
        text[len - 1] = '\0';
        len--;
    }
}

int isDigitText(char c) {
    /* Evita depender de locales: los indices OBJ son digitos ASCII. */
    return c >= '0' && c <= '9';
}

int skipOptionalObjIndex(char *token, int *pos) {
    int hasDigits;

    /* Valida una parte opcional de v/vt/vn sin guardar ese valor. */
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

    /* Soporta indices tipo 1, 1/2, 1/2/3 y 1//3. */
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

int indexInRange(int index, int vertexCount) {
    /* En OBJ el indice 0 no existe; negativos son relativos a vertices ya leidos. */
    if (index == 0 || vertexCount <= 0) {
        return 0;
    }

    if (index > 0 && index > vertexCount) {
        return 0;
    }

    if (index < 0 && -index > vertexCount) {
        return 0;
    }

    return 1;
}

int parseIndexList(char *args, int minItems, int vertexCount, int lineNo, char *label) {
    char *token;
    int index;
    int count;

    /* Lee listas de indices usadas por caras f y lineas l. */
    if (args == NULL) {
        printf("ERROR linea %d: %s sin indices.\n", lineNo, label);
        return 0;
    }

    count = 0;
    token = strtok(args, " \t\r\n");
    while (token != NULL) {
        if (count >= MAX_ITEMS_PER_LINE) {
            printf("ERROR linea %d: demasiados indices en %s.\n", lineNo, label);
            return 0;
        }

        if (!parseObjIndex(token, &index)) {
            printf("ERROR linea %d: indice OBJ invalido: %s\n", lineNo, token);
            return 0;
        }

        if (!indexInRange(index, vertexCount)) {
            printf("ERROR linea %d: indice fuera de rango: %s\n", lineNo, token);
            return 0;
        }

        count++;
        token = strtok(NULL, " \t\r\n");
    }

    if (count < minItems) {
        printf("ERROR linea %d: %s necesita al menos %d indices.\n",
               lineNo, label, minItems);
        return 0;
    }

    return 1;
}

int parseVertex(char *args, int lineNo, struct ObjStats *stats) {
    float x;
    float y;
    float z;
    float w;
    char extra[16];
    int readCount;

    /* v requiere X Y Z; tambien se acepta W opcional como permite OBJ. */
    if (args == NULL) {
        printf("ERROR linea %d: vertice sin coordenadas.\n", lineNo);
        return 0;
    }

    extra[0] = '\0';
    readCount = sscanf(args, " %f %f %f %f %15s", &x, &y, &z, &w, extra);
    if (readCount < 3 || readCount > 4) {
        printf("ERROR linea %d: vertice invalido, se esperaba v x y z.\n", lineNo);
        return 0;
    }

    if (stats->vertices == 0) {
        stats->minX = x;
        stats->maxX = x;
        stats->minY = y;
        stats->maxY = y;
        stats->minZ = z;
        stats->maxZ = z;
    } else {
        if (x < stats->minX) stats->minX = x;
        if (x > stats->maxX) stats->maxX = x;
        if (y < stats->minY) stats->minY = y;
        if (y > stats->maxY) stats->maxY = y;
        if (z < stats->minZ) stats->minZ = z;
        if (z > stats->maxZ) stats->maxZ = z;
    }

    stats->vertices++;
    return 1;
}

int parseFloatLine(char *args, int minValues, int maxValues, int lineNo, char *label) {
    float a;
    float b;
    float c;
    float d;
    char extra[16];
    int readCount;

    /* Valida lineas numericas conocidas que esta prueba todavia no usa para dibujar. */
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

int parseObjLine(char *line, int lineNo, struct ObjStats *stats) {
    char *clean;
    char *keyword;
    char *args;

    /* Se limpia la linea y luego se decide por palabra clave OBJ. */
    stripComment(line);
    clean = trimStart(line);
    trimEnd(clean);

    if (clean[0] == '\0') {
        return 1;
    }

    keyword = strtok(clean, " \t\r\n");
    args = strtok(NULL, "\n");

    if (strcmp(keyword, "v") == 0) {
        return parseVertex(args, lineNo, stats);
    }

    if (strcmp(keyword, "f") == 0) {
        if (!parseIndexList(args, 3, stats->vertices, lineNo, "cara f")) {
            return 0;
        }
        stats->faces++;
        return 1;
    }

    if (strcmp(keyword, "l") == 0) {
        if (!parseIndexList(args, 2, stats->vertices, lineNo, "linea l")) {
            return 0;
        }
        stats->lines++;
        return 1;
    }

    if (strcmp(keyword, "vn") == 0) {
        stats->ignored++;
        return parseFloatLine(args, 3, 3, lineNo, "normal vn");
    }

    if (strcmp(keyword, "vt") == 0) {
        stats->ignored++;
        return parseFloatLine(args, 1, 3, lineNo, "textura vt");
    }

    if (strcmp(keyword, "o") == 0 ||
        strcmp(keyword, "g") == 0 ||
        strcmp(keyword, "s") == 0 ||
        strcmp(keyword, "usemtl") == 0 ||
        strcmp(keyword, "mtllib") == 0) {
        /* Se reconoce la palabra clave, pero no se usa para geometria. */
        stats->ignored++;
        return 1;
    }

    printf("AVISO linea %d: palabra OBJ no interpretada: %s\n", lineNo, keyword);
    stats->warnings++;
    return 1;
}

int main(void) {
    FILE *file;
    char line[MAX_LINE];
    int lineNo;
    struct ObjStats stats;

    initStats(&stats);
    file = fopen(OBJ_PATH, "r");
    if (file == NULL) {
        printf("ERROR: no se pudo abrir el archivo OBJ:\n%s\n", OBJ_PATH);
        waitExit();
        return 1;
    }

    lineNo = 0;
    while (fgets(line, MAX_LINE, file) != NULL) {
        lineNo++;
        if (!parseObjLine(line, lineNo, &stats)) {
            fclose(file);
            waitExit();
            return 1;
        }
    }

    fclose(file);

    if (stats.vertices < 1) {
        printf("ERROR: el OBJ no contiene vertices v.\n");
        waitExit();
        return 1;
    }

    if (stats.faces < 1 && stats.lines < 1) {
        printf("ERROR: el OBJ no contiene caras f ni lineas l.\n");
        waitExit();
        return 1;
    }

    printf("OK: sintaxis OBJ basica reconocida.\n");
    printf("Archivo: %s\n", OBJ_PATH);
    printf("Lineas leidas: %d\n", lineNo);
    printf("Vertices v: %d\n", stats.vertices);
    printf("Caras f: %d\n", stats.faces);
    printf("Lineas l: %d\n", stats.lines);
    printf("Lineas reconocidas pero ignoradas: %d\n", stats.ignored);
    printf("Avisos: %d\n", stats.warnings);
    printf("Limites X: %.2f a %.2f\n", stats.minX, stats.maxX);
    printf("Limites Y: %.2f a %.2f\n", stats.minY, stats.maxY);
    printf("Limites Z: %.2f a %.2f\n", stats.minZ, stats.maxZ);

    waitExit();
    return 0;
}
