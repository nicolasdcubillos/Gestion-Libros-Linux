#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include "libproyecto.h"

#define TAMBUFFER 10


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

sem_t semEntrada, semElementos, semEspacios;

// Entrada: Asegura que sólamente un proceso entre a la vez a modificar el buffer.
// totalElementos: Semáforo que empieza en 0 y que tendrá el total de elementos existentes en el buffer.
// totalEspacios: Semáforo que se inicializa en el tamaño del buffer.

Solicitud BUFFER[TAMBUFFER];
Libro* libros;
int numeroLibros, solicitudProcesando = 0, numeroElementos = 0;

/*
    (Fecha) Función encargada de convertir la fecha actual, desde el time_t, y convertirlo
    a la estructura 'Fecha' que se creó con este fin.
*/

Fecha fechaActual () {

    char cadena [128] = {0}, *token;
    Fecha fecha;

    time_t tiempo = time (NULL);
    struct tm *tlocal = localtime(&tiempo);
    
    strftime(cadena, 15, "%d-%m-%Y", tlocal);
    token = strtok(cadena, "-");
    fecha.dia = atoi(token);

    token = strtok(NULL, "-");
    fecha.mes = atoi(token);

    token = strtok(NULL, "-");
    fecha.ano = atoi(token);

    return fecha;

}

/*
    (int) Esta función retorna el número de días que tiene un determinado mes que recibe por parámetro
    como un número entero (int mes)
*/

int diamax (int mes) {

    int meses30 [4]; // Meses que tienen 30 dias
    int i;

    meses30[0] = 4;
    meses30[1] = 6;
    meses30[2] = 9;
    meses30[3] = 11;

    if (mes == 2)
        return 28;

    for (i = 0; i < 4; i++) {
        if (meses30[i] == mes) {
            return 30;
        }
    }

    return 31;

}

/*
    (Fecha) Aumenta una semana a la fecha que recibe por parámetro y al final retorna esta misma. Esta función
    se encarga de validar si es necesario aumentar un mes, un año o un día según corresponda.
*/

Fecha aumentarSemana (Fecha fecha) {

    fecha.dia += 7;

    if (fecha.dia > diamax(fecha.mes)) {
        fecha.dia -= diamax(fecha.mes);
        fecha.mes++;
        if (fecha.mes > 12) {
            fecha.ano++;
            fecha.mes = 1;
        }
    }

    return fecha;

}

/*
    (Fecha) Esta función compara dos fechas que recibe por parámetro (fecha1 y fecha2.) Retornará un número entero, que será
    0 si las dos fechas son iguales, 1 si fecha1 es mayor a fecha2 o -1 si la segunda fecha es mayor a la primera.
*/

int compararFecha (Fecha fecha1, Fecha fecha2) { // Retorna 0 si son iguales, 1 si la primera es mayor. -1 si la segunda es mayor

    if (fecha1.dia == fecha2.dia && fecha1.mes == fecha2.mes && fecha1.ano == fecha1.ano) return 0;
    if (fecha1.dia > fecha2.dia && fecha1.mes >= fecha2.mes && fecha1.ano >= fecha2.ano) return 1;
    return -1;

}

/*
    (void) En caso de que ocurra un error al momento de leer los argumentos, se llamará esta función que imprimirá
    el correcto uso del 'argv' y los parámetros esperados.
*/

void errorArgv () {
        printf("---- [!] Error: Argumentos invalidos.\n");
        printf("[!] Ej: ./receptor -p <pipeNominal> -f <fileEntrada> -s <fileSalida>\n\n");
        printf("[!] -p: flag que indica que a continuación está el nombre del pipeNominal.\n");
        printf("[!] pipeNominal: nombre del pipeNominal.\n");
        printf("[!] -f: flag que indica que a continuación está el nombre del archivo de la BD inicial.\n");
        printf("[!] fileEntrada: nombre del archivo en donde se encuentra la BD inicial.\n");
        printf("[!] -s: flag que indica que a continuación está el nombre del archivo de la BD de salida.\n");
        printf("[!] fileSalida: nombre del archivo de salida en el cual se actualizara la BD con los cambios realizados.\n");
        exit(EXIT_FAILURE);
}

/*
    (int) Esta función accede a la colección de libros y retornará el indice del primer ejemplar que cumpla el criterio de 
    la solicitud enviada, ya que valida si el libro que está buscando en la colección, es para prestar, devolver o renovar.
    En caso que no encuentre ningún libro con el criterio dado, retornará -1 si no existe o -2 si no es válido el procedimiento.
*/

