## Sandbox application

Sandbox app linked to Daedalus Engine as a testing ground for engine features. It implements `CreateApplication()` and is the executable target to run/debug.

Build it with any configuration (`Debug`, `Release`, `ReleaseDevTools`, see the root README). The executable is placed in `runtime/bin/<platform>-x64/<Configuration>/`. In Visual Studio, set `dlSandbox` as the startup project.

## Runtime scenes

Scene content is loaded entirely from runtime assets, not compiled into the application. Manually place your scene under `runtime/asset` and select it using `SceneFile` in the `[Path]` section of `runtime/path.ini`. The path is relative to the configured `AssetsFolder` (default: `asset`). Without a setting, the application loads `runtime/asset/scenes/startup.scene.json`.

The two JSON files in `scenes/` are examples only; they are not included in the sandbox project or copied automatically. For example, manually copy `scenes/nuke.scene.json` to `runtime/asset/scenes/nuke.scene.json` and set `SceneFile=scenes/nuke.scene.json`. Provide all referenced models, textures, and skybox images under the asset folder as well. Changing the startup scene does not require rebuilding. See [Scene serialization](../../docs/SceneSerialization.md) for the format and loading workflow.
