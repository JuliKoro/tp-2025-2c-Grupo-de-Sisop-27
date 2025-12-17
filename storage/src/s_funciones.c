#include "s_funciones.h"

pthread_mutex_t g_mutexBitmap;

pthread_mutex_t g_mutexHashIndex;

t_bitarray* g_bitmap = NULL;

t_config* g_hash_config = NULL;

pthread_mutex_t g_metadata_locks[128];

pthread_mutex_t* g_blocks_mutex = NULL;

pthread_mutex_t g_mutexCantidadWorkers;




void inicializarSemaforos() {
    //Por ahora solo tenemos el mutex del bitmap

    log_debug(g_logger_storage, "Inicializando semaforos...");
    pthread_mutex_init(&g_mutexBitmap, NULL);
    pthread_mutex_init(&g_mutexHashIndex, NULL);
    pthread_mutex_init(&g_mutexCantidadWorkers, NULL);

    //Inicializamos los mutex para los metadata.config (128 posibles archivos)
    for(int i = 0; i < 128; i++) {
        pthread_mutex_init(&g_metadata_locks[i], NULL);
    }

    if (g_superblock_config == NULL) {
        log_error(g_logger_storage, "Error CRITICO: Intentando inicializar mutex de bloques sin superblock cargado.");
        return;
    }

    int cantidad_bloques = g_superblock_config->cantidad_bloques;
    
    // Asignamos memoria exacta para N mutexes
    g_blocks_mutex = malloc(sizeof(pthread_mutex_t) * cantidad_bloques);
    
    if (g_blocks_mutex == NULL) {
        log_error(g_logger_storage, "Error: No se pudo reservar memoria para mutex de bloques.");
        return;
    }

    // Inicializamos uno por uno
    for (int i = 0; i < cantidad_bloques; i++) {
        pthread_mutex_init(&g_blocks_mutex[i], NULL);
    }
    
    log_debug(g_logger_storage, "Semaforos inicializados. Mutex de bloques creados: %d", cantidad_bloques);
}

void destruirSemaforos() {
    pthread_mutex_destroy(&g_mutexBitmap);
    pthread_mutex_destroy(&g_mutexHashIndex);
    pthread_mutex_destroy(&g_mutexCantidadWorkers);

    for(int i = 0; i < 128; i++) {
        pthread_mutex_destroy(&g_metadata_locks[i]);
    }

    if (g_blocks_mutex != NULL) {
        for (int i = 0; i < g_superblock_config->cantidad_bloques; i++) {
            pthread_mutex_destroy(&g_blocks_mutex[i]);
        }
        free(g_blocks_mutex); // Liberar el malloc
        g_blocks_mutex = NULL;
    }
}

int hash_str(char* str) {
    int sum = 0;
    while (*str) sum += *str++;
    return sum;
}

void lock_file_metadata(char* nombreFile) {
    int index = hash_str(nombreFile) % 128;
    pthread_mutex_lock(&g_metadata_locks[index]);
}

void unlock_file_metadata(char* nombreFile) {
    int index = hash_str(nombreFile) % 128;
    pthread_mutex_unlock(&g_metadata_locks[index]);
}

void mostrarBitmap(){
    size_t cantidad_bits = bitarray_get_max_bit(g_bitmap);
    char* resultado = malloc(cantidad_bits + 1); // +1 para el terminador '\0'
    if (resultado == NULL) {
        log_debug(g_logger_storage, "No se pudo alocar memoria para mostrar el bitmap");
        return; // No se pudo alocar memoria
    }

    pthread_mutex_lock(&g_mutexBitmap); // Protegemos la lectura
    for (size_t i = 0; i < cantidad_bits; i++) {
        resultado[i] = bitarray_test_bit(g_bitmap, i) ? '1' : '0';
    }
    pthread_mutex_unlock(&g_mutexBitmap);

    resultado[cantidad_bits] = '\0'; // Agregamos el terminador del string

    log_debug(g_logger_storage, "Bitmap actual: %s", resultado);

    free(resultado);

}

