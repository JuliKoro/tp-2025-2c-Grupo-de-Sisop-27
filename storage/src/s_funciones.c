#include "s_funciones.h"

void copiarArchivo(char* origen, char* destino) {
    //Primero abro el archivo de origen en modo lectura

    int fd_origen =  open(origen, O_RDONLY);
    if(fd_origen == -1){
        log_error(logger_storage, "CopiarArchivo: Error al abrir el archivo de origen %s", origen);
        return;
    }
    

    //Luego creo y abro el archivo de destino para escritura
    int fd_destino = open(destino, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if(fd_destino == -1){
        log_error(logger_storage, "CopiarArchivo: Error al crear y abrir el archivo de destino %s", destino);
        return;
    }

    //Luego leo el archivo de origen y lo escribo en el de destino
    char buffer[4096];
    ssize_t bytes_leidos;
    while((bytes_leidos = read(fd_origen, buffer, sizeof(buffer))) > 0){
        if(write(fd_destino, buffer, bytes_leidos) != bytes_leidos){
            log_error(logger_storage, "Error al escribir en el archivo de destino %s", destino);
            break;
        }
    }

    //Luego cierro los archivos
    close(fd_origen);
    close(fd_destino);

    if(bytes_leidos == -1){
        log_error(logger_storage, "Error al leer el archivo de origen %s", origen);
    } else {
        log_info(logger_storage, "Archivo %s copiado exitosamente a %s", origen, destino);
    }
}

void crearArchivo(char* path){
    log_info(logger_storage, "Creando el archivo %s", path);

    int fd = open(path, O_CREAT | O_WRONLY, 0666);
    if(fd == -1){
        log_error(logger_storage, "Error al crear el archivo %s", path);
        return;
    }
    close(fd);
    log_info(logger_storage, "Archivo %s creado exitosamente", path);
}

void inicializarPuntoMontaje(char* path){
    struct stat st = {0};
    //Nos fijamos si existe la ruta
    if(stat(path, &st) == 0){
        log_info(logger_storage, "El punto de montaje %s ya existe. Eliminandolo para fresh start", path);

        //Procedemos a borrar

        char comando [512];
        snprintf(comando, sizeof(comando), "rm -rf %s", path);

        if(system(comando) != 0){
            log_error(logger_storage, "Error al eliminar el punto de montaje %s", path);
            exit(EXIT_FAILURE);
        }
    }

    //Creamos el punto de montaje
    log_info(logger_storage, "Creando el punto de montaje en %s", path);
    if(mkdir(path, 0777) == -1){
        log_error(logger_storage, "Error al crear el punto de montaje en %s", path);
        exit(EXIT_FAILURE);
    } else {
        log_info(logger_storage, "Punto de montaje %s creado exitosamente", path);
    }

    log_info(logger_storage, "Creando el archivo bitmap.bin en %s", path);
    char* bitmap_path = string_duplicate(path);
    string_append(&bitmap_path, "/bitmap.bin");
    crearArchivo(bitmap_path);
    free(bitmap_path);

    log_info(logger_storage, "Creando el archivo blocks_hash_index.config en %s", path);
    char* blocks_hash_index_path = string_duplicate(path);
    string_append(&blocks_hash_index_path, "/blocks_hash_index.config");
    crearArchivo(blocks_hash_index_path);
    free(blocks_hash_index_path);
    
    
    
    log_info(logger_storage, "Creando directorios nativos en %s...", path);
    char* physical_blocks = string_duplicate(path);
    string_append(&physical_blocks, "/physical_blocks");
    if(mkdir(physical_blocks, 0777) == -1){
        log_error(logger_storage, "Error al crear el directorio physical_blocks en %s", path);
        exit(EXIT_FAILURE);
    } else {
        log_info(logger_storage, "Directorio physical_blocks creado exitosamente en %s", path);
    }
    char* files_folder = string_duplicate(path);
    string_append(&files_folder, "/files");
    if(mkdir(files_folder, 0777) == -1){
        log_error(logger_storage, "Error al crear el directorio files en %s", path);
        exit(EXIT_FAILURE);
    } else {
        log_info(logger_storage, "Directorio files creado exitosamente en %s", path);
    }
    log_info(logger_storage, "Directorios nativos creados exitosamente. Liberando memoria asignada dinamicamente para este procedimiento");
    free(physical_blocks);
    free(files_folder);

}



