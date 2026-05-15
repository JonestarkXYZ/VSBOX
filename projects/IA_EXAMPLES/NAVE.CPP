#include <graphics.h>
#include <conio.h>
#include <dos.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define SCREEN_W 640
#define SCREEN_H 480

#define MAX_VERTICES 80
#define MAX_EDGES 120
#define MAX_BULLETS 16
#define MAX_STARS 70

#define CAMERA_DISTANCE 390.0
#define PROJECTION_ZOOM 250.0
#define CAMERA_X_ANGLE 0.70
#define MANUAL_BANK 0.55
#define AUTOPILOT_BANK 0.42
#define BANK_SMOOTH 0.24
#define FIRE_COOLDOWN 9
#define MAX_DEMO_FRAMES 2200

#define BULLET_SCALE_DECAY 0.03
#define BULLET_ANGLE_DECAY 0.3

#define KEY_ESC 27
#define KEY_SPACE 32
#define KEY_UP 72
#define KEY_DOWN 80
#define KEY_LEFT 75
#define KEY_RIGHT 77

struct Vec3 {
    float x;
    float y;
    float z;
};

struct Model {
    struct Vec3 vertices[MAX_VERTICES];
    int edges[MAX_EDGES][2];
    int vertexCount;
    int edgeCount;
    char loadedPath[90];
};

struct Bullet {
    int active;
    float x;
    float y;
    float speed;
    float angle;
    float scale;
};

struct Star {
    int x;
    int y;
    int speed;
};

struct Bullet bullets[MAX_BULLETS];
struct Star stars[MAX_STARS];
int projectedX[MAX_VERTICES];
int projectedY[MAX_VERTICES];

void clearModel(struct Model *model) {
    // Reinicia el modelo para evitar usar datos viejos si la carga falla.
    model->vertexCount = 0;
    model->edgeCount = 0;
    model->loadedPath[0] = '\0';
}

int readVertices(FILE *file, struct Model *model, int count) {
    int i;
    float x;
    float y;
    float z;

    // Limita el tamano para que el arreglo fijo sea seguro en Turbo C++.
    if (count < 1 || count > MAX_VERTICES) {
        return 0;
    }

    model->vertexCount = count;
    for (i = 0; i < count; i++) {
        if (fscanf(file, "%f %f %f", &x, &y, &z) != 3) {
            return 0;
        }
        model->vertices[i].x = x;
        model->vertices[i].y = y;
        model->vertices[i].z = z;
    }

    return 1;
}

int readEdges(FILE *file, struct Model *model, int count) {
    int i;
    int a;
    int b;

    // Las aristas conectan vertices por indice, igual que en projects/Show3D.
    if (count < 1 || count > MAX_EDGES) {
        return 0;
    }

    model->edgeCount = count;
    for (i = 0; i < count; i++) {
        if (fscanf(file, "%d %d", &a, &b) != 2) {
            return 0;
        }
        if (a < 0 || b < 0 || a >= model->vertexCount || b >= model->vertexCount) {
            return 0;
        }
        model->edges[i][0] = a;
        model->edges[i][1] = b;
    }

    return 1;
}

int tryLoadModel(struct Model *model, char *path) {
    FILE *file;
    char word[24];
    int count;
    int ok;

    file = fopen(path, "r");
    if (!file) {
        return 0;
    }

    clearModel(model);
    ok = 1;

    while (ok && fscanf(file, "%23s", word) == 1) {
        if (strcmp(word, "VERTICES") == 0) {
            if (fscanf(file, "%d", &count) != 1) {
                ok = 0;
            } else {
                ok = readVertices(file, model, count);
            }
        } else if (strcmp(word, "ARISTAS") == 0) {
            if (fscanf(file, "%d", &count) != 1) {
                ok = 0;
            } else {
                ok = readEdges(file, model, count);
            }
        }
    }

    fclose(file);

    if (!ok || model->vertexCount < 2 || model->edgeCount < 1) {
        clearModel(model);
        return 0;
    }

    strcpy(model->loadedPath, path);
    return 1;
}

