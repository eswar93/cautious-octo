# Modern C++ in this project (and what to avoid on a microcontroller)

## Used here — find each one in the code
| Feature | Where | Why it's good embedded C++ |
|---|---|---|
| Class templates + `static_assert` | `SampleWindow<N>`, `FixedQueue<T,N>` | Capacity is a compile-time constant: no heap, size errors caught at build time |
| `constexpr` / `inline constexpr` | `kModel`, `predict_rule`, `Debouncer` | Model weights live in flash, evaluated at compile time where possible |
| `std::array`, `std::span` | everywhere | Bounds-aware views instead of raw pointer + length pairs |
| `std::optional` | `features()`, `poll()`, `pop()` | "No result" is part of the type; replaces `bool` + out-parameter |
| `enum class` | `AlertKind` | No implicit int conversions; `switch` is checked by `-Wswitch` |
| `[[nodiscard]]`, `noexcept` | most functions | Compiler flags ignored results; `noexcept` documents/enables no-throw code |
| Designated initializers (C++20) | `kModel`, tests | Self-documenting aggregate init, order-checked by the compiler |
| RAII-style value types | `Pipeline` owns its state | No manual init/free pairs |
| `std::source_location` | test helper `check()` | Line numbers in test failures without macros |

## Deliberately NOT used in firmware
- Exceptions, RTTI (`-fno-exceptions -fno-rtti`): code size and unpredictable behaviour.
- Heap (`new`, `std::vector`, `std::string`, `std::function`): fragmentation, non-deterministic timing.
- Function-local statics (need thread-safe-init guards; we build with `-fno-threadsafe-statics`).
- `<iostream>`: large; only the host tests use it.

## Exercises (in increasing difficulty)
1. Add a `concept` (C++20) that constrains `FixedQueue<T,N>` to trivially copyable `T`, replacing the `static_assert`.
2. Make `Pipeline` a template on a `Predictor` policy so the logistic model and the `predict_rule` baseline are swappable at compile time; compare their alert timing in a test.
3. Replace the `Features` array + index namespace with a small struct and `constexpr` accessors; keep the golden test green.
4. Port `esp32_fw/main` to C++ (`main.cpp`; ESP-IDF supports it) and share the sensor-parsing logic as a header-only class with host tests.
5. Add `clang-tidy` (`modernize-*`, `cppcoreguidelines-*`) and `-fsanitize=address,undefined` to the host test build.
