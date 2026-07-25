#pragma once

#include "Api.h"

// Feature handover.
//
// Syringe installs every hook when the DLL loads, long before any consumer can
// negotiate, so handing a subsystem over cannot mean "skip installing the hook".
// It means the hook stands down at runtime: it checks here on entry and returns 0,
// falling through to the game -- where the consumer's own hook at the same address
// takes over.
//
// Checks must stay live. No hook may cache the answer at init, because the
// consumer sets these from its own ExeRun hook and hook ordering there is not
// something we control.

namespace Interop
{
	//! Whether a subsystem has been handed to another extension.
	bool IsDisabled(AntaresFeature feature);

	//! Hand a subsystem over. False if this build does not know the feature,
	//! in which case nothing changed.
	bool Disable(AntaresFeature feature);
}
