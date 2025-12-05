#include "operaciones.h"

void simularRetardoOperacion(){
    log_debug(g_logger_storage, "Durmiendo tiempo de retardo de operacion: %d ms", g_storage_config->retardo_operacion);
    usleep(g_storage_config->retardo_operacion * 1000);
}

void simularRetardoAccesoBloque(){
    log_debug(g_logger_storage, "Durmiendo tiempo de retardo de acceso a bloque: %d ms", g_storage_config->retardo_acceso_bloque);
    usleep(g_storage_config->retardo_acceso_bloque * 1000);
}

void* leerBloqueFisico(int bloqueFisico){
    simularRetardoAccesoBloque();
    log_debug(g_logger_storage, "Leyendo bloque fisico: %d", bloqueFisico);
    char* pathBloque = string_from_format("%s/physical_blocks/block%04d.dat", g_storage_config->punto_montaje, bloqueFisico);

    int fd = open (pathBloque, O_RDONLY);
    if (fd == -1) {
        log_error(g_logger_storage, "Error al abrir el bloque fisico %d en %s: %s", bloqueFisico, pathBloque, strerror(errno));
        free(pathBloque);
        return NULL;
    } 
    size_t tamanioBloque = g_superblock_config->block_size;
    void* buffer = malloc(tamanioBloque);
    if (buffer == NULL) {
    log_error(g_logger_storage, "Error de malloc al reservar memoria para el buffer %d", bloqueFisico);
    close(fd);
    free(pathBloque);
    return NULL;
    }
    
    ssize_t bytesLeidos = read(fd, buffer, tamanioBloque);

    if (bytesLeidos == -1){
        log_error(g_logger_storage, "Error al leer bytes desde el bloque fisico %d en %s: %s", bloqueFisico, pathBloque, strerror(errno));
        free(buffer);
        close(fd);
        free(pathBloque);
        return NULL;
    }
    if ((size_t)bytesLeidos < tamanioBloque) {
        log_warning(g_logger_storage, "Lectura corta en bloque %d. Leídos %zd de %zu", bloqueFisico, bytesLeidos, tamanioBloque);
    }

    close(fd);
    free(pathBloque);

    log_debug(g_logger_storage, "Bloque físico %d leído exitosamente.", bloqueFisico);
    return buffer;
}

