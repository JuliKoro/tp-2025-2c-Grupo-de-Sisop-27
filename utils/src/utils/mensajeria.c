#include "mensajeria.h"

/***********************************************************************************************************************/
/***                                                    SOCKETS                                                      ***/
/***********************************************************************************************************************/

int iniciar_servidor(const char* puerto){
    int socket_servidor; 
    struct addrinfo hints, *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; //IPV4
    hints.ai_socktype = SOCK_STREAM; //TCP
    hints.ai_flags = AI_PASSIVE;

    int err = getaddrinfo(NULL, puerto, &hints, &server_info);
    if(err !=0){
        fprintf(stderr, "Fallo getaddrinfo - %s\n", gai_strerror(err));
    }

    socket_servidor = socket(server_info->ai_family,
                            server_info->ai_socktype,
                            server_info->ai_protocol);
    if(socket_servidor == -1){
        fprintf(stderr, "Error al crear el socket: %s\n", strerror(errno));
        freeaddrinfo(server_info);
        return -1;
    }             
    
    //reusa puertos
    setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int));

    //bindeo socket a puerto
    if(bind(socket_servidor, server_info->ai_addr, server_info->ai_addrlen) == -1){
        fprintf(stderr, "Error en el bind: %s\n", strerror(errno));
        close(socket_servidor);
        freeaddrinfo(server_info);
        return -1;
    }

    //escucho
    if(listen(socket_servidor, SOMAXCONN) == -1){
        fprintf(stderr, "Error en listen: %s\n", strerror(errno));
        close(socket_servidor);
        freeaddrinfo(server_info);
        return -1;
    }

    //free memory
    freeaddrinfo(server_info);

    //log y return el socket
    printf("Servidor escuchando en el puerto %s\n", puerto);
    return socket_servidor;
}

int crear_conexion(const char* ip, const char* puerto){
    struct addrinfo hints, *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; //IPV4
    hints.ai_socktype = SOCK_STREAM; //TCP

    int err = getaddrinfo(ip, puerto, &hints, &server_info);
    if(err !=0){
        fprintf(stderr, "Fallo getaddrinfo al conectar con %s:%s - %s\n", ip, puerto, gai_strerror(err));
        return -1;
    }

    //creo socket
    int socket_cliente = socket(server_info->ai_family,
                                server_info->ai_socktype,
                                server_info->ai_protocol);
    
    if(socket_cliente == -1){
        fprintf(stderr, "Error al crear socket %s\n", strerror(errno));
        freeaddrinfo(server_info);
        return -1;
    }

    if(connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1){
        fprintf(stderr, "Fallo al conectar con %s: %s - %s\n", ip, puerto, strerror(errno));
        freeaddrinfo(server_info);
        close(socket_cliente);
        return -1;
    }

    printf("Conectado con exito a %s puerto %s\n", ip, puerto);
    freeaddrinfo(server_info);
    return socket_cliente;
}

int esperar_cliente(int socket_servidor){
    int socket_cliente = accept(socket_servidor, NULL, NULL);
    if(socket_cliente == -1){
        fprintf(stderr, "Error en el accept(): %s\n", strerror(errno));
        return -1;
    }
    printf("Se conecto un cliente al socket %d\n", socket_servidor);
    return socket_cliente;
}

int enviar_string(int socket, char* string){
    if (string == NULL){
        fprintf(stderr, "Error: el string es NULL\n");
        return -1;
    }
    int size = strlen(string) +1;

    if(send(socket, &size, sizeof(size), 0) == -1){
        fprintf(stderr, "Error al enviar el tamaño del string a traves del socket %d: %s\n", socket, strerror(errno));
        return -1;
    }
    if (send(socket, string, size, 0) == -1){
        fprintf(stderr, "Error al enviar el string a traves del socket %d: %s\n", socket, strerror(errno));
        return -1;
    }
    return 0;
}

