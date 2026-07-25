#pragma once

#include <CCINIClass.h>
#include <AbstractClass.h>

#include "../_Container.hpp"
#include "../../Ares.h"

#include "../../Misc/Debug.h"

class AbstractExt
{
public:
	using base_type = AbstractClass;

	class ExtData final : public Extension<AbstractClass, ExtData>
	{
	public:
		static constexpr DWORD Canary = 0xAB5005BA;

		ExtData(AbstractClass* OwnerObject) : Extension<AbstractClass, ExtData>(OwnerObject)
		{ }

		~ExtData() = default;

		void InvalidatePointer(void *ptr, bool bRemoved) {
		}
	};

	class ExtContainer final : public Container<AbstractExt, ExtContainer> {
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;
};
