#include "LimboApp.h"
#include "System.h"

int main() {
    System::HideAllWindows();
    // System::HideTaskbar();
    {
        LimboApp limbo;
        while (limbo.Run());
    }
}
