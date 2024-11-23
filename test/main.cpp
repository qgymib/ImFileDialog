#include <Windows.h>
#include <ImFileDialog.hpp>
#include <iostream>

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    ImFileDialog::FileDialog *dialog = new ImFileDialog::FileDialog("", "All\n*.*");

    std::cout << "sleep" << std::endl;
    Sleep(3 * 1000);

    std::cout << "delete" << std::endl;
    delete dialog;

    return 0;
}
