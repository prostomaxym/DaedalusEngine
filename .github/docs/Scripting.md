# Lua gameplay scripting

The Phase 2 implementation uses Lua and sol2 privately inside `ECS/Scripting`. Every `LuaScript` owns an isolated `LuaRuntime`/VM. Native C++ scripts remain supported alongside Lua; one Lua script asset per entity is supported through the existing type-keyed `NativeScriptComponent`.

## Registration and runtime assets

Register the built-in adapter with `LuaScript::Register(registry, engine.GetAssets().GetFiles())` before loading a scene. The sandbox does this alongside its native adapters. Registration copies the VFS mount configuration into adapters/runtimes; the registry can be destroyed without invalidating existing scripts. Asset contents are still read from the mounted source and are not copied into saved scenes.

A scene entity's existing `scripts` array accepts:

```json
{
  "type": "Lua",
  "properties": {
	"asset": "assets:/scripts/mover.lua",
	"speed": 2.5
  }
}
```

`asset` is a reserved string property containing a canonical mounted `.lua` path. It is not relative to the containing scene/prefab. Other properties use the existing bool/number/string/vec3 schema. Each runtime permits at most 128 parameters, with nonempty keys up to 128 bytes, strings up to 4096 bytes, finite numbers and finite vectors bounded to magnitude 100000000 per component. Script reads are text-only and capped at 256 KiB. Directory and ZIP mounts share the same loading path; deploy scripts manually with the rest of the runtime assets. No example assets are automatically copied.

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

Top-level code has `properties`, `state`, `vec3` and allowed Lua libraries, but no `ecs`, `input` or `events`. Define callbacks there; do engine work inside active callbacks. This restriction prevents failed staging/reloads from changing the live scene. Active callback side effects are not transactional: mutations completed before a later Lua error remain applied.

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
| `vec3(x, y, z)` | New vector value with writable `x`, `y`, `z`; nonnumeric arguments return nil. No component references are retained. |
| `input.down(name)` / `pressed` / `held` / `released` | Existing named action queries and enable gates. |
| `input.axis(name)` | Existing named digital/analog axis query. |
| `events.emit(topic, payload)` | Queue a scene-local `GameMessage`; both arguments are strings; return false for invalid values or a full queue. |

Script-facing string/vector arguments are type-checked before native conversions. Invalid arguments return neutral/failure results. TRS setters reject nonfinite values and vectors outside the bounds above. They do not resize colliders or remove existing unit-scale-ancestor physics restrictions. Input queries return neutral values in headless tools without an input service; ordinary input APIs retain their fail-fast lifetime contract.

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

`LuaRuntimeTest` supplies 47 headless regressions for lifecycle, bindings and malformed arguments, VM isolation, restricted capabilities, instruction/memory limits, failed/successful reload, state recovery, empty-source initialization/restart, deferred and worker-posted events, physics-source filtering, factory cleanup/duplicate protection, record round-trips, prefab overrides and ZIP loading. All 47 passed five repetitions in each Windows configuration. Full executable and CTest suites passed: Release 697 tests (694 passed / 3 skipped), ReleaseDevTools 697 (693 passed / 4 skipped). See the roadmap for validation details.

Multiple Lua assets per entity, script module imports, coroutine gameplay, arbitrary component/physics/render bindings, general savegame VM snapshots, asset-registry change subscriptions, an editor debugger and configurable security policies are not implemented. Linux/WSL and Android builds/runtime checks, and interactive scene behavior, remain separate validation work.
