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

    pthread_mutex_lock(&g_blocks_mutex[bloqueFisico]);
    int fd = open (pathBloque, O_RDONLY);
    if (fd == -1) {
        log_error(g_logger_storage, "Error al abrir el bloque fisico %d en %s: %s", bloqueFisico, pathBloque, strerror(errno));
        free(pathBloque);
        return NULL;
    } 
    size_t tamanioBloque = g_superblock_config->block_size;
    void* buffer = calloc(1, tamanioBloque);
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
        pthread_mutex_unlock(&g_blocks_mutex[bloqueFisico]);
        free(pathBloque);
        return NULL;
    }
    if ((size_t)bytesLeidos < tamanioBloque) {
        log_warning(g_logger_storage, "Lectura corta en bloque %d. Leídos %zd de %zu", bloqueFisico, bytesLeidos, tamanioBloque);
    }


    close(fd);
    pthread_mutex_unlock(&g_blocks_mutex[bloqueFisico]);
    free(pathBloque);

    log_debug(g_logger_storage, "Bloque físico %d leído exitosamente.", bloqueFisico);
    return buffer;
}

int create(uint32_t query_id, char* nombreFile, char* nombreTag){
    simularRetardoOperacion();
    lock_file_metadata(nombreFile);
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
        status = ERROR_YA_EXISTE;
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
    
    unlock_file_metadata(nombreFile);
    return status;
}

int tag (uint32_t query_id,char* nombreFile, char* tagOrigen, char* tagDestino){
    simularRetardoOperacion();
    lock_file_metadata(nombreFile);
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
        status = ERROR_FILE_NO_EXISTE;
        goto cleanup;
    }
    if(stat(pathDestino, &st) == 0) {
        log_error(g_logger_storage, "Error TAG: El tag destino %s ya existe para el archivo %s.", tagDestino, nombreFile);
        status = ERROR_YA_EXISTE;
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

    unlock_file_metadata(nombreFile);
    return status;
}

int leer(uint32_t query_id, char* nombreFile, char* nombreTag, uint32_t bloqueLogico, void** bufferSalida) {
    simularRetardoOperacion();

    log_debug(g_logger_storage, "QUERY ID: %d Iniciando operación READ para file: %s, tag: %s, bloqueLogico: %d",query_id, nombreFile, nombreTag, bloqueLogico);

    char* pathFile = string_from_format("%s/files/%s", 
                                        g_storage_config->punto_montaje, 
                                        nombreFile);

    char* pathTag = string_from_format("%s/%s", 
                                       pathFile, 
                                       nombreTag);

    struct stat st = {0};
    char** bloquesLogicos = NULL;
    t_config* metadataConfig = NULL;
    int status = -1;


    if(stat(pathFile, &st) == -1) {
        log_error(g_logger_storage, "Error READ: El archivo %s no existe.", nombreFile);
        status = ERROR_FILE_NO_EXISTE;
        goto cleanup;
    }

    if(stat(pathTag, &st) == -1) {
        log_error(g_logger_storage, "Error READ: El tag %s no existe para el archivo %s.", nombreTag, nombreFile);
        status = ERROR_FILE_NO_EXISTE;
        goto cleanup;

    }
    char* pathMetadata = string_from_format("%s/metadata.config", pathTag);
    lock_file_metadata(nombreFile);
    metadataConfig = config_create(pathMetadata);
    if (metadataConfig == NULL) {
        log_error(g_logger_storage, "Error READ: No se pudo leer el metadata.config en %s.", pathTag);
        unlock_file_metadata(nombreFile);
        goto cleanup;
    }

    bloquesLogicos = config_get_array_value(metadataConfig, "BLOCKS");
    if(bloqueLogico>=string_array_size(bloquesLogicos)){
        log_error(g_logger_storage, "[QUERY %d] Error READ: Lectura fuera de limite. Bloque lógico %d no existe (Total: %d).", query_id, bloqueLogico, string_array_size(bloquesLogicos));
        status = ERROR_FUERA_DE_LIMITE;
        goto cleanup;    
    }
    int bloqueFisico = atoi(bloquesLogicos[bloqueLogico]);
    unlock_file_metadata(nombreFile);
    *bufferSalida = leerBloqueFisico(bloqueFisico);

    if (*bufferSalida == NULL) {
        log_error(g_logger_storage, "Error READ: No se pudo leer el bloque físico %d para el bloque lógico %d en el tag %s del archivo %s.", bloqueFisico, bloqueLogico, nombreTag, nombreFile);
        status = -1;
        goto cleanup;
    }

    status = EXEC_OK;
    log_info(g_logger_storage, "##<%d> - Bloque Lógico Leído %s:%s - Número de Bloque: %d", query_id, nombreFile, nombreTag, bloqueLogico);
    log_debug(g_logger_storage, "Data leida: %s", (char*)(*bufferSalida));



    cleanup:
    if (bloquesLogicos) string_array_destroy(bloquesLogicos);
    if (metadataConfig) config_destroy(metadataConfig);
    free(pathFile);
    free(pathTag);
    free(pathMetadata);
    return status;
    
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
        log_debug(g_logger_storage, "Bloque físico %d NO liberado. Tiene %u referencias restantes.", numeroBloque, st.st_nlink);
    }

    free(pathBloqueFisico);
}

 