char* recibir_string(int socket){
    int size;
    int bytes_recibidos;

    //Recibo tamaño del string, chequeo errores
    bytes_recibidos = recv(socket, &size, sizeof(size), MSG_WAITALL);
    if(bytes_recibidos <= 0){
        if(bytes_recibidos == -1){
            fprintf(stderr, "Error al recibir el tamaño del string: %s\n", strerror(errno));
            return NULL;
        }
        return NULL;
    }
    //Reservo espacio para recibir el string
    char* string = malloc(size);
    if(string == NULL){
        fprintf(stderr, "Error al reservar memoria para el string: %s\n", strerror(errno));
        return NULL;
    }
    
    //Recibo el string, chequeo errores
    bytes_recibidos = recv(socket, string, size, MSG_WAITALL);
    if(bytes_recibidos <= 0){
        if(bytes_recibidos == -1){
            fprintf(stderr, "Error al recibir el string: %s\n", strerror(errno));
            return NULL;
        }
        free(string);
        return NULL;
    }

    string[size - 1] = '\0';
    return string;
}

int enviar_entero(int socket, int valor) {
    //Envio al socket un valor entero, chequeo errores
    if(send(socket, &valor, sizeof(int), 0) == -1){
        fprintf(stderr,"Error al enviar numero entero %d : %s\n", valor, strerror(errno));
        return -1;
    } 

    printf("Valor entero enviado!\n");
    return 0;
}

int recibir_entero(int socket, int* valor_recibido) {
    int id;

    //Recibo valor de id y chequeo errores
    if(recv(socket, &id, sizeof(int), MSG_WAITALL) == -1){
        fprintf(stderr,"Error al recibir el valor: %s\n", strerror(errno));
        return -1;
    }

    //Casteo el u_int8_t a id_modulo_t y la almaceno en la variable que se recibio para eso
    *valor_recibido = id;
    
    printf("Valor entero recibido!\n");
    
    return 0;
}

/***********************************************************************************************************************/
/***                                           FUNCIONES DE MANEJO DE BUFFER                                         ***/
/***********************************************************************************************************************/

t_buffer *crear_buffer(uint32_t size){
    t_buffer *buffer = malloc(sizeof(*buffer));
    buffer->size = size;
    buffer->offset = 0;
    buffer->stream = malloc(size); //Se libera con destruir_buffer
    return buffer;
}

void destruir_buffer(t_buffer *buffer){
    if(buffer){
        free(buffer->stream);
        free(buffer);
    }
}

void buffer_add(t_buffer *buffer, void *data, uint32_t size){
    if(buffer->offset + size > buffer->size){
        fprintf(stderr, "Buffer overflow: El buffer no tiene espacio para agregar los datos\n");
        return;
    }
    memcpy(buffer->stream + buffer->offset, data, size);
    buffer->offset += size;
}

void buffer_read(t_buffer *buffer, void *data, uint32_t size){
    memcpy(data, buffer->stream + buffer->offset, size);
    buffer->offset += size;
}

void buffer_add_uint32(t_buffer *buffer, uint32_t data){
    buffer_add(buffer, &data, sizeof(data));
}

uint32_t buffer_read_uint32(t_buffer *buffer){
    uint32_t data;
    buffer_read(buffer, &data, sizeof(data));
    return data;
}

void buffer_add_uint8(t_buffer *buffer, uint8_t data){
    buffer_add(buffer, &data, sizeof(data));
}

uint8_t buffer_read_uint8(t_buffer *buffer){
    uint8_t data;
    buffer_read(buffer, &data, sizeof(data));
    return data;
}

void buffer_add_string(t_buffer *buffer, uint32_t length, char* string){
    buffer_add_uint32(buffer, length);
    buffer_add(buffer, string, length);
}

char *buffer_read_string(t_buffer *buffer){
    uint32_t length = buffer_read_uint32(buffer);
    char *string = malloc(length + 1); //Hay que liberar
    buffer_read(buffer, string, length);
    string[length] = '\0';
    return string;
}

