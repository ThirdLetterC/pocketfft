set shell := ["bash", "-uc"]

cflags := "-std=c2x -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined,leak -g"
ldflags := "-fsanitize=address,undefined,leak -lm"

build:
    cc {{cflags}} pocketfft.c ffttest.c -o pocketfft_test {{ldflags}}

test: build
    ./pocketfft_test

fmt:
    clang-format -i *.c *.h
