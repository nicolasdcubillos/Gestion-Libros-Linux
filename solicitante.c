#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <signal.h>
#include "libproyecto.h"

/*
                      ANGELLO MATEO JAIMES RINCÓN
                    NICOLÁS DAVID CUBILLOS CUBILLOS
                     ANGELA MARÍA OSPINA ARELLANO
                      PROYECTO | SEGUNDA ENTREGA
                         SISTEMAS OPERATIVOS
                         NOVIEMBRE 9 DE 2021

                     BIBLIOTECA - PRESTAMO DE LIBROS
                    PONTIFICIA UNIVERSIDAD JAVERIANA
                      PROFESORA: MARIELA CURIEL H.
*/

/*
    (int) Función que imprime el menú y retorna la opción seleccionada.
*/

int menu () {

    int opcion;

    printf("\n---- [1] Devolver un libro.  \n");
    printf("---- [2] Renovar el prestamo. \n");
    printf("---- [3] Pedir un libro. \n");
    printf("---- [0] Salir. \n");
    printf("---- Opcion: ");
    scanf("%d", &opcion);

    return opcion;

}

/*
    (void) Envía una solicitud al proceso receptor por medio del pipe nominal enviado por parámetro 'pipeReceptor'.
    Recibe también, una solicitud, que es la que escribirá en este pipe.

*/

void enviarSolicitud (Solicitud solicitud, char* pipeReceptor) {

    int fd, creado = 0, respuesta, bytes, fdRespuesta;

    mode_t fifo_mode = S_IRUSR | S_IWUSR;

    unlink(solicitud.pipeRespuesta);

    if (mkfifo (solicitud.pipeRespuesta, fifo_mode) == -1) {
        perror("Error al crear el pipe de respuesta: ");
        exit(1);
    }

    printf("-> Enviando solicitud... Espere un momento.\n");

    do {
        fd = open (pipeReceptor, O_WRONLY | O_NONBLOCK);
        if (fd == -1) {
            perror("Error en la apertura del pipe receptor: ");
            sleep(1);
        } else {
            int status = write (fd, &solicitud, sizeof(Solicitud));
            if (status > 0) {
                creado = 1;
            }
        }
    } while (creado == 0);

    fdRespuesta = open (solicitud.pipeRespuesta, O_RDONLY);

    do {
        sleep(1);
        
        bytes = read (fdRespuesta, &respuesta, sizeof(int));

        if (bytes == -1) {
            perror("Error en la lectura del pipe de respuesta:");
            exit(1);
        }

        if (respuesta) {
            printf("Su solicitud fue procesada con éxito.\n");
        } else {
            printf("Su solicitud no pudo ser procesada correctamente.\n");
        }

        unlink(solicitud.pipeRespuesta);
        break;
    } while (bytes);

    close (fd);

}

/*
    (void) En caso de que ocurra un error al momento de leer los argumentos, se llamará esta función que imprimirá
    el correcto uso del 'argv' y los parámetros esperados.
*/

void errorArgv() {

    printf("---- [!] Error: Argumentos invalidos.\n");
    printf("[!] Ej: ./solicitante [-i <archivoEntrada>] -p <pipeReceptor> \n\n");
    printf("[!] (opcional) -i: flag que indica que a continuación está el nombre del archivo de entrada.\n");
    printf("[!] (opcional) archivoEntrada: nombre del archivo de entrada de solicitudes.\n");
    printf("[!] -p: flag que indica que a continuación está el nombre del pipe nominal.\n");
    printf("[!] pipeNominal: nombre del pipeNominal.\n");
    exit(EXIT_FAILURE);

}

/*
    (void) En caso de que el proceso necesita leer las solicitudes mediante un archivo, se llama esta función, y es 
    la encargada de leer todas las solicitudes que allí se encuentren (en archivoEntrada) y posteriormente
    despacharlas haciendo uso de la función 'enviarSolicitud'.
*/

void leerArchivoSol (char* archivoEntrada, char* pipeReceptor) {

    char linea [MAX_LINE_LENGTH];
    char* token = NULL;
    
    FILE *fileSol;

    Solicitud solicitud;
    sprintf(solicitud.pipeRespuesta, "%d", getpid());

    fileSol = fopen(archivoEntrada, "r");

    if (fileSol == NULL) {
        perror("No se pudo abrir el archivo.\n");
        exit(EXIT_FAILURE);
    }

    while (!feof(fileSol)) {
        fgets(linea, sizeof(linea), fileSol);

        if (strstr(linea, ",") == NULL){
            printf("[!] Se encontró una solicitud inválida.\n");
            continue;
        }
        token = strtok(linea, ",");
        if (token == NULL){
            printf("[!] Se encontró una solicitud inválida.\n");
            continue;
        }
        solicitud.operacion = *token;

        token = strtok(NULL, ",");
        if (token == NULL){
            printf("[!] Se encontró una solicitud inválida.\n");
            continue;
        }
        strcpy(solicitud.nombreLibro, token);

        token = strtok(NULL, ",");
        if (token == NULL){
            printf("[!] Se encontró una solicitud inválida.\n");
            continue;
        }
        solicitud.ISBN = atoi(token);

        enviarSolicitud(solicitud, pipeReceptor); 
    }

    fclose(fileSol);
}


int main (int argc, char** argv) {

    if (argc != 5 && argc != 3) {
        errorArgv();
    }

    int i = 0, opcion;
    char pipeReceptor [MAX_LINE_LENGTH] = {0}, archivoEntrada [MAX_LINE_LENGTH] = {0};
    char nombreLibro [MAX_LINE_LENGTH] = {0}, *linea;
    linea = (char*) malloc (MAX_LINE_LENGTH);

    if (linea == NULL) {
        perror("No se pudo asignar memoria a la variable linea");
        exit(EXIT_FAILURE);
    }
    
    Solicitud solicitud;
    sprintf(solicitud.pipeRespuesta, "%d", getpid());
    
    if (argc == 5) {
        for (i = 1; i < argc; i += 2) {
            if (!strcmp(argv[i], "-i")) {
                strcpy(archivoEntrada, argv[i+1]);
            } else if (!strcmp(argv[i], "-p")) {
                strcpy(pipeReceptor, argv[i+1]);
            } else {
                errorArgv();
            }
        }

        if (!(int)strlen(pipeReceptor) || !(int)strlen(archivoEntrada)) { // Dos flag iguales.
            errorArgv();
        }

        leerArchivoSol (archivoEntrada, pipeReceptor);
    
    } else {
        if (!strcmp(argv[1], "-p")) {
            strcpy(pipeReceptor, argv[2]);
        } else {
            errorArgv();
        }

        do {
            opcion = menu();
            switch (opcion) {
                case 1: {
                    solicitud.operacion = 'D';
                    printf("------ Devolucion de libros ------ \n");
                    break;
                }
                case 2: {
                    solicitud.operacion = 'R';
                    printf("------ Renovacion de libros ------ \n");
                    break;
                }
                case 3: {
                    solicitud.operacion = 'P';
                    printf("------ Prestamo de libros ------ \n");
                    break;
                }
                case 0: {
                    break;
                }
                default: {
                    printf("[!] Opcion incorrecta. Ingrese '0' para salir.\n");
                }
            }

            if (opcion != 0) {
                printf("  Nombre: ");
                scanf(" %[^\n]s", solicitud.nombreLibro);
                printf("  ISBN: ");
                
                scanf("%d", &solicitud.ISBN);

                enviarSolicitud(solicitud, pipeReceptor);
            }

        } while (opcion != 0);
    }

    unlink(solicitud.pipeRespuesta);
    
    return EXIT_SUCCESS;
}