/***********************************************************************************************************************/
/***                                           FUNCIONES DE MANEJO DE PAQUETES                                       ***/
/***********************************************************************************************************************/

void* armar_stream(t_paquete *paquete){
    void* stream = malloc(paquete->datos->size + sizeof(e_codigo_operacion) + sizeof(uint32_t));
    int offset = 0;
    memcpy(stream + offset, &(paquete->codigo_operacion), sizeof(e_codigo_operacion));
    offset += sizeof(e_codigo_operacion);

    memcpy(stream +  offset, &(paquete->datos->size), sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(stream + offset, paquete->datos->stream, paquete->datos->size);

    return stream;

}

t_paquete* empaquetar_buffer(e_codigo_operacion codigo_operacion, t_buffer* buffer) {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = codigo_operacion;
    paquete->datos = buffer;
    return paquete;
}

int enviar_paquete(int socket, t_paquete *paquete){
    // Armo el stream de bytes a partir del paquete que se recibio
    void* stream = armar_stream(paquete);
    //Envio el stream de bytes a traves del socket
    if (send(socket, stream, paquete->datos->size + sizeof(e_codigo_operacion) + sizeof(uint32_t), 0) == -1){
        fprintf(stderr, "Error al enviar el paquete a traves del socket %d: %s\n", socket, strerror(errno));
        destruir_paquete(paquete);
        return -1;
    }
    //Libero el paquete si el envio fue exitoso
    destruir_paquete(paquete);
    //Libero el stream de bytes usado para el envio
    free(stream);
    return 0;
}

t_paquete* recibir_paquete(int socket){
    t_paquete* paquete = malloc(sizeof(*paquete));
    if (paquete == NULL){
        fprintf(stderr, "Error al reservar memoria para el paquete: %s\n", strerror(errno));
        return NULL;
    }
    paquete->datos = malloc(sizeof(*paquete->datos));
    if(paquete->datos == NULL){
        fprintf(stderr, "Error al reservar memoria para el buffer del paquete: %s\n", strerror(errno));
        free(paquete);
        return NULL;
    }
    
    //Recibo el codigo de operacion primero
    int bytes = recv(socket, &(paquete->codigo_operacion), sizeof(e_codigo_operacion), MSG_WAITALL);
    if(bytes == 0){
        // Desconexión limpia
        fprintf(stderr, "El cliente se ha desconectado.\n");
        destruir_paquete(paquete);
        return NULL;
    }
    if(bytes < 0){
        // Error real
        fprintf(stderr, "Error al recibir el codigo de operacion: %s\n", strerror(errno));
        destruir_paquete(paquete);
        return NULL;
    }

    //Recibo el tamaño del buffer
    bytes = recv(socket, &(paquete->datos->size), sizeof(uint32_t), MSG_WAITALL);
    if(bytes == 0){
        fprintf(stderr, "El cliente se ha desconectado.\n");
        destruir_paquete(paquete);
        return NULL;
    }
    if(bytes < 0){
        fprintf(stderr, "Error al recibir el tamaño del buffer del paquete: %s\n", strerror(errno));
        destruir_paquete(paquete);
        return NULL;
    }

    //Recibo el contenido del buffer si es que hay
    if (paquete->datos->size > 0){
        paquete->datos->stream = malloc(paquete->datos->size);
        if(paquete->datos->stream == NULL){
            //Si falla la reserva, libero el paquete y devuelvo NULL
            fprintf(stderr, "Error al reservar memoria para el contenido del buffer del paquete: %s\n", strerror(errno));
            destruir_paquete(paquete);
            return NULL;
        }
        bytes = recv(socket, paquete->datos->stream, paquete->datos->size, MSG_WAITALL);
        if(bytes == 0){
            fprintf(stderr, "El cliente se ha desconectado.\n");
            destruir_paquete(paquete);
            return NULL;
        }
        if(bytes < 0){
            fprintf(stderr, "Error al recibir el contenido del buffer del paquete: %s\n", strerror(errno));
            destruir_paquete(paquete);
            return NULL;
        }
    } else {
        paquete->datos->stream = NULL;
    }
    paquete->datos->offset = 0;
    return paquete;
}

void destruir_paquete(t_paquete *paquete){
    if(paquete){
        destruir_buffer(paquete->datos);
        free(paquete);
    }
}

/***********************************************************************************************************************/
/***                                           FUNCIONES DE SERIALIZACION DE PAQUETES                                ***/
/***********************************************************************************************************************/

t_buffer* serializar_handshake_qc_master(t_handshake_qc_master *handshake){
    // Calculo el tamaño del buffer necesario. Tamaño del tamaño del string + tamaño del int + el string
    uint32_t tamanio_buffer = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) + strlen(handshake->archivo_query);
    //Creo el buffer
    t_buffer* buffer = crear_buffer(tamanio_buffer);
    //Agrego los datos
    buffer_add_uint32(buffer, strlen(handshake->archivo_query)); // Ya lo agrega buffer_add_string
    buffer_add_uint32(buffer, handshake->prioridad);
    //Agrego el string
    buffer_add_string(buffer, strlen(handshake->archivo_query), handshake->archivo_query);
    return buffer;
}

