#include "Body.h"

#include "../ParticleType/Body.h"
#include "../Rules/Body.h"

#include <BuildingClass.h>
#include <CellClass.h>
#include <Drawing.h>
#include <FileSystem.h>
#include <FootClass.h>
#include <GameOptionsClass.h>
#include <LaserDrawClass.h>
#include <MapClass.h>
#include <ParticleTypeClass.h>
#include <RulesClass.h>
#include <ScenarioClass.h>
#include <SpotlightClass.h>
#include <Surface.h>
#include <TacticalClass.h>
#include <YRMath.h>

#include <cmath>

namespace
{
	static const int WindOffsetX = 0x8366A4;
	static const int WindOffsetY = 0x8366C4;

	float& SpawnFramesOf(ParticleSystemClass* pThis) {
		return *reinterpret_cast<float*>(&pThis->SpawnFrames);
	}

	int NextParticleIndex(ParticleTypeClass* pThis) {
		return *reinterpret_cast<int const*>(&pThis->NextParticle);
	}

	ColorStruct const* ColorListOf(ParticleTypeClass* pThis) {
		return reinterpret_cast<ColorStruct const*>(pThis->ColorList.Items);
	}

	bool IsColorSet(ColorStruct const& color) {
		return color.R || color.G || color.B;
	}

	ColorStruct Interpolate(ColorStruct const& a, ColorStruct const& b, double factor) {
		auto const inverse = 1.0 - factor;

		auto const clamp = [](double const value) -> BYTE {
			auto const rounded = static_cast<int>(value);
			if(rounded < 0) {
				return 0;
			}
			return rounded > 255 ? static_cast<BYTE>(255) : static_cast<BYTE>(rounded);
		};

		ColorStruct ret;
		ret.R = clamp(b.R * inverse + a.R * factor);
		ret.G = clamp(a.G * factor + b.G * inverse);
		ret.B = clamp(inverse * b.B + factor * a.B);
		return ret;
	}

	ColorStruct StartColorOf(ParticleTypeClass* pType, Randomizer* pRandom) {
		ColorStruct ret = { 0, 0, 0 };

		if(!pType->ColorList.Count) {
			return ret;
		}

		if(IsColorSet(pType->StartColor1) && IsColorSet(pType->StartColor2)) {
			return Interpolate(pType->StartColor1, pType->StartColor2, pRandom->RandomDouble());
		}

		return ColorListOf(pType)[0];
	}

	int CellZOffset(CellClass* pCell, Point2D const& point) {
		using func_t = int (__thiscall*)(CellClass*, Point2D const*);
		return reinterpret_cast<func_t>(0x47B3A0)(pCell, &point);
	}

	unsigned short AlphaBufferValue(int x, int y) {
		using func_t = unsigned short* (__thiscall*)(ABuffer*, int, int);
		return *reinterpret_cast<func_t>(0x4114B0)(ABuffer::Instance, x, y);
	}

	unsigned short DepthBufferValue(int x, int y) {
		using func_t = unsigned short* (__thiscall*)(ZBuffer*, int, int);
		return *reinterpret_cast<func_t>(0x7BD130)(ZBuffer::Instance, x, y);
	}

	void MatrixRotateX(Matrix3D& matrix, float theta) {
		using func_t = void (__thiscall*)(Matrix3D*, float);
		reinterpret_cast<func_t>(0x5AEF60)(&matrix, theta);
	}

	void MatrixRotateZ(Matrix3D& matrix, float theta) {
		using func_t = void (__thiscall*)(Matrix3D*, float);
		reinterpret_cast<func_t>(0x5AF1A0)(&matrix, theta);
	}

	Vector3D<float> MatrixMultiply(Matrix3D const& matrix, Vector3D<float> const& vector) {
		using func_t = Vector3D<float>* (__fastcall*)(
			Vector3D<float>*, Matrix3D const*, Vector3D<float> const*);

		Vector3D<float> ret;
		reinterpret_cast<func_t>(0x5AFB80)(&ret, &matrix, &vector);
		return ret;
	}

