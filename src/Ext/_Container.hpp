#pragma once

#include <unordered_map>

#include <CCINIClass.h>
#include <SwizzleManagerClass.h>

#include "../Misc/Debug.h"
#include "../Misc/Stream.h"
#include "../Misc/Swizzle.h"

enum class InitState {
	Blank = 0x0, // CTOR'd
	Constanted = 0x1, // Initialize() has run once, before any INI was read
	Completed = 0x2 // INI has been read and values set
};

/*
 * ==========================
 *    It's a kind of magic
 * ==========================

 * These two templates are the basis of the new class extension standard.

 * ==========================

 * Extension<T, TExt> is the parent class for the data you want to link with this
   instance of T. TExt is the deriving class itself: there are no virtual functions
   here, so the base reaches the derived hooks statically.
   ( for example, [Warhead]MindControl.Permanent= should be stored in
     WarheadTypeExt::ExtData which itself should derive from
     Extension<WarheadTypeClass, WarheadTypeExt::ExtData> )

 * ==========================

   Container<TX, TCont> is the storage for all the Extension<T> which share the same T,
    where TX is the containing class of the relevant derivate of Extension<T> and
    TCont is again the deriving container itself.
   ( for example, there is WarheadTypeExt::ExtContainer
     deriving from Container<WarheadTypeExt, WarheadTypeExt::ExtContainer> )

   Requires:
   	using base_type = T;
   	class TX::ExtData : public Extension<T, TX::ExtData> { custom_data; }
   	static constexpr DWORD TX::ExtData::Canary = (any dword value easily
   	    identifiable in a byte stream)

   Complex? Yes. That's partially why you should be happy these are premade for you.
 *
 */

// alignas(64) gives every ExtData an alignment of 64 and rounds its size up to a
// multiple of it, which is what makes MSVC emit the aligned operator new/delete.
// That needs LanguageStandard=stdcpp17; under C++14 the one-argument forms are
// emitted instead. MSVC reuses this base's tail padding, so the first derived
// member still lands at +0x08.
#pragma warning(push)
#pragma warning(disable: 4324) // padded because of the alignment specifier - that is the point
template<typename T, typename TExt>
class alignas(64) Extension {
	T* AttachedToObject;
	InitState Initialized;
public:

	Extension(T* const OwnerObject) :
		AttachedToObject(OwnerObject),
		Initialized(InitState::Blank)
	{ }

	Extension(const Extension &other) = delete;

	void operator = (const Extension &RHS) = delete;

	// not virtual: offset 0 of an ExtData is AttachedToObject
	~Extension() = default;

	// the object this Extension expands
	T* const& OwnerObject() const {
		return this->AttachedToObject;
	}

	void LoadFromINI(CCINIClass* pINI) {
		switch(this->Initialized) {
		case InitState::Blank:
			this->This()->Initialize(pINI);
			this->Initialized = InitState::Constanted;
		case InitState::Constanted:
		case InitState::Completed:
			if(pINI == CCINIClass::INI_Rules) {
				this->This()->LoadFromRulesFile(pINI);
			}
			this->This()->LoadFromINIFile(pINI);
			this->Initialized = InitState::Completed;
		}
	}

	void InvalidatePointer(void* ptr, bool bRemoved) { }

	inline void SaveToStream(AresStreamWriter &Stm) {
		//Stm.Save(this->AttachedToObject);
		Stm.Save(this->Initialized);
	}

	inline void LoadFromStream(AresStreamReader &Stm) {
		//Stm.Load(this->AttachedToObject);
		Stm.Load(this->Initialized);
	}

protected:
	TExt* This() {
		return static_cast<TExt*>(this);
	}

	const TExt* This() const {
		return static_cast<const TExt*>(this);
	}

	// the single pre-INI hook. runs exactly once, on the first LoadFromINI, and
	// gets the INI it was called with. anything that has to happen at allocation
	// time belongs in the ExtData constructor instead.
	void Initialize(CCINIClass* pINI) { }

	// for things that only logically work in rules - countries, sides, etc
	void LoadFromRulesFile(CCINIClass* pINI) { }

	// load any ini file: rules, game mode, scenario or map
	void LoadFromINIFile(CCINIClass* pINI) { }
};
#pragma warning(pop)

// a non-virtual base class for a pointer to pointer map.
// pointers are not owned by this map, so be cautious.
class ContainerMapBase final {
public:
	using key_type = void*;
	using const_key_type = const void*;
	using value_type = void*;
	using map_type = std::unordered_map<const_key_type, value_type>;
	using const_iterator = map_type::const_iterator;
	using iterator = const_iterator;

	ContainerMapBase();
	ContainerMapBase(ContainerMapBase const&) = delete;
	~ContainerMapBase();

	ContainerMapBase& operator =(ContainerMapBase const&) = delete;
	ContainerMapBase& operator =(ContainerMapBase&&) = delete;

	value_type find(const_key_type key) const;
	void insert(const_key_type key, value_type value);
	value_type remove(const_key_type key);
	void clear();

	size_t size() const {
		return this->Items.size();
	}

	const_iterator begin() const {
		return this->Items.cbegin();
	}

	const_iterator end() const {
		return this->Items.cend();
	}

private:
	map_type Items;
};

// looks like a typed map, but is really a thin wrapper around the untyped map
// pointers are not owned here either, see that each pointer is deleted
template<typename Key, typename Value>
class ContainerMap final {
public:
	using key_type = Key*;
	using const_key_type = const Key*;
	using value_type = Value*;
	using iterator = typename std::unordered_map<key_type, value_type>::const_iterator;

	ContainerMap() = default;
	ContainerMap(ContainerMap const&) = delete;

