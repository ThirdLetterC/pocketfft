# Security Model

`pocketfft` is an in-process C23 FFT library. The public API in
`include/pocketfft/pocketfft.h` exposes only:

- complex FFT plan creation/destruction
- real FFT plan creation/destruction
- in-place forward/backward execution on caller-owned `double` buffers
- plan length queries

The library does not perform network I/O, filesystem I/O, privilege
management, authentication, authorization, sandboxing, or secret storage. Its
direct side effects are limited to heap allocation and mutation of
caller-provided buffers.

Everything passed into the public API should be treated as untrusted input.
That includes plan lengths, transform scaling factors, plan handles, and data
buffer pointers.

## Trust Boundaries

- All public inputs are caller-controlled.
- `make_cfft_plan()` and `make_rfft_plan()` return owning handles. Callers must
  release them with `destroy_cfft_plan()` and `destroy_rfft_plan()`.
- `cfft_forward()` and `cfft_backward()` mutate the caller's complex buffer in
  place.
- `rfft_forward()` and `rfft_backward()` mutate the caller's real-transform
  buffer in place.
- The API does not accept buffer lengths alongside the data pointers. Correct
  buffer sizing is therefore entirely a caller responsibility.
- Plan internals are opaque, heap-backed objects allocated by the library.
  Treat forged, stale, or foreign plan pointers as invalid.

## Protected Properties

- No direct network or filesystem attack surface in library code.
- Allocation failures are checked and reported by returning `nullptr` from plan
  constructors or `-1` from execution entry points.
- Destroy functions accept `nullptr`.
- Public entry points reject `nullptr` plan/data pointers instead of
  dereferencing them.
- A single plan contains precomputed twiddle data and can be reused across
  calls. Temporary work buffers are allocated during execution rather than
  stored in process-global state.

## Security-Relevant Limits And Caller Responsibilities

- This library is not hardened against invalid pointers, dangling pointers,
  undersized writable buffers, or forged object layouts. Passing malformed
  pointers is undefined at the C level.
- `make_cfft_plan(0)` and `make_rfft_plan(0)` fail closed by returning
  `nullptr`, but very large lengths can still trigger substantial allocation
  requests. Callers must enforce input size limits.
- FFT execution APIs only receive a raw `double *` plus a scale factor. They
  cannot verify that the pointed-to storage is large enough for the selected
  plan.
- As currently exercised by `examples/ffttest.c`, complex FFTs operate on
  interleaved real/imaginary storage and real FFTs operate on packed real
  storage. Embedders must preserve that layout contract.
- Execution is in-process and CPU-intensive. Adversarially chosen lengths can
  be used for denial of service through memory pressure or compute cost if the
  embedding application does not bound work.
- Public execution entry points reject non-finite scale factors by returning
  `-1`.
- Buffer contents are treated as ordinary numeric data. The library does not
  reject `NaN`, `Inf`, denormals, or adversarial values chosen to stress
  numeric edge cases inside caller-provided buffers.
- No constant-time or side-channel-resistant behavior is claimed.
- This library is not a sandbox or policy engine. Validation of user input,
  rate limiting, provenance, tenant isolation, and process-level containment
  belongs in the embedding application.

## Concurrency

The current implementation allocates temporary arrays per transform call and
stores precomputed tables inside the plan object. The project README states
that plans contain read-only data and may be invoked from several threads at
the same time.

That means:

- sharing a single plan across threads is an intended use case
- each thread still needs its own caller-owned input/output buffer
- the library does not provide its own locking or scheduling

Applications with stricter concurrency requirements should still validate this
behavior under their own toolchain and sanitizer configuration.

## Defensive Posture In This Codebase

- In-tree build metadata targets C23 via `-std=c23`.
- Warning policy is strict: `-Wall -Wextra -Wpedantic -Werror`.
- The `justfile` test path enables `-fsanitize=address,undefined,leak`.
- Plan construction and internal workspace allocation use checked heap
  allocation paths and fail by returning `nullptr` on allocation failure.
- Cleanup paths are centralized with explicit destroy helpers for all public
  owning objects.

## Verification

Validation paths present in this checkout:

- `zig build`
- `zig build test`
- `just test`

The current wired-in test coverage is `testing/tests.c`, which exercises
public API rejection paths for `nullptr`, oversized lengths, and non-finite
scale factors, plus reference-checked real and complex transforms through
length `97`. The repository also ships `examples/ffttest.c` as a larger
numeric sweep from `1` to `8'192`, but that example binary is not part of the
default `zig build test` or `just test` flow. The repository does not
currently include dedicated fuzzing harnesses or a published hardening profile
beyond the sanitizer-enabled `just test` build.

## Supported Versions

This repository does not currently publish a maintained-branches matrix.
Security fixes should be assumed to target the current default branch unless
the maintainer states otherwise.

## Reporting

This repository does not currently publish a dedicated private security contact
in-tree. If you need to report a vulnerability, use a maintainer-controlled
private channel when one is available and avoid posting exploit details
publicly before the issue has been assessed.
