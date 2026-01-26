.PHONY: all build run clean install-deps check-prereqs help

# Default target
all: build

# Check for required tools
check-prereqs:
	@echo "Checking prerequisites..."
	@command -v g++ >/dev/null 2>&1 || { echo "Error: g++ not found. Run 'make install-deps' to install."; exit 1; }
	@command -v cmake >/dev/null 2>&1 || { echo "Error: cmake not found. Run 'make install-deps' to install."; exit 1; }
	@pkg-config --exists gtk+-3.0 || { echo "Error: GTK3 not found. Run 'make install-deps' to install."; exit 1; }
	@echo "All prerequisites satisfied!"

# Create build directory and compile the project
build: check-prereqs
	@mkdir -p build
	@cd build && cmake .. && make
	@echo "Build complete! Binary is at: build/bin/GtkAppFolder"

# Run the application
run: build
	@./build/bin/GtkAppFolder

# Clean build artifacts
clean:
	@rm -rf build
	@echo "Build directory cleaned"

# Install required dependencies (auto-detects package manager)
install-deps:
	@./install-deps.sh

# Help target
help:
	@echo "GTK AppImage Browser - Makefile Commands"
	@echo "=========================================="
	@echo ""
	@echo "Available targets:"
	@echo "  make               - Default: build the project"
	@echo "  make build         - Build the project (checks prerequisites first)"
	@echo "  make run           - Build and run the application"
	@echo "  make clean         - Remove build artifacts"
	@echo "  make check-prereqs - Check if required tools are installed"
	@echo "  make install-deps  - Install required dependencies (Debian/Ubuntu)"
	@echo "  make help          - Show this help message"
	@echo ""
	@echo "Quick start:"
	@echo "  1. make install-deps  (first time only)"
	@echo "  2. make run           (build and launch)"

	@echo "  make help         - Show this help message"
