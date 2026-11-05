# Scene serialization

Daedalus stores authored scenes as UTF-8 JSON. Persistent records are separate from runtime entities, GPU resources, Bullet handles, and native-script instances. The parser uses the explicit, header-only `nlohmann-json` vcpkg dependency privately within ECS; client code does not depend on JSON types.

## Client workflow

1. Create an empty `std::unique_ptr<Scene>` and an application-owned `ScriptRegistry`.
2. Register the application's native-script adapters. The sandbox's `RegisterSceneScripts` in `source/dlSandbox/SceneScripts.cpp` is a complete example.
3. Call `SceneSerializer::LoadAsset(engine.GetAssets(), virtual_path_or_uuid, *scene, scripts, error)`, or use the compatible physical-file `Load` API for tools.
4. On success, transfer ownership with `Application::GetInstance()->GetEngine().GetScenes().RequestScene(std::move(scene))`. At the next frame boundary the manager stops the old scene, activates the new one and starts runtime, logging any startup failure. The loaded name and primary-camera UUID are retained.
5. The application forwards fixed updates, variable updates, rendering and viewport resizes through the scene manager. Request a null scene to unload it. Standalone tools may own scenes directly and use the corresponding runtime methods themselves.

The sandbox loads its startup scene through `PathConfig::GetSceneAssetPath()` (`assets:/` plus `SceneFile`). Set `SceneFile` in `[Path]` of `runtime/path.ini`. `AssetsFolder` defaults to the runtime `asset` directory and may instead identify a ZIP file, for example `asset/game.zip`; ZIP contents use the same asset-root-relative layout. The default scene is `scenes/startup.scene.json`; no scene/archive is bundled or copied automatically. The legacy `GetScenePath()` remains for directory-backed physical loading.

To deploy an archive, first generate/preserve source UUID sidecars in a directory-backed asset tree, then ZIP the tree with each `<source>.meta` adjacent to its source. ZIP mounts are read-only: missing or invalid sidecars cannot be repaired inside them. Use ordinary ZIP tooling; encryption is unsupported. Asset/scene UUIDs use the nonzero decimal ID stored in the sidecar, distinct from entity UUIDs in scene JSON.

For Windows PowerShell, packaging `runtime/asset/*` into `runtime/game.zip` keeps entries relative to the asset root; set a single `AssetsFolder=game.zip` key in `[Path]`. `Compress-Archive` may store entry names with backslashes: VFS canonicalizes these separators, while scene/resource references still use forward slashes. Archives containing both separator spellings of the same canonical name are rejected. No extraction or runtime archive rewriting is performed. Font loading also uses VFS. Missing sidecars are registry failures after a successful mount, not an `Unknown mount` error; preserve adjacent `.meta` files when packaging.

Place your scene JSON and its referenced models, textures, and skybox images under the runtime asset folder manually. The files in `source/dlSandbox/scenes` are reference examples only: they are not part of the sandbox target and are not automatically deployed by builds or explicitly installed from source. To try one, manually copy `nuke.scene.json` or `anor_londo.scene.json` into `runtime/asset/scenes`, supply its referenced assets under `runtime/asset`, and set `[Path]` `SceneFile=scenes/nuke.scene.json` or `SceneFile=scenes/anor_londo.scene.json`. Alternatively, name your deployed scene `startup.scene.json` to use the default. Switching scenes requires changing configuration, not rebuilding the application.

Installation still packages the contents of the runtime asset folder like other manually supplied assets. Missing or invalid scene files produce a logged loading error; there is no fallback to an example scene.

### API responsibilities

