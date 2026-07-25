#pragma once

#include "Stream.h"

#include <memory>
#include <type_traits>

#include <GeneralStructures.h>  // for Timer.h, which needs Fundamentals.h first

// Savegame format guard. Types without a Load/Save member go through
// AresStreamObject's default, a raw sizeof(T) blob, so their size *is* the
// savegame format. Several CDTimerClass instances live inside serialized ExtData,
// and Ares 3.0p1 streams each as a raw 12-byte blob with StartTime at +0 and
// TimeLeft at +8. If either assertion fires, the format has silently changed.
static_assert(sizeof(CDTimerClass) == 0xC, "CDTimerClass must stay 12 bytes: it is serialized as a raw blob");
static_assert(offsetof(CDTimerClass, StartTime) == 0x0, "CDTimerClass::StartTime must stay at +0");
static_assert(offsetof(CDTimerClass, TimeLeft) == 0x8, "CDTimerClass::TimeLeft must stay at +8");

namespace Savegame {
	template <typename T>
	bool ReadAresStream(AresStreamReader &Stm, T &Value, bool RegisterForChange = true);

	template <typename T>
	bool WriteAresStream(AresStreamWriter &Stm, const T &Value);

	template <typename T>
	T* RestoreObject(AresStreamReader &Stm, bool RegisterForChange = true);

	template <typename T>
	bool PersistObject(AresStreamWriter &Stm, const T* pValue);

	template <typename T>
	struct AresStreamObject {
		bool ReadFromStream(AresStreamReader &Stm, T &Value, bool RegisterForChange) const;

		bool WriteToStream(AresStreamWriter &Stm, const T &Value) const;
	};

	template <typename T>
	struct ObjectFactory {
		std::unique_ptr<T> operator() (AresStreamReader &Stm) const {
			return std::make_unique<T>();
		}
	};
}
