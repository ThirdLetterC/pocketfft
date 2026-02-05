set shell := ["bash", "-uc"]

cflags := "-std=c23 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined,leak -g -Iinclude"
ldflags := "-fsanitize=address,undefined,leak -lm"

build:
    cc {{cflags}} src/pocketfft.c examples/ffttest.c -o pocketfft_test {{ldflags}}

test: build
    ./pocketfft_test

fmt:
    clang-format -i src/*.c examples/*.c include/pocketfft/*.h

clean:
    rm -f pocketfft_test a.out
    rm -rf .zig-cache zig-out