| API | Purpose |
| --- | --- |
| `SceneJson::Parse` / `Serialize` | Convert JSON text and persistent records; no graphics/physics initialization required. |
| `SceneJson::Load` / `Save` | Read/write persistent records as JSON files. |
| `SceneJson::Load(files, virtual_path, ...)` | Read records through directory/ZIP VFS without graphics. |
| `SceneJson::Validate` | Validate the v1–v4 record schema, hierarchy/links and renderer shadow-map budget. |
| `SceneJson::Upgrade` | Apply engine-owned ordered version transitions, validating every intermediate record. |
| `SceneComposer::Expand` | Recursively expand prefab records through directory/ZIP VFS into a validated flat record, without runtime construction. |
| `SceneComposer::ExpandPrefab` | Compose a single prefab anchor, allowing camera-free templates, without runtime construction. |
| `SceneSerializer::Instantiate` | Expand prefabs, then build a runtime `Scene` from records. |
| `SceneSerializer::SpawnPrefab` | Transactionally append a prefab to an existing stopped or running scene at a scene boundary. |
| `Scene::GetPrefabOrigins` | Borrow an entity's runtime source lineage, ordered outermost to innermost. |
| `SceneSerializer::Capture` | Capture a stopped scene as expanded records without prefab provenance. |
| `SceneSerializer::Load` / `Save` | Combine the file and runtime operations. |
| `SceneSerializer::LoadAsset` | Load by virtual path or asset UUID using an engine asset pipeline; scene activation remains explicit. |

All operations return `bool` and provide a diagnostic in `error`; log it through the engine logger. Parse/capture failures leave their output records unchanged. Loading requires an **empty, stopped** target. Invalid documents, script bindings, and missing assets are rejected before construction; construction failures remove newly created entities and restore the target's previous name and paused state. Failed resources are not cached as successful models or cubemaps. Successfully loaded resources may remain in the existing resource-manager caches after a failed scene load.

Resource instantiation must run on the application/GL-context thread after rendering, physics, and resource services have been constructed. Each Scene creates its own physics world through the physics factory; there is no shared global Bullet world. Record parsing/validation is independent of those services. Loading does not start runtime. Loading a camera-free record is permitted for tools/tests, but starting runtime requires a camera and an application/window.

## Version 4 file structure

The reference scenes in `source/dlSandbox/scenes` remain v1 migration examples. See `source/dlEngine/ECS/Serialization/SceneRecord.h` for settings/defaults; camera/light pose members there are legacy v1 migration inputs, not v2–v4 pose authority.

### Scene

- `version`: required integer, currently `4` (`CurrentSceneVersion`). Versions 1–3 are upgraded on Parse/Load and Instantiate; unsupported versions fail. Capture produces v4. Serialize/Save preserve an explicitly supplied version and omit v4-only fields from legacy exports; call Upgrade before exporting a legacy record as v4.
- `name`: scene display name, defaults to an empty string.
- `primary_camera`: entity UUID string. Required when any camera is present; it must reference a camera or a prefab anchor whose template selects a camera. Selection does not depend on the entity's name.
- `entities`: required array of entity objects.
- `base_prefab`: optional v4 prefab path for a variant. Variants require an empty `entities` array and empty/omitted `primary_camera`; both are inherited from the base.
- `overrides`: v4 array of typed entity overrides, permitted at scene level only with `base_prefab`.

### Entities and components

Every entity requires `id`, a unique, nonzero decimal uint64 **string**. Strings preserve UUIDs above JavaScript's exact-integer range. Leading zeros, duplicate IDs, and invalid/overflowing IDs fail. `name` defaults to `Entity` and must not be empty.

An optional `parent` UUID string references another entity in the same scene; omission means a root. Forward references are allowed. Missing parents, self-parenting, cycles and non-unit-scale physics ancestors are rejected before construction. Local `transform` TRS is preserved; world matrices compose as `parent_world * local`. Optional `links` maps nonempty names to entity UUID strings, for example `"links": { "camera": "12", "spot_light": "13" }`. Links do not imply parenting or ownership. All targets must exist; reserved `camera` and `spot_light` links must target the corresponding components. Parent links, named links and world poses exist before script construction.

Components are named fields on each entity, not C++ type names or memory snapshots:

