# Makefile for fastkst_localtime
# Author: lmk (newtypez@gmail.com)

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -O2 -fPIC
LDFLAGS = -lpthread

# Target names
LIB_NAME = libfastkst_localtime
TEST_NAME = fastkst_localtime_test
STATIC_LIB = $(LIB_NAME).a
SHARED_LIB = $(LIB_NAME).so
EXAMPLE = example

# Source files
SRC = fastkst_localtime.c
OBJ = fastkst_localtime.o
TEST_OBJ = fastkst_localtime_test.o
EXAMPLE_SRC = example.c

# Installation directories
PREFIX ?= /usr/local
LIBDIR = $(PREFIX)/lib
INCLUDEDIR = $(PREFIX)/include

# Default target
.PHONY: all
all: static shared

# Build static library
.PHONY: static
static: $(STATIC_LIB)

$(STATIC_LIB): $(OBJ)
	ar rcs $@ $^
	@echo "Static library built: $(STATIC_LIB)"

# Build shared library
.PHONY: shared
shared: $(SHARED_LIB)

$(SHARED_LIB): $(SRC)
	$(CC) $(CFLAGS) -shared -o $@ $^
	@echo "Shared library built: $(SHARED_LIB)"

# Build object file
$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c -o $@ $<

# Build test executable
.PHONY: test
test: $(TEST_NAME)

$(TEST_NAME): $(SRC)
	$(CC) $(CFLAGS) -DTEST_FASTKST_LOCALTIME -o $@ $< $(LDFLAGS)
	@echo "Test executable built: $(TEST_NAME)"

# Build example program
.PHONY: example
example: $(EXAMPLE)

$(EXAMPLE): $(EXAMPLE_SRC) $(STATIC_LIB)
	$(CC) $(CFLAGS) -o $@ $< $(STATIC_LIB)
	@echo "Example program built: $(EXAMPLE)"

# Run test
.PHONY: run-test
run-test: test
	@echo "Running tests..."
	./$(TEST_NAME)

# Run benchmark only (requires test executable)
.PHONY: benchmark
benchmark: test
	@echo "Running benchmark..."
	./$(TEST_NAME) | grep -A 20 "Performance Benchmark"

# Clean build artifacts
.PHONY: clean
clean:
	rm -f $(OBJ) $(TEST_OBJ) $(STATIC_LIB) $(SHARED_LIB) $(TEST_NAME) $(EXAMPLE)
	@echo "Clean complete"

# Install libraries and headers
.PHONY: install
install: static shared
	install -d $(DESTDIR)$(LIBDIR)
	install -d $(DESTDIR)$(INCLUDEDIR)
	install -m 644 $(STATIC_LIB) $(DESTDIR)$(LIBDIR)/
	install -m 755 $(SHARED_LIB) $(DESTDIR)$(LIBDIR)/
	@echo "Installation complete"
	@echo "Note: Header installation requires creating a header file first"

# Uninstall libraries
.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(LIBDIR)/$(STATIC_LIB)
	rm -f $(DESTDIR)$(LIBDIR)/$(SHARED_LIB)
	@echo "Uninstall complete"

# Create header file (extract declarations from source)
.PHONY: header
header: fastkst_localtime.h

fastkst_localtime.h:
	@echo "Creating header file..."
	@echo "/**" > $@
	@echo " * @file fastkst_localtime.h" >> $@
	@echo " * @brief High performance KST (UTC+9) localtime implementation" >> $@
	@echo " * @author lmk (newtypez@gmail.com)" >> $@
	@echo " * @version 0.1" >> $@
	@echo " * @date 2025-12-31" >> $@
	@echo " */" >> $@
	@echo "" >> $@
	@echo "#ifndef FASTKST_LOCALTIME_H" >> $@
	@echo "#define FASTKST_LOCALTIME_H" >> $@
	@echo "" >> $@
	@echo "#include <time.h>" >> $@
	@echo "" >> $@
	@echo "/**" >> $@
	@echo " * @brief High performance localtime for KST (64-bit safe version)" >> $@
	@echo " * @param[in] t time_t (supports 64-bit)" >> $@
	@echo " * @param[out] tp struct tm" >> $@
	@echo " * @return int 1 success, 0 fail" >> $@
	@echo " */" >> $@
	@echo "int fastkst_localtime(time_t t, struct tm *tp);" >> $@
	@echo "" >> $@
	@echo "/**" >> $@
	@echo " * @brief Thread-safe wrapper with additional validation" >> $@
	@echo " * @param[in] t time_t" >> $@
	@echo " * @param[out] tp struct tm" >> $@
	@echo " * @param[out] err_code error code (optional, can be NULL)" >> $@
	@echo " * @return int 1 success, 0 fail" >> $@
	@echo " */" >> $@
	@echo "int fastkst_localtime_safe(time_t t, struct tm *tp, int *err_code);" >> $@
	@echo "" >> $@
	@echo "#endif /* FASTKST_LOCALTIME_H */" >> $@
	@echo "Header file created: $@"