int eliminarTag(u_int32_t query_id, char* nombreFile, char* nombreTag){
    simularRetardoOperacion();
    lock_file_metadata(nombreFile);
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
        status = ERROR_FILE_NO_EXISTE;
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
    //Log obligatorio: Tag Eliminado
    log_info(g_logger_storage, "##<QUERY_ID %d> - Tag Eliminado <%s>:<%s>", query_id, nombreFile, nombreTag);

    cleanup:
    if (bloquesLogicos) string_array_destroy(bloquesLogicos);
    if (metadataConfig) config_destroy(metadataConfig);
    free(pathFile);
    free(pathTag);
    free(pathMetadata);
    unlock_file_metadata(nombreFile);
    return status;
}


t_list* blocks_to_list(char** blocks_str) {
    t_list* lista = list_create();
    if (blocks_str == NULL) return lista;

    for (int i = 0; blocks_str[i] != NULL; i++) {
        int* nro = malloc(sizeof(int));
        *nro = atoi(blocks_str[i]);
        list_add(lista, nro);
    }
    return lista;
}

char* list_to_string_array(t_list* lista) {
    char* str = string_new();
    string_append(&str, "[");
    
    for (int i = 0; i < list_size(lista); i++) {
        int* nro = list_get(lista, i);
        char* nro_str = string_itoa(*nro);
        string_append(&str, nro_str);
        if (i < list_size(lista) - 1) {
            string_append(&str, ",");
        }
        free(nro_str);
    }
    string_append(&str, "]");
    return str;
}

