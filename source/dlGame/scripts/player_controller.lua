local person_distance = properties.third_person_distance or 7
local person_height = properties.third_person_height or 4
local turn_speed = properties.turn_speed or 540
if state.third_person == nil then state.third_person = true end
if state.spotlight_enabled == nil then state.spotlight_enabled = true end

local function CameraEntity()
	if camera.position(ecs.self) then return ecs.self end
	return ecs.link(ecs.self, "camera")
end

local function SpotLightEntity()
	local id = ecs.link(ecs.self, "spot_light")
	if ecs.valid(id) then return id end
	return ecs.self
end

local function UpdateCamera(camera_id, dt)
	local forward = input.movement("MoveForward", "MoveBackward")
	local right = input.movement("MoveRight", "MoveLeft")
	local up = (input.held("MoveUp") and 1 or 0) - (input.held("MoveDown") and 1 or 0)
	camera.move(camera_id, vec3(right, up, forward), dt)

	local mouse = input.mouse_position()
	if state.mouse_x == nil then
		state.mouse_x = mouse.x
		state.mouse_y = mouse.y
	end
	local dx = mouse.x - state.mouse_x
	local dy = state.mouse_y - mouse.y
	state.mouse_x = mouse.x
	state.mouse_y = mouse.y
	camera.look(camera_id, dx * dt * 2, dy * dt * 2)
	if input.gamepad_enabled() then
		camera.look(camera_id, input.axis("LookRight") * dt * 100, -input.axis("LookUp") * dt * 100)
	end
end

local function UpdatePlayerView(camera_id)
	if not input.released("ChangePOV") then return end
	state.third_person = not state.third_person
	ecs.set_visible(ecs.self, state.third_person)
	camera.set_person(camera_id, state.third_person and person_distance or 0, state.third_person and person_height or 0)
end

local function UpdatePlayerModel(camera_id, dt)
	ecs.set_world_pose(ecs.self, camera.position(camera_id), ecs.world_rotation(ecs.self))
	local forward = camera.direction(camera_id)
	local right = camera.right(camera_id)
	local f = input.movement("MoveForward", "MoveBackward")
	local r = input.movement("MoveRight", "MoveLeft")
	local movement = ecs.local_direction(ecs.parent(ecs.self), vec3(forward.x * f + right.x * r, forward.y * f + right.y * r, forward.z * f + right.z * r))
	if movement.x * movement.x + movement.z * movement.z <= 0 then return end

	local desired_yaw = math.deg(math.atan(movement.x, movement.z)) % 360
	local model_forward = ecs.forward(ecs.self)
	local current_yaw = math.deg(math.atan(model_forward.x, model_forward.z)) % 360
	local delta_yaw = desired_yaw - current_yaw
	if delta_yaw > 180 then
		delta_yaw = delta_yaw - 360
	elseif delta_yaw < -180 then
		delta_yaw = delta_yaw + 360
	end
	local step = turn_speed * dt
	local yaw = (current_yaw + math.max(-step, math.min(step, delta_yaw))) % 360
	ecs.set_rotation(ecs.self, vec3(0, yaw, 0))
end

local function UpdateSpotLight(camera_id)
	local light_id = SpotLightEntity()
	local power = ecs.spotlight_power(light_id)
	if power == nil then return end
	state.spotlight_power = state.spotlight_power or power
	if input.released("Flashlight") then
		state.spotlight_enabled = not state.spotlight_enabled
		ecs.set_spotlight_power(light_id, state.spotlight_enabled and state.spotlight_power or 0)
	end
	if light_id == ecs.self then return end

	local position = camera.position(camera_id)
	if state.third_person then
		local direction = camera.direction(camera_id)
		position.x = position.x + direction.x
		position.y = position.y + direction.y
		position.z = position.z + direction.z
		ecs.set_world_pose(light_id, position, ecs.world_rotation(ecs.self), 180)
	else
		ecs.set_world_pose(light_id, position, ecs.world_rotation(camera_id))
	end
end

function on_update(dt)
	local camera_id = CameraEntity()
	if not camera.position(camera_id) then return end
	UpdateCamera(camera_id, dt)
	UpdatePlayerView(camera_id)
	UpdatePlayerModel(camera_id, dt)
	UpdateSpotLight(camera_id)
end

function on_stop()
	ecs.set_visible(ecs.self, true)
	camera.set_person(CameraEntity(), person_distance, person_height)
	if state.spotlight_power then
		ecs.set_spotlight_power(SpotLightEntity(), state.spotlight_power)
	end
end
