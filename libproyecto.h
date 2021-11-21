#define MAX_LINE_LENGTH 128

typedef struct {
    int ano;
    int mes;
    int dia;
} Fecha;

typedef struct {
    int bloqueado; // 0 Disponible - Cualquier otro: PID del proceso que está usando este ejemplar.
    int numeroEjemplar;
    int estado; // 0 Prestado - 1 Disponible
    Fecha fecha;
} Ejemplares;

typedef struct {
    char nombreLibro [MAX_LINE_LENGTH];
    int ISBN;
    int numeroEjemplares;
    Ejemplares* ejemplares;
} Libro;

typedef struct {
    char operacion;
    char nombreLibro [MAX_LINE_LENGTH];
    int ISBN;
    char pipeRespuesta [MAX_LINE_LENGTH];
} Solicitud;