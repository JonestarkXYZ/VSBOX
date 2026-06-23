/* LUIS F. RAMIREZ CURIEL
UNAM
FAC. INGENIERIA

PROGRAMA QUE EMULA UNA SE¥AL CUADRADA ENVIADA POR UN MULTIVIBRADOR
ASTABLE USANDO EL PUERTO PARALELO
LA SALIDA USADA ES D0(0x01)
7/SEPTIEMBRE/2005

*/


#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>

int identificar_flecha();

/*-----------------------------PROGRAMA PRINCIPAL---------------------*/

void main ()
{
   unsigned int dato=0x01, tiempo_encendido, tiempo_apagado=1;
   int ciclos, puerto = 0x378;
   int flecha;
   float T;
   char respuesta;
   outport(puerto,0x00);
   clrscr();
   printf("\n\nEste programa emula una se¤al cuadrada enviada por un\n");
   printf("multivibrador astable\n");
   printf("\nEl tiempo de encendido se controla con las flechas del tablero\n");
   printf("\nPara empezar oprimir cualcuier tecla\n");
   printf("\nPara terminar apretar ESC\n");
   getch();
   tiempo_encendido=500;
   while (flecha!=5)
   {
      while (kbhit()==0)
      {
	 T = tiempo_encendido * 0.001;
	 outport (puerto, dato);
	 printf ("\ntiempo=%g [s]", T);
	 delay (tiempo_encendido);
	 outport (puerto, 0x00);
	 delay (tiempo_apagado);
      }
      flecha=identificar_flecha();
      if (flecha ==1)
      {
	 if (tiempo_encendido>= 2500)
	    tiempo_encendido= 2500;
	 else if (tiempo_encendido>= 500 && tiempo_encendido<2500)
	    tiempo_encendido=tiempo_encendido+100;
	 else if (tiempo_encendido>= 50 && tiempo_encendido<500)
	    tiempo_encendido=tiempo_encendido+50;
	 else if (tiempo_encendido>= 10 && tiempo_encendido<50)
	    tiempo_encendido=tiempo_encendido+10;
	 else if (tiempo_encendido<10)
	    tiempo_encendido=tiempo_encendido+1;
      }
      if (flecha == 2)
      {
	 if (tiempo_encendido<=1)
	    tiempo_encendido= 1;
	 else if (tiempo_encendido> 1 && tiempo_encendido<=10)
	    tiempo_encendido=tiempo_encendido-1;
	 else if (tiempo_encendido> 10 && tiempo_encendido<=50)
	    tiempo_encendido=tiempo_encendido-10;
	 else if (tiempo_encendido>50 && tiempo_encendido<=500)
	    tiempo_encendido=tiempo_encendido-50;
	 else if (tiempo_encendido>500)
	    tiempo_encendido=tiempo_encendido-100;
      }
   }
   outport (puerto, 0x00);
}


/*--------------------------FUNCION QUE DETECTA LAS FLECHAS-------------*/
int identificar_flecha()
{
   char ltr[3];
   int i=1, pregunta;
   ltr[0]='a';
   while(i<=2)
   {
      ltr[i]=getch();
      if (ltr[i] == 27)
      {
	 i = 3;
	 pregunta = 5;
      }
      if (ltr[i-1]==0)
      {
	 pregunta=0;
	 if (ltr[i]=='H')
	    pregunta=1;
	 if (ltr[i]=='P')
	    pregunta=2;
	 if (ltr[i]=='K')
	    pregunta=3;
	 if (ltr[i]=='M')
	    pregunta=4;
	 i = 3;
      }
      if (ltr[1]!=0 && ltr[1] != 27)
      {
	 i = 3;
	 pregunta=0;
      }
      if (ltr[i]==0)
	 i++;
   }
   return pregunta;
}