# Gestion-Libros-Linux
Sistema de préstamo, devolución y renovación de libros (simulación de biblioteca), por medio de un servidor y varios clientes. Desarrollado en lenguaje C para Linux.

Un proceso receptor (servidor) recibe y procesa peticiones de los diferentes procesos solicitantes (clientes), los cuales contienen diferentes tipos de peticiones que se pueden realizar sobre los diferentes libros existentes en la base de datos. La comunicación cliente-servidor se hace mediante pipes nominales de Linux y el procesamiento de solicitudes, mediante un buffer que es gestionado por un thread. Implementación de semáforos de Linux para el acceso a recursos compartidos.

# Instalación

make

# Utilización

./receptor -p <pipeNominal> -f <fileEntrada> -s <fileSalida>
./solicitante [-i <archivoEntrada>] -p <pipeReceptor>
  