int loadModelByName(struct Model *model, char *fileName) {
    int i;
    char paths[3][90];

    // Construye rutas DOS compatibles para cargar cualquier modelo del ejemplo.
    strcpy(paths[0], "MODELS\\");
    strcat(paths[0], fileName);

    strcpy(paths[1], "models\\");
    strcat(paths[1], fileName);

    strcpy(paths[2], "C:\\PROJECTS\\IA_EXA~1\\MODELS\\");
    strcat(paths[2], fileName);

    for (i = 0; i < 3; i++) {
        if (tryLoadModel(model, paths[i])) {
            return 1;
        }
    }

    return 0;
}

int loadShipModel(struct Model *model) {
    // Modelo principal de la nave low-poly.
    return loadModelByName(model, "NAVE.TXT");
}

int loadBulletModel(struct Model *model) {
    // Modelo pequeno usado para cada disparo 3D.
    return loadModelByName(model, "DISPARO.TXT");
}

void rotatePair(float *a, float *b, float angle) {
    float oldA;
    float oldB;
    float cosA;
    float sinA;

    // Rota dos componentes de un vector para construir rotaciones 3D simples.
    oldA = *a;
    oldB = *b;
    cosA = cos(angle);
    sinA = sin(angle);

    *a = oldA * cosA - oldB * sinA;
    *b = oldA * sinA + oldB * cosA;
}

void projectVertex(struct Vec3 source, float yaw, float pitch, float roll,
                   float scale, int centerX, int centerY, int *screenX, int *screenY) {
    float x;
    float y;
    float z;
    float cameraY;
    float depth;
    float factor;
    float sinView;
    float cosView;

    // Copia local para no modificar los vertices cargados desde el archivo.
    x = source.x;
    y = source.y;
    z = source.z;

    // Sistema del modelo: X=lateral, Y=frente/atras, Z=altura.
    rotatePair(&x, &y, yaw);
    rotatePair(&y, &z, pitch);
    rotatePair(&x, &z, roll);

    // Camara trasera levemente elevada: mezcla Y y Z como vista oblicua.
    sinView = sin(CAMERA_X_ANGLE);
    cosView = cos(CAMERA_X_ANGLE);
    cameraY = (y * sinView) + (z * cosView);
    depth = CAMERA_DISTANCE + (y * cosView) - (z * sinView);

    if (depth < 35.0) {
        depth = 35.0;
    }

    factor = (PROJECTION_ZOOM / depth) * scale;
    *screenX = centerX + (int)(x * factor);
    *screenY = centerY - (int)(cameraY * factor);
}

void drawModel(struct Model *model, int centerX, int centerY,
               float scale, float yaw, float pitch, float roll, int color) {
    int i;
    int a;
    int b;

    // Proyecta todos los vertices una sola vez por cuadro.
    for (i = 0; i < model->vertexCount; i++) {
        projectVertex(model->vertices[i], yaw, pitch, roll, scale,
                      centerX, centerY, &projectedX[i], &projectedY[i]);
    }

    // Dibuja primero una sombra leve para separar la nave del fondo.
    setcolor(DARKGRAY);
    for (i = 0; i < model->edgeCount; i++) {
        a = model->edges[i][0];
        b = model->edges[i][1];
        line(projectedX[a] + 1, projectedY[a] + 1,
             projectedX[b] + 1, projectedY[b] + 1);
    }

    // Dibujo wireframe principal, basado completamente en aristas del modelo.
    setcolor(color);
    for (i = 0; i < model->edgeCount; i++) {
        a = model->edges[i][0];
        b = model->edges[i][1];
        line(projectedX[a], projectedY[a], projectedX[b], projectedY[b]);
    }
}

void resetStar(int i, int top) {
    // Estrellas 2D simples para dar movimiento sin cargar mas archivos.
    stars[i].x = rand() % SCREEN_W;
    stars[i].y = top ? 0 : rand() % SCREEN_H;
    stars[i].speed = 1 + rand() % 4;
}

void initStars() {
    int i;

    for (i = 0; i < MAX_STARS; i++) {
        resetStar(i, 0);
    }
}

void updateStars() {
    int i;

    for (i = 0; i < MAX_STARS; i++) {
        stars[i].y += stars[i].speed;
        if (stars[i].y >= SCREEN_H) {
            resetStar(i, 1);
        }
    }
}