| Field | Persistent contents |
| --- | --- |
| `transform` | Local `translation`, `rotation`, `scale`, each a three-number array. Rotation is pitch/yaw/roll in degrees. Defaults are zero translation/rotation and unit scale; zero scale is rejected. |
| `model` | Asset-root-relative or canonical virtual `path`, plus numeric `import_flags` (`ModelParserFlags`/Assimp bitmask). Material dependencies use VFS-relative resolution, including ZIP entries and embedded textures; runtime material overrides are not saved. |
| `camera` | `projection` (`perspective` or `orthographic`), projection parameters, viewport policy, and control settings. Pose comes from entity TRS. |
| `directional_light`, `point_light`, `spot_light` | Ambient/diffuse/specular colors, power, range, angles, shadow settings. Pose comes from entity TRS. |
| `skybox` | Six `faces` in right, left, top, bottom, front, back order, plus rotation in degrees. Images must decode to matching square faces. |
| `rigid_body`, `kinematic_body` | Collider shape, initial pose, dimensions, mass, and optional model-bound derivation settings. An entity cannot have both body types. |
| `scripts` | Array of stable registered `type` strings and `properties` objects. |
| `prefab` | Version 3+: asset `path` and optional `exports` mapping anchor link names to template entity UUID strings; v4 adds an `overrides` array. |

Omitted optional components are absent. Omitted configuration properties use record defaults; explicit `null` is not an omitted component. Unknown fields/components, duplicate JSON object keys, duplicate script types, invalid UTF-8, nonfinite/out-of-range numbers, and wrong value types fail rather than being silently dropped.

### Cameras

Perspective cameras use `fov` in degrees, `aspect_ratio`, `near_plane`, and `far_plane`. With `viewport_aspect: true` (the default), the aspect ratio updates from the actual viewport at runtime startup and resize. Set it to `false` for a fixed aspect ratio. Orthographic cameras use `left`, `right`, `bottom`, `top`, `near_plane`, and `far_plane`.

Entity TRS defines the view: -Z is forward and +Y is up. World position includes ancestor scale; orientation composes rotations without scale/shear. Versions 2–4 reject camera `position`, `up`, `yaw`, and `pitch` fields, including inside overrides, rather than silently accepting duplicate pose authority. `movement_speed`, `sensitivity`, `zoom_speed`, `distance`, and `height` configure controls and the third-person offset. Height follows parent +Y, or world +Y for roots. CameraController edits local TRS, including through a `camera` link; idle input preserves roll, while look input uses yaw/pitch controls. Transient input/zoom accumulators are not stored.

### Lights

Positions derive from entity world matrices; directions use rotated -Z without inherited scale/shear. Versions 2–4 reject light `position` and `direction` fields, including inside overrides. File angles (`inner_angle`, `outer_angle`) are in degrees, not shader cosine values. Directional shadow cascades must be in `1..32`; spotlights support exactly one cascade. The total enabled shadow-map count must not exceed the current geometry shader's 32 invocations. Point lights consume six maps regardless of their stored cascade setting.

### Physics

Shapes are `box`, `sphere`, `model_box`, and `model_sphere`:

- `box`: positive `half_extents`.
- `sphere`: positive `radius`.
- `model_box`: requires a model; resolves to `model AABB extents * bounds_scale + bounds_offset`. Resolved dimensions must be finite and positive.
- `model_sphere`: requires a model; uses its bounding-sphere radius.

Model-derived dimensions deliberately use **unscaled model bounds**; a body's own visual scale does not resize its collider. Root rigid bodies retain their authored body `position`/Euler `rotation` creation pose, preserving independent static-floor offsets. Parented bodies initialize from entity world TRS instead. All ancestors of a physics body must have exactly unit local scale. Parenting a subtree beneath incompatible scale is rejected; adding a body under scaled ancestors detaches it with a warning, preserving local TRS. Direct ancestor-scale edits detach affected bodies at their last Bullet world pose at the next physics boundary, also with a warning. Mass must be nonnegative; kinematic mass must be zero. Physics components own their handles exclusively and are movable, not copyable.