int create(uint32_t query_id, char* nombreFile, char* nombreTag){
    simularRetardoOperacion();
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
    simularRetardoOperacion();
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

void* leer(uint32_t query_id, char* nombreFile, char* nombreTag, uint32_t bloqueLogico){
    simularRetardoOperacion();

    log_debug(g_logger_storage, "QUERY ID: %d Iniciando operación READ para file: %s, tag: %s, bloqueLogico: %d",query_id, nombreFile, nombreTag, bloqueLogico);

    char* pathFile = string_from_format("%s/files/%s", 
                                        g_storage_config->punto_montaje, 
                                        nombreFile);

    char* pathTag = string_from_format("%s/%s", 
                                       pathFile, 
                                       nombreTag);

    struct stat st = {0};
    void* buffer = NULL;
    char** bloquesLogicos = NULL;
    t_config* metadataConfig = NULL;


    if(stat(pathFile, &st) == -1) {
        log_error(g_logger_storage, "Error READ: El archivo %s no existe.", nombreFile);
        goto cleanup;
    }

    if(stat(pathTag, &st) == -1) {
        log_error(g_logger_storage, "Error READ: El tag %s no existe para el archivo %s.", nombreTag, nombreFile);
        goto cleanup;

    }
    char* pathMetadata = string_from_format("%s/metadata.config", pathTag);
    metadataConfig = config_create(pathMetadata);
    if (metadataConfig == NULL) {
        log_error(g_logger_storage, "Error READ: No se pudo leer el metadata.config en %s.", pathTag);
        goto cleanup;
    }

    bloquesLogicos = config_get_array_value(metadataConfig, "BLOCKS");
    if(bloqueLogico>=string_array_size(bloquesLogicos)){
        log_error(g_logger_storage, "[QUERY %d] Error READ: Lectura fuera de limite. Bloque lógico %d no existe (Total: %d).", query_id, bloqueLogico, string_array_size(bloquesLogicos));
        goto cleanup;    
    }
    int bloqueFisico = atoi(bloquesLogicos[bloqueLogico]);
    buffer = leerBloqueFisico(bloqueFisico);

    if (buffer == NULL) {
        log_error(g_logger_storage, "Error READ: No se pudo leer el bloque físico %d para el bloque lógico %d en el tag %s del archivo %s.", bloqueFisico, bloqueLogico, nombreTag, nombreFile);
        goto cleanup;
    }

    log_info(g_logger_storage, "##<%d> - Bloque Lógico Leído %s:%s - Número de Bloque: %d", query_id, nombreFile, nombreTag, bloqueLogico);

    string_array_destroy(bloquesLogicos);
    config_destroy(metadataConfig);
    free(pathFile);
    free(pathTag);
    free(pathMetadata);

    return buffer;

    cleanup:
    if (buffer) free(buffer);
    if (bloquesLogicos) string_array_destroy(bloquesLogicos);
    if (metadataConfig) config_destroy(metadataConfig);
    free(pathFile);
    free(pathTag);
    free(pathMetadata);
    
    return NULL;
    
}

 void liberarBloqueSiEsNecesario(u_int32_t query_id, int numeroBloque){
    char* pathBloqueFisico = string_from_format("%s/physical_blocks/block%04d.dat", g_storage_config->punto_montaje, numeroBloque);
    struct stat st = {0};

    if (stat(pathBloqueFisico, &st) == -1) {
        log_error(g_logger_storage, "Error al hacer stat del bloque físico %d para liberación", numeroBloque);
        free(pathBloqueFisico);
        return;
    }

    // Si st_nlink es 1, significa que solo queda la referencia dentro de /physical_blocks.
    // Ningún archivo lógico (en /files) lo está apuntando.
    if (st.st_nlink == 1) {
        pthread_mutex_lock(&g_mutexBitmap);
        if (bitarray_test_bit(g_bitmap, numeroBloque)) {
            bitarray_clean_bit(g_bitmap, numeroBloque);
            log_debug(g_logger_storage, "##<QUERY ID %d> Bloque físico %d liberado en bitmap (nlink=1).", query_id, numeroBloque);
        }
        pthread_mutex_unlock(&g_mutexBitmap);
    } else {
        log_debug(g_logger_storage, "Bloque físico %d NO liberado. Tiene %lu referencias restantes.", numeroBloque, st.st_nlink);
    }

    free(pathBloqueFisico);
}

 

int eliminarTag(u_int32_t query_id, char* nombreFile, char* nombreTag){
    simularRetardoOperacion();
    log_info(g_logger_storage, "QUERY ID: %d Iniciando operación DELETE para file: %s, tag: %s",query_id, nombreFile, nombreTag);

    char* pathFile = string_from_format("%s/files/%s", 
                                        g_storage_config->punto_montaje, 
                                        nombreFile);

    char* pathTag = string_from_format("%s/%s", 
                                       pathFile, 
                                       nombreTag);

    char* pathMetadata = string_from_format("%s/metadata.config", 
                                            pathTag);


    int status = -1;
    struct stat st = {0};
    t_config* metadataConfig = NULL;
    char** bloquesLogicos = NULL;

    //Valido existencia file

    if(stat(pathFile, &st) == -1) {
        log_error(g_logger_storage, "Error DELETE: El archivo %s no existe.", nombreFile);
        goto cleanup;
    }

    //Leo metadata.config del tag para ver los blooques asignados

    metadataConfig = config_create(pathMetadata);
    if (metadataConfig == NULL) {
        log_error(g_logger_storage, "Error DELETE: No se pudo leer el metadata.config en %s.", pathTag);
        goto cleanup;
    }

    bloquesLogicos = config_get_array_value(metadataConfig, "BLOCKS");

    //Borro el directorio del tag, se van a borrar hard links
    char* comandoRemove = string_from_format("rm -rf %s", pathTag);
    int resultadoRemove = system(comandoRemove);
    free(comandoRemove);


    if (resultadoRemove != 0) {
        log_error(g_logger_storage, "Error al ejecutar rm -rf para eliminar el tag %s", pathTag);
        goto cleanup;
    }
    log_info(g_logger_storage,"##<QUERY_ID %d> - Tag eliminado <%s>:<%s>", query_id, nombreFile, nombreTag);

    //Hay que actualizar bitmap, liberar bloques huerfanos

    int i = 0;
    while(bloquesLogicos[i] != NULL) {
        int nroBloque = atoi(bloquesLogicos[i]);
        
        liberarBloqueSiEsNecesario(query_id, nroBloque);
        if (!bitarray_test_bit(g_bitmap, nroBloque)) { // Chequeo rápido post-liberación (sin lock, solo para log)
             log_info(g_logger_storage, "##<QUERY_ID %d> - Bloque Físico Liberado %d", query_id, nroBloque);
        }

        i++;
    }


    status = 0;
    log_info(g_logger_storage, "Operación DELETE finalizada exitosamente.");

    cleanup:
    if (bloquesLogicos) string_array_destroy(bloquesLogicos);
    if (metadataConfig) config_destroy(metadataConfig);
    free(pathFile);
    free(pathTag);
    free(pathMetadata);

    return status;
}