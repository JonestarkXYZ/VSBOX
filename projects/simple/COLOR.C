#include "graphics.h"
#include "conio.h"
#include "dos.h"

int main() {
    int gd = DETECT, gm;
    clrscr();
    
    // Inicializa el modo gráfico
    initgraph(&gd, &gm, "BGI");

    // Dibuja un cuadrado rojo
    setcolor(RED);
    rectangle(100, 100, 200, 200);
    setfillstyle(SOLID_FILL, RED);
    floodfill(150, 150, RED);

    // Dibuja un cuadrado azul
    setcolor(BLUE);
    rectangle(250, 100, 350, 200);
    setfillstyle(SOLID_FILL, BLUE);
    floodfill(300, 150, BLUE);

    // Dibuja un círculo verde
    setcolor(GREEN);
    circle(450, 150, 50);
    setfillstyle(SOLID_FILL, GREEN);
    floodfill(450, 150, GREEN);

    // Espera una tecla antes de cerrar
    getch();
    
    // Cierra el modo gráfico
    closegraph();

    return 0;
}
