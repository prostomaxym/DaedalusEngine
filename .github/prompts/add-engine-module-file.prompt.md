---
mode: agent
description: Add a new class/system to a dlEngine module
---
Add ${input:feature:new class or system} to the `${input:module:dlEngine module, e.g. Renderer}` module.
1. Inspect the module for existing patterns and reuse them.
2. Create `.h/.cpp` following `.github/copilot-instructions.md` conventions.
3. Add files to the module `CMakeLists.txt`.
4. Add unit tests if the logic is testable without GL.
5. Build, run tests, and check Linux-compatibility (include case, no Windows-only APIs).
