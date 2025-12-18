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

/**
* @brief Envia un valor entero
* @param socket el socket al cual se envia el mensaje
* @param valor el valor entero a enviar
* @return 0 si se envio correctamente, -1 si hay error.
*/
int enviar_entero(int socket, int valor);

/**
* @brief Recibe un valor entero
* @param socket el socket desde el cual se recibe el mensaje
* @param valor_recibido donde se guardara el valor recibido para luego evaluar
* @return 0 si se recibio correctamente, -1 si hay error.
*/
int recibir_entero(int socket, int* valor_recibido);

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
 * @brief Empaqueta un buffer en un paquete con el codigo de operacion especificado
 * @param codigo_operacion el codigo de operacion del paquete
 * @param buffer el buffer a empaquetar
 * @return el paquete resultante
 */
t_paquete* empaquetar_buffer(e_codigo_operacion codigo_operacion, t_buffer* buffer);

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

/**
 * @brief Serializa un t_asignacion_query
 * @param asignacion el *t_asignacion_query a serializar
 * @return el *t_buffer resultante
 */
t_buffer* serializar_asignacion_query(t_asignacion_query* asignacion);

/**
 * @brief Deserializa un t_asignacion_query
 * @param buffer el *t_buffer a deserializar
 * @return el *t_asignacion_query resultante. 
 * @note La estructura debe ser liberada por el que la recibe
 */
t_asignacion_query* deserializar_asignacion_query(t_buffer* buffer);

/**
 * @brief Serializa un t_tam_pagina 
 * @param tam_pagina_struct el *t_tam_pagina (struct con el tamaño de página/bloque) a serializar
 * @return el *t_buffer resultante
 * @note Utilizado en el handshake entre worker y storage
 */
t_buffer* serializar_tam_pagina(t_tam_pagina* tam_pagina_struct);

/**
 * @brief Deserializa un t_tam_pagina 
 * @param buffer el *t_buffer a deserializar
 * @return el *t_tam_pagina (struct con el tamaño de página/bloque) resultante. 
 * @note La estructura debe ser liberada por el que la recibe
 * @note Utilizado en el handshake entre worker y storage
 */
t_tam_pagina* deserializar_tam_pagina(t_buffer* buffer);

/**
 * @brief Serializa un t_resultado_query
 * @param resultado el *t_resultado_query a serializar
 * @return el *t_buffer resultante
 */
t_buffer* serializar_resultado_query(t_resultado_query* resultado);

/**
 * @brief Deserializa un t_resultado_query
 * @param buffer el *t_buffer a deserializar
 * @return el *t_resultado_query resultante. 
 * @note La estructura debe ser liberada por el que la recibe
 */
t_resultado_query* deserializar_resultado_query(t_buffer* buffer);

/**
 * @brief Serializa un código de error (t_resultado_ejecucion)
 * @param resultado Puntero al código de error a serializar
 * @return t_buffer* Buffer con la serialización
 */
t_buffer* serializar_cod_error(t_resultado_ejecucion* resultado);

/**
 * @brief Deserializa un código de error (t_resultado_ejecucion)
 * @param buffer Puntero al t_buffer a deserializar
 * @return t_resultado_ejecucion* Estructura t_resultado_ejecucion deserializada
 */
t_resultado_ejecucion* deserializar_cod_error(t_buffer* buffer);

// ============================================================================
// SERIALIZACION DE INSTRUCCIONES
// ============================================================================
/**
 * @brief Serializa una instrucción CREATE
 * @param create Puntero a t_create a serializar
 * @return t_buffer* Buffer con la serialización
 */
t_buffer* serializar_create(t_create* create);

/**
 * @brief Deserializa una instrucción CREATE
 * @param buffer Puntero al t_buffer a deserializar
 * @return t_create* Estructura t_create deserializada
 */
t_create* deserializar_create(t_buffer* buffer);

/**
 * @brief Serializa una instrucción TRUNCATE
 * @param truncate Puntero a t_truncate a serializar
 * @return t_buffer* Buffer con la serialización
 */
t_buffer* serializar_truncate(t_truncate* truncate);

/**
 * @brief Deserializa una instrucción TRUNCATE
 * @param buffer Puntero al t_buffer a deserializar
 * @return t_truncate* Estructura t_truncate deserializada
 */
t_truncate* deserializar_truncate(t_buffer* buffer);

/**
 * @brief Serializa una instrucción WRITE
 * @param write Puntero a t_write a serializar
 * @return t_buffer* Buffer con la serialización
 * @note DEPRECADO
 */
t_buffer* serializar_write(t_write* write);

/**
 * @brief Deserializa una instrucción WRITE
 * @param buffer Puntero al t_buffer a deserializar
 * @return t_write* Estructura t_write deserializada
 * @note DEPRECADO
 */
t_write* deserializar_write(t_buffer* buffer);

/**
 * @brief Serializa una instrucción READ
 * @param read Puntero a t_read a serializar
 * @return t_buffer* Buffer con la serialización
 * @note DEPRECADO
 */
t_buffer* serializar_read(t_read* read);