t_handshake_qc_master* deserializar_handshake_qc_master(t_buffer *buffer){
    t_handshake_qc_master* handshake = malloc(sizeof(*handshake));
    buffer_read_uint32(buffer); //DUMMY FIELD?
    handshake->prioridad = buffer_read_uint32(buffer);
    handshake->archivo_query = buffer_read_string(buffer);
    return handshake;
}

t_buffer* serializar_handshake_worker_storage(t_handshake_worker_storage *handshake){
    uint32_t tamanio_buffer = sizeof(uint32_t);
    t_buffer* buffer = crear_buffer(tamanio_buffer);
    buffer_add_uint32(buffer, handshake->id_worker);
    return buffer;
}

t_handshake_worker_storage* deserializar_handshake_worker_storage(t_buffer *buffer){
    t_handshake_worker_storage* handshake = malloc(sizeof(*handshake));
    handshake->id_worker = buffer_read_uint32(buffer);
    return handshake;
}

t_buffer* serializar_handshake_worker_master(t_handshake_worker_master *handshake){
    uint32_t tamanio_buffer = sizeof(uint32_t);
    t_buffer* buffer = crear_buffer(tamanio_buffer);
    buffer_add_uint32(buffer, handshake->id_worker);
    return buffer;
}

t_handshake_worker_master* deserializar_handshake_worker_master(t_buffer *buffer){
    t_handshake_worker_master* handshake = malloc(sizeof(*handshake));
    handshake->id_worker = buffer_read_uint32(buffer);
    return handshake;
}

t_buffer* serializar_asignacion_query(t_asignacion_query* asignacion){
    uint32_t tamanio_buffer = 3 * sizeof(uint32_t) + strlen(asignacion->path_query);
    t_buffer* buffer = crear_buffer(tamanio_buffer);
    buffer_add_string(buffer, strlen(asignacion->path_query), asignacion->path_query);
    buffer_add_uint32(buffer, asignacion->id_query);
    buffer_add_uint32(buffer, asignacion->pc);
    return buffer;
}

t_asignacion_query* deserializar_asignacion_query(t_buffer* buffer){
    t_asignacion_query* asignacion = malloc(sizeof(t_asignacion_query));
    asignacion->path_query = buffer_read_string(buffer);
    asignacion->id_query = buffer_read_uint32(buffer);
    asignacion->pc = buffer_read_uint32(buffer);
    return asignacion;
}

t_buffer* serializar_tam_pagina(t_tam_pagina* tam_pagina_struct) {
    uint32_t tamanio_buffer = sizeof(uint32_t);
    t_buffer* buffer = crear_buffer(tamanio_buffer);
    buffer_add_uint32(buffer, tam_pagina_struct->tam_pagina);
    return buffer;
}

