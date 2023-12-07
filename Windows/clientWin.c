#include "../src/windows/mainFuctsWin.c"

SERVICE_STATUS        ServiceStatus;
SERVICE_STATUS_HANDLE hStatus;
HANDLE                hThread = NULL;
DWORD                 dwErr = 0;

void SvcMn(int argc, char** argv);
void CtrlHdlr(DWORD request);
DWORD WINAPI WorkerThread(LPVOID lpParam);

int AsSvc() {
    SERVICE_TABLE_ENTRY ServiceTable[2];
    ServiceTable[0].lpServiceName = "VMService";
    ServiceTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION)SvcMn;
    ServiceTable[1].lpServiceName = NULL;
    ServiceTable[1].lpServiceProc = NULL;

    StartServiceCtrlDispatcher(ServiceTable);
    return 0;
}

void SvcMn(int argc, char** argv) {
    ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    ServiceStatus.dwWin32ExitCode = 0;
    ServiceStatus.dwServiceSpecificExitCode = 0;
    ServiceStatus.dwCheckPoint = 0;
    ServiceStatus.dwWaitHint = 0;
    hStatus = RegisterServiceCtrlHandler("VMService", (LPHANDLER_FUNCTION)CtrlHdlr);
    if (hStatus == (SERVICE_STATUS_HANDLE)0) {
        // Handle error
        return;
    }
    ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(hStatus, &ServiceStatus);

    // Create a worker thread to do the actual work
    hThread = CreateThread(NULL, 0, WorkerThread, NULL, 0, NULL);

    // Wait for the worker thread to exit
    WaitForSingleObject(hThread, INFINITE);
    return;
}

void CtrlHdlr(DWORD request) {
    switch (request) {
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            ServiceStatus.dwWin32ExitCode = 0;
            ServiceStatus.dwCurrentState = SERVICE_STOPPED;
            SetServiceStatus(hStatus, &ServiceStatus);
            return;
        default:
            break;
    }
    SetServiceStatus(hStatus, &ServiceStatus);
    return;
}

DWORD WINAPI WorkerThread(LPVOID lpParam) {
    cnctn();
    WSACleanup(); // Clean up Winsock
    return 0;
}

int startAPP(){
    cnctn();
    WSACleanup(); // Clean up Winsock
    return 0;
}

int main(int argc, char const *argv[]){
    if (argc > 1 && strcmp(argv[1], "service") == 0) { 
        AsSvc();
    } else {
        startAPP();
    }
}
