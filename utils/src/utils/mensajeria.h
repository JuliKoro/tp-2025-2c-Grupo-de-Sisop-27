#ifndef MENSAJERIA_H_
#define MENSAJERIA_H_

#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>


/**
* @brief Obtiene informacion de la ip, crea un socket, lo conecta con el 
* servidor
* @param ip la ip del server
* @param puerto puerto donde escucha el servidor
* @return Devuelve el descriptor del socket conectado, o valor negativo
* si hay error.
*/
int crear_conexion(const char* ip, const char* puerto);

/**
* @brief Recibe el socket del servidor y el logger del modulo
* @param socket_servidor el socket del servidor que va a aceptar conexiones
* @param logger el logger del modulo
* @return devuelve un socket conectado al socket del servidor
*/
int esperar_cliente(int socket_servidor);

/**
* @brief Envia un string al socket que se pasa por parametro
* @param mensaje el mensaje a enviar
* @param socket el socket de destino
* @param logger el logger del modulo que envia el mensaje
* @return Nada 
*/



#endif




