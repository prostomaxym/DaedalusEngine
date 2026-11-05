# Lua gameplay scripting

The Phase 2 implementation uses Lua and sol2 privately inside `ECS/Scripting`. Every `LuaScript` owns an isolated `LuaRuntime`/VM. Native C++ scripts remain supported alongside Lua; one Lua script asset per entity is supported through the existing type-keyed `NativeScriptComponent`.

## Registration and runtime assets

Register the built-in adapter with `LuaScript::Register(registry, engine.GetAssets().GetFiles())` before loading a scene. The sandbox registers only this adapter; its example scenes use Lua exclusively. Other clients can still register native adapters. Registration copies the VFS mount configuration into adapters/runtimes; the registry can be destroyed without invalidating existing scripts. Asset contents are still read from the mounted source and are not copied into saved scenes.

Configure the script root alongside assets and shaders in `path.ini`:

```ini
[Path]
AssetsFolder=asset
ShadersFolder=shader
ScriptsFolder=script
```

`ScriptsFolder` defaults to `script`, resolved relative to the directory containing `path.ini` (normally `runtime/script`). `PathConfig::GetScriptsPath()` returns the resolved root; saving uses portable INI-relative paths. Engine startup mounts that folder as `scripts:/` and includes its sources in the existing asset registry/metadata and change scans. Missing directories enumerate as empty so applications without Lua scripts still start normally. `paths.scripts` exposes the configured root as a read-only console CVar; changing startup mounts requires restarting, not a live INI reload.

Place `mover.lua` directly inside the configured script folder and reference it as `scripts:/mover.lua`. The root is independent of `AssetsFolder` and `ShadersFolder`; no implicit search inside the scene's asset folder occurs. Existing explicit paths such as `assets:/scripts/mover.lua` remain supported when that source is still mounted; saved records are not rewritten automatically. Keep separately indexed content roots disjoint to avoid duplicate UUID sidecars through aliased mounts.

A scene entity's existing `scripts` array accepts:

```json
{
  "type": "Lua",
  "properties": {
	"asset": "scripts:/mover.lua",
	"speed": 2.5
  }
}
```

`asset` is a reserved string property containing a canonical mounted `.lua` path. It is not relative to the containing scene/prefab. Other properties use the existing bool/number/string/vec3 schema. Each runtime permits at most 128 parameters, with nonempty keys up to 128 bytes, strings up to 4096 bytes, finite numbers and finite vectors bounded to magnitude 100000000 per component. Script reads are text-only and capped at 256 KiB. Directory and ZIP mounts share the same loading path; deploy scripts manually with the rest of the runtime assets. No example assets are automatically copied.

`ScriptsFolder=script/game.zip` selects a read-only ZIP mount with the same `scripts:/` references. Generate/preserve adjacent `.meta` sidecars in a directory-backed script tree before packaging; the registry does not repair metadata inside archives. Runtime Lua reads and its 0.5-second hot-reload poll still use bounded VFS reads; registry scanning does not replace the per-VM reload contract. Desktop installs include the manually supplied `runtime/script` tree when present. Custom script roots must be distributed separately with matching configuration. Android bundling accepts `-ScriptDirectory`, stages it under private `script/`, and installs missing files only; use `ScriptsFolder=script` for that layout. Existing device scripts are preserved.

Validation reads and executes top-level code in a restricted staging VM before scene construction. The factory stages it again before attaching an executable runtime, and failures participate in existing scene/prefab rollback. V5 records already support the adapter; no new scene version is needed. Prefab variants/instances can replace the existing whole `scripts` collection. Use entity links rather than embedding entity UUIDs in arbitrary properties: prefab composition does not remap script-property strings.

Scene capture preserves the authored asset path and properties, not the contents of the Lua VM, modified `properties` values, or transient `state`. Expanded scene saves remain independent of source-prefab edits; Lua source assets are still external runtime assets.

## Script lifecycle

