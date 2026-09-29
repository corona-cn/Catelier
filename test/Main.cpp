#include <windows.h>

#include "util/Logger.hpp"
#include "demo/catelier/foundation/ArrayList.hpp"
#include "demo/catelier/foundation/SinglyLinkedList.hpp"
#include "demo/catelier/foundation/DoublyLinkedList.hpp"
#include "demo/catelier/foundation/CircularLinkedList.hpp"

using namespace Catelier;
int main() {
    SetConsoleOutputCP(CP_UTF8);

    {
        using namespace test::demo::foundation;
        // demoArrayList();
        // demoSinglyLinkedList();
        // demoDoublyLinkedList();
        demoCircularLinkedList();
    }

    return 0;
}