t_tam_pagina* deserializar_tam_pagina(t_buffer* buffer) {
    t_tam_pagina* tam_pagina_struct = malloc(sizeof(t_tam_pagina));
    tam_pagina_struct->tam_pagina = buffer_read_uint32(buffer);
    return tam_pagina_struct;
}

t_buffer* serializar_resultado_query(t_resultado_query* resultado) {
    uint32_t tamanio_buffer = sizeof(uint32_t) + sizeof(t_resultado_ejecucion) + sizeof(uint32_t);
    if (resultado->mensaje_error != NULL) {
        tamanio_buffer += sizeof(uint32_t) + strlen(resultado->mensaje_error);
    } else {
        tamanio_buffer += sizeof(uint32_t); // Longitud 0 para mensaje_error
    }

    t_buffer* buffer = crear_buffer(tamanio_buffer);
    buffer_add_uint32(buffer, resultado->id_query);
    buffer_add_uint32(buffer, (uint32_t)resultado->estado);
    buffer_add_uint32(buffer, resultado->pc_final);

    if (resultado->mensaje_error != NULL) {
        buffer_add_string(buffer, strlen(resultado->mensaje_error), resultado->mensaje_error);
    } else {
        buffer_add_uint32(buffer, 0); // Longitud 0 para mensaje_error
    }

    return buffer;
}

t_resultado_query* deserializar_resultado_query(t_buffer* buffer) {
    t_resultado_query* resultado = malloc(sizeof(t_resultado_query));
    resultado->id_query = buffer_read_uint32(buffer);
    resultado->estado = (t_resultado_ejecucion)buffer_read_uint32(buffer);
    resultado->pc_final = buffer_read_uint32(buffer);

    uint32_t longitud_mensaje = buffer_read_uint32(buffer);
    if (longitud_mensaje > 0) {
        resultado->mensaje_error = buffer_read_string(buffer);
    } else {
        resultado->mensaje_error = NULL;
    }

    return resultado;
}

// ============================================================================
// SERIALIZACION DE INSTRUCCIONES
// ============================================================================

// Serialización para cada tipo de isntruccion

t_buffer* serializar_create(t_create* create) {
    uint32_t size = sizeof(uint32_t) + // id_query
                    sizeof(uint32_t) + strlen(create->file_name) + 1 + // file_name (len + string)
                    sizeof(uint32_t) + strlen(create->tag_name) + 1;   // tag_name (len + string)

    t_buffer* buffer = crear_buffer(size);
    buffer_add_uint32(buffer, create->id_query);
    buffer_add_string(buffer, strlen(create->file_name) + 1, create->file_name);
    buffer_add_string(buffer, strlen(create->tag_name) + 1, create->tag_name);
    return buffer;
}

t_create* deserializar_create(t_buffer* buffer) {
    t_create* create = malloc(sizeof(t_create));
    create->id_query = buffer_read_uint32(buffer);
    create->file_name = buffer_read_string(buffer);
    create->tag_name = buffer_read_string(buffer);
    return create;
}

t_buffer* serializar_truncate(t_truncate* truncate) {
    uint32_t size = sizeof(uint32_t) + // id_query
                    sizeof(uint32_t) + strlen(truncate->file_name) + 1 + // file_name
                    sizeof(uint32_t) + strlen(truncate->tag_name) + 1 +  // tag_name
                    sizeof(uint32_t); // size

    t_buffer* buffer = crear_buffer(size);
    buffer_add_uint32(buffer, truncate->id_query);
    buffer_add_string(buffer, strlen(truncate->file_name) + 1, truncate->file_name);
    buffer_add_string(buffer, strlen(truncate->tag_name) + 1, truncate->tag_name);
    buffer_add_uint32(buffer, truncate->size);
    return buffer;
}

t_truncate* deserializar_truncate(t_buffer* buffer) {
    t_truncate* truncate = malloc(sizeof(t_truncate));
    truncate->id_query = buffer_read_uint32(buffer);
    truncate->file_name = buffer_read_string(buffer);
    truncate->tag_name = buffer_read_string(buffer);
    truncate->size = buffer_read_uint32(buffer);
    return truncate;
}