int truncate_file(uint32_t query_id, char* nombreFile, char* nombreTag, uint32_t nuevoTamanio){
    simularRetardoOperacion();
    lock_file_metadata(nombreFile);
    log_info(g_logger_storage, "QUERY ID: %d Iniciando operación TRUNCATE para file: %s, tag: %s, nuevoTamanio: %d",query_id, nombreFile, nombreTag, nuevoTamanio);

    char* pathFile = string_from_format("%s/files/%s", g_storage_config->punto_montaje, nombreFile);
    char* pathTag = string_from_format("%s/%s", pathFile, nombreTag);
    char* pathMetadata = string_from_format("%s/metadata.config", pathTag);

    int status = -1;
    t_config* metadata = NULL;
    t_list* lista_bloques = NULL;
    char** blocks_str = NULL;
    char* blocks_formatted = NULL;

    //Algunas validaciones

    struct stat st;
    if (stat(pathTag, &st) == -1) {
        log_error(g_logger_storage, "Error TRUNCATE: El File:Tag %s:%s no existe.", nombreFile, nombreTag);
        status = ERROR_FILE_NO_EXISTE;
        goto cleanup;
    }


    metadata = config_create(pathMetadata);
    if (!metadata) {
        log_error(g_logger_storage, "Error TRUNCATE: No se pudo abrir metadata.");
        goto cleanup;
    }

    //Me fijo que no este COMMITED

    char* estado = config_get_string_value(metadata, "ESTADO");
    if (strcmp(estado, "COMMITED") == 0) {
        log_error(g_logger_storage, "Error TRUNCATE: El archivo está COMMITED, no se puede modificar.");
        status = ERROR_ESCRITURA_NO_PERMITIDA;
        goto cleanup;
    }

    //Calculo el tamanio del bloque
    int block_size = g_superblock_config->block_size;

    //Me fijo que el nuevo tamanio sea multiplo del block_size
    if (nuevoTamanio % block_size != 0) {
        log_error(g_logger_storage, "Error TRUNCATE: El tamaño %d no es múltiplo del block_size %d", nuevoTamanio, block_size);
        goto cleanup;
    }


    int bloques_necesarios = nuevoTamanio / block_size;
    blocks_str = config_get_array_value(metadata, "BLOCKS");
    lista_bloques = blocks_to_list(blocks_str);
    int bloques_actuales = list_size(lista_bloques);

    log_debug(g_logger_storage, "Truncate: Bloques Actuales: %d -> Bloques Necesarios: %d", bloques_actuales, bloques_necesarios);
     

    //Ahora el truncate

    // AGRANDAR
    if (bloques_necesarios > bloques_actuales) {
        char* path_fisico_0 = string_from_format("%s/physical_blocks/block0000.dat", g_storage_config->punto_montaje);
        
        for (int i = bloques_actuales; i < bloques_necesarios; i++) {
            // 1. Creo hard link apuntando al bloque 0
            char* path_logico = string_from_format("%s/logical_blocks/%06d.dat", pathTag, i);
            
            if (link(path_fisico_0, path_logico) == -1) {
                log_error(g_logger_storage, "Error al crear link al bloque 0 para el bloque lógico %d", i);
                free(path_logico);
                free(path_fisico_0);
                goto cleanup; 
            }
            free(path_logico);

            // 2. Agrego '0' a la lista en memoria
            int* nuevo_bloque = malloc(sizeof(int));
            *nuevo_bloque = 0;
            list_add(lista_bloques, nuevo_bloque);
            
            // Log obligatorio: Hard Link Agregado
            log_info(g_logger_storage, "##<QUERY_ID %d> - Hard Link Agregado <File:Tag> %s:%s - Bloque Lógico %d ref a Bloque Físico 0", query_id, nombreFile, nombreTag, i);
        }
        free(path_fisico_0);
    } // ACHICAR 
    else if (bloques_necesarios < bloques_actuales) {
        for (int i = bloques_actuales - 1; i >= bloques_necesarios; i--) {
            // 1. Obtener qué bloque físico es (para ver si hay que liberarlo del bitmap)
            int* ptr_nro = list_get(lista_bloques, i);
            int bloque_fisico = *ptr_nro;

            // 2. Eliminar el hard link
            char* path_logico = string_from_format("%s/logical_blocks/%06d.dat", pathTag, i);
            if (unlink(path_logico) == -1) {
                log_error(g_logger_storage, "Error al borrar hard link lógico %d", i);
                free(path_logico);
                goto cleanup;
            }
            free(path_logico);
            
            // Log obligatorio: Hard Link Eliminado
            log_info(g_logger_storage, "##<QUERY_ID %d> - Hard Link Eliminado <File:Tag> %s:%s - Bloque Lógico %d ref a Bloque Físico %d", query_id, nombreFile, nombreTag, i, bloque_fisico);

            // 3. Verificar Bitmap
            liberarBloqueSiEsNecesario(query_id, bloque_fisico);

            // 4. Quitar de la lista (Memory cleanup del int*)
            free(list_remove(lista_bloques, i));
        }
    }

    // Guardo cambios en Metadata
    blocks_formatted = list_to_string_array(lista_bloques);
    char* size_str = string_itoa(nuevoTamanio);
    
    config_set_value(metadata, "BLOCKS", blocks_formatted);
    config_set_value(metadata, "TAMAÑO", size_str);
    config_save(metadata);
    
    status = 0;
    log_info(g_logger_storage, "##<QUERY_ID %d> - File Truncado %s:%s Nuevo Tamaño: %d", query_id, nombreFile, nombreTag, nuevoTamanio);

    // Cleanup local
    free(blocks_formatted);
    free(size_str);

    cleanup:
    if (metadata) config_destroy(metadata);
    if (lista_bloques) list_destroy_and_destroy_elements(lista_bloques, free);
    if (blocks_str) string_array_destroy(blocks_str);
    free(pathFile);
    free(pathTag);
    free(pathMetadata);
    unlock_file_metadata(nombreFile);
    return status;
}

