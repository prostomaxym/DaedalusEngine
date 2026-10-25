---
applyTo: "source/dlUnitTests/**"
---
# Unit test rules
- Framework: GoogleTest/GoogleMock (from vcpkg).
- Test names: `TEST(<Subject>Test, <behavior_in_snake_case>)`; one behavior per test.
- Tests must be deterministic, fast, and must not need a window, GL context, GPU or files outside the repo.
- Floating-point comparisons: use `EXPECT_NEAR` / `EXPECT_FLOAT_EQ`, never `EXPECT_EQ`.
- Register new test files in `source/dlUnitTests/CMakeLists.txt` (`PROJECT_FILES`).
- Run: `ctest --test-dir build -C Release --output-on-failure`.
