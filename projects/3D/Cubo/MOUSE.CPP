#include <graphics.h>
#include <math.h>
#include <conio.h>
#include <dos.h>

#define SCREEN_CENTER_X 320
#define SCREEN_CENTER_Y 175

union REGS in, out;

int mouseX, mouseY;
int visualPage = 0;
int activePage = 1;

// Aristas del cubo (pares de vértices conectados)
int edges[12][2] = {
    {0,1},{1,2},{2,3},{3,0},
    {4,5},{5,6},{6,7},{7,4},
    {0,4},{1,5},{2,6},{3,7}
};

// Rotación en plano (usado para rotar en X o Y)
void rotate(float *a, float *b, float angle) {
    float temp = *a;
    *a = *a * cos(angle) - *b * sin(angle);
    *b = temp * sin(angle) + *b * cos(angle);
}

// Proyección 3D a 2D (simplificada)
void project(float x, float y, float z, int *sx, int *sy) {
    float dz = z + 300.0; // Asegura que nunca sea < 1
    if (dz < 1.0) dz = 1.0;
    float scale = 100.0 / dz;

    *sx = SCREEN_CENTER_X + (int)(x * scale);
    *sy = SCREEN_CENTER_Y - (int)(y * scale);
}

void showMouse() {
    in.x.ax = 1;          // Función 1: mostrar cursor
    int86(0x33, &in, &out);
}

void getMousePos() {
    in.x.ax = 3;          // Función 3: obtener posición y estado de botones
    int86(0x33, &in, &out);
    
    mouseX = out.x.cx;    // Posición X del mouse
    mouseY = out.x.dx;    // Posición Y del mouse
}

void initDoubleBufferPages() {
    // Usa una pagina oculta para dibujar el cubo y cambia a ella al terminar.
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
    // Muestra el frame completo sin que se vea el redibujado parcial.
    setvisualpage(activePage);
    visualPage = activePage;
    activePage = 1 - activePage;
}

int main() {
    int gd = VGA, gm = VGAMED;
    clrscr();
    initgraph(&gd, &gm, "BGI");
    initDoubleBufferPages();

    // Inicializar mouse
    in.x.ax = 0;
    int86(0x33, &in, &out);

    // Posicionar puntero del mouse
    in.x.ax = 4;               // Función 4: mover puntero del mouse
    in.x.cx = 320;               // Coordenada X
    in.x.dx = SCREEN_CENTER_Y;    // Coordenada Y
    int86(0x33, &in, &out);
    showMouse();

    // Posicion de cada vértice del cubo
    float cube[8][3] = {
        {-100,-100,-100},{100,-100,-100},{100,100,-100},{-100,100,-100},
        {-100,-100,100},{100,-100,100},{100,100,100},{-100,100,100}
    };

    float angle = 0;
    int i, frame;
    int projected[8][2];

    for (frame = 0; frame < 500; frame++) {

        getMousePos();
        setactivepage(activePage);
        setwritemode(COPY_PUT);
        cleardevice();

        // Rotar el cubo
        for (i = 0; i < 8; i++) {
            float x = cube[i][0];
            float y = cube[i][1];
            float z = cube[i][2];

            // Rotar en eje Y y X
            //rotate(&z, &x, angle * 1);   // Eje Z
            //rotate(&y, &z, angle * 1);   // Eje X
            //rotate(&y, &x, angle * 1); // Eje Y

            rotate(&z, &x, -mouseX * 0.005);   // Eje Z
            rotate(&y, &z, -mouseY * 0.005);   // Eje X

            project(x, y, z, &projected[i][0], &projected[i][1]);
        }

        // Dibujar aristas
        setcolor(WHITE);
        for (i = 0; i < 12; i++) { //12 aristas
            int a = edges[i][0];
            int b = edges[i][1];
            line(projected[a][0], projected[a][1],
                 projected[b][0], projected[b][1]);
        }

        flipDoubleBufferPage();
        delay(34);
        angle += 0.05;
        //angle += 1;
    }

    setactivepage(0);
    setvisualpage(0);
    cleardevice();
    setcolor(RED);
    outtextxy(10, 10, "Fin del giro. Presiona una tecla...");
    //getch();
    closegraph();
    return 0;
}