int reservarBloqueLibre() {
    pthread_mutex_lock(&g_mutexBitmap);
    
    size_t max_bits = bitarray_get_max_bit(g_bitmap);
    int bloque_libre = -1;

    for (size_t i = 0; i < max_bits; i++) {
        if (!bitarray_test_bit(g_bitmap, i)) {
            bitarray_set_bit(g_bitmap, i);
            bloque_libre = (int)i;
            break;
        }
    }
    pthread_mutex_unlock(&g_mutexBitmap);
    mostrarBitmap();
    return bloque_libre;
}

int escribirFisicoDirecto(int bloqueFisico, void* buffer, int tam_buffer) {
    simularRetardoAccesoBloque();
    
    char* path = string_from_format("%s/physical_blocks/block%04d.dat", g_storage_config->punto_montaje, bloqueFisico);
    
    pthread_mutex_lock(&g_blocks_mutex[bloqueFisico]);

    int fd = open(path, O_WRONLY, 0664);
    if (fd == -1) {
        log_error(g_logger_storage, "Error abriendo bloque físico %d para escritura", bloqueFisico);
        free(path);
        // pthread_mutex_unlock(&g_mutex_blocks[bloque_fisico]);
        return -1;
    }

    ssize_t escrito = write(fd, buffer, tam_buffer);
    close(fd);
    
    pthread_mutex_unlock(&g_blocks_mutex[bloqueFisico]);

    free(path);
    return (escrito == tam_buffer) ? 0 : -1;
}