int buscarEjemplar (Libro libro, Solicitud solicitud) {

    int i;

    for (i = 0; i < libro.numeroEjemplares; i++) {
        if (solicitud.operacion == 'P') {
            if (libro.ejemplares[i].estado) 
                return i;
        } else if (solicitud.operacion == 'D') {
            if (!libro.ejemplares[i].estado) 
                return i;
        } else if (solicitud.operacion == 'R') {
            if (!libro.ejemplares[i].estado && libro.ejemplares[i].bloqueado == 0 || libro.ejemplares[i].bloqueado == atoi(solicitud.pipeRespuesta)) {
                libro.ejemplares[i].bloqueado = atoi(solicitud.pipeRespuesta);
                return i;
            }
        } else {
            return -2; // No es valido el procedimiento
        }   
    }

    return -1; // No existe

}

/*
    (int) Cambia el estado de un libro de prestado a devuelto y lo retorna como un número entero.
*/

int cambiarEstado (int estado) {
    return estado == 0 ? 1 : 0;
}

/*
    (int) Esta función recibe por parámetro: la colección de libros (libros), el número de libros (numeroLibros) y una
    determinada solicitud (solicitud). Se encarga de procesar esta solicitud dentro de la colección de libros y hacer su
    respectiva actualización.

    Retorna 0 en caso de que no se pueda procesar la solicitud, o 1 si la solicitud se procesó correctamente
    y se actualizó sobre la base de datos (vector libros).
*/

int procesarSolicitud (Solicitud solicitud) {

    int i, numeroEjemplar;
    

    for (i = 0; i < numeroLibros; i++) {
       if (libros[i].ISBN == solicitud.ISBN && !strcmp(libros[i].nombreLibro, solicitud.nombreLibro)) {
            numeroEjemplar = buscarEjemplar(libros[i], solicitud);
            if (numeroEjemplar == -1) {
                printf("[!] Ocurrio un error al procesar la solicitud. No hay ejemplares disponibles para este tipo de operacion (%c)\n", solicitud.operacion);
                return 0; // Devuelve 0 si la operacion falló
            } else if (numeroEjemplar == -2) {
                printf("[!] Ocurrio un error al procesar la solicitud. El tipo de solicitud ingresado (%c) no es valido.\n", solicitud.operacion);
                return 0; // Devuelve 0 si la operacion falló
            }

            if (solicitud.operacion == 'R') {
                if (compararFecha(libros[i].ejemplares[numeroEjemplar].fecha, fechaActual()) >= 0) {
                    libros[i].ejemplares[numeroEjemplar].fecha = aumentarSemana(libros[i].ejemplares[numeroEjemplar].fecha);
                } else {
                    libros[i].ejemplares[numeroEjemplar].fecha = fechaActual();
                }
            } else {
                libros[i].ejemplares[numeroEjemplar].estado = cambiarEstado(libros[i].ejemplares[numeroEjemplar].estado);
                libros[i].ejemplares[numeroEjemplar].fecha = fechaActual();
            }

            return 1; // Devuelve 1 si la operación se pudo completar
        }
    }
    return 0; // Devuelve 0 si la operacion falló
}

/*
    (Libro*) Esta función se encarga de leer la base de datos desde un archivo que es enviado por parámetro (fileDatos) y también
    de actualizar el número de libros que se encuentran en el mismo. Lee línea por línea del archivo donde se encuentra la base de datos
    y la almacena, al final retorna este vector lleno (libros).
*/

