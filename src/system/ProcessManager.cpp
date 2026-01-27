#include "ProcessManager.h"
#include <iostream>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <filesystem>

namespace fs = std::filesystem;

namespace System {

bool ProcessManager::launch_appimage(const std::string& path) {
    std::cout << "Launching: " << path << std::endl;
    
    // Make executable if needed
    if (!make_executable(path)) {
        std::cerr << "Failed to make executable: " << path << std::endl;
        return false;
    }
    
    // Launch the AppImage in background
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        execl(path.c_str(), path.c_str(), nullptr);
        // If execl returns, there was an error
        std::cerr << "Failed to launch: " << path << std::endl;
        exit(1);
    } else if (pid < 0) {
        std::cerr << "Failed to fork process" << std::endl;
        return false;
    }
    
    return true;
}

bool ProcessManager::make_executable(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        if (!(st.st_mode & S_IXUSR)) {
            // Make it executable
            return chmod(path.c_str(), st.st_mode | S_IXUSR | S_IXGRP | S_IXOTH) == 0;
        }
        return true;
    }
    return false;
}

bool ProcessManager::has_fuse2() {
    // Check if libfuse2 is installed by looking for common FUSE2 files
    std::vector<std::string> fuse2_paths = {
        "/usr/lib/libfuse.so.2",
        "/usr/lib64/libfuse.so.2",
        "/usr/lib/x86_64-linux-gnu/libfuse.so.2",
        "/lib/x86_64-linux-gnu/libfuse.so.2"
    };
    
    for (const auto& path : fuse2_paths) {
        if (fs::exists(path)) {
            return true;
        }
    }
    
    // Also try running fusermount
    int result = system("which fusermount > /dev/null 2>&1");
    return WIFEXITED(result) && WEXITSTATUS(result) == 0;
}

std::string ProcessManager::detect_package_manager() {
    std::vector<std::string> managers = {
        "apt-get", "dnf", "yum", "pacman", "zypper"
    };
    
    for (const auto& manager : managers) {
        std::string cmd = "which " + manager + " > /dev/null 2>&1";
        int result = system(cmd.c_str());
        if (WIFEXITED(result) && WEXITSTATUS(result) == 0) {
            return manager;
        }
    }
    
    return "";
}

bool ProcessManager::install_fuse2_requirements() {
    std::string pkg_manager = detect_package_manager();
    
    if (pkg_manager.empty()) {
        std::cerr << "No supported package manager found" << std::endl;
        return false;
    }
    
    std::string install_cmd;
    
    if (pkg_manager == "apt-get") {
        install_cmd = "pkexec apt-get install -y libfuse2";
    } else if (pkg_manager == "dnf") {
        install_cmd = "pkexec dnf install -y fuse-libs";
    } else if (pkg_manager == "yum") {
        install_cmd = "pkexec yum install -y fuse-libs";
    } else if (pkg_manager == "pacman") {
        install_cmd = "pkexec pacman -S --noconfirm fuse2";
    } else if (pkg_manager == "zypper") {
        install_cmd = "pkexec zypper install -y libfuse2";
    } else {
        return false;
    }
    
    std::cout << "Installing FUSE2 with: " << install_cmd << std::endl;
    int result = system(install_cmd.c_str());
    
    return WIFEXITED(result) && WEXITSTATUS(result) == 0;
}

bool ProcessManager::open_in_file_manager(const std::string& path) {
    // Try xdg-open first (most compatible)
    std::string cmd = "xdg-open \"" + path + "\" &";
    int result = system(cmd.c_str());
    
    if (WIFEXITED(result) && WEXITSTATUS(result) == 0) {
        return true;
    }
    
    // Fallback to nautilus, dolphin, thunar
    std::vector<std::string> file_managers = {"nautilus", "dolphin", "thunar"};
    for (const auto& fm : file_managers) {
        cmd = fm + " \"" + path + "\" &";
        result = system(cmd.c_str());
        if (WIFEXITED(result) && WEXITSTATUS(result) == 0) {
            return true;
        }
    }
    
    return false;
}

} // namespace System
