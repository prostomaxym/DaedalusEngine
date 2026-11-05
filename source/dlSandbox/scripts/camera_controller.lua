local function CameraEntity()
	if camera.position(ecs.self) then return ecs.self end
	return ecs.link(ecs.self, "camera")
end

function on_update(dt)
	local camera_id = CameraEntity()
	if not camera.position(camera_id) then return end

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