t_buffer* serializar_write(t_write* write) {
    uint32_t size = sizeof(uint32_t) + // id_query
                    sizeof(uint32_t) + strlen(write->file_name) + 1 + // file_name
                    sizeof(uint32_t) + strlen(write->tag_name) + 1 +  // tag_name
                    sizeof(uint32_t) + // offset
                    sizeof(uint32_t) + // size
                    write->size;       // content
    t_buffer* buffer = crear_buffer(size);
    buffer_add_uint32(buffer, write->id_query);
    buffer_add_string(buffer, strlen(write->file_name) + 1, write->file_name);
    buffer_add_string(buffer, strlen(write->tag_name) + 1, write->tag_name);
    buffer_add_uint32(buffer, write->offset);
    buffer_add_uint32(buffer, write->size);
    buffer_add(buffer, write->content, write->size);
    return buffer;
}

t_write* deserializar_write(t_buffer* buffer) {
    t_write* write = malloc(sizeof(t_write));
    write->id_query = buffer_read_uint32(buffer);
    write->file_name = buffer_read_string(buffer);
    write->tag_name = buffer_read_string(buffer);
    write->offset = buffer_read_uint32(buffer);
    write->size = buffer_read_uint32(buffer);
    write->content = malloc(write->size);
    buffer_read(buffer, write->content, write->size);
    return write;
}

t_buffer* serializar_read(t_read* read) {
    uint32_t size = sizeof(uint32_t) + // id_query
                    sizeof(uint32_t) + strlen(read->file_name) + 1 + // file_name
                    sizeof(uint32_t) + strlen(read->tag_name) + 1 +  // tag_name
                    sizeof(uint32_t) + // offset
                    sizeof(uint32_t);  // size
    t_buffer* buffer = crear_buffer(size);
    buffer_add_uint32(buffer, read->id_query);
    buffer_add_string(buffer, strlen(read->file_name) + 1, read->file_name);
    buffer_add_string(buffer, strlen(read->tag_name) + 1, read->tag_name);
    buffer_add_uint32(buffer, read->offset);
    buffer_add_uint32(buffer, read->size);
    return buffer;
}

t_read* deserializar_read(t_buffer* buffer) {
    t_read* read = malloc(sizeof(t_read));
    read->id_query = buffer_read_uint32(buffer);
    read->file_name = buffer_read_string(buffer);
    read->tag_name = buffer_read_string(buffer);
    read->offset = buffer_read_uint32(buffer);
    read->size = buffer_read_uint32(buffer);
    return read;
}

t_buffer* serializar_tag(t_tag* tag) {
    uint32_t size = sizeof(uint32_t) + // id_query
                    sizeof(uint32_t) + strlen(tag->file_name_origen) + 1 +
                    sizeof(uint32_t) + strlen(tag->tag_name_origen) + 1 +
                    sizeof(uint32_t) + strlen(tag->file_name_destino) + 1 +
                    sizeof(uint32_t) + strlen(tag->tag_name_destino) + 1;
    t_buffer* buffer = crear_buffer(size);
    buffer_add_uint32(buffer, tag->id_query);
    buffer_add_string(buffer, strlen(tag->file_name_origen) + 1, tag->file_name_origen);
    buffer_add_string(buffer, strlen(tag->tag_name_origen) + 1, tag->tag_name_origen);
    buffer_add_string(buffer, strlen(tag->file_name_destino) + 1, tag->file_name_destino);
    buffer_add_string(buffer, strlen(tag->tag_name_destino) + 1, tag->tag_name_destino);
    return buffer;
}

t_tag* deserializar_tag(t_buffer* buffer) {
    t_tag* tag = malloc(sizeof(t_tag));
    tag->id_query = buffer_read_uint32(buffer);
    tag->file_name_origen = buffer_read_string(buffer);
    tag->tag_name_origen = buffer_read_string(buffer);
    tag->file_name_destino = buffer_read_string(buffer);
    tag->tag_name_destino = buffer_read_string(buffer);
    return tag;
}

