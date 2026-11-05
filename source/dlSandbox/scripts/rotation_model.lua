local speed = properties.speed or 0.1
assert(type(speed) == "number", "speed must be numeric")

function on_update(dt)
	local rotation = ecs.rotation(ecs.self)
	if not rotation then return end
	-- Preserve the native example's degrees-per-millisecond speed property.
	rotation.y = rotation.y + speed * dt * 1000
	ecs.set_rotation(ecs.self, rotation)
end
