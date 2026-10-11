#include <windows.h>

namespace Catelier::test::architecture::imp { void runTests(); }

int main() {
    SetConsoleOutputCP(CP_UTF8);

    Catelier::test::architecture::imp::runTests();

    return 0;
}