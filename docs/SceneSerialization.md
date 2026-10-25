# Scene serialization

Daedalus stores authored scenes as UTF-8 JSON. Persistent records are separate from runtime entities, GPU resources, Bullet handles, and native-script instances. The parser uses the explicit, header-only `nlohmann-json` vcpkg dependency privately within ECS; client code does not depend on JSON types.

## Client workflow

1. Create an empty `Scene` and an application-owned `ScriptRegistry`.
2. Register the application's native-script adapters. The sandbox's `RegisterSceneScripts` in `source/dlSandbox/SceneScripts.cpp` is a complete example.
3. Call `SceneSerializer::Load(scene_file, scene, asset_root, scripts, error)`.
4. If loading succeeds, call `scene.OnRuntimeStart()` and check its boolean result. The loaded name and primary-camera UUID are retained.
5. Forward updates and viewport resizes as usual. `scene.Clear()` stops runtime and releases the scene's entities/resources.

The sandbox defaults to `scenes/nuke.scene.json`, relative to `PathConfig::GetAssetsPath()`. Pass `"scenes/anor_londo.scene.json"` to `ExampleLayer`'s constructor to select the other example. CMake deploys the two source-owned files from `source/dlSandbox/scenes` to the runtime asset directory. Installation also includes them under `asset/scenes`. Edit the source copies, not generated runtime copies.

### API responsibilities

| API | Purpose |
| --- | --- |
| `SceneJson::Parse` / `Serialize` | Convert JSON text and persistent records; no graphics/physics initialization required. |
| `SceneJson::Load` / `Save` | Read/write persistent records as JSON files. |
| `SceneJson::Validate` | Validate the v1 record schema and renderer shadow-map budget. |
| `SceneSerializer::Instantiate` | Build a runtime `Scene` from records. |
| `SceneSerializer::Capture` | Capture a stopped scene into records. |
| `SceneSerializer::Load` / `Save` | Combine the file and runtime operations. |

All operations return `bool` and provide a diagnostic in `error`; log it through the engine logger. Parse/capture failures leave their output records unchanged. Loading requires an **empty, stopped** target. Invalid documents, script bindings, and missing assets are rejected before construction; construction failures remove newly created entities and restore the target's previous name and paused state. Failed resources are not cached as successful models or cubemaps. Successfully loaded resources may remain in the existing resource-manager caches after a failed scene load.

Resource instantiation must run on the application/GL-context thread after rendering, physics, and resource services have been initialized. Record parsing/validation is independent of those services. Loading does not start runtime. Loading a camera-free record is permitted for tools/tests, but starting runtime requires a camera and an application/window.

## Version 1 file structure

See `source/dlSandbox/scenes/nuke.scene.json` for a complete hand-authored example, and `source/dlEngine/ECS/Serialization/SceneRecord.h` for every property and its default.

### Scene

- `version`: required integer, currently `1`. Unsupported versions fail; there is no implicit migration.
- `name`: scene display name, defaults to an empty string.
- `primary_camera`: entity UUID string. Required when any camera is present; it must reference an entity with a camera. Selection does not depend on the entity's name.
- `entities`: required array of entity objects.

### Entities and components

Every entity requires `id`, a unique, nonzero decimal uint64 **string**. Strings preserve UUIDs above JavaScript's exact-integer range. Leading zeros, duplicate IDs, and invalid/overflowing IDs fail. `name` defaults to `Entity` and must not be empty.

Components are named fields on each entity, not C++ type names or memory snapshots:

| Field | Persistent contents |
| --- | --- |
| `transform` | `translation`, `rotation`, `scale`, each a three-number array. Rotation is pitch/yaw/roll in degrees. Defaults are zero translation/rotation and unit scale; zero scale is rejected. |
| `model` | Asset-root-relative `path` and numeric `import_flags` (`ModelParserFlags`/Assimp bitmask). Model materials and textures continue to be imported with the model; custom runtime material overrides are not saved. |
| `camera` | `projection` (`perspective` or `orthographic`), position/up/yaw/pitch, projection parameters, viewport policy, and control settings. |
| `directional_light`, `point_light`, `spot_light` | Position, direction, ambient/diffuse/specular colors, power, range, angles, shadow settings. |
| `skybox` | Six `faces` in right, left, top, bottom, front, back order, plus rotation in degrees. Images must decode to matching square faces. |
| `rigid_body`, `kinematic_body` | Collider shape, initial pose, dimensions, mass, and optional model-bound derivation settings. An entity cannot have both body types. |
| `scripts` | Array of stable registered `type` strings and `properties` objects. |

Omitted optional components are absent. Omitted configuration properties use record defaults; explicit `null` is not an omitted component. Unknown fields/components, duplicate JSON object keys, duplicate script types, invalid UTF-8, nonfinite/out-of-range numbers, and wrong value types fail rather than being silently dropped.

### Cameras

Perspective cameras use `fov` in degrees, `aspect_ratio`, `near_plane`, and `far_plane`. With `viewport_aspect: true` (the default), the aspect ratio updates from the actual viewport at runtime startup and resize. Set it to `false` for a fixed aspect ratio. Orthographic cameras use `left`, `right`, `bottom`, `top`, `near_plane`, and `far_plane`.

