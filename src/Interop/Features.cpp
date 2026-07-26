#include "Features.h"

#include "../Misc/Debug.h"

#include <bitset>

namespace
{
	constexpr auto FeatureCount = static_cast<size_t>(AntaresFeature::Count);

	std::bitset<FeatureCount> DisabledFeatures;

	// parallel to AntaresFeature, for the log line only
	const char* const FeatureNames[] = { "EBolt" };

	static_assert(std::size(FeatureNames) == FeatureCount,
		"FeatureNames must list every AntaresFeature");
}

bool Interop::IsDisabled(AntaresFeature feature)
{
	auto const index = static_cast<size_t>(feature);
	return index < FeatureCount && DisabledFeatures[index];
}

bool Interop::Disable(AntaresFeature feature)
{
	auto const index = static_cast<size_t>(feature);

	if(index >= FeatureCount) {
		return false;
	}

	if(!DisabledFeatures[index]) {
		DisabledFeatures[index] = true;
		Debug::Log("[Interop] %s handed over to another extension.\n", FeatureNames[index]);
	}

	return true;
}
