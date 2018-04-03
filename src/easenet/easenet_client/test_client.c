
#include <stdio.h>
#include <enet.h>

#pragma comment(lib, "easenet.lib")


#define CLIENTS_COUNT_PER_THREAD 50
#define TEST_THREADS_COUNT 1
#define TEST_DATA_SIZE 9000

    char data[TEST_DATA_SIZE];

UINT64 volatile recvDataSize = 0;
DWORD volatile recvDataCount = 0;

UINT64 volatile sendDataSize = 0;
DWORD volatile sendDataCount = 0;

LONG volatile errorCount = 0;
LONG volatile threadsCount = TEST_THREADS_COUNT;
LONG volatile clientsCount = 0;

CRITICAL_SECTION __g_client_lock;
CRITICAL_SECTION __g_server_lock;

#define client_lock() EnterCriticalSection(&__g_client_lock)
#define client_unlock() LeaveCriticalSection(&__g_client_lock)
#define server_lock() EnterCriticalSection(&__g_server_lock)
#define server_unlock() LeaveCriticalSection(&__g_server_lock)

void gotoxy(int x, int y)
{
    COORD pos;  

    pos.X=x;
    pos.Y=y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE),pos);
}

int _stdcall callback(enet_handle_t* client, void* data1, int size, void* param)
{
    char data2[TEST_DATA_SIZE];
    int len;

    client_lock();

    sendDataSize+=size;

    client_unlock();

    //printf("[%llu]client recv %d bytes\n", GetTickCount64(), size);
    //len = rand()%1000+4000;
    len = TEST_DATA_SIZE;
    //printf("[%llu]client send %d bytes\n", GetTickCount64(), len);
    enet_tcp_client_send(client, data2, len);

    return 0;
}

int ENET_CALLBACK server_callback(enet_handle_t* server, ENET_TCP_SERVER_CLIENT* client, void* data1, int size, void* param)
{
    // Echo
    server_lock();

    recvDataSize+=size;

    server_unlock();

    enet_tcp_server_send_to_client(server, client, data1, size);

    return 0;
}

void init_data(char* data, int size)
{
    int i = 0;

    while (size--) {
        data[size] = size%256;
    }
}

int testing = 0;

DWORD WINAPI server_pressure_test_thread(LPVOID param)
{
    enet_handle_t* client[CLIENTS_COUNT_PER_THREAD];
    enet_status_t rc = 0;
    int i;

    init_data(data, TEST_DATA_SIZE);
    data[TEST_DATA_SIZE-1] = 0;

    for (i = 0; i < CLIENTS_COUNT_PER_THREAD; i++) {
        rc = enet_new_tcp_client(inet_addr("127.0.0.1"), 3244, ENET_CLIENT_ATTR_USE_PACKET, 0, callback,  &client[i]);
        if (rc != ENET_CODE_OK) {
            printf("create cilent failed (%d)\n", rc);
            InterlockedDecrement(&threadsCount);
            InterlockedIncrement(&errorCount);
            return 0;
        }
        rc = enet_tcp_client_connect(client[i], 5000);
        if (rc != ENET_CODE_OK) {
            enet_delete_tcp_client(client[i]);
            printf("connect to server failed (%d)\n", rc);
            InterlockedDecrement(&threadsCount);
            InterlockedIncrement(&errorCount);
            return 0;
        }
        Sleep(10*TEST_THREADS_COUNT);
        rc = enet_tcp_client_send(client[i], data, TEST_DATA_SIZE);
    }

   // rc = enet_tcp_client_send(client[0], data, TEST_DATA_SIZE);
    
    while (testing) {

        Sleep(1000);
    }

    
    for (i = 0; i < CLIENTS_COUNT_PER_THREAD; i++) {
        enet_tcp_client_disconnect(client[i]);
        enet_delete_tcp_client(client[i]);
    }

    return 0;
}

DWORD WINAPI display_result_thread(LPVOID param)
{
    INT64 time;
    INT64 startTime = GetTickCount64();

    while (testing) {
        gotoxy(0, 0);

        time = GetTickCount64() - startTime + 1;

        client_lock();
        server_lock();

        printf(
            "======= TEST RESULT =======\n"
            "Threads count  : %d\n"
            "Error Count    : %d\n"
            "Server download: %llu bytes, %llu MB/sec\n"
            "Server upload  : %llu bytes, %llu MB/sec\n"
            ,threadsCount, errorCount, sendDataSize, (INT64)sendDataSize*1000/1024/1024/time, recvDataSize
            , (INT64)recvDataSize*1000/1024/1024/time);

        client_unlock();
        server_unlock();

        Sleep(200);
    }

    return 0;
}


void quit(char* msg)
{
    printf(msg);
    getchar();
    exit(0);
}


void main()
{
    HANDLE thread[TEST_THREADS_COUNT];
    DWORD threadId[TEST_THREADS_COUNT];
    enet_handle_t *server;
    enet_status_t rc;

    int i = 0;
    
    InitializeCriticalSection(&__g_client_lock);
    InitializeCriticalSection(&__g_server_lock);

    testing = 1;

    rc = enet_new_tcp_server(3244, ENET_SERVER_ATTR_USE_PACKET | 1, 500, 0, 0, server_callback, 0, &server);
    if (rc != ENET_CODE_OK) {
        quit("create server failed");
    }

    rc = enet_start_tcp_server(server);
    if (rc != ENET_CODE_OK) {
        quit("start server failed");
    }

    CreateThread(0, 0, display_result_thread, 0, 0, 0);

    for (i = 0; i < TEST_THREADS_COUNT; i++) {
        thread[i] = CreateThread(0, 0, server_pressure_test_thread, &threadId[i], 0, &threadId[i]);
        if (thread[i] == 0 || thread[i] == INVALID_HANDLE_VALUE) {
            quit("create thread failed");
        }
    }

    getchar();

    testing = 0;
    printf("shutting down...\n");

    WaitForMultipleObjects(TEST_THREADS_COUNT, thread, TRUE, INFINITE);

}