void drawStars() {
    int i;
    int color;

    for (i = 0; i < MAX_STARS; i++) {
        color = (stars[i].speed > 2) ? LIGHTGRAY : DARKGRAY;
        putpixel(stars[i].x, stars[i].y, color);
        if (stars[i].speed > 3 && stars[i].y > 1) {
            putpixel(stars[i].x, stars[i].y - 1, color);
        }
    }
}

void clearBullets() {
    int i;

    // Todas las balas empiezan desactivadas.
    for (i = 0; i < MAX_BULLETS; i++) {
        bullets[i].active = 0;
        bullets[i].x = 0.0;
        bullets[i].y = 0.0;
        bullets[i].speed = 0.0;
        bullets[i].angle = 0.0;
        bullets[i].scale = 0.0;
    }
}

void fireBullet(float shipX, float shipY, int *cooldown) {
    int i;

    if (*cooldown > 0) {
        return;
    }

    // Reutiliza el primer espacio libre para no reservar memoria dinamica.
    for (i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].active = 1;
            bullets[i].x = shipX;
            bullets[i].y = shipY - 78.0;
            bullets[i].speed = 12.0;
            bullets[i].angle = 0.20;
            bullets[i].scale = 0.95;
            *cooldown = FIRE_COOLDOWN;
            return;
        }
    }
}

void updateBullets() {
    int i;

    for (i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            bullets[i].y -= bullets[i].speed;
            bullets[i].angle += BULLET_ANGLE_DECAY;
            bullets[i].scale -= BULLET_SCALE_DECAY;

            // El proyectil se reduce con la distancia y se elimina al salir.
            if (bullets[i].y < -12.0 || bullets[i].scale <= 0.20) {
                bullets[i].active = 0;
            }
        }
    }
}

void drawBullets(struct Model *bulletModel) {
    int i;

    // Cada bala se dibuja con su propio modelo 3D, no como linea 2D plana.
    for (i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            drawModel(bulletModel, (int)bullets[i].x, (int)bullets[i].y,
                      bullets[i].scale, 0.0, 0.0, bullets[i].angle, YELLOW);
        }
    }
}

void drawEngineGlow(int shipX, int shipY, int frame) {
    int flame;

    // El motor no pertenece al modelo; es un efecto 2D sobre la nave.
    flame = 16 + (frame % 7);
    setcolor(RED);
    line(shipX - 12, shipY + 78, shipX - 4, shipY + 78 + flame);
    line(shipX + 12, shipY + 78, shipX + 4, shipY + 78 + flame);
    setcolor(YELLOW);
    line(shipX, shipY + 82, shipX, shipY + 82 + flame);
}

void clampShip(float *x, float *y) {
    // Evita que la nave salga de la zona visible.
    if (*x < 90.0) {
        *x = 90.0;
    }
    if (*x > SCREEN_W - 90.0) {
        *x = SCREEN_W - 90.0;
    }
    if (*y < 170.0) {
        *y = 170.0;
    }
    if (*y > SCREEN_H - 92.0) {
        *y = SCREEN_H - 92.0;
    }
}

int handleInput(float *shipX, float *shipY, int *autoPilot,
                int *cooldown, int *bankCommand) {
    int key;
    int scan;

    // Lee todas las teclas pendientes para que el movimiento responda mejor.
    *bankCommand = 0;

    while (kbhit()) {
        key = getch();

        if (key == 0 || key == 224) {
            scan = getch();
            *autoPilot = 0;

            if (scan == KEY_LEFT) {
                *shipX -= 14.0;
                *bankCommand = -1;
            } else if (scan == KEY_RIGHT) {
                *shipX += 14.0;
                *bankCommand = 1;
            } else if (scan == KEY_UP) {
                *shipY -= 12.0;
            } else if (scan == KEY_DOWN) {
                *shipY += 12.0;
            }
        } else {
            if (key == KEY_ESC) {
                return 1;
            }

            if (key == KEY_SPACE) {
                fireBullet(*shipX, *shipY, cooldown);
            } else if (key == 'a' || key == 'A') {
                *autoPilot = 0;
                *shipX -= 14.0;
                *bankCommand = -1;
            } else if (key == 'd' || key == 'D') {
                *autoPilot = 0;
                *shipX += 14.0;
                *bankCommand = 1;
            } else if (key == 'w' || key == 'W') {
                *autoPilot = 0;
                *shipY -= 12.0;
            } else if (key == 's' || key == 'S') {
                *autoPilot = 0;
                *shipY += 12.0;
            }
        }
    }

    return 0;
}

