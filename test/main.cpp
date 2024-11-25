#if defined(_WIN32)
#include <Windows.h>
#else
#include <unistd.h>
#endif
#include <ImFileDialog.hpp>
#include <iostream>

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    ImFileDialog::OpenDialog *dialog = new ImFileDialog::OpenDialog("All:*.*");

    std::cout << "sleep" << std::endl;
#if defined(_WIN32)
    Sleep(1 * 1000);
#else
    sleep(1);
#endif

    std::cout << "delete" << std::endl;
    delete dialog;

    return 0;
}
