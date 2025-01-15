
#include <iostream>

#ifdef _DEBUG

#pragma comment(lib, "SockComm_Debug.lib")

#else

#pragma comment(lib, "SockComm.lib")

#endif // _DEBUG

int main()
{
    std::cout << "Hello World!\n";
}