void drawHud(int autoPilot) {
    // Texto minimo para probar controles dentro de DOSBox.
    setcolor(LIGHTGRAY);
    outtextxy(8, 8, "NAVE 3D WIREFRAME - FLECHAS/WASD, ESPACIO, ESC");

    if (autoPilot) {
        setcolor(DARKGRAY);
        outtextxy(8, 22, "MODO DEMO ACTIVO");
    }
}

int main() {
    struct Model ship;
    struct Model bulletModel;
    int gd;
    int gm;
    int done;
    int frame;
    int autoPilot;
    int cooldown;
    int bankCommand;
    float shipX;
    float shipY;
    float yaw;
    float pitch;
    float roll;
    float bank;
    float bankTarget;

    clearModel(&ship);
    if (!loadShipModel(&ship)) {
        printf("No se pudo abrir projects\\IA_EXAMPLES\\models\\NAVE.TXT\n");
        printf("Verifica projects\\IA_EXAMPLES\\models\\NAVE.TXT\n");
        printf("Presiona una tecla para salir...");
        getch();
        return 1;
    }

    clearModel(&bulletModel);
    if (!loadBulletModel(&bulletModel)) {
        printf("No se pudo abrir projects\\IA_EXAMPLES\\models\\DISPARO.TXT\n");
        printf("Verifica projects\\IA_EXAMPLES\\models\\DISPARO.TXT\n");
        printf("Presiona una tecla para salir...");
        getch();
        return 1;
    }

    gd = DETECT;
    gm = 0;
    initgraph(&gd, &gm, "BGI");

    srand((unsigned)time(NULL));
    initStars();
    clearBullets();

    done = 0;
    frame = 0;
    autoPilot = 1;
    cooldown = 0;
    bankCommand = 0;
    shipX = SCREEN_W / 2.0;
    shipY = SCREEN_H - 115.0;
    bank = 0.0;
    bankTarget = 0.0;

    while (!done && frame < MAX_DEMO_FRAMES) {
        done = handleInput(&shipX, &shipY, &autoPilot, &cooldown, &bankCommand);

        // Si no hay teclas, la demo se mueve sola para mostrar el modelo.
        if (autoPilot) {
            shipX = SCREEN_W / 2.0 + (float)(sin(frame * 0.035) * 118.0);
            shipY = SCREEN_H - 116.0 + (float)(sin(frame * 0.021) * 18.0);
            // Invierte el ladeo para que el giro lateral coincida visualmente.
            bankTarget = (float)(-sin(frame * 0.035) * AUTOPILOT_BANK);
            if ((frame % 28) == 0) {
                fireBullet(shipX, shipY, &cooldown);
            }
        } else {
            // Izquierda y derecha usan el giro opuesto al signo del movimiento.
            bankTarget = (float)-bankCommand * MANUAL_BANK;
        }

        clampShip(&shipX, &shipY);
        bank += (bankTarget - bank) * BANK_SMOOTH;

        if (cooldown > 0) {
            cooldown--;
        }

        updateStars();
        updateBullets();

        // La camara ya inclina el eje X; estas rotaciones solo dan vida al modelo.
        yaw = (float)(sin(frame * 0.030) * 0.10);
        pitch = (float)(sin(frame * 0.018) * 0.07);
        roll = bank;

        cleardevice();
        drawStars();
        drawHud(autoPilot);
        drawBullets(&bulletModel);
        drawModel(&ship, (int)shipX, (int)shipY, 1.45, yaw, pitch, roll, LIGHTCYAN);
        drawEngineGlow((int)shipX, (int)shipY, frame);

        delay(28);
        frame++;
    }

    closegraph();
    return 0;
}