int leerBD (char* fileDatos) {

    int ejemplarActual = -1, libroActual = -1;
    char *token = NULL, *tokenFecha = NULL, linea [MAX_LINE_LENGTH] = {0};
    FILE *file;
    file = fopen(fileDatos, "r");

    if (file == NULL) {
        perror("No se pudo abrir el archivo.\n");
        exit(EXIT_FAILURE);
    }

    while (!feof(file)) {
        strcpy(linea, "");
        fgets(linea, sizeof(linea), file);

        if (linea == "") {
            printf("[!] Se encontró una linea en la base de datos inválida.\n");
            continue;
        }
        token = strtok(linea, ",");
        if (token == NULL){
            printf("[!] Se encontró una linea en el archivo %s inválida.\n", fileDatos);
            continue;
        } else if (strlen(token) >= 3 || *token > 57 || *token < 48) {
            numeroLibros = numeroLibros + 1;
        }
    }    

    libros = (Libro*) malloc (numeroLibros * sizeof(Libro));
    token = NULL;

    if (libros == NULL) {
        perror("No se pudo asignar memoria a la variable libros.");
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_SET);

    while (!feof(file)) {
        if (ejemplarActual == libros[libroActual].numeroEjemplares - 1) {
            ejemplarActual = -1;
            libroActual++;

            fgets(linea, sizeof(linea), file);

            token = strtok(linea, ",");

            if (token == NULL){
                continue;
            }

            strcpy(libros[libroActual].nombreLibro, token);
            //printf("Libro: %s\n", libros[libroActual].nombreLibro);

            token = strtok(NULL, ",");

            if (token == NULL){
                continue;
            }
            libros[libroActual].ISBN = atoi(token);

            token = strtok(NULL, ",");

            if (token == NULL){
                continue;
            }

            libros[libroActual].numeroEjemplares = atoi(token);

            libros[libroActual].ejemplares = (Ejemplares*) malloc(libros[libroActual].numeroEjemplares * sizeof(Ejemplares));

            if (libros[libroActual].ejemplares == NULL) {
                perror("No se pudo asignar memoria a los ejemplares de un libro.");
                exit(EXIT_FAILURE);
            }

        } else {
            ejemplarActual++;
            libros[libroActual].ejemplares[ejemplarActual].bloqueado = 0;

            fgets(linea, sizeof(linea), file);
            token = strtok(linea, ",");

            if (token == NULL){
                continue;
            }
            
            libros[libroActual].ejemplares[ejemplarActual].numeroEjemplar = atoi(token);
            token = strtok(NULL, ",");

            if (token == NULL){
                continue;
            }

            if (toupper(*token) == 'P') {
                libros[libroActual].ejemplares[ejemplarActual].estado = 0;
            } else if (toupper(*token) == 'D') {
                libros[libroActual].ejemplares[ejemplarActual].estado = 1;
            }

            token = strtok(NULL, ",");
            
            if (token == NULL){
                continue;
            }

            tokenFecha = strtok(token, "-");

            if (tokenFecha == NULL){
                continue;
            }

            libros[libroActual].ejemplares[ejemplarActual].fecha.dia = atoi(tokenFecha);
            tokenFecha = strtok(NULL, "-");

            if (tokenFecha == NULL){
                continue;
            }

            libros[libroActual].ejemplares[ejemplarActual].fecha.mes = atoi(tokenFecha);
            tokenFecha = strtok(NULL, "-");

            if (tokenFecha == NULL){
                continue;
            }

            libros[libroActual].ejemplares[ejemplarActual].fecha.ano = atoi(tokenFecha);

            if (libros[libroActual].ejemplares[ejemplarActual].fecha.dia > diamax(libros[libroActual].ejemplares[ejemplarActual].fecha.mes) || libros[libroActual].ejemplares[ejemplarActual].fecha.mes > 12 || libros[libroActual].ejemplares[ejemplarActual].fecha.dia < 0 || libros[libroActual].ejemplares[ejemplarActual].fecha.dia > 31 || libros[libroActual].ejemplares[ejemplarActual].fecha.mes < 0 || libros[libroActual].ejemplares[ejemplarActual].fecha.ano < 0) {
                printf("[!] Error en la BD: La fecha del libro %s (ejemplar %d) es incorrecta.\n", libros[libroActual].nombreLibro, ejemplarActual);
                exit(EXIT_FAILURE);
            }
        }        
    }

    fclose(file);
    return numeroLibros;
}

/*
    (void) Función que actualiza la base de datos luego de terminar de hacer todos los cambios dentro de la misma.
    Lo hace en el 'fileSalida' que se especifica por parámetro. Se llama antes de acabar el proceso receptor.
*/

void escribirBD(char* fileSalida) {

    int i, j;
    FILE* file;
    char salida [MAX_LINE_LENGTH];
    char buffer [MAX_LINE_LENGTH];

    file = fopen (fileSalida, "w");
    for (i = 0; i < numeroLibros; i++) {
        strcpy(salida, libros[i].nombreLibro);

        strcat(salida, ",");
        sprintf(buffer, "%d", libros[i].ISBN);

        strcat(salida, buffer);
        strcat(salida, ",");

        sprintf(buffer, "%d", libros[i].numeroEjemplares);
        strcat(salida, buffer);
        strcat(salida, "\n");

        fputs(salida, file);
        
        for (j = 0; j < libros[i].numeroEjemplares; j++) {
            sprintf(buffer, "%d", libros[i].ejemplares[j].numeroEjemplar);
            strcpy(salida, buffer);
            strcat(salida, ",");

            if (libros[i].ejemplares[j].estado) {
                strcat(salida, "D,");
            } else {
                strcat(salida, "P,");
            }

            if (libros[i].ejemplares[j].fecha.dia < 10) {
                strcat(salida, "0");
            }
            sprintf(buffer, "%d", libros[i].ejemplares[j].fecha.dia);
            strcat(salida, buffer);
            strcat(salida, "-");

            if (libros[i].ejemplares[j].fecha.mes < 10) {
                strcat(salida, "0");
            }
            sprintf(buffer, "%d", libros[i].ejemplares[j].fecha.mes);
            strcat(salida, buffer);
            strcat(salida, "-");

            sprintf(buffer, "%d", libros[i].ejemplares[j].fecha.ano);
            strcat(salida, buffer);
            strcat(salida, "\n");

            fputs(salida, file);
        }
    }

    fclose(file);

}