The stored body description is its creation configuration, not Bullet's evolving state. Use `scene.GetPhysicsWorld()` for body queries, gravity, transforms and collision callbacks; handles from other scenes or removed bodies are rejected. Components attach through Scene hooks. Runtime stop releases bodies; restart recreates them. Kinematic and parented static bodies follow entity poses; dynamic bodies remain world-authoritative after initialization and write back local TRS in parent-first order, so moving a parent does not teleport an active dynamic child. `Scene::StepPhysics` supports explicit headless physics-only stepping.

### Version 1 migration

Upgrade first validates the original v1 contract, including root-only bodies and valid camera/light directions. Legacy camera/light positions were world-space and independent of entity TRS. A dedicated pose entity retains its ID and receives the converted TRS. An entity with multiple pose components, a model/body/skybox, a parent, or children keeps its original transform and hierarchy anchor; each independent camera/light moves to a new root entity. New IDs are the lowest available nonzero decimal IDs, avoiding collisions deterministically. Named `camera`, `directional_light`, `point_light`, and `spot_light` links connect the source to the new entities. Primary-camera selection follows a moved camera. Existing conflicting reserved links cause a diagnostic, not silent replacement.

Scripts remain on their original entities. Custom adapters/scripts must support links if they require moved components; engine CameraController and sandbox adapters do. The example player uses its linked camera and spotlight, retaining independent model facing and aim. Migration changes records in memory only; source/runtime asset files are not rewritten.

### Ordered migrations

`SceneJson::Upgrade` validates the source, copies it, then applies engine-owned transitions one version at a time until `CurrentSceneVersion`. Each transition must advance exactly one version and its result is validated before continuing. Failure leaves the destination unchanged; in-place and repeated upgrades are supported. The v1→v2 transition performs the pose conversion above; v2→v3 and v3→v4 are additive and preserve entity configuration. Versions 1–2 cannot contain `prefab`; versions 1–3 cannot use variants or nonempty override arrays. Adding a future version requires updating the engine's ordered transition table and schema; clients cannot register custom transitions. Capture produces v4; Serialize preserves an explicitly supplied supported version.

### Load-time prefabs

Prefab assets are ordinary scene records with their own entity IDs and optional primary camera. A scene instance is a normal entity with a `prefab` field. For example, a scene may select anchor `100` as its primary camera:

```json
{
  "version": 3,
  "name": "Prefab example",
  "primary_camera": "100",
  "entities": [
	{
	  "id": "100",
	  "name": "Player instance",
	  "transform": { "translation": [10, 0, 0] },
	  "prefab": {
		"path": "prefabs/player.prefab.json",
		"exports": { "body": "1", "camera": "2" }
	  }
	}
  ]
}
```

Here the template must contain body `1` and camera `2`, and select a primary camera. These are template IDs, not the final scene IDs. Each instance gets deterministic lowest-available IDs; internal parents and named links remap independently. Imported roots become children of the anchor, preserving their local TRS. The anchor may have its own components, scripts, links and parent. Exports publish remapped template entities as anchor links; missing targets, invalid reserved-link component types and collisions with authored links fail. If the template selects a primary camera, a `camera` link is also published unless explicitly provided. Selecting the anchor forwards to that template camera when the anchor has no direct camera. Cameras never become primary implicitly: the containing scene must make a valid selection.

Nested prefabs follow the same rules. Paths without a mount are relative to `assets:/`, including inside nested files, not relative to the containing file. Directory and ZIP mounts are supported without extraction. Physical Instantiate/Load mounts the supplied asset root for composition. Parse/Load at the `SceneJson` level only upgrades records; `SceneComposer::Expand` or runtime Instantiate performs expansion. Script-property strings are untyped and are never rewritten as entity references; use named links for remapped relationships.