	bool IsStrangeBuilding(BuildingClass* pBuilding) {
		return (*reinterpret_cast<bool(__thiscall ***)(BuildingClass*)>(pBuilding))[0x80 / 4](pBuilding);
	}

	// the particles wander with the wind, age their frames and expire
	void MoveDrawParticles(ParticleSystemExt::ExtData* pThis)
	{
		auto const pRandom = &ScenarioClass::Instance->Random;

		if(Unsorted::CurrentFrame & 1) {
			for(auto& item : pThis->DrawData) {
				if(pRandom->RandomRanged(0, 3)) {
					continue;
				}

				auto x = item.VelocityX;
				auto y = item.VelocityY;

				auto const delta = pRandom->RandomRanged(-1, 1);
				if(pRandom->RandomRanged(0, 1)) {
					x += delta;
				} else {
					y += delta;
				}

				item.VelocityX = x < -5 ? -5 : (x > 5 ? 5 : x);
				item.VelocityY = y < -5 ? -5 : (y > 5 ? 5 : y);
			}
		}

		for(auto& item : pThis->DrawData) {
			auto const pType = item.LinkedParticleType;

			auto const frame = item.ImageFrame;
			auto const last = static_cast<int>(pType->EndStateAI);

			if(frame < last) {
				auto const age = pType->MaxEC - item.Duration + item.Order;
				auto const advance = static_cast<int>(pType->StateAIAdvance) + item.Order % 2;

				if(!(age % advance)) {
					item.ImageFrame = frame + 1;

					if(frame + 1 >= last && pType->DeleteOnStateLimit) {
						item.Expired = 1;
					}
				}
			}

			if(item.VelocityZ > 3.0f) {
				item.VelocityZ -= pType->Deacc;
			}

			if(--item.Duration <= 0) {
				item.Expired = 1;
			}
		}

		auto const direction = RulesClass::Instance->WindDirection;
		auto const windX = reinterpret_cast<int const*>(WindOffsetX)[direction];
		auto const windY = reinterpret_cast<int const*>(WindOffsetY)[direction];

		for(auto& item : pThis->DrawData) {
			auto const effect = item.LinkedParticleType->WindEffect;

			auto const oldCoords = item.Location;
			auto const newY = oldCoords.Y + item.VelocityY + windY * effect;
			auto const newZ = oldCoords.Z + static_cast<int>(item.VelocityZ);

			item.Location.X = oldCoords.X + item.VelocityX + windX * effect;
			item.Location.Y = newY;
			item.Location.Z = newZ;

			auto const cell = CellClass::Coord2Cell(oldCoords);
			auto const pCell = MapClass::Instance.GetCellAt(cell);

			if(static_cast<bool>(pCell->Flags & CellFlags::BridgeHead)) {
				Point2D const point = { oldCoords.X, oldCoords.Y };
				auto const bridge = CellZOffset(pCell, point) + 416;

				if(oldCoords.Z < bridge && newZ >= bridge - 260) {
					item.Expired = 1;
				}
			}
		}
	}

