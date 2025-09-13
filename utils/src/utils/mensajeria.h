#ifndef MENSAJERIA_H_
#define MENSAJERIA_H_

#define _GNU_SOURCE

#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include "estructuras.h"


/***********************************************************************************************************************/
/***                                                    SOCKETS                                                      ***/
/***********************************************************************************************************************/

/**
 * @brief Inicia un socket escuchando en el puerto especificado
 * @param puerto el puerto en el que se va a iniciar el servidor
 * @return el socket escuchando en puerto
 */
int iniciar_servidor(const char* puerto);

/**
* @brief Obtiene informacion de la ip, crea un socket, lo conecta con el 
* servidor.
* @param ip la ip del server
* @param puerto puerto donde escucha el servidor
* @return Devuelve el descriptor del socket conectado, o valor negativo
* si hay error.
*/
int crear_conexion(const char* ip, const char* puerto);

/**
* @brief Hace un accept() sobre el socket recibido.
* @param socket_servidor el socket del servidor que va a aceptar conexiones
* @return devuelve un socket conectado al servidor, listo para enviar y recibir datos
*/
int esperar_cliente(int socket_servidor);

/**
* @brief Envia un string a traves de un socket
* @param socket el socket a traves del cual se envia el string
* @param string el string a enviar
* @return 0 si se envio correctamente, -1 si hay error.
*/
int enviar_string(int socket, char* string);

/**
* @brief Recibe un string a traves de un socket
* @param socket el socket a traves del cual se recibe el string
* @return el string recibido
*/
char* recibir_string(int socket);

/***********************************************************************************************************************/
/***                                           FUNCIONES DE MANEJO DE BUFFER                                         ***/
/***********************************************************************************************************************/

/**
* @brief Crea un buffer
* @param size el tamaño del buffer
* @return el *t_buffer creado
*/
t_buffer *crear_buffer(uint32_t size);

/**
* @brief Destruye un buffer
* @param buffer el *t_buffer a destruir
*/
void destruir_buffer(t_buffer *buffer);

/**
* @brief Agrega datos a un buffer
* @param buffer el *t_buffer a donde se va a agregar datos
* @param data los datos a agregar
* @param size el tamaño de los datos a agregar
*/
void buffer_add(t_buffer *buffer, void *data, uint32_t size);

/**
* @brief Guarda <size> bytes del principio del buffer y los guarda en <data>, avanza el offset
* @param buffer el *t_buffer a donde se va a agregar datos
* @param data Puntero a donde se van a guardar los datos
* @param size el tamaño de los datos a guardar
*/
void buffer_read(t_buffer *buffer, void *data, uint32_t size);

/**
* @brief Agrega un uint32_t a un buffer
* @param buffer el *t_buffer a donde se va a agregar el uint32_t
* @param data el uint32_t a agregar
*/
void buffer_add_uint32(t_buffer *buffer, uint32_t data);

/**
* @brief Lee un uint32_t del buffer y avanza el offset
* @param buffer el *t_buffer a donde se va a leer el uint32_t
* @return el uint32_t leido
*/
uint32_t buffer_read_uint32(t_buffer *buffer);

/**
* @brief Agrega un uint8_t a un buffer
* @param buffer el *t_buffer a donde se va a agregar el uint8_t
* @param data el uint8_t a agregar
*/
void buffer_add_uint8(t_buffer *buffer, uint8_t data);

/**
* @brief Lee un uint8_t del buffer y avanza el offset
* @param buffer el *t_buffer a donde se va a leer el uint8_t
* @return el uint8_t leido
*/
uint8_t buffer_read_uint8(t_buffer *buffer);

/**
* @brief Agrega un string a un buffer
* @param buffer el *t_buffer a donde se va a agregar el string
* @param length el largo del string
* @param string el string a agregar
*/
void buffer_add_string(t_buffer *buffer, uint32_t length, char* string);

/**
* @brief Lee un string del buffer y avanza el offset
* @param buffer el *t_buffer a donde se va a leer el string
* @return el string leido (Debe ser liberado por el que llame a esta funcion)
*/
char *buffer_read_string(t_buffer *buffer);

/***********************************************************************************************************************/
/***                                           FUNCIONES DE MANEJO DE PAQUETES                                       ***/
/***********************************************************************************************************************/

/**
* @brief Arma el stream de bytes para enviar a traves de socket
* @param paquete el *t_paquete a partir del cual se arma el stream
* @return el puntero al stream de bytes
*/
void* armar_stream(t_paquete *paquete);

/**
* @brief Envia un paquete a traves de socket. Lo libera despues de enviarlo.
* @param socket el socket a traves del cual se envia el paquete
* @param paquete el *t_paquete a enviar
* @return 0 si se envio correctamente, -1 si hay error.
*/
int enviar_paquete(int socket, t_paquete *paquete);

/**
* @brief Recibe un paquete a traves de socket. Debe ser liberado por quien lo recibe
* @param socket el socket a traves del cual se recibe el paquete
* @return el *t_paquete recibido
*/
t_paquete* recibir_paquete(int socket);

/**
* @brief Destruye un paquete, todos sus contenidos
* @param paquete el *t_paquete a destruir
*/
void destruir_paquete(t_paquete *paquete);

/***********************************************************************************************************************/
/***                                           FUNCIONES DE SERIALIZACION DE PAQUETES                                ***/
/***********************************************************************************************************************/

/**
* @brief Serializa un t_handshake_qc_master
* @param handshake el *t_handshake_qc_master a serializar
* @return el *t_buffer resultante
*/
t_buffer* serializar_handshake_qc_master(t_handshake_qc_master *handshake);

/**
* @brief Deserializa un t_handshake_qc_master
* @param buffer el *t_buffer a deserializar
* @return el *t_handshake_qc_master resultante. La estructura debe ser liberada por el que la recibe
*/
t_handshake_qc_master* deserializar_handshake_qc_master(t_buffer *buffer);


/**
* @brief Serializa un t_handshake_worker_storage
* @param handshake el *t_handshake_worker_storage a serializar
* @return el *t_buffer resultante
*/
t_buffer* serializar_handshake_worker_storage(t_handshake_worker_storage *handshake);

/**
* @brief Deserializa un t_handshake_worker_storage
* @param buffer el *t_buffer a deserializar
* @return el *t_handshake_worker_storage resultante. La estructura debe ser liberada por el que la recibe
*/
t_handshake_worker_storage* deserializar_handshake_worker_storage(t_buffer *buffer);

/**
* @brief Serializa un t_handshake_worker_master
* @param handshake el *t_handshake_worker_master a serializar
* @return el *t_buffer resultante
*/
t_buffer* serializar_handshake_worker_master(t_handshake_worker_master *handshake);

/**
* @brief Deserializa un t_handshake_worker_master
* @param buffer el *t_buffer a deserializar
* @return el *t_handshake_worker_master resultante. La estructura debe ser liberada por el que la recibe
*/
t_handshake_worker_master* deserializar_handshake_worker_master(t_buffer *buffer);

#endif
 