t_buffer* serializar_commit(t_commit* commit) {
    uint32_t size = sizeof(uint32_t) + // id_query
                    sizeof(uint32_t) + strlen(commit->file_name) + 1 +
                    sizeof(uint32_t) + strlen(commit->tag_name) + 1;
    t_buffer* buffer = crear_buffer(size);
    buffer_add_uint32(buffer, commit->id_query);
    buffer_add_string(buffer, strlen(commit->file_name) + 1, commit->file_name);
    buffer_add_string(buffer, strlen(commit->tag_name) + 1, commit->tag_name);
    return buffer;
}

t_commit* deserializar_commit(t_buffer* buffer) {
    t_commit* commit = malloc(sizeof(t_commit));
    commit->id_query = buffer_read_uint32(buffer);
    commit->file_name = buffer_read_string(buffer);
    commit->tag_name = buffer_read_string(buffer);
    return commit;
}

t_buffer* serializar_flush(t_flush* flush) {
    uint32_t size = sizeof(uint32_t) + // id_query
                    sizeof(uint32_t) + strlen(flush->file_name) + 1 +
                    sizeof(uint32_t) + strlen(flush->tag_name) + 1;
    t_buffer* buffer = crear_buffer(size);
    buffer_add_uint32(buffer, flush->id_query);
    buffer_add_string(buffer, strlen(flush->file_name) + 1, flush->file_name);
    buffer_add_string(buffer, strlen(flush->tag_name) + 1, flush->tag_name);
    return buffer;
}

t_flush* deserializar_flush(t_buffer* buffer) {
    t_flush* flush = malloc(sizeof(t_flush));
    flush->id_query = buffer_read_uint32(buffer);
    flush->file_name = buffer_read_string(buffer);
    flush->tag_name = buffer_read_string(buffer);
    return flush;
}

t_buffer* serializar_delete(t_delete* delete) {
    uint32_t size = sizeof(uint32_t) + // id_query
                    sizeof(uint32_t) + strlen(delete->file_name) + 1 +
                    sizeof(uint32_t) + strlen(delete->tag_name) + 1;
    t_buffer* buffer = crear_buffer(size);
    buffer_add_uint32(buffer, delete->id_query);
    buffer_add_string(buffer, strlen(delete->file_name) + 1, delete->file_name);
    buffer_add_string(buffer, strlen(delete->tag_name) + 1, delete->tag_name);
    return buffer;
}

t_delete* deserializar_delete(t_buffer* buffer) {
    t_delete* delete = malloc(sizeof(t_delete));
    delete->id_query = buffer_read_uint32(buffer);
    delete->file_name = buffer_read_string(buffer);
    delete->tag_name = buffer_read_string(buffer);
    return delete;
}

t_buffer* serializar_solicitud_read(t_sol_read* solicitud) {
    uint32_t size = sizeof(uint32_t) + // id_query
                    sizeof(uint32_t) + strlen(solicitud->file_name) + 1 +
                    sizeof(uint32_t) + strlen(solicitud->tag_name) + 1 +
                    sizeof(uint32_t) + // numero_bloque
                    sizeof(uint32_t);  // tamanio
    t_buffer* buffer = crear_buffer(size);
    buffer_add_uint32(buffer, solicitud->id_query);
    buffer_add_string(buffer, strlen(solicitud->file_name) + 1, solicitud->file_name);
    buffer_add_string(buffer, strlen(solicitud->tag_name) + 1, solicitud->tag_name);
    buffer_add_uint32(buffer, solicitud->numero_bloque);
    buffer_add_uint32(buffer, solicitud->tamanio);
    return buffer;
}