	// steps the airborne particles and lets the ground, walls and buildings stop them
	void MoveMovementParticles(ParticleSystemExt::ExtData* pThis)
	{
		auto const gravity = static_cast<float>(RulesClass::Instance->Gravity);

		for(auto& item : pThis->MovementData) {
			if(--item.Duration <= 0) {
				item.Expired = 1;
				continue;
			}

			item.Velocity.Z -= gravity;

			CoordStruct const oldCoords = {
				static_cast<int>(item.Location.X),
				static_cast<int>(item.Location.Y),
				static_cast<int>(item.Location.Z)
			};

			auto const newX = item.Location.X + item.Velocity.X;
			auto const newY = item.Location.Y + item.Velocity.Y;
			auto const newZ = (item.Velocity.Z - gravity) + item.Location.Z;

			item.Location.X = newX;
			item.Location.Y = newY;
			item.Location.Z = newZ;

			CoordStruct const coords = {
				static_cast<int>(newX), static_cast<int>(newY), static_cast<int>(newZ)
			};

			auto const pCell = MapClass::Instance.GetCellAt(coords);

			Point2D const point = { coords.X, coords.Y };
			auto const height = CellZOffset(pCell, point);

			if(coords.Z < height) {
				item.Expired = 1;
				continue;
			}

			if((static_cast<bool>(pCell->Flags & CellFlags::BridgeHead))
				|| (static_cast<bool>(MapClass::Instance.GetCellAt(oldCoords)->Flags & CellFlags::BridgeHead)))
			{
				auto const bridge = height + 416;

				if(coords.Z >= bridge) {
					if(oldCoords.Z < bridge) {
						item.Expired = 1;
						continue;
					}
				} else if(oldCoords.Z >= bridge) {
					item.Expired = 1;
					continue;
				}
			}

			if(coords.Z - 150 < height) {
				auto const pBuilding = pCell->GetBuilding();

				if(!pBuilding) {
					if(pCell->ConnectsToOverlay(-1, -1)) {
						item.Expired = 1;
					}
					continue;
				}

				auto const pType = reinterpret_cast<BYTE*>(pBuilding->Type);
				if(!pType[0x16BF] || *reinterpret_cast<DWORD*>(reinterpret_cast<BYTE*>(pBuilding) + 0x618) < 8u) {
					if(!IsStrangeBuilding(pBuilding)) {
						item.Expired = 1;
					}
				}
			}
		}
	}

	// drops every particle that expired this frame out of both vectors
	void CompactParticles(ParticleSystemExt::ExtData* pThis)
	{
		auto& movement = pThis->MovementData;
		auto movementTo = movement.begin();
		auto const movementEnd = movement.end();

		while(movementTo != movementEnd && !movementTo->Expired) {
			++movementTo;
		}

		if(movementTo != movementEnd) {
			for(auto it = movementTo + 1; it != movementEnd; ++it) {
				if(!it->Expired) {
					*movementTo++ = *it;
				}
			}
		}

		if(movementTo != movement.end()) {
			movement.erase(movementTo, movement.end());
		}

		auto& draw = pThis->DrawData;
		auto drawTo = draw.begin();
		auto const drawEnd = draw.end();

		while(drawTo != drawEnd && !drawTo->Expired) {
			++drawTo;
		}

		if(drawTo != drawEnd) {
			for(auto it = drawTo + 1; it != drawEnd; ++it) {
				if(!it->Expired) {
					*drawTo++ = *it;
				}
			}
		}

		if(drawTo != draw.end()) {
			draw.erase(drawTo, draw.end());
		}
	}

	// walks the airborne particles along the held type's colour list
	void AgeParticleColors(ParticleSystemExt::ExtData* pThis)
	{
		auto const pType = pThis->HeldParticleType;
		auto const pScen = ScenarioClass::Instance;

		auto const last = pType->ColorList.Count - 2;
		auto const speed = pType->ColorSpeed;

		for(auto& item : pThis->MovementData) {
			auto const factor = pScen->Random.RandomDouble() * 0.05 + speed + item.ColorFactor;

			auto result = 1.0;
			if(factor <= 1.0) {
				result = factor;
			} else if(item.ColorIndex < last) {
				result = 0.0;
				item.ColorIndex = item.ColorIndex + 1;
			}

			item.ColorFactor = static_cast<float>(result);
		}
	}
}

