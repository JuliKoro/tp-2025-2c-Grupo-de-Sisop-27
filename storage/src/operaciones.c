#include "operaciones.h"

int create(uint32_t query_id, char* nombreFile, char* nombreTag){
    log_debug(g_logger_storage, "Durmiendo tiempo de retardo de operacion: %d ms", g_storage_config->retardo_operacion);
    usleep(g_storage_config->retardo_operacion * 1000);
    log_info(g_logger_storage, "QUERY ID: %d Iniciando operación CREATE para file: %s, tag: %s",query_id, nombreFile, nombreTag);

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
        log_debug(g_logger_storage, "Directorio %s creado exitosamente.", pathFile);
        log_info(g_logger_storage,"##<QUERY_ID %d> - File Creado <%s>:<%s>", query_id, nombreFile, nombreTag);
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
        log_debug(g_logger_storage, "Tag %s creado exitosamente.", nombreTag);
        log_info(g_logger_storage,"##<QUERY_ID %d> - Tag creado <%s>:<%s>", query_id, nombreFile, nombreTag);

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

int tag (uint32_t query_id,char* nombreFile, char* tagOrigen, char* tagDestino){
    log_debug(g_logger_storage, "Durmiendo tiempo de retardo de operacion: %d ms", g_storage_config->retardo_operacion);
    usleep(g_storage_config->retardo_operacion * 1000);

    log_info(g_logger_storage, "QUERY ID: %d Iniciando operación TAG para file: %s, tagOrigen: %s, tagDestino: %s",query_id, nombreFile, tagOrigen, tagDestino);

    char* pathOrigen = string_from_format("%s/files/%s/%s", 
                                        g_storage_config->punto_montaje, 
                                        nombreFile,
                                        tagOrigen);

    char* pathDestino = string_from_format("%s/files/%s/%s", 
                                        g_storage_config->punto_montaje, 
                                        nombreFile,
                                        tagDestino);


    int status = -1;
    struct stat st = {0};

    if( stat(pathOrigen, &st) == -1) {
        log_error(g_logger_storage, "Error TAG: El tag origen %s no existe para el archivo %s.", tagOrigen, nombreFile);
        goto cleanup;
    }
    if(stat(pathDestino, &st) == 0) {
        log_error(g_logger_storage, "Error TAG: El tag destino %s ya existe para el archivo %s.", tagDestino, nombreFile);
        goto cleanup;
    } else {
        char* comandoCopy = string_from_format("cp -r %s %s", pathOrigen, pathDestino);
        int resultadoCopy = system(comandoCopy);
        free(comandoCopy);
        if (resultadoCopy != 0) {
            log_error(g_logger_storage, "Error al copiar el tag de %s a %s para el archivo %s.", tagOrigen, tagDestino, nombreFile);
            goto cleanup;  
        } else {
            log_info(g_logger_storage,"##<QUERY_ID %d> - Tag copiado de <%s> a <%s> para el file <%s>", query_id, tagOrigen, tagDestino, nombreFile);
            char* pathMetadataDestino = string_from_format("%s/metadata.config", pathDestino);
            t_config* metadataConfig = config_create(pathMetadataDestino);
            if (metadataConfig == NULL) {
                log_error(g_logger_storage, "Error al leer el metadata.config en %s despues del copy.", pathMetadataDestino);
                free(pathMetadataDestino);
                goto cleanup; 
            } else {
                config_set_value(metadataConfig, "ESTADO", "WORK_IN_PROGRESS");
                config_save(metadataConfig);
                config_destroy(metadataConfig);
                free(pathMetadataDestino);
                status = 0;
                log_debug(g_logger_storage, "Operación TAG exitosa de %s a %s para el archivo %s.", tagOrigen, tagDestino, nombreFile);
                log_info(g_logger_storage, "QUERY ID %d > - Tag creado %s:%s", query_id, nombreFile, tagDestino);
            }
        }


    }
    cleanup:
    free(pathOrigen);
    free(pathDestino);

    return status;
}