# Create example source file
.PHONY: create-example
create-example:
	@echo "Creating example source file..."
	@echo "#include <stdio.h>" > $(EXAMPLE_SRC)
	@echo "#include <time.h>" >> $(EXAMPLE_SRC)
	@echo "" >> $(EXAMPLE_SRC)
	@echo "int fastkst_localtime(time_t t, struct tm *tp);" >> $(EXAMPLE_SRC)
	@echo "int fastkst_localtime_safe(time_t t, struct tm *tp, int *err_code);" >> $(EXAMPLE_SRC)
	@echo "" >> $(EXAMPLE_SRC)
	@echo "int main(void) {" >> $(EXAMPLE_SRC)
	@echo "    time_t now = time(NULL);" >> $(EXAMPLE_SRC)
	@echo "    struct tm kst_time;" >> $(EXAMPLE_SRC)
	@echo "    int error_code;" >> $(EXAMPLE_SRC)
	@echo "" >> $(EXAMPLE_SRC)
	@echo "    printf(\"Current Unix timestamp: %lld\\\\n\", (long long)now);" >> $(EXAMPLE_SRC)
	@echo "" >> $(EXAMPLE_SRC)
	@echo "    if (fastkst_localtime_safe(now, &kst_time, &error_code) == 1) {" >> $(EXAMPLE_SRC)
	@echo "        printf(\"KST Time: %04d-%02d-%02d %02d:%02d:%02d %s\\\\n\"," >> $(EXAMPLE_SRC)
	@echo "               kst_time.tm_year + 1900," >> $(EXAMPLE_SRC)
	@echo "               kst_time.tm_mon + 1," >> $(EXAMPLE_SRC)
	@echo "               kst_time.tm_mday," >> $(EXAMPLE_SRC)
	@echo "               kst_time.tm_hour," >> $(EXAMPLE_SRC)
	@echo "               kst_time.tm_min," >> $(EXAMPLE_SRC)
	@echo "               kst_time.tm_sec," >> $(EXAMPLE_SRC)
	@echo "               kst_time.tm_zone);" >> $(EXAMPLE_SRC)
	@echo "        printf(\"Day of week: %d, Day of year: %d\\\\n\"," >> $(EXAMPLE_SRC)
	@echo "               kst_time.tm_wday, kst_time.tm_yday);" >> $(EXAMPLE_SRC)
	@echo "    } else {" >> $(EXAMPLE_SRC)
	@echo "        fprintf(stderr, \"Error: %d\\\\n\", error_code);" >> $(EXAMPLE_SRC)
	@echo "        return 1;" >> $(EXAMPLE_SRC)
	@echo "    }" >> $(EXAMPLE_SRC)
	@echo "" >> $(EXAMPLE_SRC)
	@echo "    return 0;" >> $(EXAMPLE_SRC)
	@echo "}" >> $(EXAMPLE_SRC)
	@echo "Example source file created: $(EXAMPLE_SRC)"

# Help target
.PHONY: help
help:
	@echo "fastkst_localtime Makefile targets:"
	@echo ""
	@echo "  make              - Build both static and shared libraries"
	@echo "  make static       - Build static library ($(STATIC_LIB))"
	@echo "  make shared       - Build shared library ($(SHARED_LIB))"
	@echo "  make test         - Build test executable"
	@echo "  make run-test     - Build and run all tests"
	@echo "  make benchmark    - Build and run performance benchmark"
	@echo "  make example      - Build example program"
	@echo "  make create-example - Create example source file"
	@echo "  make header       - Generate header file"
	@echo "  make install      - Install libraries (PREFIX=$(PREFIX))"
	@echo "  make uninstall    - Uninstall libraries"
	@echo "  make clean        - Remove all build artifacts"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Variables:"
	@echo "  PREFIX=$(PREFIX)  - Installation prefix"
	@echo "  CC=$(CC)          - C compiler"
	@echo "  CFLAGS=$(CFLAGS)  - Compiler flags"
