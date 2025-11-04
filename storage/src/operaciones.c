#include "operaciones.h"

// int create(char* nombreFile, char* nombreTag){


//     log_debug(g_logger_storage, "Creando archivo %s con tag %s", nombreFile, nombreTag);
//     char* pathFile = string_new();
//     string_append(&pathFile, g_storage_config->punto_montaje);
//     string_append(&pathFile, "/files/");
//     string_append(&pathFile, nombreFile);

//     char* pathTag = string_duplicate(pathFile);
//     string_append(&pathTag, "/");
//     string_append(&pathTag, nombreTag);

//     char* pathLogicalBlocks = string_duplicate(pathTag);
//     string_append(&pathLogicalBlocks, "/logical_blocks");

//     struct stat st = {0};
//     //Nos fijamos si existe la ruta
//     if(stat(pathFile, &st) == 0){
//         log_info(g_logger_storage, "El archivo %s ya existe. Se procede a ver si existe el tag", pathFile);
//         //Chequeo de existencia de tag
//         if(stat(pathTag, &st) == 0){
//             log_error(g_logger_storage, "El tag %s ya existe para el archivo %s. No se puede crear", nombreTag, nombreFile);
//             free(pathFile);
//             free(pathTag);
//             free(pathLogicalBlocks);
//             return -1;
//         //Creacion del tag cuando no existe
//         } else { 
//             if(mkdir(pathTag, 0777) == -1){
//                 log_error(g_logger_storage, "Error al crear el tag %s para el archivo %s", nombreTag, nombreFile);
//                 free(pathFile);
//                 free(pathTag);
//                 free(pathLogicalBlocks);
//                 return -1;
//             } else {
//                 log_info(g_logger_storage, "Tag %s creado exitosamente para el archivo %s", nombreTag, nombreFile);
                
                

//                 if(mkdir(pathLogicalBlocks, 0777) == -1){
//                     log_error(g_logger_storage, "Error al crear el directorio logical_blocks en %s", pathLogicalBlocks);
//                     free(pathLogicalBlocks);
//                     free(pathFile);
//                     free(pathTag);
//                     return -1;
//                 } else {
//                     log_info(g_logger_storage, "Directorio logical_blocks creado exitosamente en %s", pathLogicalBlocks);
//                 }

//                 string_append(&pathTag, "/metadata.config");

//                 escribirMetadataConfig(pathTag, 0, NULL, 0, "WORK_IN_PROGRESS");

//                 free(pathFile);
//                 free(pathTag);
//                 free(pathLogicalBlocks);
//                 return 0;
//             }

//         }

    //Creacion de file y tag cuando no existe
//     } else {
//         if (mkdir(pathFile, 0777) == -1){
//             log_error(g_logger_storage, "Error al crear el archivo %s", nombreFile);
//             free(pathFile);
//             free(pathTag);
//             return -1;
//         } else {
//             log_info(g_logger_storage, "Archivo %s creado exitosamente", nombreFile);
//             if(mkdir(pathTag, 0777) == -1){
//                 log_error(g_logger_storage, "Error al crear el tag %s para el archivo %s", nombreTag, nombreFile);
//                 free(pathFile);
//                 free(pathTag);
//                 free(pathLogicalBlocks);
//                 return -1;
//             } else {
//                 log_info(g_logger_storage, "Tag %s creado exitosamente para el archivo %s", nombreTag, nombreFile);
//                 escribirMetadataConfig(pathTag, 0, NULL, 0, "WORK_IN_PROGRESS");
//                 if(mkdir(pathLogicalBlocks, 0777) == -1){
//                     log_error(g_logger_storage, "Error al crear el directorio logical_blocks en %s", pathLogicalBlocks);
//                     free(pathLogicalBlocks);
//                     free(pathFile);
//                     free(pathTag);
//                     return -1;
//                 } else {
//                     log_info(g_logger_storage, "Directorio logical_blocks creado exitosamente en %s", pathLogicalBlocks);
                    
//                 }

//                 free(pathFile);
//                 free(pathTag);
//                 free(pathLogicalBlocks);
//                 return 0;
//             }
//         }
//     }
//     return 0;
// }

int create(char* nombreFile, char* nombreTag){
     log_info(g_logger_storage, "Iniciando operación CREATE para file: %s, tag: %s", nombreFile, nombreTag);

    char* pathFile = string_from_format("%s/files/%s", 
                                        g_storage_config->punto_montaje, 
                                        nombreFile);
    
    char* pathTag = string_from_format("%s/%s", 
                                       pathFile, 
                                       nombreTag);
    char* pathLogicalBlocks = string_from_format("%s/logical_blocks", 
                                                pathTag);
    
    char* pathMetadata = string_from_format("%s/metadata.config", 
                                            pathTag);

    int status = -1; 
    struct stat st = {0};


    //Chequeo existencia file
    if (stat(pathFile, &st) == -1) {
        // No existe, lo creamos
        log_info(g_logger_storage, "Archivo %s no existe. Creandolo", nombreFile);
        if (mkdir(pathFile, 0777) == -1) {
            log_error(g_logger_storage, "Error al crear directorio %s: %s", pathFile, strerror(errno));
            goto cleanup; // Ir a la limpieza
        }
        log_info(g_logger_storage, "Directorio %s creado exitosamente.", pathFile);
    } else {
        log_info(g_logger_storage, "El archivo %s ya existe. Se procede a ver si existe el tag", pathFile);
    }

    //Chequeo TAG
    if (stat(pathTag, &st) == 0) {
        // El tag YA existe. Esto es un error.
        log_error(g_logger_storage, "Error CREATE: El tag %s ya existe para el archivo %s.", nombreTag, nombreFile);
        goto cleanup;
    } else {
        // El tag no existe, lo creamos
        if (mkdir(pathTag, 0777) == -1) {
            log_error(g_logger_storage, "Error al crear directorio de tag %s: %s", pathTag, strerror(errno));
            goto cleanup;
        }
        log_info(g_logger_storage, "Tag %s creado exitosamente.", nombreTag);
    }

    //Chequeo logical_blocks
    if (mkdir(pathLogicalBlocks, 0777) == -1) {
        log_error(g_logger_storage, "Error al crear directorio logical_blocks en %s: %s", pathLogicalBlocks, strerror(errno));
        goto cleanup;
    }
    log_info(g_logger_storage, "Directorio logical_blocks creado exitosamente en %s", pathLogicalBlocks);

    
     
    log_debug(g_logger_storage, "Creando archivo metadata.config vacío en: %s", pathMetadata);
    crearArchivo(pathMetadata, 0); 
    escribirMetadataConfig(pathMetadata, 0, NULL, 0, "WORK_IN_PROGRESS");

 
    log_info(g_logger_storage, "Operación CREATE exitosa para %s/%s", nombreFile, nombreTag);
    status = 0;

    cleanup:
    // Limpieza de memoria
    free(pathFile);
    free(pathTag);
    free(pathLogicalBlocks);
    free(pathMetadata);
    
    return status;
}