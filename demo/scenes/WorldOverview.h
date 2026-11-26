#pragma once
#include <string>
#include <utility>
#include <vector>
#include "MapView.h"

namespace Demo {
	/// Everything the fast-travel map needs to show a world, taken from the world scene before it is
	/// unloaded: the baked (flat colour, tiny) map grid with its fog of war, the markers, and where the
	/// save points are. Lets the travel map list worlds whose scenes are not in memory.
	struct WorldOverview {
		struct SavePointInfo {
			std::string id;
			float x = 0.f, y = 0.f;
			float spawnX = 0.f, spawnY = 0.f; // SavePoint::GetSpawnPosition, the fallback landing spot
		};

		std::string sceneId;
		MapView mapView;
		std::vector<MapView::Marker> markers;
		std::vector<SavePointInfo> savePoints;

		const SavePointInfo* FindSavePoint(const std::string& id) const {
			for (const auto& sp : savePoints) if (sp.id == id) return &sp;
			return nullptr;
		}

		/// A walkable position beside the save point, so fast travel never lands in a wall or off the map.
		std::pair<float, float> FindSpawnNear(const SavePointInfo& point) const {
			const float px = point.x, py = point.y;
			// The player's whole footprint has to be on floor, not just one point
			auto clear = [&](float x, float y) {
				constexpr float kPad = 10.f;
				return mapView.IsWalkable(x, y) && mapView.IsWalkable(x - kPad, y) && mapView.IsWalkable(x + kPad, y) && mapView.IsWalkable(x, y - kPad) && mapView.IsWalkable(x, y + kPad);
			};
			// Prefer in front of the point, then sideways, then behind, widening the search each ring
			for (float r : {32.f, 48.f, 64.f, 96.f}) {
				const float offsets[][2] = {{0, r}, {r, 0}, {-r, 0}, {r, r}, {-r, r}, {0, -r}, {r, -r}, {-r, -r}};
				for (const auto& o : offsets) {
					if (clear(px + o[0], py + o[1])) return {px + o[0], py + o[1]};
				}
			}
			return {point.spawnX, point.spawnY};
		}
	};
}