t_sol_read* deserializar_solicitud_read(t_buffer* buffer) {
    t_sol_read* solicitud = malloc(sizeof(t_sol_read));
    solicitud->id_query = buffer_read_uint32(buffer);
    solicitud->file_name = buffer_read_string(buffer);
    solicitud->tag_name = buffer_read_string(buffer);
    solicitud->numero_bloque = buffer_read_uint32(buffer);
    solicitud->tamanio = buffer_read_uint32(buffer);
    return solicitud;
}

t_buffer* serializar_bloque_leido(t_bloque_leido* bloque) {
    uint32_t size = sizeof(uint32_t) + // id_query
                    sizeof(uint32_t) + strlen(bloque->file_name) + 1 + // file_name
                    sizeof(uint32_t) + strlen(bloque->tag_name) + 1 + // tag_name
                    sizeof(uint32_t) + // tamanio
                    bloque->tamanio;   // contenido
    t_buffer* buffer = crear_buffer(size);
    buffer_add_uint32(buffer, bloque->id_query);
    buffer_add_string(buffer, strlen(bloque->file_name) + 1, bloque->file_name);
    buffer_add_string(buffer, strlen(bloque->tag_name) + 1, bloque->tag_name);
    buffer_add_uint32(buffer, bloque->tamanio);
    buffer_add(buffer, bloque->contenido, bloque->tamanio);
    return buffer;
}

t_bloque_leido* deserializar_bloque_leido(t_buffer* buffer) {
    t_bloque_leido* bloque = malloc(sizeof(t_bloque_leido));
    bloque->id_query = buffer_read_uint32(buffer);
    bloque->file_name = buffer_read_string(buffer);
    bloque->tag_name = buffer_read_string(buffer);
    bloque->tamanio = buffer_read_uint32(buffer);
    bloque->contenido = malloc(bloque->tamanio);
    buffer_read(buffer, bloque->contenido, bloque->tamanio);
    return bloque;
}

t_buffer* serializar_solicitud_write(t_sol_write* solicitud) {
    uint32_t size = sizeof(uint32_t) + // id_query
                    sizeof(uint32_t) + strlen(solicitud->file_name) + 1 +
                    sizeof(uint32_t) + strlen(solicitud->tag_name) + 1 +
                    sizeof(uint32_t) + // numero_bloque
                    sizeof(uint32_t) + // tamanio
                    solicitud->tamanio; // contenido
    t_buffer* buffer = crear_buffer(size);
    buffer_add_uint32(buffer, solicitud->id_query);
    buffer_add_string(buffer, strlen(solicitud->file_name) + 1, solicitud->file_name);
    buffer_add_string(buffer, strlen(solicitud->tag_name) + 1, solicitud->tag_name);
    buffer_add_uint32(buffer, solicitud->numero_bloque);
    buffer_add_uint32(buffer, solicitud->tamanio);
    buffer_add(buffer, solicitud->contenido, solicitud->tamanio);
    return buffer;
}

t_sol_write* deserializar_solicitud_write(t_buffer* buffer) {
    t_sol_write* solicitud = malloc(sizeof(t_sol_write));
    solicitud->id_query = buffer_read_uint32(buffer);
    solicitud->file_name = buffer_read_string(buffer);
    solicitud->tag_name = buffer_read_string(buffer);
    solicitud->numero_bloque = buffer_read_uint32(buffer);
    solicitud->tamanio = buffer_read_uint32(buffer);
    solicitud->contenido = malloc(solicitud->tamanio);
    buffer_read(buffer, solicitud->contenido, solicitud->tamanio);
    return solicitud;
}

t_buffer* serializar_cod_error(t_resultado_ejecucion* resultado) {
    t_buffer* buffer = crear_buffer(sizeof(uint32_t));
    buffer_add_uint32(buffer, (uint32_t)(*resultado));
    return buffer;
}

t_resultado_ejecucion* deserializar_cod_error(t_buffer* buffer) {
    t_resultado_ejecucion* resultado = malloc(sizeof(t_resultado_ejecucion));
    *resultado = (t_resultado_ejecucion)buffer_read_uint32(buffer);
    return resultado;
}