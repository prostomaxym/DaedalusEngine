function on_update(dt)
	local camera_id = ecs.self
	local position = camera.position(camera_id)
	if not position then
		camera_id = ecs.link(ecs.self, "camera")
		position = camera.position(camera_id)
	end
	if not position then return end
	log.trace(string.format("Position - X: %f / Y: %f / Z: %f", position.x, position.y, position.z))
end