`position`, `up`, `yaw`, and `pitch` define the camera's view independently of the entity transform, preserving existing engine behavior. `movement_speed`, `sensitivity`, `zoom_speed`, `distance`, and `height` configure controls and the third-person offset. The file is an authored definition, so transient input/zoom accumulators are not stored.

### Lights

Positions and directions remain independent of the entity transform. File angles (`inner_angle`, `outer_angle`) are in degrees, not shader cosine values. Point lights do not require a nonzero direction. Directional and spot lights do. Directional shadow cascades must be in `1..32`; spotlights support exactly one cascade. The total enabled shadow-map count must not exceed the current geometry shader's 32 invocations. Point lights consume six maps regardless of their stored cascade setting.

### Physics

Shapes are `box`, `sphere`, `model_box`, and `model_sphere`:

- `box`: positive `half_extents`.
- `sphere`: positive `radius`.
- `model_box`: requires a model; resolves to `model AABB extents * bounds_scale + bounds_offset`. Resolved dimensions must be finite and positive.
- `model_sphere`: requires a model; uses its bounding-sphere radius.

Model-derived dimensions deliberately use **unscaled model bounds**, matching the previous sandbox. Body `position` and Euler `rotation` are independent of the entity transform; for example, the Nuke static floor has a distinct collision pose. Mass must be nonnegative; kinematic mass must be zero. Physics components own their handles exclusively and are movable, not copyable.

The stored body description is its creation configuration, not Bullet's evolving state. The existing runtime kinematic system continues to synchronize kinematic bodies with entity transforms.

### Asset references

References use forward slashes relative to the `asset_root` supplied to the loader. Absolute paths, drive-qualified paths, backslashes, `.`/`..` path segments, and paths resolving outside the asset root (including symlinks) fail. Model cache identity includes the normalized path and import flags, so different import settings do not accidentally reuse a previous import.

Meshes, shaders, texture pixels, OpenGL IDs, and native pointers are not embedded in scene files. External model/material assets must still be deployed separately and use correct filename case for Linux.

## Native scripts

Each `ScriptRegistry::Register<T>` supplies:

- A stable string name; do not serialize `typeid` names or `std::type_index`.
- A validator for properties and required entity components. Validation occurs before resource construction.
- A factory that attaches the script to the supplied entity's `NativeScriptComponent` and returns the created instance (or null on failure).
- A capture callback for authored constructor/configuration properties.

Property values support booleans, finite numbers (`double`), strings, and three-number vectors. Validators must check property names/types/ranges and construction prerequisites before factories access them. Factories should not start runtime, mutate scene ownership, or rely on other scripts' construction order. All entities, ordinary components, the scene name, and primary-camera selection exist before scripts are constructed.

Register a C++ type under only one name. Unknown scripts and unsupported component storages fail capture rather than disappearing from a saved file. Capture sorts native-script names and entity UUIDs for stable output. Script runtime state, such as tank movement progress or previous mouse coordinates, is intentionally reset on load.

Sandbox bindings:

| Script | Properties / prerequisites |
| --- | --- |
| `CameraController` | No properties; camera required. |
| `LogPositionScript` | No properties; camera required. |
| `PlayerControllerScript` | No properties; camera, model, kinematic body, and spotlight required. |
| `RotationModelScript` | Numeric `speed`, preserving its existing degrees-per-millisecond convention. |
| `TankMovementScript` | Positive numeric `distance`, nonzero `forward_direction` vector. |

## Saving and recovery

Stop runtime before `Capture` or `SceneSerializer::Save`. Entity transforms and current camera/light configuration are captured; native-script adapters and physics descriptions capture authored configuration, not a consistent running-game snapshot. To retain a pristine authored scene after playing, retain its original `SceneRecord` or reload the source file instead of saving the simulated scene.

`SceneJson::Save` validates first, writes and closes a sibling `.tmp` file, renames an existing regular target to `.bak`, then promotes the temporary file. Replacement failure attempts to restore the original backup. Existing recovery files are never overwritten; the diagnostic identifies the affected path. If interrupted during replacement, inspect/recover `.bak` and `.tmp` before retrying. Directory targets are rejected.

This is checked, cross-platform replacement, **not an atomic/power-loss-durable transaction**. Concurrent writes to the same path are unsupported; callers must serialize saves. The containing directory must already exist.

## Scope and validation

Version 1 covers authored scene definitions used by the sandbox. It does not implement savegames, hierarchy, prefabs, hot reload, live scene replacement, embedded assets, custom material serialization, or arbitrary reflected components. Scene and ECS registry objects are not moved because systems retain references to them.

Configure tests with `ENABLE_TESTS=ON`, build, then run `ctest --test-dir build -C Release --output-on-failure` (or select the configuration actually built). `SceneJsonTest`, `SceneSerializerTest`, `SceneBodyTest`, and `SceneFileTest` run without a window/GL context. Visual behavior and asset import still require a sandbox smoke test with the external assets available.
