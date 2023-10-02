#include "Furniture.h"

#include "Registry/Util/RayCast.h"
#include "Registry/Util/RayCast/ObjectBound.h"

namespace Registry
{

#define MAPENTRY(value) \
	{                     \
		#value, value       \
	}

	using enum FurnitureType;
	static inline const std::map<RE::BSFixedString, FurnitureType, FixedStringCompare> FurniTable = {
		MAPENTRY(BedRoll),
		MAPENTRY(BedSingle),
		MAPENTRY(BedDouble),
		MAPENTRY(Wall),
		MAPENTRY(Railing),
		MAPENTRY(CraftCookingPot),
		MAPENTRY(CraftAlchemy),
		MAPENTRY(CraftEnchanting),
		MAPENTRY(CraftSmithing),
		MAPENTRY(CraftAnvil),
		MAPENTRY(CraftWorkbench),
		MAPENTRY(CraftGrindstone),
		MAPENTRY(Table),
		MAPENTRY(TableCounter),
		MAPENTRY(Chair),
		MAPENTRY(ChairCommon),
		MAPENTRY(ChairWood),
		MAPENTRY(ChairBar),
		MAPENTRY(ChairNoble),
		MAPENTRY(ChairMisc),
		MAPENTRY(Bench),
		MAPENTRY(BenchNoble),
		MAPENTRY(BenchMisc),
		MAPENTRY(Throne),
		MAPENTRY(ThroneRiften),
		MAPENTRY(ThroneNordic),
		MAPENTRY(XCross),
		MAPENTRY(Pillory),
	};

	FurnitureDetails::FurnitureDetails(const YAML::Node& a_node)
	{
		for (auto&& it : a_node) {
			const auto typestr = it.as<std::string>();
			const auto where = FurniTable.find(typestr);
			if (where == FurniTable.end()) {
				logger::error("Unrecognized Furniture: '{}'", typestr);
				continue;
			}
			const auto offsetnode = it["Offset"];
			if (offsetnode.IsDefined() || !offsetnode.IsSequence()) {
				logger::error("Missing 'Offset' node for type '{}'", typestr);
				continue;
			}
			std::vector<Coordinate> offsets{};
			const auto push = [&](const YAML::Node& node) {
				const auto vec = node.as<std::vector<float>>();
				if (vec.size() != 4) {
					logger::error("Invalid offset size. Expected 4 but got {}", vec.size());
					return;
				}
				Coordinate coords(vec);
				offsets.push_back(coords);
			};
			if (offsetnode[0].IsSequence()) {
				offsets.reserve(offsetnode.size());
				for (auto&& offset : offsetnode) {
					push(offset);
				}
			} else {
				push(offsetnode);
			}
			if (offsets.empty()) {
				logger::error("Type '{}' is defined but has no valid offsets", typestr);
				continue;
			}
			_data.emplace_back(where->second, std::move(offsets));
		}
	}

	std::vector<std::pair<FurnitureType, std::vector<Coordinate>>> FurnitureDetails::GetCoordinatesInBound(
		RE::TESObjectREFR* a_ref,
		stl::enumeration<FurnitureType> a_filter) const
	{
		const auto niobj = a_ref->Get3D();
		const auto ninode = niobj ? niobj->AsNode() : nullptr;
		if (!ninode)
			return {};
		const auto boundingbox = MakeBoundingBox(ninode);
		if (!boundingbox)
			return {};
		auto centerstart = boundingbox->GetCenterWorld();
		centerstart.z = boundingbox->worldBoundMax.z;

		const glm::vec4 tracestart{
			centerstart.x,
			centerstart.y,
			centerstart.z,
			0.0f
		};
		const auto basecoords = Coordinate(a_ref);
		std::vector<std::pair<FurnitureType, std::vector<Coordinate>>> ret{};
		for (auto&& [type, offsetlist] : _data) {
			if (!a_filter.any(type)) {
				continue;
			}
			std::vector<Coordinate> vec;
			for (auto&& offset : offsetlist) {
				// Get location for current coordinates
				// Add some units height to it so we hover above ground to avoid hitting a rug or gold coin
				auto coords = basecoords;
				offset.Apply(coords);
				coords.location.z += 16.0f;
				const glm::vec4 traceend_A{ coords.location, 0.0f };
				// Check if path to offset is free, if not we likely hit some static, like a wall, or some actor
				const auto resA = Raycast::hkpCastRay(tracestart, traceend_A, { a_ref });
				if (resA.hit && glm::distance(resA.hitPos, traceend_A) > 16.0) {
					continue;
				}
				// 2nd cast to see if there is no ceiling above either
				auto traceend_B = traceend_A;
				traceend_B.z += 128.0f;
				const auto resB = Raycast::hkpCastRay(traceend_A, traceend_B, { a_ref });
				if (resB.hit) {
					continue;
				}
				vec.push_back(coords);
			}
			ret.emplace_back(type, vec);
		}
		return ret;
	}