	ContainerMap& operator =(ContainerMap const&) = delete;
	ContainerMap& operator =(ContainerMap&&) = delete;

	value_type find(const_key_type key) const {
		return static_cast<value_type>(this->Items.find(key));
	}

	value_type insert(const_key_type key, value_type value) {
		this->Items.insert(key, value);
		return value;
	}

	value_type remove(const_key_type key) {
		return static_cast<value_type>(this->Items.remove(key));
	}

	void clear() {
		this->Items.clear();
	}

	size_t size() const {
		return this->Items.size();
	}

	iterator begin() const {
		auto ret = this->Items.begin();
		return reinterpret_cast<iterator&>(ret);
	}

	iterator end() const {
		auto ret = this->Items.end();
		return reinterpret_cast<iterator&>(ret);
	}

private:
	ContainerMapBase Items;
};

template<typename T, typename TCont>
class Container {
private:
	using base_type = typename T::base_type;
	using extension_type = typename T::ExtData;
	using key_type = base_type*;
	using const_key_type = const base_type*;
	using value_type = extension_type*;
	using map_type = ContainerMap<base_type, extension_type>;

	map_type Items;

	base_type* SavingObject;
	IStream* SavingStream;
	const char* Name;

public:
	explicit Container(const char* pName) : Items(),
		SavingObject(nullptr),
		SavingStream(nullptr),
		Name(pName)
	{ }

	// not virtual: Items sits at offset 0
	~Container() = default;

	void PointerGotInvalid(void *ptr, bool bRemoved) {
		this->This()->InvalidatePointer(ptr, bRemoved);
		if(!this->This()->InvalidateExtDataIgnorable(ptr)) {
			this->InvalidateExtDataPointer(ptr, bRemoved);
		}
	}

protected:
	TCont* This() {
		return static_cast<TCont*>(this);
	}

	const TCont* This() const {
		return static_cast<const TCont*>(this);
	}

	void InvalidatePointer(void *ptr, bool bRemoved) {
	}

	bool InvalidateExtDataIgnorable(void* const ptr) const {
		return true;
	}

	void InvalidateExtDataPointer(void *ptr, bool bRemoved) {
		for(const auto& i : this->Items) {
			i.second->InvalidatePointer(ptr, bRemoved);
		}
	}

public:
	value_type FindOrAllocate(key_type key) {
		if(auto const ptr = this->Items.find(key)) {
			return ptr;
		}
		return this->Items.insert(key, new extension_type(key));
	}

	value_type Find(const_key_type key) const {
		return this->Items.find(key);
	}

	void Remove(const_key_type key) {
		delete this->Items.remove(key);
	}

	void Clear() {
		if(this->Items.size()) {
			Debug::Log(Debug::Severity::Fatal, "Cleared %u items from %s.\n",
				this->Items.size(), this->Name);
			this->Items.clear();
		}
	}

	void LoadAllFromINI(CCINIClass *pINI) {
		for(const auto& i : this->Items) {
			i.second->LoadFromINI(pINI);
		}
	}

	void LoadFromINI(const_key_type key, CCINIClass *pINI) {
		if(auto const ptr = this->Items.find(key)) {
			ptr->LoadFromINI(pINI);
		}
	}

	void PrepareStream(key_type key, IStream *pStm) {
		if(key && pStm) {
			this->SavingObject = key;
			this->SavingStream = pStm;
		} else {
			Debug::Log("[PrepareStream] Object or Stream not set for '%s': %p, %p\n",
				this->Name, key, pStm);
		}
	}

	void SaveStatic() {
		if(!this->This()->Save(this->SavingObject, this->SavingStream)) {
			Debug::FatalErrorAndExit("[SaveStatic] Saving failed!\n");
		}

		this->SavingObject = nullptr;
		this->SavingStream = nullptr;
	}

	void LoadStatic() {
		if(!this->This()->Load(this->SavingObject, this->SavingStream)) {
			Debug::FatalErrorAndExit("[LoadStatic] Loading failed!\n");
		}

		this->SavingObject = nullptr;
		this->SavingStream = nullptr;
	}

protected:
	// shadow this method to do type-specific stuff
	bool Save(key_type key, IStream *pStm) {
		return this->SaveKey(key, pStm) != nullptr;
	}

	// shadow this method to do type-specific stuff
	bool Load(key_type key, IStream *pStm) {
		return this->LoadKey(key, pStm) != nullptr;
	}

	value_type SaveKey(key_type key, IStream *pStm) {
		// get the value data
		auto buffer = this->Find(key);
		if(!buffer) {
			return nullptr;
		}

		// write the current pointer, the size of the block, and the canary
		AresByteStream saver(sizeof(*buffer));
		AresStreamWriter writer(saver);

		writer.Save(extension_type::Canary);
		writer.Save(buffer);

		// save the data
		buffer->SaveToStream(writer);

		// save the block
		if(!saver.WriteBlockToStream(pStm)) {
			Debug::Log("[SaveKey] Failed to save data.\n");
			return nullptr;
		}

		// done
		return buffer;
	}

	value_type LoadKey(key_type key, IStream *pStm) {
		// get the value data
		auto buffer = this->FindOrAllocate(key);
		if(!buffer) {
			return nullptr;
		}

		AresByteStream loader(0);
		if(!loader.ReadBlockFromStream(pStm)) {
			return nullptr;
		}

		AresStreamReader reader(loader);
		if(reader.Expect(extension_type::Canary) && reader.RegisterChange(buffer)) {
			buffer->LoadFromStream(reader);
			if(reader.ExpectEndOfBlock()) {
				return buffer;
			}
		}

		return nullptr;
	}
};