void ParticleSystemExt::ExtData::HandleSmoke()
{
	auto const pThis = this->OwnerObject();

	if(auto const pOwner = pThis->Owner) {
		if((pOwner->AbstractFlags & AbstractFlags::Object) != AbstractFlags::None) {
			auto const coords = pOwner->GetCoords();
			pThis->SetLocation(coords + pThis->SpawnDistanceToOwner);
		}
	}

	MoveDrawParticles(this);

	auto const pRandom = &ScenarioClass::Instance->Random;

	for(auto index = this->DrawData.size(); index; --index) {
		if(!this->DrawData[index - 1].Expired) {
			continue;
		}

		auto const nextIndex = NextParticleIndex(this->DrawData[index - 1].LinkedParticleType);
		if(nextIndex < 0) {
			continue;
		}

		auto const pNext = ParticleTypeClass::Array.Items[nextIndex];

		auto const frame = static_cast<int>(static_cast<BYTE>(pNext->StartStateAI));
		auto const duration = static_cast<unsigned short>(pNext->MaxEC)
			+ pRandom->RandomRanged(0, pNext->MaxEC);
		auto const velocityZ = this->DrawData[index - 1].VelocityZ;
		auto const count = static_cast<int>(this->DrawData.size());
		auto const order = this->DrawData[index - 1].Order;

		auto const translucency = static_cast<BYTE>(this->DrawData[index - 1].Translucency
			+ (pRandom->RandomRanged(0, 5) ? 25 : 0));

		auto const spread = pNext->Radius << 8;
		auto const coords = this->DrawData[index - 1].Location;
		auto const offsetX = pRandom->RandomRanged(-spread, spread);
		auto const offsetY = pRandom->RandomRanged(-spread, spread);

		DrawDataItem item;
		item.Location.X = coords.X + offsetX;
		item.Location.Y = coords.Y + offsetY;
		item.Location.Z = coords.Z;
		item.VelocityX = 0;
		item.VelocityY = 0;
		item.VelocityZ = velocityZ;
		item.Order = order + count + 1;
		item.ImageFrame = frame;
		item.Duration = duration;
		item.LinkedParticleType = pNext;
		item.Translucency = translucency;
		item.Expired = 0;
		item.Unknown2A = 0;
		item.Unknown2B = 0;

		this->DrawData.push_back(item);

		item.Location.X = coords.X - offsetX;
		item.Location.Y = coords.Y - offsetY;
		item.Location.Z = coords.Z;
		item.Order = order + count + 2;

		this->DrawData.push_back(item);
	}

	CompactParticles(this);

	auto pFoot = static_cast<FootClass*>(pThis->Owner);
	if(!pFoot || (pFoot->AbstractFlags & AbstractFlags::Foot) == AbstractFlags::None) {
		pFoot = nullptr;
	}

	auto const pType = pThis->Type;

	if(!pThis->TimeToDie && pThis->IsAlive
		&& !(Unsorted::CurrentFrame % static_cast<int>(SpawnFramesOf(pThis)))
		&& (!pFoot || pFoot->TubeIndex < 0))
	{
		if(auto const pParticleType = this->HeldParticleType) {
			auto const radius = pType->SpawnRadius + 1;
			auto const offsetX = pRandom->RandomRanged(-radius, radius);
			auto const offsetY = pRandom->RandomRanged(-radius, radius);

			CoordStruct const coords = {
				pThis->Location.X + offsetX,
				pThis->Location.Y + offsetY,
				pThis->Location.Z + 10
			};

			this->DrawData.emplace_back();
			auto& item = this->DrawData.back();

			item.Location = coords;

			auto height = MapClass::Instance.GetCellFloorHeight(coords);
			if(height < coords.Z) {
				height = coords.Z;
			}
			item.Location.Z = height;

			item.VelocityY = 0;
			item.VelocityX = 0;

			auto const translucency = static_cast<BYTE>(pParticleType->Translucency);
			item.Translucency = translucency;

			if(SpawnFramesOf(pThis) > pType->SpawnTranslucencyCutoff) {
				item.Translucency = static_cast<BYTE>(translucency + 25);
			}

			auto velocity = pParticleType->Velocity
				- (SpawnFramesOf(pThis) - static_cast<double>(pType->SpawnFrames)) * 0.35;
			if(velocity < 2.0) {
				velocity = 2.0;
			}
			item.VelocityZ = static_cast<float>(velocity);

			item.Order = pThis->Fetch_ID() + static_cast<int>(this->DrawData.size());
			item.Duration = static_cast<unsigned short>(pParticleType->MaxEC)
				+ pRandom->RandomRanged(0, pParticleType->MaxEC);
			item.ImageFrame = static_cast<int>(static_cast<BYTE>(pParticleType->StartStateAI));
			item.LinkedParticleType = pParticleType;
			item.Expired = 0;
		}
	}

	auto const frames = pType->Slowdown + SpawnFramesOf(pThis);
	SpawnFramesOf(pThis) = frames;

	if(frames > pType->SpawnCutoff) {
		pThis->TimeToDie = true;
	}
}

