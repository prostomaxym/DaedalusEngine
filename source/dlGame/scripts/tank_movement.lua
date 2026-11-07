local distance = properties.distance or 50
local direction = properties.forward_direction or vec3(1, 0, 0)
assert(type(distance) == "number" and distance > 0, "distance must be positive")
assert(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z > 0, "forward_direction must be nonzero")

state.length = state.length or 0
state.angle = state.angle or 0
if state.rotating == nil then state.rotating = false end
if state.upwards == nil then state.upwards = true end

function on_update(dt)
	local position = ecs.position(ecs.self)
	local rotation = ecs.rotation(ecs.self)
	if not position or not rotation then return end
	local movement = dt * 5
	local turn = dt * 54

	if not state.rotating then
		if state.upwards then
			movement = math.min(movement, distance - state.length)
			state.length = state.length + movement
			if state.length >= distance then state.rotating = true end
		else
			movement = math.min(movement, state.length)
			state.length = state.length - movement
			movement = -movement
			if state.length <= 0 then state.rotating = true end
		end
		position.x = position.x + movement * direction.x
		position.y = position.y + movement * direction.y
		position.z = position.z + movement * direction.z
	end

	if state.rotating and state.length >= distance then
		turn = math.min(turn, 180 - state.angle)
		state.angle = state.angle + turn
		rotation.y = rotation.y - turn
		if state.angle >= 180 then
			state.rotating = false
			state.upwards = false
		end
	elseif state.rotating and state.length <= 0 then
		turn = math.min(turn, state.angle)
		state.angle = state.angle - turn
		rotation.y = rotation.y + turn
		if state.angle <= 0 then
			state.rotating = false
			state.upwards = true
		end
	end

	ecs.set_position(ecs.self, position)
	ecs.set_rotation(ecs.self, rotation)
end
