#include <ImFileDialog.hpp>
#include <iostream>

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    ImFileDialog::FileDialog dialog("", "All\n*.*");
    while (!dialog.Query())
    {
    }

    ImFileDialog::StringVec result;
    bool ret = dialog.GetResult(result);
    std::cout << "ret:" << ret << std::endl;

    return 0;
}
