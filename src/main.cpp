#include "AppImageBrowser.h"
#include <adwaita.h>
#include <iostream>

int main(int argc, char* argv[]) {
    adw_init();
    try {
        AppImageBrowser browser;
        browser.run(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