void insertarSolicitud (Solicitud solicitud) {
    sem_wait(&semEspacios);
    sem_wait(&semEntrada);

    BUFFER[numeroElementos] = solicitud;
    numeroElementos = (numeroElementos + 1) % TAMBUFFER;
    
    sem_post(&semEntrada);
    sem_post(&semElementos);
}

void procesarSolicitudes (Solicitud* solicitudes) {
    int respuesta, fdRespuesta, creado = 0;

    while (1) {
        sem_wait(&semElementos);
        sem_wait(&semEntrada);

        printf ("-> Solicitud entrante: ");
        switch (solicitudes[solicitudProcesando].operacion) {
            case 'P': {
                printf("Prestamo de libro %s (ISBN: %d)\n", solicitudes[solicitudProcesando].nombreLibro, solicitudes[solicitudProcesando].ISBN);
                break;
            }

            case 'R': {
                printf("Renovación de libro %s (ISBN: %d)\n", solicitudes[solicitudProcesando].nombreLibro, solicitudes[solicitudProcesando].ISBN);
                break;
            }

            case 'D': {
                printf("Devolución de libro %s (ISBN: %d)\n", solicitudes[solicitudProcesando].nombreLibro, solicitudes[solicitudProcesando].ISBN);
                break;
            }
        }

        respuesta = procesarSolicitud (solicitudes[solicitudProcesando]);

        do {
            fdRespuesta = open (solicitudes[solicitudProcesando].pipeRespuesta, O_WRONLY);
            if (fdRespuesta == -1) {
                perror("Error en la apertura del pipe receptor: ");
                sleep(1);
            } else {
                write(fdRespuesta, &respuesta, sizeof(int));
                creado = 1;
            }
        } while (creado == 0);

        close (fdRespuesta);

        solicitudProcesando = (solicitudProcesando + 1) % TAMBUFFER;

        sem_post(&semEntrada);
        sem_post(&semEspacios); // Sale algo del buffer
    }
}

int main (int argc, char** argv) {

    if (argc != 7) {
        errorArgv();
    }    
    
    int i = 0, fd, numeroLibros = 0, bytes, fdRespuesta, respuesta = 5, creado = 0, tiempo = 0;
    char pipeReceptor [64] = {0}, fileDatos [64] = {0}, fileSalida [64] = {0};
    char linea [MAX_LINE_LENGTH];

    Solicitud solicitud;

    for (i = 1; i < argc; i += 2) {
        if (!strcmp(argv[i], "-p")) {
            strcpy(pipeReceptor, argv[i+1]);
        } else if (!strcmp(argv[i], "-f")) {
            strcpy(fileDatos, argv[i+1]);
        } else if (!strcmp(argv[i], "-s")) {
            strcpy(fileSalida, argv[i+1]);
        } else {
            errorArgv();
        }
    }
    
    if (!(int)strlen(pipeReceptor) || !(int)strlen(fileDatos) || !(int)strlen(fileSalida)) {
        errorArgv();
    }

    numeroLibros = leerBD(fileDatos);

    if (numeroLibros > 0) {
        printf("Se leyo correctamente la base de datos desde el file %s.\n", fileDatos);
    } else {
        printf("Hubo un error al abrir la base de datos %s.\n", fileDatos);
        exit(EXIT_FAILURE);
    }

    mode_t fifo_mode = S_IRUSR | S_IWUSR;

    unlink (pipeReceptor); 
    if (mkfifo (pipeReceptor, fifo_mode) == -1) {
        perror("Error al crear el pipe receptor.");
        exit(1);
    }

    pthread_t thread1;

    pthread_create(&thread1, NULL, (void*)procesarSolicitudes, (void*)BUFFER);

    sem_init(&semEntrada, 0, 1);
    sem_init(&semEspacios, 0, TAMBUFFER);
    sem_init(&semElementos, 0, 0);

    fd = open (pipeReceptor, O_RDONLY);

    do {
        tiempo++;
        printf("Leyendo solicitudes... (%d/7)\n", tiempo);
        sleep(1);
        
        if (tiempo == 7) {
            break;
        }
        
        bytes = read (fd, &solicitud, sizeof(Solicitud));

        if (bytes == -1) {
            perror("Error en la lectura del pipe:");
            exit(1);
        }

        if (bytes) {
            tiempo = 0;
            insertarSolicitud(solicitud);
            // Se llena el buffer con la solicitud que acaba de leer.
            
        }
    } while (1);

    close (fd);
    unlink(pipeReceptor);
    escribirBD(fileSalida);

    return EXIT_SUCCESS;

}