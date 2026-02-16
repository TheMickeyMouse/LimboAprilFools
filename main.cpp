#include "LimboApp.h"
#include "System.h"

int main() {
    System::HideAllWindows();
#ifdef NDEBUG
    System::HideConsole();
#endif
    // System::HideTaskbar();
    {
        LimboApp limbo;
        while (limbo.Run());
    }
}