Expansion rejects active reference cycles and enforces 16 nested reference levels, 1024 total prefab/variant reference loads, 100000 total entities and 64 MiB of cumulative prefab bytes (including repeated reads). Flat validation checks the combined shadow-map budget, hierarchy, links and unit-scale physics ancestors before any runtime entity or script is constructed. Root-only body creation offsets are not converted into inherited offsets: once embedded under an anchor, bodies initialize from entity world TRS. Author reusable body poses in entity transforms.

Composition failure leaves destination records unchanged. Runtime construction retains the existing rollback behavior. Capture/Save writes the expanded entities, remapped links and actual primary camera, without prefab references/provenance; retain the authored input record to preserve its instances. Reloading an expanded save does not pick up later source-prefab changes.

### Variants and per-instance overrides

A v4 variant is a scene record with `base_prefab`, an empty `entities` array, and optional `name` and `overrides`. Base references may chain and share the same active-path cycle, depth, reference-load, byte and entity budgets as nested prefabs. The base supplies entity IDs and primary-camera selection; variants do not add/delete entities or replace that selection. A variant may also be loaded directly as a scene definition.

An override requires `id`, targeting an entity authored in that template (including a nested prefab's anchor, but not its not-yet-expanded descendants). Optional `name`, `transform`, `parent`, `links`, `scripts`, and component fields replace the corresponding complete value. Component settings omitted inside a replacement use record defaults, not inherited field values. An omitted override field leaves the inherited value intact. An empty `parent` string makes a template root; an empty `links` object or `scripts` array clears that collection. Parents and links use template-local UUIDs and are remapped normally. Script strings remain untyped.

The `remove` array removes optional `model`, `camera`, `directional_light`, `point_light`, `spot_light`, `skybox`, `rigid_body` or `kinematic_body` components. Unknown/duplicate removals, replacing and removing the same component, duplicate override targets, and missing targets fail. Removal of an already absent component is a no-op. IDs, prefab references and whole entities are not overrideable. Component removal must leave valid primary-camera selection, links, collider dependencies and hierarchy.

Precedence is base → each derived variant → instance `prefab.overrides`. Every layer is validated after application, before nested expansion. Instance overrides apply before remapping and nested composition, so they can adjust a nested anchor's transform. Resolved component/hierarchy/shadow constraints must still hold; overrides cannot bypass validation.

### Runtime spawning and provenance

`SceneSerializer::SpawnPrefab(prefab, scene, assets, scripts, instance, error, transform, parent, pipeline)` accepts the same `PrefabRecord` (path, exports, overrides), optional anchor-local TRS, optional existing parent UUID, and optional asset pipeline. Physical asset roots and directory/ZIP pipelines use the same composition path. A new anchor is always created. All new UUIDs are nonzero and allocated independently of template IDs, avoiding existing scene entities. Camera-free prefabs are allowed.

Call synchronously on the scene's owning application/graphics thread at a boundary outside scene script, physics and render iteration; concurrent scene access is unsupported. Calls during those iterations or another construction operation are rejected. There is no deferred-spawn queue: script/collision callbacks must arrange a later boundary call. Parsing/IO and construction are not allocation-free or asynchronous.

Before mutation, spawning validates the combined destination hierarchy, existing ancestor scales, shadow-map budget, entity limit, new script adapters and asset references. Links, parents, world poses, provenance and parented Bullet body poses are ready before factories run. New runtime cameras use the current viewport when configured to do so. Existing primary-camera selection, name, pause and running state are unchanged; selecting a spawned camera is a separate `SetPrimaryCamera` call. If the scene previously had no selected camera, explicitly select a spawned camera before runtime startup or capture (capture rejects cameras without a primary selection, except the existing `Main Camera` compatibility fallback). Spawning does not start runtime.

Stopped-scene bodies still initialize from the latest TRS at their first physics step/startup, so transform edits before activation are honored. Spawning prepares their initial Bullet poses for factories without making stopped dynamic bodies world-authoritative prematurely; already-running dynamic bodies become world-authoritative immediately.

Failure leaves the output `instance` UUID unchanged and destroys only entities created by that operation, releasing their bodies/lights/scripts/provenance. Resource caches may retain successfully loaded resources, as with ordinary loading. Script validators/factories may inspect the assembled scene but must not mutate unrelated entities, scene metadata or lifecycle, or create untracked entities; arbitrary client side effects cannot be rolled back.

`Scene::GetPrefabOrigins(uuid)` returns a borrowed outermost-to-innermost lineage. Each `PrefabOrigin` contains the normalized selected prefab/variant `path`, remapped anchor `instance` UUID string and template-local `template_entity` UUID string (empty for that instance's own anchor). An outer layer identifies entities in its expanded template; inner layers retain their original local IDs. This is historical source identity, not ownership or live asset binding: entity destruction erases its metadata, but destroying an anchor does not cascade-delete children or rewrite surviving lineage. The borrowed span must not outlive entity destruction or scene clearing. Capture omits lineage; reloading expanded captures has none. Source edits do not automatically update already spawned instances.

### Asset references

References use forward slashes relative to the physical `asset_root`, or canonical virtual paths such as `assets:/models/example.obj` with `LoadAsset`. Relative references in that workflow use the asset mount. Absolute/drive-qualified paths, backslashes, traversal and escaping symlinks fail. Assimp material dependencies may use parent-relative paths only when normalized within the same VFS mount. Cache identity includes normalized source and import flags. Scene capture retains virtual references, and ZIP scenes must be saved/exported to a physical JSON file rather than modifying an archive entry.

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
| `CameraController` | No properties; camera component or `camera` link required. |
| `LogPositionScript` | No properties; camera component or `camera` link required. |
| `PlayerControllerScript` | No properties; camera component/link, model, kinematic body, and spotlight component/link required. |
| `RotationModelScript` | Numeric `speed`, preserving its existing degrees-per-millisecond convention. |
| `TankMovementScript` | Positive numeric `distance`, nonzero `forward_direction` vector. |

## Saving and recovery

Stop runtime before `Capture` or `SceneSerializer::Save`. Entity transforms and current camera/light configuration are captured; native-script adapters and physics descriptions capture authored configuration, not a consistent running-game snapshot. To retain a pristine authored scene after playing, retain its original `SceneRecord` or reload the source file instead of saving the simulated scene.

`SceneJson::Save` validates first, writes and closes a sibling `.tmp` file, renames an existing regular target to `.bak`, then promotes the temporary file. Replacement failure attempts to restore the original backup. Existing recovery files are never overwritten; the diagnostic identifies the affected path. If interrupted during replacement, inspect/recover `.bak` and `.tmp` before retrying. Directory targets are rejected.

This is checked, cross-platform replacement, **not an atomic/power-loss-durable transaction**. Concurrent writes to the same path are unsupported; callers must serialize saves. The containing directory must already exist.

## Scope and validation

Version 4 covers authored scene definitions, local transform hierarchies, unit-scale-ancestor physics parenting, named links, ordered migrations, prefab composition, variants, typed per-instance overrides, synchronous boundary spawning and runtime provenance. It is not a savegame, reference-preserving prefab serializer, automatic restart, custom material serializer or arbitrary reflected-component framework. Model/texture/cubemap resources can hot-reload while retaining shared identity; entities, scripts and model-derived colliders are not recreated. SceneManager supports explicit whole-scene replacement at a frame boundary. Scene JSON does not embed binary assets; model formats may contain embedded textures handled by the importer. Scene/ECS registry objects are not moved because systems retain references to them.

Configure tests with `ENABLE_TESTS=ON`, build, then run `ctest --test-dir build -C Release --output-on-failure` (or the configuration built). Scene tests and the asset follow-up tests cover record/VFS/ZIP loading and CPU model/texture imports without a GL context. GPU upload/replacement and visual behavior still require a separate sandbox smoke test.