	std::vector<std::pair<FurnitureType, Coordinate>> FurnitureDetails::GetClosestCoordinateInBound(
		RE::TESObjectREFR* a_ref,
		stl::enumeration<FurnitureType> a_filter,
		const RE::TESObjectREFR* a_center) const
	{
		const auto centercoordinates = Coordinate(a_center);
		const auto valids = GetCoordinatesInBound(a_ref, a_filter);
		std::vector<std::pair<FurnitureType, Coordinate>> ret{};
		for (auto&& [type, coordinates] : valids) {
			float distance = std::numeric_limits<float>::max();
			Coordinate distance_coords;

			for (auto&& coords : coordinates) {
				const auto d = glm::distance(coords.location, centercoordinates.location);
				if (d < distance) {
					distance_coords = coords;
					distance = d;
				}
			}
			if (distance != std::numeric_limits<float>::max()) {
				ret.emplace_back(type, distance_coords);
			}
		}
		return ret;
	}

	FurnitureType BedHandler::GetBedType(const RE::TESObjectREFR* a_reference)
	{
		if (a_reference->HasKeyword(GameForms::FurnitureBedRoll)) {
			return FurnitureType::BedRoll;
		}
		if (std::string name{ a_reference->GetName() }; name.empty() || AsLower(name).find("bed") == std::string::npos)
			return FurnitureType::None;
		const auto root = a_reference->Get3D();
		const auto extra = root ? root->GetExtraData("FRN") : nullptr;
		const auto node = extra ? netimmerse_cast<RE::BSFurnitureMarkerNode*>(extra) : nullptr;
		if (!node) {
			return FurnitureType::None;
		}

		size_t sleepmarkers = 0;
		for (auto&& marker : node->markers) {
			if (marker.animationType.all(RE::BSFurnitureMarker::AnimationType::kSleep)) {
				sleepmarkers += 1;
			}
		}
		switch (sleepmarkers) {
		case 0:
			return FurnitureType::None;
		case 1:
			return FurnitureType::BedSingle;
		default:
			return FurnitureType::BedDouble;
		}
	}

	std::vector<RE::TESObjectREFR*> BedHandler::GetBedsInArea(RE::TESObjectREFR* a_center, float a_radius, float a_radiusz)
	{
		const auto center = a_center->GetPosition();
		std::vector<RE::TESObjectREFR*> ret{};
		const auto add = [&](RE::TESObjectREFR& ref) {
			if (!ref.GetBaseObject()->Is(RE::FormType::Furniture) && a_radiusz > 0.0f ? (std::fabs(center.z - ref.GetPosition().z) <= a_radiusz) : true)
				if (Registry::BedHandler::IsBed(&ref))
					ret.push_back(&ref);
			return RE::BSContainer::ForEachResult::kContinue;
		};
		const auto TES = RE::TES::GetSingleton();
		if (const auto interior = TES->interiorCell; interior) {
			interior->ForEachReferenceInRange(center, a_radius, add);
		} else if (const auto grids = TES->gridCells; grids) {
			// Derived from: https://github.com/powerof3/PapyrusExtenderSSE
			auto gridLength = grids->length;
			if (gridLength > 0) {
				float yPlus = center.y + a_radius;
				float yMinus = center.y - a_radius;
				float xPlus = center.x + a_radius;
				float xMinus = center.x - a_radius;
				for (uint32_t x = 0, y = 0; (x < gridLength && y < gridLength); x++, y++) {
					const auto gridcell = grids->GetCell(x, y);
					if (gridcell && gridcell->IsAttached()) {
						auto cellCoords = gridcell->GetCoordinates();
						if (!cellCoords)
							continue;
						float worldX = cellCoords->worldX;
						float worldY = cellCoords->worldY;
						if (worldX < xPlus && (worldX + 4096.0) > xMinus && worldY < yPlus && (worldY + 4096.0) > yMinus) {
							gridcell->ForEachReferenceInRange(center, a_radius, add);
						}
					}
				}
			}
			if (!ret.empty()) {
				std::sort(ret.begin(), ret.end(), [&](RE::TESObjectREFR* a_refA, RE::TESObjectREFR* a_refB) {
					return center.GetDistance(a_refA->GetPosition()) < center.GetDistance(a_refB->GetPosition());
				});
			}
		}
		return ret;
	}

	bool BedHandler::IsBed(const RE::TESObjectREFR* a_reference)
	{
		return GetBedType(a_reference) != FurnitureType::None;
	}

}	 // namespace Registry
