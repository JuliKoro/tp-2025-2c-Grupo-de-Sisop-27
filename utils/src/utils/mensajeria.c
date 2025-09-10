#include "mensajeria.h"


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
        close(socket_servidor);
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
    printf("Enhorabuena!!! Se conecto un cliente.");
    return socket_cliente;
}