int escribirBloque(uint32_t query_id, char* nombreFile, char* nombreTag, uint32_t bloqueLogico, void* datos){
    simularRetardoOperacion(); 
    lock_file_metadata(nombreFile);
    log_info(g_logger_storage, "QUERY ID: %d Iniciando WRITE en %s:%s Bloque Lógico: %d", query_id, nombreFile, nombreTag, bloqueLogico);

    //Armo los paths
    char* pathFile = string_from_format("%s/files/%s", g_storage_config->punto_montaje, nombreFile);
    char* pathTag = string_from_format("%s/%s", pathFile, nombreTag);
    char* pathMetadata = string_from_format("%s/metadata.config", pathTag);

    //Status arranca en -1 como default, sera el valor que devolvamos
    int status = -1;
    t_config* metadata = NULL;
    char** blocks_str = NULL;
    t_list* lista_bloques = NULL;
    char* blocks_formatted = NULL;

    //Validaciones
    struct stat st;
    if (stat(pathTag, &st) == -1) {
        log_error(g_logger_storage, "Error WRITE: El File:Tag %s:%s no existe", nombreFile, nombreTag);
        status = ERROR_FILE_NO_EXISTE;
        goto cleanup;
    }

    metadata = config_create(pathMetadata);
    if (!metadata) {
        log_error(g_logger_storage, "Error WRITE: No se pudo leer metadata.");
        goto cleanup;
    }

    char* estado = config_get_string_value(metadata, "ESTADO");
    if (strcmp(estado, "COMMITED") == 0) {
        log_error(g_logger_storage, "Error WRITE: El archivo está COMMITED.");
        status = ERROR_ESCRITURA_NO_PERMITIDA;
        goto cleanup;
    }

    //2. Obtener lista de bloques y verificar rango
    blocks_str = config_get_array_value(metadata, "BLOCKS");
    lista_bloques = blocks_to_list(blocks_str);

    if (bloqueLogico >= list_size(lista_bloques)) {
        log_error(g_logger_storage, "Error WRITE: Bloque lógico %d fuera de rango.", bloqueLogico);
        status = ERROR_FUERA_DE_LIMITE;
        goto cleanup;
    }

    // Obtener el bloque físico correspondiente a partir del bloque lógico del file
    int* ptr_bloque = list_get(lista_bloques, bloqueLogico);
    int bloque_fisico_actual = *ptr_bloque;

    // 3. Analizar Referencias (st_nlink)
    char* path_fisico_actual = string_from_format("%s/physical_blocks/block%04d.dat", g_storage_config->punto_montaje, bloque_fisico_actual);
    if (stat(path_fisico_actual, &st) == -1) {
        log_error(g_logger_storage, "Error interno: stat falló en bloque físico %d", bloque_fisico_actual);
        free(path_fisico_actual);
        goto cleanup;
    }
    free(path_fisico_actual);

    int nlink = st.st_nlink;

    // 4. Lógica Copy-On-Write
    if (nlink > 1) {
        log_debug(g_logger_storage, "Bloque %d compartido (nlink=%d). Aplicando Copy-On-Write...", bloque_fisico_actual, nlink);

        // 4.1 Reservar bloque nuevo
        int bloque_nuevo = reservarBloqueLibre();
        if (bloque_nuevo == -1) {
            log_error(g_logger_storage, "Error WRITE: Espacio insuficiente (Bitmap lleno).");
            status = ERROR_ESPACIO_INSUFICIENTE;
            goto cleanup;
        }
        
        // Log Obligatorio: Reservado
        log_info(g_logger_storage, "##<QUERY_ID %d> - Bloque Físico Reservado %d", query_id, bloque_nuevo);

        // 4.2 Escribir en el nuevo bloque
        if (escribirFisicoDirecto(bloque_nuevo, datos, g_superblock_config->block_size) == -1) {
            // Rollback: Liberar el bloque si falló la escritura. Al reservarlo se pone en 1, lo volvemos a 0 si falló.
            pthread_mutex_lock(&g_mutexBitmap);
            bitarray_clean_bit(g_bitmap, bloque_nuevo);
            pthread_mutex_unlock(&g_mutexBitmap);
            goto cleanup;
        }

        // 4.3 Actualizar Hard Link Lógico (Unlink viejo -> Link nuevo)
        char* path_logico = string_from_format("%s/logical_blocks/%06d.dat", pathTag, bloqueLogico);
        char* path_fisico_nuevo = string_from_format("%s/physical_blocks/block%04d.dat", g_storage_config->punto_montaje, bloque_nuevo);
        
        //Borramos el link viejo y chequeamos error para evitar inconsistencias
        if (unlink(path_logico) == -1) {
             log_error(g_logger_storage, "Error critico: No se pudo borrar el link viejo. Abortando Copy-On-Write.");
             // Rollback del bitmap
             pthread_mutex_lock(&g_mutexBitmap);
             bitarray_clean_bit(g_bitmap, bloque_nuevo);
             pthread_mutex_unlock(&g_mutexBitmap);
             
             free(path_logico); free(path_fisico_nuevo);
             status = -7; //Cambiar por error posta. TODO
             goto cleanup;
        }
        
        if (link(path_fisico_nuevo, path_logico) == -1) {
             log_error(g_logger_storage, "Error crítico creando link al nuevo bloque %d. INICIANDO ROLLBACK.", bloque_nuevo);
             
             //  INICIO ROLLBACK
             
             // 1. Restaurar el enlace al bloque FISICO ACTUAL (el compartido/viejo)
             char* path_fisico_viejo = string_from_format("%s/physical_blocks/block%04d.dat", g_storage_config->punto_montaje, bloque_fisico_actual);
             
             if (link(path_fisico_viejo, path_logico) == -1) {
                 // Si esto falla, el FS quedó corrupto de verdad (perdimos la referencia).
                 log_error(g_logger_storage, "FATAL: Falló el rollback. El archivo %s:%s quedó corrupto (falta bloque lógico %d).", nombreFile, nombreTag, bloqueLogico);
             } else {
                 log_info(g_logger_storage, "Rollback exitoso: Enlace restaurado al bloque original %d.", bloque_fisico_actual);
             }
             free(path_fisico_viejo);

             // 2. Liberar el bloque nuevo que habíamos reservado (ya no sirve)
             pthread_mutex_lock(&g_mutexBitmap);
             bitarray_clean_bit(g_bitmap, bloque_nuevo);
             pthread_mutex_unlock(&g_mutexBitmap);

             //FIN ROLLBACK

             free(path_logico);
             free(path_fisico_nuevo);
             status = -7; //Cambiar por error posta. TODO
             goto cleanup;
        }
        
        free(path_logico);
        free(path_fisico_nuevo);

        // 4.4 Actualizar Metadata en Memoria y Disco
        *ptr_bloque = bloque_nuevo; // Actualizamos el valor en la lista
        
        blocks_formatted = list_to_string_array(lista_bloques);
        config_set_value(metadata, "BLOCKS", blocks_formatted);
        config_save(metadata);
        free(blocks_formatted);

    } else {
        // Bloque Propio (nlink == 1): Sobrescribir directo 
        log_debug(g_logger_storage, "Bloque %d exclusivo. Sobrescribiendo...", bloque_fisico_actual);
        if (escribirFisicoDirecto(bloque_fisico_actual, datos, g_superblock_config->block_size) == -1) {
            goto cleanup;
        }
    }

    // 5. Log Obligatorio Final
    log_info(g_logger_storage, "##<QUERY_ID %d> - Bloque Lógico Escrito <File:Tag> %s:%s - Número de Bloque: %d ", query_id, nombreFile, nombreTag, bloqueLogico);
    status = 0;

    cleanup:
    if (metadata) config_destroy(metadata);
    if (lista_bloques) list_destroy_and_destroy_elements(lista_bloques, free);
    if (blocks_str) string_array_destroy(blocks_str);
    free(pathFile);
    free(pathTag);
    free(pathMetadata);
    unlock_file_metadata(nombreFile);
    return status;
}

