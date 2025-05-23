#include "DemoApp.h"

int main() {
    DemoApp* app = new DemoApp();
    if (!app->init(1400,900)) return -1;
    app->run();
    return 0;
}