```lua
state.distance = state.distance or 0

function on_update(dt)
	local position = ecs.position(ecs.self)
	if position then
		local movement = properties.speed * input.axis("MoveHorizontal") * dt
		position.x = position.x + movement
		if ecs.set_position(ecs.self, position) then
			state.distance = state.distance + math.abs(movement)
		end
	end
end

function on_event(event)
	if event.type == "GameMessage" and event.name == "reset" then
		state.distance = 0
	end
end
```

All callbacks are optional and must be functions when supplied:

| Callback | Contract |
|---|---|
| `on_start()` | First active script update after initial load/restart, before `on_update`. |
| `on_update(dt)` | Variable scene update, in seconds; suspended by scene pause. |
| `on_event(event)` | Deferred event snapshot before the next script update; only initialized scripts receive events. |
| `on_reload()` | Runs in the candidate VM with copied state, before commit and before engine bindings are available. Must be pure state migration. |
| `on_stop()` | Runs once for a started, healthy VM on scene stop or script removal, then the VM is released. Faulted VMs skip it. |

Top-level code has `properties`, `state`, `vec3` and allowed Lua libraries, but no `ecs`, `camera`, `input`, `events` or `log`. Define callbacks there; do engine work inside active callbacks. This restriction prevents failed staging/reloads from changing the live scene. Active callback side effects are not transactional: mutations completed before a later Lua error remain applied.

Scripts run on their constructing scene thread. A protected callback error disables that VM and logs a bounded diagnostic through the engine logger, without stopping other entities. A source edit or explicit reload can recover it. Native scripts have default no-op event and stop hooks. `Scene::UpdateScripts(dt)` is an explicit headless/owner-thread entry point, analogous to `StepPhysics`; it bypasses normal running/paused checks and must not be used to step a paused game accidentally. It rejects nested scene iteration/construction.

## Bindings

IDs are decimal strings, never Lua numbers or borrowed entity pointers. Lookups resolve the UUID against the owning scene every time. Missing/destroyed entities return false/nil/empty results. No registry, component pointer, entity creation/destruction, renderer, physics-world handle or general C++ object is exposed.

| API | Meaning |
|---|---|
| `ecs.self` | Current entity UUID string. |
| `ecs.valid(id)` | Whether the UUID currently resolves in this scene. |
| `ecs.find(name)` / `ecs.name(id)` | Lookup by name / read tag; missing results are empty strings. |
| `ecs.link(id, name)` | Named link target UUID string; `"0"` if absent. |
| `ecs.position(id)` / `rotation(id)` / `scale(id)` | Copy local TRS as a vector, or nil if unavailable. Rotation is Euler degrees. |
| `ecs.set_position(id, value)` / `set_rotation` / `set_scale` | Validate a vector and update local TRS; return bool. World caches refresh through normal scene processing. |
| `ecs.parent(id)` | Parent UUID string, or `"0"` for a root/missing entity. |
| `ecs.world_position(id)` / `world_rotation(id)` | Refresh world caches and return copied position / Euler degrees, or nil. |
| `ecs.set_world_pose(id, position, rotation[, yaw_offset])` | Apply world position and Euler degrees through the parent, retaining local scale. Optional local-Y rotation offset in degrees defaults to zero. Returns bool. |
| `ecs.forward(id)` | Local orientation's +Z forward vector (for model facing), or nil. |
| `ecs.local_direction(id, direction)` | Inverse world orientation applied to a copied direction, ignoring scale. `"0"` denotes the identity/root frame; missing IDs return nil. |
| `ecs.set_visible(id, bool)` | Toggle model submission without removing the renderable component. Transient; not serialized. Returns bool. |
| `ecs.spotlight_power(id)` / `set_spotlight_power(id, number)` | Read/write the existing spotlight power; nil/false if absent. Power must be nonnegative. |
| `camera.position(id)` / `direction(id)` / `right(id)` | Refresh world caches and return copied camera vectors, or nil. Position is the camera's base position, before third-person view offsets. |
| `camera.move(id, axes, dt)` | Move in camera-right/reference-up/forward directions (`axes.x/y/z`), using the configured movement speed and nonnegative seconds. Write back world pose; return bool. |
| `camera.look(id, x, y)` | Apply sensitivity-scaled yaw/pitch offsets, clamp pitch to ±89 degrees and write back world pose. Zero offsets preserve authored roll. Returns bool. |
| `camera.set_person(id, distance, height)` | Set third-person view offsets; distance must be nonnegative. Returns bool. |
| `vec3(x, y, z)` | New vector value with writable `x`, `y`, `z`; nonnumeric arguments return nil. No component references are retained. |
| `input.down(name)` / `pressed` / `held` / `released` | Existing named action queries and enable gates. |
| `input.axis(name)` | Existing named digital/analog axis query. |
| `input.movement(positive, negative)` | Existing `Input::GetMovementAction` query, including named-axis and keyboard/gamepad enable gates. |
| `input.mouse_position()` / `gamepad_enabled()` | Copied mouse position as `vec3(x,y,0)` / gamepad enable gate. Neutral without an input service. |
| `log.trace(message)` | Write a string of at most 4096 bytes through the engine Application logger at Trace level; return bool. |
| `events.emit(topic, payload)` | Queue a scene-local `GameMessage`; both arguments are strings; return false for invalid values or a full queue. |

