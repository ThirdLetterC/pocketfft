set shell := ["bash", "-lc"]

cc := env_var_or_default("CC", "cc")
cflags := "-std=c2x -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined,leak -g -Iinclude"
ldflags := "-fsanitize=address,undefined,leak -lm"

build:
    {{cc}} {{cflags}} src/pocketfft.c testing/tests.c -o pocketfft_test {{ldflags}}

test: build
    ./pocketfft_test

fmt:
    clang-format -i src/*.c examples/*.c testing/*.c include/pocketfft/*.h

clean:
    rm -f pocketfft_test a.out
    rm -rf .zig-cache zig-out