/**
 * @brief Deserializa una instrucción READ
 * @param buffer Puntero al t_buffer a deserializar
 * @return t_read* Estructura t_read deserializada
 * @note DEPRECADO
 */
t_read* deserializar_read(t_buffer* buffer);

/**
 * @brief Serializa una instrucción TAG
 * @param tag Puntero a t_tag a serializar
 * @return t_buffer* Buffer con la serialización
 */
t_buffer* serializar_tag(t_tag* tag);

/**
 * @brief Deserializa una instrucción TAG
 * @param buffer Puntero al t_buffer a deserializar
 * @return t_tag* Estructura t_tag deserializada
 */
t_tag* deserializar_tag(t_buffer* buffer);

/**
 * @brief Serializa una instrucción COMMIT
 * @param commit Puntero a t_commit a serializar
 * @return t_buffer* Buffer con la serialización
 */
t_buffer* serializar_commit(t_commit* commit);

/**
 * @brief Deserializa una instrucción COMMIT
 * @param buffer Puntero al t_buffer a deserializar
 * @return t_commit* Estructura t_commit deserializada
 */
t_commit* deserializar_commit(t_buffer* buffer);

/**
 * @brief Serializa una instrucción FLUSH
 * @param flush Puntero a t_flush a serializar
 * @return t_buffer* Buffer con la serialización
 */
t_buffer* serializar_flush(t_flush* flush);

/**
 * @brief Deserializa una instrucción FLUSH
 * @param buffer Puntero al t_buffer a deserializar
 * @return t_flush* Estructura t_flush deserializada
 */
t_flush* deserializar_flush(t_buffer* buffer);

/**
 * @brief Serializa una instrucción DELETE
 * @param delete Puntero a t_delete a serializar
 * @return t_buffer* Buffer con la serialización
 */
t_buffer* serializar_delete(t_delete* delete);

/**
 * @brief Deserializa una instrucción DELETE
 * @param buffer Puntero al t_buffer a deserializar
 * @return t_delete* Estructura t_delete deserializada
 */
t_delete* deserializar_delete(t_buffer* buffer);

// SOLICITUDES DE OPERACIONES A STORAGE
/**
 * @brief Serializa una solicitud de operación de memoria interna
 * @param solicitud Puntero a t_sol_read a serializar
 * @return t_buffer* Buffer con la serialización
 */
t_buffer* serializar_solicitud_operacion(t_sol_read* solicitud);

/**
 * @brief Deserializa una solicitud de operación de memoria interna
 * @param buffer Puntero al t_buffer a deserializar
 * @return t_sol_read* Estructura t_sol_read deserializada
 */
t_sol_read* deserializar_solicitud_operacion(t_buffer* buffer);

/**
 * @brief Serializa una solicitud de lectura de memoria interna
 * @param solicitud Puntero a t_sol_read a serializar
 * @return t_buffer* Buffer con la serialización
 */
t_buffer* serializar_solicitud_read(t_sol_read* solicitud);

/**
 * @brief Deserializa una solicitud de lectura de memoria interna
 * @param buffer Puntero al t_buffer a deserializar
 * @return t_sol_read* Estructura t_sol_read deserializada
 */
t_sol_read* deserializar_solicitud_read(t_buffer* buffer);

/**
 * @brief Serializa un bloque leído de memoria interna
 * @param bloque Puntero a t_bloque_leido a serializar
 * @return t_buffer* Buffer con la serialización
 */
t_buffer* serializar_bloque_leido(t_bloque_leido* bloque);

/**
 * @brief Deserializa un bloque leído de memoria interna
 * @param buffer Puntero al t_buffer a deserializar
 * @return t_bloque_leido* Estructura t_bloque_leido deserializada
 */
t_bloque_leido* deserializar_bloque_leido(t_buffer* buffer);

/**
 * @brief Serializa una solicitud de escritura de memoria interna (flush)
 * @param solicitud Puntero a t_sol_write a serializar
 * @return t_buffer* Buffer con la serialización
 */
t_buffer* serializar_solicitud_write(t_sol_write* solicitud);

/**
 * @brief Deserializa una solicitud de escritura de memoria interna (flush)
 * @param buffer Puntero al t_buffer a deserializar
 * @return t_sol_write* Estructura t_sol_write deserializada
 */
t_sol_write* deserializar_solicitud_write(t_buffer* buffer);

/**
 * @brief Serializa un mensaje de lectura para el Master
 * @param info_leida Puntero a t_msj_leido a serializar
 * @return t_buffer* Buffer con la serialización
 * @note Enviado desde Worker -> Master -> Query Control (OP_READ)
 */
t_buffer* serializar_lectura(t_msj_leido* info_leida);

/**
 * @brief Deserializa un mensaje de lectura para el Master
 * @param buffer Puntero al t_buffer a deserializar
 * @return t_msj_leido* Estructura t_msj_leido deserializada
 * @note Recibido en Master -> Query Control (OP_READ)
 */
t_msj_leido* deserializar_lectura(t_buffer* buffer);

#endif
 
