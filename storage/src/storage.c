#include <utils/hello.h>
#include <utils/configs.h>
#include <utils/mensajeria.h>
#include "conexion.h"
#include "operaciones.h"
#include <commons/string.h>

t_log* g_logger_storage = NULL;

storage_conf* g_storage_config = NULL;

superblock_conf* g_superblock_config = NULL;

int g_cantidadWorkers = 0;

int main(int argc, char* argv[]) {
    saludar("storage");

    if(argc < 3){
        fprintf(stderr, "Uso: %s <nombre_archivo_configuracion> <nombre_archivo_superblock config>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    char* nombre_archivo_configuracion = argv[1];
    g_storage_config = get_configs_storage(nombre_archivo_configuracion);
    char* path_superblock = string_duplicate(g_storage_config->punto_montaje);
    string_append(&path_superblock, "/");
    string_append(&path_superblock, argv[2]);
    printf("PUERTO_ESCUCHA: %d\n", g_storage_config->puerto_escucha);
    printf("PUNTO_MONTAJE: %s\n", g_storage_config->punto_montaje);
    printf("LOG_LEVEL: %s\n", g_storage_config->log_level);

    char puerto_escucha[10];
    sprintf(puerto_escucha, "%d", g_storage_config->puerto_escucha);
    int socket_servidor = iniciar_servidor(puerto_escucha);

    //Inicializamos el logger y el hash index config global para poder manipularlo en todo el archivo.
    g_logger_storage = iniciarLoggerStorage(g_storage_config->log_level);
    //inicializarHashIndex(); esto lo muevo a inicializarPuntoMontaje y a cargarPuntoMontaje

    if(socket_servidor == -1){
        log_error(g_logger_storage, "Error al iniciar el servidor en el puerto %s", puerto_escucha);
        return EXIT_FAILURE;
    }

    //Logica de creacion de archivos y rutas
    log_debug(g_logger_storage, "Iniciando logica de creacion de archivos y rutas");
    log_debug(g_logger_storage, "PUNTO_MONTAJE: %s", g_storage_config->punto_montaje);
    log_debug(g_logger_storage, "FRESH_START: %s", g_storage_config->fresh_start?"true":"false");
    
    log_debug(g_logger_storage, "LEYENDO ARCHIVO DE CONFIGURACION SUPERBLOCK");
    g_superblock_config = get_configs_superblock(argv[2]);
    printf("Tamanio del sistema de archivos: %d \n", g_superblock_config->fs_size);
    printf("Tamanio de los bloques: %d \n", g_superblock_config->block_size);
    printf("Cantidad de bloques con esta superbock config: %d \n", g_superblock_config->cantidad_bloques);

    inicializarSemaforos();

    if(g_storage_config->fresh_start){
        log_debug(g_logger_storage, "Copiando superblock.config a %s", path_superblock);
        inicializarPuntoMontaje(g_storage_config->punto_montaje);
        copiarArchivo(argv[2], path_superblock);
    } else {
        log_debug(g_logger_storage, "No se realiza fresh start, se carga configuarion existente");
        cargarPuntoMontaje(g_storage_config->punto_montaje);
    }


    while (1) {
        log_info(g_logger_storage, "Iniciando ciclo de espera de clientes");
        pthread_t thread;
        int *fd_conexion_ptr = malloc(sizeof(int));
        *fd_conexion_ptr = esperar_cliente(socket_servidor);
        if(*fd_conexion_ptr == -1){
            fprintf(stderr, "Error al esperar cliente\n");
            return EXIT_FAILURE;
        }
        t_paquete* paquete = recibir_paquete(*fd_conexion_ptr);
        if(paquete == NULL){
            fprintf(stderr, "Error al recibir paquete\n");
            return EXIT_FAILURE;
        }

        switch(paquete->codigo_operacion) {
            t_thread_args* thread_args;
            case HANDSHAKE_WORKER_STORAGE:
                thread_args = malloc(sizeof(t_thread_args));
                thread_args->paquete = paquete;
                thread_args->fd_conexion = fd_conexion_ptr;
                pthread_create(&thread, NULL, atender_worker, (void*) thread_args);
                pthread_detach(thread);
                break;
            default:
                fprintf(stderr, "Error: el paquete no es de tipo HANDSHAKE_<MODULO>_MASTER\n");
                destruir_paquete(paquete);
                return EXIT_FAILURE;
        }
    }

    log_info(g_logger_storage, "Guardando índice de hashes en disco...");
    pthread_mutex_lock(&g_mutexHashIndex);
    config_save(g_hash_config); 
    pthread_mutex_unlock(&g_mutexHashIndex);

    config_destroy(g_hash_config);
    pthread_mutex_destroy(&g_mutexHashIndex);//TODO: mover esto a una funcion general con todas las tareas necesarias cuando termina el proceso.
    return 0;
}
