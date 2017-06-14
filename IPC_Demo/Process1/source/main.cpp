#include <iostream>
#include <sstream>
#include <string>
#include <conio.h>
#include <Windows.h>

// change this to change the process number
const int processNumber = 1;

// change this to change the wanted key
const char inputKey = 'A';

HANDLE globalMutexHandle = INVALID_HANDLE_VALUE; // win32 mutex object
HANDLE globalFileMappingHandle = NULL; // a handle to a memory mapped file
void* sharedMemoryPointer = NULL; // a pointer to a shared block of memory

                                  // creates a global win32 mutex
void createMutex()
{
    globalMutexHandle = CreateMutex(NULL, FALSE, "Foo");
    if (globalMutexHandle == INVALID_HANDLE_VALUE)
    {
        throw "Failed to create mutex";
    }
}

// destroys the global mutex
void destroyMutex()
{
    CloseHandle(globalMutexHandle);
    globalMutexHandle = INVALID_HANDLE_VALUE;
}

// try to obtain ownership of the global mutex
// returns true if the mutex was obtained
// returns false otherwise
bool getMutex()
{
    DWORD timeOut = 0;
    switch (WaitForSingleObject(globalMutexHandle, timeOut))
    {
    case WAIT_OBJECT_0:
        // mutex is now owned by this process
        return true;

    case WAIT_TIMEOUT:
        // mutex is already owned by another process
        return false;

    case WAIT_ABANDONED:
        // another process owned the mutex but it terminated
        // mutex may be in an inconsitant state
        throw "WaitForSingleObject abandoned";

    case WAIT_FAILED:
        throw "WaitForSingleObject failed";

    default:
        throw "unknown wait code";
    }
}

// relinquishes ownership of the global mutex
void releaseMutex()
{
    if (!ReleaseMutex(globalMutexHandle))
    {
        throw "ReleaseMutex failed";
    }
}

// creates a block of memory with the specified name and size
void createSharedMemory(const char* name, unsigned numBytes)
{
    // create file mapping object
    globalFileMappingHandle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, numBytes, name);
    if (globalFileMappingHandle == NULL)
    {
        throw "CreateFileMapping failed";
    }

    // creates a view into the file mapping
    // The view is effectively a block of
    // memory backed up by the system paging file.
    sharedMemoryPointer = MapViewOfFile(globalFileMappingHandle, FILE_MAP_ALL_ACCESS, 0, 0, numBytes);
    if (!sharedMemoryPointer)
    {
        throw "MapViewOfFile failed";
    }
}

// releases the block of shared memory
void releaseSharedMemory()
{
    if (sharedMemoryPointer)
    {
        if (UnmapViewOfFile(sharedMemoryPointer))
        {
            sharedMemoryPointer = nullptr;
        }
        else
        {
            throw "UnmapViewOfFile failed";
        }
    }

    if (globalFileMappingHandle)
    {
        CloseHandle(globalFileMappingHandle);
        globalFileMappingHandle = NULL;
    }
}

// returns true if the specified key is being held
bool keyIsDown(int code)
{
    return (GetAsyncKeyState(code) & 0x8000) != 0;
}

// fake game loop that demonstrates mutex ownership
void mutexLoop()
{
    system("cls");
    std::cout << "======================\n";
    std::cout << "Mutex demo - Process " << processNumber << "\n";
    std::cout << "======================\n\n";
    std::cout << "Usage:\n";
    std::cout << "    - Press '" << inputKey << "' to own mutex\n";
    std::cout << "    - Press ESC to quit\n\n";

    std::cout << "Press any key to start game loop...\n\n";
    _getch();

    bool owned = false;
    while (true)
    {
        std::cout << (owned ? "I own the mutex" : "I don't own the mutex") << "\n";

        // Acquire ownership of the mutex as long as the input key is held down
        if (!owned && keyIsDown(inputKey))
        {
            owned = getMutex();
        }
        // when the input key is released release the mutex
        if (owned && !keyIsDown(inputKey))
        {
            releaseMutex();
            owned = false;
        }

        // sleep for 100 seconds to make the text readable
        Sleep(100);

        // Quit the app on ESCAPE
        if (keyIsDown(VK_ESCAPE))
        {
            break;
        }

    } // while
}

// fake game loop that demonstrates writing to shared memory
void sharedMemoryLoop()
{
    system("cls");
    std::cout << "==============================\n";
    std::cout << "Shared Memory demo - Process " << processNumber << "\n";
    std::cout << "==============================\n\n";
    std::cout << "Usage:\n";
    std::cout << "    - Press '" << inputKey << "' to write to shared memory\n";
    std::cout << "    - Press ESC to quit\n\n";

    // write "---------------" into shared memory
    strcpy((char *)sharedMemoryPointer, "---------------");

    std::cout << "Press any key to start game loop...\n\n";
    _getch();

    bool owned = false;
    while (true)
    {
        std::cout << (owned ? "I own the mutex" : "I don't own the mutex") << "\n";

        // Write a different chunk of text into shared memory
        // each time the input key is pressed. But keep ownership
        // of the mutex for as long as the input key is held down.
        if (!owned && keyIsDown(inputKey))
        {
            owned = getMutex();
            if (owned)
            {
                std::ostringstream oss;
                static int count = 0;
                oss << "Hello world " << count++;

                std::cout << "-----------------> writing " << oss.str() << "\n\n";
                strcpy((char *)sharedMemoryPointer, oss.str().c_str());
            }
        }
        // release mutex if the input key is released
        if (owned && !keyIsDown(inputKey))
        {
            releaseMutex();
            owned = false;
        }

        // sleep for 100 seconds to make the text readable
        Sleep(100);

        // Quit the app on ESCAPE
        if (keyIsDown(VK_ESCAPE))
        {
            break;
        }

    } // while
}

int main()
{
    try
    {
        std::cout << "Select demo" << std::endl;
        std::cout << "1: Mutex Demo\n";
        std::cout << "2: Shared Memory Demo\n";

        bool exit = false;
        while (!exit)
        {
            int userChoice = _getch();
            // mutex demo
            if (userChoice == ('1'))
            {
                // create the mutex
                createMutex();
                mutexLoop(); // run mutex loop
                destroyMutex();
                exit = true;
            }
            // shared memory demo
            else if (userChoice == ('2'))
            {
                // create a block of shared memory
                createSharedMemory("MemoryFoo", 10000);
                sharedMemoryLoop(); // run shared memory loop
                releaseSharedMemory();
                exit = true;
            }
        }

        std::cout << "\nFinished! press any key...\n";
        _getch();
    }
    catch (const char* errorMessage)
    {
        std::cout << "EXCEPTION " << errorMessage << std::endl;
    }
    return 0;
}