void ParticleSystemExt::ExtData::HandleSpark()
{
	auto const pThis = this->OwnerObject();
	auto const pRandom = &ScenarioClass::Instance->Random;

	auto const frames = pThis->SparkSpawnFrames;
	auto const remaining = frames - 1;

	if(remaining >= 0) {
		pThis->SparkSpawnFrames = remaining;

		if(remaining <= 0) {
			pThis->TimeToDie = true;
		}

		auto const roll = pRandom->RandomDouble();
		if(roll < 0.3) {
			auto radius = pThis->SpotlightRadius - 3;
			if(radius < 17) {
				radius = 17;
			}
			pThis->SpotlightRadius = radius;
		} else if(roll < 0.6) {
			auto radius = pThis->SpotlightRadius + 3;
			if(radius > 41) {
				radius = 41;
			}
			pThis->SpotlightRadius = radius;
		}

		auto const pType = pThis->Type;

		if(!remaining || pType->SpawnSparkPercentage > pRandom->RandomDouble()) {
			auto const pParticleType = this->HeldParticleType;

			auto const cap = pType->ParticleCap >= 0 ? pType->ParticleCap : 0;
			auto const half = cap / 2;
			auto const count = half + pRandom->RandomRanged(0, half);

			this->MovementData.reserve(count + this->MovementData.size());

			for(auto index = count; index > 0; --index) {
				auto const velocityX = static_cast<double>(
					pRandom->RandomRanged(0, pParticleType->XVelocity));
				auto const velocityY = static_cast<double>(
					pRandom->RandomRanged(0, pParticleType->YVelocity));
				auto const velocityZ = static_cast<double>(pParticleType->MinZVelocity
					+ pRandom->RandomRanged(0, pParticleType->ZVelocityRange));

				auto const magnitude = std::sqrt(
					velocityZ * velocityZ + velocityY * velocityY + velocityX * velocityX);

				auto directionX = pType->SpawnDirection.X + velocityX;
				auto directionY = pType->SpawnDirection.Y + velocityY;
				auto directionZ = pType->SpawnDirection.Z + velocityZ;

				auto const length = std::sqrt(
					directionZ * directionZ + directionX * directionX + directionY * directionY);

				if(length != 0.0) {
					auto const scale = 1.0 / length;
					directionX *= scale;
					directionY *= scale;
					directionZ *= scale;
				}

				this->MovementData.emplace_back();
				auto& item = this->MovementData.back();

				item.Velocity.X = static_cast<float>(directionX * magnitude);
				item.Velocity.Y = static_cast<float>(directionY * magnitude);
				item.Velocity.Z = static_cast<float>(directionZ * magnitude);

				item.Location.X = static_cast<float>(pThis->Location.X);
				item.Location.Y = static_cast<float>(pThis->Location.Y);
				item.Location.Z = static_cast<float>(pThis->Location.Z);

				item.Speed = 0.0f;
				item.ColorFactor = 0.0f;
				item.ColorIndex = 0;
				item.Expired = 0;

				item.Duration = static_cast<unsigned short>(pParticleType->MaxEC)
					+ pRandom->RandomRanged(0, pParticleType->MaxEC);

				item.Color = StartColorOf(pParticleType, pRandom);
			}

			if(GameOptionsClass::Instance.DetailLevel == 2) {
				if(frames == pType->SparkSpawnFrames && !pType->OneFrameLight
					&& pType->LightSize > 0)
				{
					GameCreate<SpotlightClass>(pThis->Location, pType->LightSize);
				}
			}
		}
	}

	MoveMovementParticles(this);
	CompactParticles(this);
	AgeParticleColors(this);
}

