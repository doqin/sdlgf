#pragma once
#include <string>
#include <tmxlite/Map.hpp>
#include <vector>
#include <memory>
#include <functional>
#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>
#include "../Camera.h"
#include "../Texture.h"
#include "MapLayer.h"
#include "ICollider.h"
#include "ColliderManager.h"
#include "RectangleCollider.h"

namespace sdlgf {
	class Map {
	public:
		struct ObjectArea {
			float x;
			float y;
			float width;
			float height;
		};
	private:
		GraphicsDevice* graphicsDevice;
		tmx::Map map;
		std::vector<std::shared_ptr<Texture>> textures;
		std::vector<std::shared_ptr<MapLayer>> layers;
		std::vector<std::shared_ptr<ICollider>> colliders;
        
		std::unordered_map<std::string, std::vector<ObjectArea>> objectAreasByLayer;
		std::unordered_map<std::string, std::function<void(const ObjectArea&)>> areaUpdateHandlers;

		// Occupied tile coordinates per tile layer (keyed by lowercase layer name) and the
		// collision rectangles, kept so overview maps can be built without re-reading the TMX.
		std::unordered_map<std::string, std::unordered_set<std::int64_t>> tileCells;
		std::vector<std::string> tileLayerNames;
		std::vector<ObjectArea> collisionAreas;
	public:
		static std::int64_t PackTile(int tileX, int tileY) {
			return (static_cast<std::int64_t>(tileX) << 32) | static_cast<std::uint32_t>(tileY);
		}
		static void UnpackTile(std::int64_t packed, int& tileX, int& tileY) {
			tileX = static_cast<int>(packed >> 32);
			tileY = static_cast<int>(static_cast<std::uint32_t>(packed & 0xFFFFFFFF));
		}
		Map(GraphicsDevice* graphicsDevice) : graphicsDevice(graphicsDevice) {}
		void Create(std::weak_ptr<TransformManager> transformManager, std::weak_ptr<ColliderManager> colliderManager, std::string pathToTmx);
		void Draw(const Camera& camera);
        void UpdateAreas(float pointX, float pointY);
		void SetAreaUpdateHandler(const std::string& layerName, std::function<void(const ObjectArea&)> handler);
		std::vector<std::shared_ptr<Texture>>& GetTextures();
		std::vector<std::shared_ptr<MapLayer>>& GetLayers();
		std::vector<std::shared_ptr<ICollider>>& GetColliders();

		int GetTileWidth() const { return static_cast<int>(map.getTileSize().x); }
		int GetTileHeight() const { return static_cast<int>(map.getTileSize().y); }
		// Lowercase names of every tile layer, in file order.
		const std::vector<std::string>& GetTileLayerNames() const { return tileLayerNames; }
		// Occupied tile coordinates of a tile layer (PackTile keys); empty for unknown layers.
		const std::unordered_set<std::int64_t>& GetTileCells(const std::string& lowerLayerName) const;
		// Rectangles of a non-collision object layer (empty for unknown layers).
		const std::vector<ObjectArea>& GetAreas(const std::string& layerName) const;
		const std::vector<ObjectArea>& GetCollisionAreas() const { return collisionAreas; }
		friend class MapLayer;
	};
}