/*
 *	Tarea 2 - Analizador Sintactico	
 *	Curso: Compiladores y Lenguajes de Bajo de Nivel
 *  Alumnos : Jose de Jesus Cantero Maciel
 */ 

#include "analizadorLexico.h"
#include "analizadorSintactico.h"


/************** Funcion principal ****************/
int main(int argc,char* args[])
{
	// inicializar analizador lexico
	int complex=0;

	initTabla();
	initTablaSimbolos();

         if(argc > 1)
	     {
		 if (!(archivo=fopen(args[1],"rt")))
		 //if (!(archivo=fopen("fuente.txt", "r")))
		 {
			printf("Archivo no encontrado.");
			exit(1);
		 }            
		 //else{
            parse();
            if(HayError==FALSE){
               printf("\nEl fuente es sintacticamente correcto\n");                                  
            }
            fclose(archivo);
            system("pause");
           	return 0;
         //}
        }else{
		 printf("Debe pasar como parametro el path al archivo fuente.");
		 exit(1);
	    }
	    system("pause");
}
