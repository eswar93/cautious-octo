# Host-side tests for the portable core.  Windows: use MSYS2/MinGW or WSL2.
CC ?= gcc
CFLAGS = -std=c99 -Wall -Wextra -Werror -O1
test: test/test_core.c tw.c link_monitor.c predictor.c
	$(CC) $(CFLAGS) -o test_core $^ -lm
	./test_core
clean:
	rm -f test_core test_core.exe
