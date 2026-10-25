---
mode: agent
description: Add gtest unit tests for a class or function
---
Write unit tests for ${input:target:class or function to test}.
1. Read the target and its dependencies; identify behaviors, edge cases and error paths.
2. Add tests in `source/dlUnitTests` following `.github/instructions/tests.instructions.md`.
3. Register the new file in `source/dlUnitTests/CMakeLists.txt`.
4. Build and run the tests; report results. If the code is untestable without GL/window, suggest the minimal refactor instead of hacking around it.