Script-facing string/vector arguments are type-checked before native conversions. Invalid arguments return neutral/failure results. TRS setters reject nonfinite values and vectors outside the bounds above. They do not resize colliders or remove existing unit-scale-ancestor physics restrictions. Input queries return neutral values in headless tools without an input service; ordinary input APIs retain their fail-fast lifetime contract.

Gameplay scalar arguments must be finite numbers with absolute value at most 100000000; vector arguments use the same per-component bounds as TRS. Camera/spotlight bindings require an existing component and never create or remove components. World/camera reads refresh scene-derived poses; setters write authoritative TRS rather than keeping independent camera transforms.

## Sandbox examples

`source/dlSandbox/scripts` contains standalone camera, player, rotation, tank patrol and position-logging Lua examples. They replace the sandbox's native example classes and adapters, but are not listed in project targets, compiled, or copied by builds. Manually place them in `ScriptsFolder`; the reference scenes use matching `scripts:/` paths. The player asset includes camera controls because one Lua asset per entity and no module imports are supported. It uses named `camera`/`spot_light` links after legacy scene migration, retains model components when hiding them, and toggles existing spotlight power. Healthy stop callbacks restore model visibility, third-person offsets and flashlight power. See the [sandbox README](../../source/dlSandbox/README.md) for deployment and properties.

## Events and thread safety

Each scene owns an `EventEngine` with a 256-pending-event capacity. C++ producers may call `QueueScriptEvent(std::unique_ptr<Event>)` from any thread while the scene is alive; producers must join before scene destruction. Dispatch and all Lua/ECS access remain owner-thread operations. Events emitted during a snapshot or script update wait for the next update, so event handlers cannot recursively dispatch themselves. Capacity counts pending events; a currently draining snapshot is separate. Overflow returns false; application forwarding ignores overflow. Stopping a scene discards its pending script events, including messages emitted by stop callbacks. The queue remains available on stopped scenes for headless tools; posts after that cleanup are accepted until the next explicit script update or restart. Producers must coordinate stop/restart if messages must not cross that boundary; native event hooks can receive these later posts even before Lua VMs initialize.

Unhandled application events are cloned into the active running scene after layer/overlay processing; handled events do not reach scripts. Scene pause prevents draining/updates but does not stop bounded producer enqueueing. Events queued before a script's first start are not delivered to that uninitialized VM. Physics events from another scene source are rejected, including stale contacts from a previous scene.

Lua receives copied tables:

- All events: `type` (the engine event type name).
- `GameMessage`: `name`, `payload`, `sender` UUID string. Lua-emitted topics are 1–128 bytes and payloads 0–4096 bytes. Native code uses `GameEvent`; scene-local Lua emission is not automatically broadcast to application layers or other scenes.
- Keyboard: `key`; presses also include `repeat_count`.
- Mouse buttons: `button`; mouse movement/scroll: `x`, `y`.
- Gamepad buttons: `button`; presses also include `repeat_count`. Analog event objects include `axis` and `value` when supplied by native producers; this does not add platform analog event generation.
- Physics contacts: `scene`, `first`, `second` UUID strings, `phase` (0 enter / 1 stay / 2 exit), copied `position`/`normal`, `impulse`, and `trigger`.