char* obtener_hash_bloque(int numeroBloque) {
    char* path = string_from_format("%s/physical_blocks/block%04d.dat", g_storage_config->punto_montaje, numeroBloque);
    
    // Leemos el contenido entero del bloque
    pthread_mutex_lock(&g_blocks_mutex[numeroBloque]);
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        free(path);
        return NULL;
    }

    struct stat st;
    fstat(fd, &st);
    void* buffer = malloc(st.st_size);
    read(fd, buffer, st.st_size);
    close(fd);
    pthread_mutex_unlock(&g_blocks_mutex[numeroBloque]);
    free(path);


    char* hash = crypto_md5(buffer, st.st_size); 
    
    free(buffer);
    return hash;
}

int commitFile(uint32_t query_id, char* nombreFile, char* nombreTag) {
    simularRetardoOperacion();
    log_info(g_logger_storage, "QUERY ID: %d Iniciando COMMIT para %s:%s", query_id, nombreFile, nombreTag);

    // Lock de Metadata 
    lock_file_metadata(nombreFile);

    char* pathFile = string_from_format("%s/files/%s", g_storage_config->punto_montaje, nombreFile);
    char* pathTag = string_from_format("%s/%s", pathFile, nombreTag);
    char* pathMetadata = string_from_format("%s/metadata.config", pathTag);

    int status = -1;
    t_config* metadata = NULL;
    t_list* lista_bloques = NULL;
    char** blocks_str = NULL;
    bool hubo_cambios = false;

    struct stat st;
    if (stat(pathTag, &st) == -1) {
        log_error(g_logger_storage, "Error COMMIT: El File:Tag %s:%s no existe.", nombreFile, nombreTag);
        status = ERROR_FILE_NO_EXISTE;
        goto cleanup_sin_config;
    }

    metadata = config_create(pathMetadata);

    // Check si ya está commited
    char* estado_actual = config_get_string_value(metadata, "ESTADO");
    if (strcmp(estado_actual, "COMMITED") == 0) {
        log_info(g_logger_storage, "El archivo ya estaba COMMITED. Operación ignorada.");
        status = 0;
        goto cleanup;
    }

    // 2. Cambiar estado a COMMITED
    config_set_value(metadata, "ESTADO", "COMMITED");
    log_info(g_logger_storage, "##<QUERY_ID %d> - Commit de File: Tag %s:%s", query_id, nombreFile, nombreTag);

    // 3. Ciclo de Deduplicación
    blocks_str = config_get_array_value(metadata, "BLOCKS");
    lista_bloques = blocks_to_list(blocks_str); 

    for (int i = 0; i < list_size(lista_bloques); i++) {
        int* ptr_bloque = list_get(lista_bloques, i);
        int bloque_actual = *ptr_bloque;

        // A. Calcular Hash
        char* hash = obtener_hash_bloque(bloque_actual);
        if (!hash) {
            log_error(g_logger_storage, "Error calculando hash para bloque %d", bloque_actual);
            status = -1;
            goto cleanup;
        }

        // B. Buscar en Índice (Zona Crítica Global)
        pthread_mutex_lock(&g_mutexHashIndex);
        
        if (config_has_property(g_hash_config, hash)) {
            // --- CASO DUPLICADO: DEDUPLICAR ---
            char* bloque_original_str = config_get_string_value(g_hash_config, hash); // Ej: "block0010"
            int bloque_original = atoi(bloque_original_str + 5); // +5 para saltar "block"

            // Si el bloque actual NO es el original, procedemos a unificar
            if (bloque_actual != bloque_original) {
                log_debug(g_logger_storage, "Deduplicando: Bloque %d es igual al %d (Hash: %s)", bloque_actual, bloque_original, hash);

                // 1. Hard Link: Borrar el actual, linkear al original
                char* path_logico = string_from_format("%s/logical_blocks/%06d.dat", pathTag, i);
                char* path_fisico_original = string_from_format("%s/physical_blocks/block%04d.dat", g_storage_config->punto_montaje, bloque_original);
                
                unlink(path_logico);
                link(path_fisico_original, path_logico);
                
                free(path_logico); free(path_fisico_original);

                // 2. Liberar el bloque físico actual si quedó huérfano
                liberarBloqueSiEsNecesario(query_id, bloque_actual);

                // 3. Actualizar lista en memoria
                *ptr_bloque = bloque_original;
                hubo_cambios = true;

                // Log obligatorio
                log_info(g_logger_storage, "##<QUERY_ID %d> - Deduplicación de Bloque <File:Tag> %s:%s - Bloque Lógico %d se reasigna de %d a %d", 
                         query_id, nombreFile, nombreTag, i, bloque_actual, bloque_original);
            }

        } else {
            // --- CASO NUEVO: REGISTRAR ---
            char* valor_bloque = string_from_format("block%04d", bloque_actual);
            config_set_value(g_hash_config, hash, valor_bloque);
            config_save(g_hash_config); // Guardo el archivo de hashes inmediatamente
            free(valor_bloque);
        }

        pthread_mutex_unlock(&g_mutexHashIndex);
        free(hash);
    }

    // 4. Guardar cambios finales en Metadata
    if (hubo_cambios) {
        char* blocks_formatted = list_to_string_array(lista_bloques); 
        config_set_value(metadata, "BLOCKS", blocks_formatted);
        free(blocks_formatted);
    }
    
    config_save(metadata);
    status = 0;

cleanup:
    if (metadata) config_destroy(metadata);
    if (lista_bloques) list_destroy_and_destroy_elements(lista_bloques, free);
    if (blocks_str) string_array_destroy(blocks_str);
    
cleanup_sin_config:
    free(pathFile);
    free(pathTag);
    free(pathMetadata);
    
    unlock_file_metadata(nombreFile); // Liberar el lock de archivo
    return status;
}