void ParticleSystemExt::ExtData::HandleRailgun()
{
	auto const pThis = this->OwnerObject();

	if(!pThis->TimeToDie && this->MovementData.empty()) {
		auto const source = pThis->Location;
		auto const target = pThis->TargetCoords;

		pThis->TimeToDie = true;

		auto const deltaX = source.X - target.X;
		auto const deltaY = source.Y - target.Y;
		auto const deltaZ = source.Z - target.Z;

		auto const flat = static_cast<double>(deltaY) * deltaY
			+ static_cast<double>(deltaX) * deltaX;
		auto const distance = std::sqrt(flat + static_cast<double>(deltaZ) * deltaZ);
		auto const flatDistance = std::sqrt(flat);

		auto clampedZ = static_cast<double>(deltaZ);
		if(distance < clampedZ) {
			clampedZ = distance;
		}
		if(clampedZ < -distance) {
			clampedZ = -distance;
		}

		auto clampedX = static_cast<double>(deltaX);
		if(flatDistance < clampedX) {
			clampedX = flatDistance;
		}
		if(clampedX < -flatDistance) {
			clampedX = -flatDistance;
		}

		auto const pitch = Math::asin(clampedZ / distance);
		auto const yaw = Math::acos(clampedX / flatDistance)
			* (((deltaY >> 31) & -2) + 1);

		Matrix3D matrix;
		matrix.Data[0] = 1.0f; matrix.Data[1] = 0.0f; matrix.Data[2] = 0.0f; matrix.Data[3] = 0.0f;
		matrix.Data[4] = 0.0f; matrix.Data[5] = 1.0f; matrix.Data[6] = 0.0f; matrix.Data[7] = 0.0f;
		matrix.Data[8] = 0.0f; matrix.Data[9] = 0.0f; matrix.Data[10] = 1.0f; matrix.Data[11] = 0.0f;

		MatrixRotateZ(matrix, static_cast<float>(yaw));
		MatrixRotateX(matrix, static_cast<float>(pitch));

		auto const pRandom = &ScenarioClass::Instance->Random;
		auto const pType = pThis->Type;
		auto const pParticleType = this->HeldParticleType;

		auto const spiralDelta = pType->SpiralDeltaPerCoord;
		auto const speed = static_cast<double>(pParticleType->Velocity);
		auto const spiralRadius = pType->SpiralRadius;
		auto const positionPerturbation = pType->PositionPerturbationCoefficient;
		auto const movementPerturbation = pType->MovementPerturbationCoefficient;
		auto const velocityPerturbation = pType->VelocityPerturbationCoefficient;

		auto const count = static_cast<int>(pType->ParticlesPerCoord * distance);
		this->MovementData.reserve(count);

		auto drift = 0.0;

		for(auto index = 0; index < count; ++index) {
			auto const progress = static_cast<double>(index) / count;
			auto const radians = progress * distance * spiralDelta;

			Vector3D<float> spiral;
			spiral.X = 0.0f;
			spiral.Y = static_cast<float>(Math::cos(radians));
			spiral.Z = static_cast<float>(Math::sin(radians));

			auto const rotated = MatrixMultiply(matrix, spiral);

			auto const inverse = 1.0 - progress;

			auto const x = static_cast<int>(target.X * inverse + source.X * progress
				+ ((pRandom->RandomDouble() - 0.5) * positionPerturbation
					+ rotated.X * spiralRadius));
			auto const y = static_cast<int>(inverse * target.Y + progress * source.Y
				+ (spiralRadius * rotated.Y
					+ positionPerturbation * (pRandom->RandomDouble() - 0.5)));
			auto const z = static_cast<int>(source.Z * progress + target.Z * inverse
				+ ((pRandom->RandomDouble() - 0.5) * positionPerturbation
					+ rotated.Z * spiralRadius));

			Vector3D<float> movement;
			movement.X = static_cast<float>(
				(pRandom->RandomDouble() - 0.5) * movementPerturbation + rotated.X);
			movement.Y = static_cast<float>(
				(pRandom->RandomDouble() - 0.5) * movementPerturbation + rotated.Y);
			movement.Z = static_cast<float>(
				(pRandom->RandomDouble() - 0.5) * movementPerturbation + rotated.Z);

			auto const length = std::sqrt(static_cast<double>(movement.Z) * movement.Z
				+ static_cast<double>(movement.X) * movement.X
				+ static_cast<double>(movement.Y) * movement.Y);

			if(length != 0.0) {
				auto const scale = 1.0 / length;
				movement.X = static_cast<float>(movement.X * scale);
				movement.Y = static_cast<float>(scale * movement.Y);
				movement.Z = static_cast<float>(scale * movement.Z);
			}

			auto const wobble = 0.5
				* ((pRandom->RandomDouble() - 0.5 + drift) * velocityPerturbation);

			drift = wobble;
			if(drift > velocityPerturbation) {
				drift = velocityPerturbation;
			}
			if(-movementPerturbation > drift) {
				drift = -movementPerturbation;
			}

			this->MovementData.emplace_back();
			auto& item = this->MovementData.back();

			item.Velocity = movement;
			item.Location.X = static_cast<float>(x);
			item.Location.Y = static_cast<float>(y);
			item.Location.Z = static_cast<float>(z);
			item.Speed = static_cast<float>(speed + drift);
			item.ColorFactor = 0.0f;
			item.ColorIndex = 0;
			item.Expired = 0;

			item.Duration = static_cast<unsigned short>(pParticleType->MaxEC)
				+ pRandom->RandomRanged(0, 9);

			item.Color = StartColorOf(pParticleType, pRandom);
		}

		if(pType->Laser) {
			ColorStruct const empty = { 0, 0, 0 };
			GameCreate<LaserDrawClass>(source, target, 0, BYTE(1), pType->LaserColor,
				empty, empty, 10, false, true, 0.5f, 0.0f);
		}
	}

	auto const pRandom = &ScenarioClass::Instance->Random;

	for(auto& item : this->MovementData) {
		auto const speed = item.Speed;
		item.Speed = static_cast<float>((pRandom->RandomDouble() - 0.5) * 0.1 + speed);

		auto const velocityY = item.Velocity.Y;
		auto const velocityZ = item.Velocity.Z;

		item.Location.X = item.Velocity.X * speed + item.Location.X;
		item.Location.Y = velocityY * speed + item.Location.Y;
		item.Location.Z = speed * velocityZ + item.Location.Z;

		if(--item.Duration <= 0) {
			item.Expired = 1;
		}
	}

	CompactParticles(this);
	AgeParticleColors(this);
}