Lua tables cannot mark native events handled. Topic filtering is performed in `on_event`; there are no subscription handles or priorities. Game messages and scene delivery establish the basic scene-messaging foundation without a global subscription bus.

## Hot reload

On active updates, each runtime polls its VFS source after 0.5 accumulated update seconds. No worker accesses Lua. Source bytes are compared with the last attempted version, avoiding repeated compilation/logging for an unchanged broken source. `GetRuntime().Reload()` explicitly retries immediately. Directory edits and externally replaced ZIP content are read through VFS; the engine does not rewrite archives.

A candidate VM receives fresh authored `properties` plus a copy of the old flat `state`. It executes top-level code and `on_reload`, validates callbacks/state, installs engine bindings, then replaces the old VM. Syntax, budget, state-validation or reload-callback failure leaves old code/state active. A running script does not repeat `on_start` or call `on_stop` merely because of reload. Stop/restart creates a fresh VM from authored parameters.

`state` is a flat, string-keyed table of bool/finite number/bounded string/vec3 values with the same property bounds. Other globals may hold temporary tables/functions, but they reset on reload. Nested tables/functions/userdata are not transferable state. An invalid state on a healthy VM prevents reload; reset/restart or repair the running state. If an already-faulted VM has invalid state, recovery falls back to authored parameters rather than making it unrecoverable.

## Restricted execution and limits

Only base, math, table, string and UTF-8 libraries are opened. OS, IO, debug, package, coroutine and native module loading are absent. Dynamic load/file functions, `require`, metatable/raw mutation helpers, printing, explicit garbage collection, `string.dump`, `pcall` and `xpcall` are removed. Protected calls/coroutines are unavailable so scripts cannot repeatedly swallow the engine's budget error.

Every top-level execution and callback has a 100000-Lua-instruction budget, checked in 1000-instruction increments. A custom allocator rejects growth beyond 8 MiB of accounted Lua heap per VM during protected execution. Trusted host setup and callback argument/result marshaling, including copied event tables, occur outside that allocator enforcement; C++ binding/driver/third-party allocations are not accounted. The instruction hook does not preempt work inside C library functions, including potentially expensive string-pattern matching. These limits are not a wall-clock deadline, total-process memory quota or OS security boundary. Do not treat this in-process runtime as isolation for hostile scripts; engine/native-library defects can still affect the process. Lua's C++ vcpkg variant is linked so Lua failures unwind C++ frames instead of longjmp crossing sol2 objects. Lua 5.5 uses its selected-library initialization API; earlier versions use sol2's library loader.

## Validation and remaining scope

`LuaRuntimeTest` supplies 59 headless regressions for lifecycle, bindings and malformed arguments, VM isolation, restricted capabilities, instruction/memory limits, failed/successful reload, state recovery, empty-source initialization/restart, deferred and worker-posted events, physics-source filtering, factory cleanup/duplicate protection, record round-trips, prefab overrides, configured script-folder loading/reload/capture, legacy mounted paths and ZIP loading. Gameplay coverage includes parent-aware camera/world poses, visibility/power toggles, example-scene asset validation, rotation speed, patrol reload state, logging and linked player cleanup. `InputActionsTest` runs the real Lua controller examples for movement, device gates, analog axes, player facing and release-edge toggles. PathConfig, registry/pipeline and installer tests separately cover relative/portable roots, source metadata/change detection and install-missing preservation. See the roadmap for configuration-specific validation results.

Multiple Lua assets per entity, script module imports, coroutine gameplay, arbitrary component/physics/render bindings, general savegame VM snapshots, asset-registry change subscriptions, an editor debugger and configurable security policies are not implemented. Linux/WSL and Android builds/runtime checks, and interactive scene behavior, remain separate validation work.