void copiarArchivo(char* origen, char* destino) {
    //Primero abro el archivo de origen en modo lectura

    int fd_origen =  open(origen, O_RDONLY);
    if(fd_origen == -1){
        log_error(g_logger_storage, "CopiarArchivo: Error al abrir el archivo de origen %s", origen);
        return;
    }
    

    //Luego creo y abro el archivo de destino para escritura
    int fd_destino = open(destino, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if(fd_destino == -1){
        log_error(g_logger_storage, "CopiarArchivo: Error al crear y abrir el archivo de destino %s", destino);
        return;
    }

    //Luego leo el archivo de origen y lo escribo en el de destino
    char buffer[4096];
    ssize_t bytes_leidos;
    while((bytes_leidos = read(fd_origen, buffer, sizeof(buffer))) > 0){
        if(write(fd_destino, buffer, bytes_leidos) != bytes_leidos){
            log_error(g_logger_storage, "Error al escribir en el archivo de destino %s", destino);
            break;
        }
    }

    //Luego cierro los archivos
    close(fd_origen);
    close(fd_destino);

    if(bytes_leidos == -1){
        log_error(g_logger_storage, "Error al leer el archivo de origen %s", origen);
    } else {
        log_info(g_logger_storage, "Archivo %s copiado exitosamente a %s", origen, destino);
    }
}

void crearArchivo(char* path, off_t tamanio){
    log_info(g_logger_storage, "Creando el archivo %s", path);

    int fd = open(path, O_CREAT | O_WRONLY, 0666);
    if(fd == -1){
        log_error(g_logger_storage, "Error al crear el archivo %s", path);
        return;
    }

    if(tamanio > 0){
        if(ftruncate(fd, tamanio) == -1){
            log_error(g_logger_storage, "Error al establecer el tamanio del archivo %s", path);
            close(fd);
            return;
        }
    }
    close(fd);
    log_info(g_logger_storage, "Archivo %s creado exitosamente", path);
}

void crearBloques(){
    log_debug(g_logger_storage, "Creando %d bloques...", g_superblock_config->cantidad_bloques);
    char* path_bloques = string_duplicate(g_storage_config->punto_montaje);
    string_append(&path_bloques, "/physical_blocks");

    for(int i=0; i<g_superblock_config->cantidad_bloques; i++){
        char* rutaCompleta = string_from_format("%s/block%04d.dat",path_bloques,i);
        int fd = open(rutaCompleta, O_CREAT | O_RDWR, 0664); //TODO: Revisar permisos

        if (fd != -1) {
            // Establecer el tamaño del archivo a los bytes que indique el superblock.config
            ftruncate(fd, g_superblock_config->block_size); 
            close(fd);
        } else {
            log_error(g_logger_storage, "open devolvio error al crear bloque %s", rutaCompleta);
         }
        free(rutaCompleta);
    }
    log_debug(g_logger_storage, "Loop de creacion de %d bloques terminado", g_superblock_config->cantidad_bloques);
    return;
}

void escribirMetadataConfig(char* path, int tamanio, int* blocks, int cantidadBloques, char* estado){
    t_config* config = config_create(path);
    if(config == NULL){
        log_error(g_logger_storage, "Error al crear el t_config para el archivo metadata.config en %s", path);
        return;
    }
    //Escribimos el tamanio
    char* tamanio_str = string_itoa(tamanio);
    config_set_value(config, "TAMAÑO", tamanio_str);
    free(tamanio_str);
    //Escribimos el array de bloques

    char* blocks_str = string_new();
    string_append(&blocks_str, "[");
    for(int i = 0; i < cantidadBloques; i++){
        char* num_str = string_itoa(blocks[i]);
        string_append(&blocks_str, num_str);
        free(num_str);
        
        if(i < cantidadBloques - 1){
            string_append(&blocks_str, ",");
        }
    }
    string_append(&blocks_str, "]");
    config_set_value(config, "BLOCKS", blocks_str);
    

    //Escribimos el estado
    config_set_value(config, "ESTADO", estado);

    config_save(config);
    config_destroy(config);
    log_info(g_logger_storage, "\n Metadata.config creado/actualizado en %s", path);
    log_debug(g_logger_storage, "TAMAÑO: %d", tamanio);
    log_debug(g_logger_storage, "BLOCKS: %s", blocks_str);
    log_debug(g_logger_storage, "ESTADO: %s", estado);

    free(blocks_str);
    
}

void escribirDatosBloque(int numeroBloque, void* datos, size_t tamanioDatos){
    char* pathBloque = string_from_format("%s/physical_blocks/block%04d.dat", g_storage_config->punto_montaje, numeroBloque);

    log_debug(g_logger_storage, "Escribiendo en el bloque %d en %s", numeroBloque, pathBloque);
    int fd = open(pathBloque, O_WRONLY, 0664);
    if (fd == -1) {
        log_error(g_logger_storage, "No se pudo abrir el bloque %d en %s", numeroBloque, pathBloque);
        free(pathBloque);
        return;
    }

    ssize_t bytesEscritos = write(fd, datos, tamanioDatos);
    if(bytesEscritos == -1){
        log_error(g_logger_storage, "Error al escribir en el bloque %d en %s", numeroBloque, pathBloque);
    } else if((size_t)bytesEscritos < tamanioDatos){
        log_warning(g_logger_storage, "Se escribieron menos bytes de los esperados en el bloque %d en %s. Esperados: %zu, Escritos: %zd", numeroBloque, pathBloque, tamanioDatos, bytesEscritos);
    } else {
        log_info(g_logger_storage, "Bloque %d escrito exitosamente en %s. Bytes escritos: %zd", numeroBloque, pathBloque, bytesEscritos);
    }

    close(fd);
    free(pathBloque);
}

void inicializarBitmap(char* path){
    log_debug(g_logger_storage, "Creando el archivo bitmap.bin en %s", path);
    char* bitmap_path = string_duplicate(path);
    string_append(&bitmap_path, "/bitmap.bin");
    size_t bitmapSize =( g_superblock_config ->cantidad_bloques + 7)/8; //Sumamos 7 para redondear hacia arriba al byte mas cercano. dividimos por 8 para obtener la cantidad de bytes necesarios.
     
    log_info(g_logger_storage, "Creando el archivo %s", path);

    int fd = open(bitmap_path, O_CREAT | O_RDWR, 0666);
    if(fd == -1){
        log_error(g_logger_storage, "Error al crear el archivo %s", path);
        return;
    }

    if(ftruncate(fd, bitmapSize) == -1){
        log_error(g_logger_storage, "Error al establecer el tamanio del archivo %s", bitmap_path);
        close(fd);
        return;
    }
    log_info(g_logger_storage, "Archivo %s creado exitosamente", bitmap_path);
    
    log_debug(g_logger_storage, "Inicializando el bitmap en memoria con mmap...");
    void* bitmap_data = mmap(NULL, bitmapSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (bitmap_data == MAP_FAILED) {
        log_error(g_logger_storage, "No se pudo mapear el bitmap a memoria");
        close(fd);
        free(bitmap_path);
        return; // Error crítico
    }
    //Ya mapeamos, cerramos el fd
    close(fd);

    g_bitmap = bitarray_create_with_mode(bitmap_data, bitmapSize, LSB_FIRST);
    log_info(g_logger_storage, "Bitmap mapeado en memoria con %zu bytes, estructura inicializada", bitmapSize);


    log_debug(g_logger_storage, "Marcando el bloque 0 como ocupado para initial_file...");
    pthread_mutex_lock(&g_mutexBitmap);
    if(!bitarray_test_bit(g_bitmap, 0)){
        bitarray_set_bit(g_bitmap, 0);
        log_info(g_logger_storage, "Bloque 0 marcado como ocupado para initial_file.");
    }
    pthread_mutex_unlock(&g_mutexBitmap);

    mostrarBitmap();

    free(bitmap_path);

}

void cargarBitmap(char* path){
    log_info(g_logger_storage, "Cargando bitmap desde %s", path);
    char* bitmap_path = string_duplicate(path);
    string_append(&bitmap_path, "/bitmap.bin");
    size_t bitmapSize =( g_superblock_config ->cantidad_bloques + 7)/8; //Sumamos 7 para redondear hacia arriba al byte mas cercano. dividimos por 8 para obtener la cantidad de bytes necesarios.

    log_info(g_logger_storage, "Abriendo el archivo %s para cargar el bitmap", bitmap_path);

    int fd = open(bitmap_path, O_RDWR, 0666); 
    if(fd == -1){
        log_error(g_logger_storage, "Error CRITICO: No se pudo abrir el bitmap.bin en %s. ¿Está corrupto el FS?", bitmap_path);
        free(bitmap_path);
        return;
    }

    log_debug(g_logger_storage, "Mapeando el bitmap a memoria con mmap...");
    void* bitmap_data = mmap(NULL, bitmapSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (bitmap_data == MAP_FAILED) {
        log_error(g_logger_storage, "Error CRITICO: No se pudo mapear el bitmap a memoria. ¿Está corrupto el FS?");
        close(fd);
        free(bitmap_path);
        return; // Error crítico
    }
    //Ya mapeamos, cerramos el fd
    close(fd);

    g_bitmap = bitarray_create_with_mode(bitmap_data, bitmapSize, LSB_FIRST);
    log_info(g_logger_storage, "Bitmap mapeado en memoria con %zu bytes, estructura inicializada", bitmapSize);

    mostrarBitmap();

    free(bitmap_path);

   
}

void inicializarHashIndex(){
    log_debug(g_logger_storage, "Inicializando el hash index...");

    char* hash_index_path = string_duplicate(g_storage_config->punto_montaje);
    string_append(&hash_index_path, "/blocks_hash_index.config");

    g_hash_config = config_create(hash_index_path);
    if(g_hash_config == NULL){
        log_error(g_logger_storage, "Error al crear el t_config para el archivo blocks_hash_index.config en %s", hash_index_path);
        free(hash_index_path);
        return;
    }

    log_info(g_logger_storage, "Archivo blocks_hash_index.config cargado exitosamente desde %s", hash_index_path);
    free(hash_index_path);
}

char* calcularHash(int numeroBloque){
    char* pathBloque = string_from_format("%s/physical_blocks/block%04d.dat", g_storage_config->punto_montaje, numeroBloque);

    int fd = open(pathBloque, O_RDONLY);
    if (fd == -1) {
        log_error(g_logger_storage, "No se pudo abrir el bloque %d para calcular su hash.", numeroBloque);
        free(pathBloque);
        return NULL;
    }

    size_t tamanio_bloque = g_superblock_config->block_size;
    void* buffer = malloc(tamanio_bloque);
    read(fd, buffer, tamanio_bloque);
    close(fd);

    char* hashCalculado = crypto_md5(buffer, tamanio_bloque);
    
    log_debug(g_logger_storage, "Hash para bloque %d calculado: %s", numeroBloque, hashCalculado);

    free(buffer);
    free(pathBloque);

    return hashCalculado;
}

void escribirHashindex(int numeroBloque){
    char* hashCalculado = calcularHash(numeroBloque);
    if(hashCalculado == NULL){
        log_error(g_logger_storage, "No se pudo calcular el hash para el bloque %d. No se actualizara el hash index.", numeroBloque);
        return;
    }

    char* nombreBloqueStr = string_from_format("block%04d", numeroBloque);

    pthread_mutex_lock(&g_mutexHashIndex);
    config_set_value(g_hash_config, hashCalculado, nombreBloqueStr);
    pthread_mutex_unlock(&g_mutexHashIndex);

    log_info(g_logger_storage, "Hash index actualizado para el bloque %d: %s", numeroBloque, hashCalculado);
    //Testing, despues tengo que borrar estas lineas, se hace al final del main
    pthread_mutex_lock(&g_mutexHashIndex);
    config_save(g_hash_config); 
    pthread_mutex_unlock(&g_mutexHashIndex);
    //Testing, despues tengo que borrar estas lineas, se hace al final del main
    free(hashCalculado);
    free(nombreBloqueStr);
}

void crearHardLink(char* nombreFile, char* nombreTag){
    log_debug(g_logger_storage, "Creando hard link para el archivo %s en el tag %s", nombreFile, nombreTag);

    char* pathMetadata = string_from_format("%s/files/%s/%s/metadata.config", g_storage_config->punto_montaje, nombreFile, nombreTag);
    
    t_config* metadataConfig = config_create(pathMetadata);
    if(metadataConfig == NULL){
        log_error(g_logger_storage, "Error al abrir el metadata.config en %s para crear hard link", pathMetadata);
        free(pathMetadata);
        return;
    }

    char** blocksArray = config_get_array_value(metadataConfig, "BLOCKS");
    if(blocksArray == NULL){
        log_error(g_logger_storage, "Error al obtener el array de bloques del metadata.config en %s para crear hard link", pathMetadata);
        config_destroy(metadataConfig);
        free(pathMetadata);
        return;
    }

    int cantidadBloques = 0;
    while(blocksArray[cantidadBloques] != NULL){
        cantidadBloques++;
    }

    for(int i = 0; i < cantidadBloques; i++){
        int numeroBloque = atoi(blocksArray[i]);
        char* rutaFisica = string_from_format("%s/physical_blocks/block%04d.dat", g_storage_config->punto_montaje, numeroBloque);
        char* rutaLogica = string_from_format("%s/files/%s/%s/logical_blocks/%06d.dat", g_storage_config->punto_montaje, nombreFile, nombreTag, i);
        
        if(link(rutaFisica, rutaLogica) == -1){
        log_error(g_logger_storage, "Error al crear hard link de %s a %s", rutaFisica, rutaLogica);
        } else {
        log_info(g_logger_storage, "Hard link creado exitosamente de %s a %s", rutaFisica, rutaLogica);
        }
        free(rutaFisica);
        free(rutaLogica);
    }


    string_array_destroy(blocksArray);
    config_destroy(metadataConfig);
    free(pathMetadata);
  
}

void inicializarPuntoMontaje(char* path){
    struct stat st = {0};
    //Nos fijamos si existe la ruta
    if(stat(path, &st) == 0){
        log_info(g_logger_storage, "El punto de montaje %s ya existe. Eliminandolo para fresh start", path);

        //Procedemos a borrar

        char comando [512];
        snprintf(comando, sizeof(comando), "rm -rf %s", path);

        if(system(comando) != 0){
            log_error(g_logger_storage, "Error al eliminar el punto de montaje %s", path);
            exit(EXIT_FAILURE);
        } else {
            log_debug(g_logger_storage, "Punto de montaje anterior borrado con exito.");
        }
    }

    //Creamos el punto de montaje
    log_info(g_logger_storage, "Creando el punto de montaje en %s", path);
    if(mkdir(path, 0777) == -1){
        log_error(g_logger_storage, "Error al crear el punto de montaje en %s", path);
        exit(EXIT_FAILURE);
    } else {
        log_info(g_logger_storage, "Punto de montaje %s creado exitosamente", path);
    }


    log_debug(g_logger_storage, "Creando el archivo blocks_hash_index.config en %s", path);
    char* blocks_hash_index_path = string_duplicate(path);
    string_append(&blocks_hash_index_path, "/blocks_hash_index.config");
    crearArchivo(blocks_hash_index_path,0);
    free(blocks_hash_index_path);
    
    //Inicializamos el hash index
    inicializarHashIndex();
    
    log_info(g_logger_storage, "Creando directorios nativos en %s...", path);
    char* physical_blocks = string_duplicate(path);
    string_append(&physical_blocks, "/physical_blocks");
    if(mkdir(physical_blocks, 0777) == -1){
        log_error(g_logger_storage, "Error al crear el directorio physical_blocks en %s", path);
        exit(EXIT_FAILURE);
    } else {
        log_info(g_logger_storage, "Directorio physical_blocks creado exitosamente en %s", path);
    }
    char* files_folder = string_duplicate(path);
    string_append(&files_folder, "/files");
    if(mkdir(files_folder, 0777) == -1){
        log_error(g_logger_storage, "Error al crear el directorio files en %s", path);
        exit(EXIT_FAILURE);
    } else {
        log_info(g_logger_storage, "Directorio files creado exitosamente en %s", path);
    }
    
    log_debug(g_logger_storage, "Llamando a crearBloques()...");
    crearBloques();

    //Escribimos el bloque 0 con 0s
    size_t tamanioBloque = g_superblock_config->block_size;
    void* bufferCeros = malloc(tamanioBloque);
    memset(bufferCeros, 0, tamanioBloque);
    escribirDatosBloque(0, bufferCeros, tamanioBloque);
    free(bufferCeros);

    //Inicializamos el bitmap
    inicializarBitmap(path);

    //creacion de archivo inicial
    char* initial_file = string_duplicate(files_folder);
    string_append(&initial_file, "/initial_file");
    if(mkdir(initial_file, 0777) == -1){
        log_error(g_logger_storage, "Error al crear el directorio initial_file en %s", initial_file);
        exit(EXIT_FAILURE);
    } else {
        log_info(g_logger_storage, "Directorio initial_file creado exitosamente en %s", initial_file);
    }


    string_append(&initial_file, "/BASE");
    if(mkdir(initial_file, 0777) == -1){
        log_error(g_logger_storage, "Error al crear el directorio BASE en %s", initial_file);
        exit(EXIT_FAILURE);
    } else {
        log_info(g_logger_storage, "Directorio BASE creado exitosamente en %s", initial_file);
    }

    char* metadataConfigPath = string_duplicate(initial_file);;
    string_append(&metadataConfigPath, "/metadata.config");
    crearArchivo(metadataConfigPath,0);
    int initial_blocks[1] = {0};
    escribirMetadataConfig(metadataConfigPath, g_superblock_config->block_size, initial_blocks, 1, "COMMITED");

    string_append(&initial_file, "/logical_blocks");
    if(mkdir(initial_file, 0777) == -1){
        log_error(g_logger_storage, "Error al crear el directorio logical_blocks en %s", initial_file);
        exit(EXIT_FAILURE);
    } else {
        log_info(g_logger_storage, "Directorio logical_blocks creado exitosamente en %s", initial_file);
    }

    //Escribimos el hash del bloque 0 en el hash index
    escribirHashindex(0);
    //Creamos los hard links del initial_file
    crearHardLink("initial_file", "BASE");

    log_info(g_logger_storage, "Directorios nativos y bloques creados exitosamente. Liberando memoria asignada dinamicamente para este procedimiento");
    free(physical_blocks);
    free(files_folder);
    free(initial_file);
    free(metadataConfigPath);

}

void cargarPuntoMontaje(char* path){
    log_info(g_logger_storage, "Cargando punto de montaje desde %s", path);

     struct stat st = {0};
    //Nos fijamos si existe la ruta
    if(stat(path, &st) != 0){
        log_error(g_logger_storage, "El punto de montaje %s no existe. Correr modulo bajo FRESH START", path);
        return;
    }
    
    //Cargamos el superblock.config que fue copiado a nuestro FS
    char* path_superblock = string_duplicate(g_storage_config->punto_montaje);
    string_append(&path_superblock, "/");
    string_append(&path_superblock, "superblock.config");
    g_superblock_config = get_configs_superblock(path_superblock);
    free(path_superblock);
    if (g_superblock_config == NULL) {
        log_error(g_logger_storage, "Error CRITICO: No se pudo leer 'superblock.config' desde el punto de montaje. FS corrupto.");
        return; // Error
    }
    log_debug(g_logger_storage, "Superbloque cargado. FS_SIZE: %d, BLOCK_SIZE: %d, BLOCK_COUNT: %d",
             g_superblock_config->fs_size,
             g_superblock_config->block_size,
             g_superblock_config->cantidad_bloques);
             
    //Inicializamos el hash index, la funcion es reutilizable para este caso
    inicializarHashIndex();
    if (g_hash_config == NULL) {
        log_error(g_logger_storage, "Error CRITICO: No se pudo leer 'blocks_hash_index.config'. FS corrupto.");
        return; // Error
    }
    //Cargamos el bitmap
    cargarBitmap(path);
    if (g_bitmap == NULL) {
        log_error(g_logger_storage, "Error CRITICO: No se pudo leer 'bitmap.bin'. FS corrupto.");
        return; // Error
    }


    log_info(g_logger_storage, "Punto de montaje cargado exitosamente desde %s", path);

   
}