bool ParticleSystemExt::ExtData::Handled()
{
	switch(this->Behave) {
	case BehaveKind::Spark:
		this->HandleSpark();
		break;
	case BehaveKind::Railgun:
		this->HandleRailgun();
		break;
	case BehaveKind::Smoke:
		this->HandleSmoke();
		break;
	default:
		return false;
	}

	auto const pThis = this->OwnerObject();

	if(--pThis->Lifetime == 0) {
		pThis->TimeToDie = true;
	}

	if(pThis->IsAlive && pThis->TimeToDie && !pThis->Particles.Count
		&& this->MovementData.empty() && this->DrawData.empty())
	{
		pThis->Limbo();
		pThis->IsAlive = false;

		reinterpret_cast<DynamicVectorClass<AbstractClass*>*>(0xB0F698)->AddItem(pThis);
	}

	return true;
}

void ParticleSystemExt::ExtData::DrawInAir(bool const ignoreShroud)
{
	auto const pParticleType = this->HeldParticleType;
	auto const pColors = pParticleType ? ColorListOf(pParticleType) : nullptr;

	for(auto& item : this->MovementData) {
		CoordStruct const coords = {
			static_cast<int>(item.Location.X),
			static_cast<int>(item.Location.Y),
			static_cast<int>(item.Location.Z)
		};

		if(!ignoreShroud && MapClass::Instance.IsLocationShrouded(coords)) {
			continue;
		}

		Point2D point;
		TacticalClass::Instance->CoordsToClient(&const_cast<CoordStruct&>(coords), &point);

		auto const& bounds = DSurface::ViewBounds;
		point.Y += bounds.Y;

		if(point.X < bounds.X || point.X >= bounds.X + bounds.Width
			|| point.Y < bounds.Y || point.Y >= bounds.Y + bounds.Height)
		{
			continue;
		}

		auto const alpha = AlphaBufferValue(point.X, point.Y - ABuffer::Instance->Bounds.Y);
		if(!alpha) {
			continue;
		}

		auto const pZBuffer = ZBuffer::Instance;
		auto const depth = DepthBufferValue(point.X, point.Y - pZBuffer->Bounds.Y);
		auto const height = TacticalClass::AdjustForZ(coords.Z);

		auto const surface = static_cast<int>(static_cast<unsigned short>(
			static_cast<unsigned short>(pZBuffer->MaxValue)
			- static_cast<unsigned short>(point.Y)
			+ static_cast<unsigned short>(pZBuffer->Bounds.Y)));

		if(surface - height - 50 >= depth) {
			continue;
		}

		auto const index = item.ColorIndex;
		auto const& from = index ? pColors[index] : item.Color;
		auto color = Interpolate(pColors[index + 1], from, item.ColorFactor);

		if(alpha < 127u) {
			color.R = static_cast<BYTE>((alpha * color.R) >> 7);
			color.G = static_cast<BYTE>((alpha * color.G) >> 7);
			color.B = static_cast<BYTE>((alpha * color.B) >> 7);
		}

		DSurface::Temp->SetPixel(&point, Drawing::RGB_To_Int(color));
	}

	for(auto& item : this->DrawData) {
		auto const pType = item.LinkedParticleType;

		auto const pShape = pType->GetImage();
		if(!pShape) {
			continue;
		}

		auto const offset = -15 - TacticalClass::AdjustForZ(item.Location.Z);

		Point2D point;
		TacticalClass::Instance->CoordsToClient(&item.Location, &point);

		auto flags = BlitterFlags(0x2E00);
		point.Y += DSurface::ViewBounds.Y;

		if(GameOptionsClass::Instance.DetailLevel == 2) {
			auto const translucency = item.Translucency;
			if(translucency == 25) {
				flags = BlitterFlags(0x2E02);
			} else if(translucency == 50) {
				flags = BlitterFlags(0x2E04);
			} else if(translucency >= 75) {
				flags = BlitterFlags(0x2E06);
			}
		}

		auto pConvert = ParticleTypeExt::ExtMap.Find(pType)->Palette.Convert.get();
		if(!pConvert) {
			pConvert = FileSystem::ANIM_PAL;
		}

		DSurface::Temp->DrawSHP(pConvert, pShape, item.ImageFrame, &point,
			&DSurface::ViewBounds, flags, 0, offset, ZGradient::Deg90, 1000, 0,
			nullptr, 0, 0, 0);
	}
}

void ParticleSystemExt::UpdateInAir()
{
	if(!ParticleSystemClass::Array.Count || !GameOptionsClass::Instance.DetailLevel
		|| !RulesExt::DetailsCurrentlyEnabled())
	{
		return;
	}

	auto ignoreShroud = true;
	if(!*reinterpret_cast<bool*>(0xA8ED6B) && *reinterpret_cast<DWORD*>(0xB73550)) {
		ignoreShroud = !ScenarioClass::Instance->SpecialFlags.FogOfWar;
	}

	for(auto const& pSystem : ParticleSystemClass::Array) {
		ParticleSystemExt::ExtMap.Find(pSystem)->DrawInAir(ignoreShroud);
	}
}
