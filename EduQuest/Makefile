// ============================
// file: Makefile  (single-file build)
// ============================
# Usage: make, make run, make clean
cat > Makefile <<'MK'
CC ?= cc
CFLAGS ?= -std=c17 -Wall -Wextra -O2 -I src 
TARGET := eduquest
SRC := $(wildcard src/*.c) $(wildcard player/*.c)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: run clean
MK 
