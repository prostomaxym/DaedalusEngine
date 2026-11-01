#include "dlpch.h"
#include "TransformPreparation.h"
#include "ECS/Components.h"
#include "Threads/JobSystem.h"

bool Daedalus::PrepareTransforms(JobSystem& jobs, std::span<const TransformComponent> inputs, std::span<glm::mat4> outputs)
{
	if (inputs.size() != outputs.size()) return false;
	if (inputs.size() < 128)
	{
		for (size_t index = 0; index < inputs.size(); ++index) outputs[index] = inputs[index].GetTransform();
		return true;
	}
	return jobs.Wait(jobs.ParallelFor(inputs.size(), [inputs, outputs](size_t index)
	{
		outputs[index] = inputs[index].GetTransform();
	}));
}
