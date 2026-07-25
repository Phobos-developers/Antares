#pragma once

// Ares's accumulate API over the game's CRCEngine.
//
// The arithmetic lives here rather than calling CRCEngine::operator(), because
// src/Misc/Checksum.cpp hooks those very addresses (0x4A1C10..0x4A1DE0) to stop
// the game writing past the end of the staging buffer -- calling them from inside
// those handlers would jump straight back into the hook.

#include <GeneralDefinitions.h>   // for DWORD/BYTE, as the pinned Checksummer.h did
#include <CRC.h>

class Checksummer : public CRCEngine
{
	static const size_t Size = 4;

public:
	Checksummer()
	{
		this->CRC = 0;
		this->Index = 0;
		this->StagingBuffer.Composite = 0;
	}

	DWORD GetValue() const
	{
		return this->GetValueInline();
	}

	DWORD Intermediate() const
	{
		return static_cast<DWORD>(this->CRC);
	}

	void Commit()
	{
		this->CommitInline();
	}

	void Add(const void* data, size_t c_bytes)
	{
		if(data && c_bytes) {
			auto bytes = reinterpret_cast<const BYTE*>(data);

			// fill the current block
			while(this->Index != 0 && static_cast<size_t>(this->Index) < Size && c_bytes) {
				this->AddInline(*bytes++);
				--c_bytes;
			}

			// take the full blocks
			const auto blocks = c_bytes / Size;
			for(auto i = 0u; i < blocks; ++i) {
				this->CRC = Process(bytes, Size, static_cast<DWORD>(this->CRC));
				bytes += Size;
			}
			c_bytes -= blocks * Size;

			// fill in the remainder
			while(c_bytes--) {
				this->AddInline(*bytes++);
			}
		}
	}

	void Add(BYTE value)
	{
		this->AddInline(value);
	}

	void Add(bool value)
	{
		this->Add(static_cast<BYTE>(value != 0));
	}

	void Add(char value)
	{
		this->Add(static_cast<BYTE>(value));
	}

	void Add(signed char value)
	{
		this->Add(static_cast<BYTE>(value));
	}

	void Add(const char* string)
	{
		if(string) {
			this->Add(string, strlen(string));
		}
	}

	template <typename T>
	void Add(const T& value)
	{
		this->Add(&value, sizeof(T));
	}

	static DWORD Process(const void* data, size_t size, DWORD initial)
	{
		return static_cast<DWORD>(
			CRCEngine::Memory(data, static_cast<int>(size), static_cast<int>(initial)));
	}

	static DWORD Process(const char* string, DWORD initial)
	{
		return static_cast<DWORD>(CRCEngine::String(string, static_cast<int>(initial)));
	}

protected:
	BYTE* Staging() const
	{
		return reinterpret_cast<BYTE*>(const_cast<char*>(this->StagingBuffer.Buffer));
	}

	void Fill() const
	{
		// this check is missing in the original, which makes it
		// write beyond the array, ie. outside the class' memory.
		auto const index = static_cast<size_t>(this->Index);
		if(index < Size) {
			auto const bytes = this->Staging();
			bytes[index] = static_cast<BYTE>(index);
			for(auto i = index + 1; i < Size; ++i) {
				bytes[i] = bytes[0];
			}
		}
	}

	__forceinline void AddInline(BYTE value)
	{
		auto const bytes = this->Staging();

		// clear old data
		if(this->Index == 0) {
			for(size_t i = 0; i < Size; ++i) {
				bytes[i] = 0;
			}
		}

		bytes[this->Index++] = value;

		if(static_cast<size_t>(this->Index) == Size) {
			this->CommitInline();
		}
	}

	__forceinline DWORD GetValueInline() const
	{
		// nothing to check
		if(!this->Index) {
			return static_cast<DWORD>(this->CRC);
		}

		// fill the remaining bytes
		this->Fill();

		// project the value without changing internal state
		return Process(this->Staging(), Size, static_cast<DWORD>(this->CRC));
	}

	__forceinline void CommitInline()
	{
		this->CRC = static_cast<int>(this->GetValueInline());
		this->Index = 0;
	}
};

static_assert(sizeof(Checksummer) == 0xC, "Checksummer must stay the game's 12-byte CRC object");

// when using the original game implementation, use this to allocate space on
// the stack. this class has a byte of padding to prevent out of bounds writes.
class SafeChecksummer : public Checksummer
{
public:
	SafeChecksummer() : Checksummer() { }

protected:
	/*
	* this is not entirely correct
	* the original class doesn't have this member and as such its sizeof == 0xC,
	* but the code writes to the 0xC'th byte anyway... when the class is
	* allocated through the heap, it works because windows apparently aligns
	* memory blocks to 8 byte boundaries but when it's allocated on the stack,
	* all hell breaks loose
	*/
	BYTE  Padding;
};
