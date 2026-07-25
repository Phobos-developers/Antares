#include "Swizzle.h"

#include "../Ares.h"
#include "Debug.h"

AresSwizzle AresSwizzle::Instance;

HRESULT AresSwizzle::RegisterForChange(void **p) {
	if(p) {
		if(auto deref = *p) {
			this->Nodes.emplace(deref, p);
			*p = nullptr;
		}
		return S_OK;
	}
	return E_POINTER;
}

HRESULT AresSwizzle::RegisterChange(void *was, void *is) {
	auto exist = this->Changes.find(was);
	if(exist == this->Changes.end()) {
		this->Changes[was] = is;
	} else if(exist->second != is) {
		Debug::Log(Debug::Severity::Fatal, "Pointer %p declared change to both %p AND %p!\n", was, exist->second, is);
	}
	return S_OK;
}

void AresSwizzle::ConvertNodes() const {
	Debug::Log("Converting %u nodes.\n", this->Nodes.size());
	for(auto it = this->Nodes.begin(); it != this->Nodes.end(); ++it) {
		auto change = this->Changes.find(it->first);
		if(change == this->Changes.end()) {
			// a node that cannot be resolved is left alone. The old code cached
			// the last successful lookup and wrote it even when this lookup had
			// failed, so an unresolvable pointer silently became whichever
			// object happened to be resolved before it.
			Debug::Log(Debug::Severity::Fatal,
				"Pointer %p could not be remapped from %p!\n", it->second, it->first);
		} else if(auto p = it->second) {
			*p = change->second;
		}
	}
}

void AresSwizzle::Clear() {
	this->Nodes.clear();
	this->Changes.clear();
}

DEFINE_HOOK(0x6CF350, SwizzleManagerClass_ConvertNodes, 0x0)
{
	AresSwizzle::Instance.ConvertNodes();
	AresSwizzle::Instance.Clear();

	return 0x6CF400;
}

DEFINE_HOOK(0x6CF2C0, SwizzleManagerClass_Here_I_Am, 0x0)
{
	GET_STACK(void *, oldP, 0x8);
	GET_STACK(void *, newP, 0xC);

	HRESULT res = AresSwizzle::Instance.RegisterChange(oldP, newP);

	R->EAX<HRESULT>(res);
	return 0x6CF316;
}

DEFINE_HOOK(0x6CF240, SwizzleManagerClass_Swizzle, 0x0)
{
	GET_STACK(void **, ptr, 0x8);

	HRESULT res = AresSwizzle::Instance.RegisterForChange(ptr);

	R->EAX<HRESULT>(res);
	return 0x6CF2B3;
}
