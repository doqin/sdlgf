#include "pch.h"
#include "Map.h"
#include "../util/Utils.h"
#include <cstring>
#include <filesystem>
#include <algorithm>
#include <cctype>

namespace {
	bool EqualsIgnoreCase(const std::string& a, const std::string& b)
	{
		return std::equal(a.begin(), a.end(), b.begin(), b.end(), [](unsigned char left, unsigned char right) {
			return std::tolower(left) == std::tolower(right);
		});
	}
}

void sdlgf::Map::Create(std::weak_ptr<TransformManager> transformManager, std::weak_ptr<ColliderManager> colliderManager, std::string pathToTmx)
{
	if (!std::filesystem::exists(pathToTmx)) {
		throw std::invalid_argument("Could not find map file: " + pathToTmx);
	}
	if (!map.load(pathToTmx)) {
		throw std::invalid_argument("Could not load map file: " + pathToTmx);
	}
	// load the textures
	const auto& tileSets = map.getTilesets();
	assert(!tileSets.empty());
	for (const auto& ts : tileSets)
	{
		std::string imgPathStr = ts.getImagePath();
		if (!std::filesystem::exists(imgPathStr)) {
			// Skip or throw? It's better to throw or log, let's throw.
			throw std::invalid_argument("Could not find texture file: " + imgPathStr);
		}
		textures.emplace_back(std::make_shared<Texture>(graphicsDevice));
		textures.back()->LoadTexture(sdlgf::Platform::Utf8ToWide(imgPathStr));
	}
	// load the layers
	const auto& mapLayers = map.getLayers();
	for (auto i = 0u; i < mapLayers.size(); ++i)
	{
		if (mapLayers[i]->getType() == tmx::Layer::Type::Tile)
		{
			layers.emplace_back(std::make_shared<MapLayer>(graphicsDevice));
			layers.back()->Create(this, i); //just cos we're using C++14

			const auto& tileLayer = mapLayers[i]->getLayerAs<tmx::TileLayer>();
			std::string lowerName = tileLayer.getName();
			std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
				[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			if (std::find(tileLayerNames.begin(), tileLayerNames.end(), lowerName) == tileLayerNames.end()) {
				tileLayerNames.push_back(lowerName);
			}
			auto& cells = tileCells[lowerName];
			auto collectTiles = [&cells](int originX, int originY, int sizeX, int sizeY, const std::vector<tmx::TileLayer::Tile>& tiles) {
				for (int y = 0; y < sizeY; ++y) {
					for (int x = 0; x < sizeX; ++x) {
						if (tiles[static_cast<size_t>(y) * sizeX + x].ID != 0) {
							cells.insert(PackTile(originX + x, originY + y));
						}
					}
				}
			};
			if (map.isInfinite()) {
				for (const auto& chunk : tileLayer.getChunks()) {
					collectTiles(chunk.position.x, chunk.position.y, chunk.size.x, chunk.size.y, chunk.tiles);
				}
			}
			else {
				const auto mapSize = map.getTileCount();
				collectTiles(0, 0, static_cast<int>(mapSize.x), static_cast<int>(mapSize.y), tileLayer.getTiles());
			}
		}
        if (mapLayers[i]->getType() == tmx::Layer::Type::Object)
		{
			const auto& layer = mapLayers[i]->getLayerAs<tmx::ObjectGroup>();
           const auto& layerName = layer.getName();
			const bool isCollisionLayer = EqualsIgnoreCase(layerName, "collision");
			const auto& objects = layer.getObjects();
			for (auto& object : objects) {
				if (object.getShape() == tmx::Object::Shape::Rectangle) {
					auto rect = object.getAABB();
					if (isCollisionLayer) {
						collisionAreas.push_back({ rect.left, rect.top, rect.width, rect.height });
						std::shared_ptr<RectangleCollider> collider = std::make_shared<RectangleCollider>(transformManager, rect.width, rect.height, rect.left, rect.top);
						colliders.push_back(collider);
						colliderManager.lock()->Add(collider);
					}
					else {
						objectAreasByLayer[layerName].push_back({ rect.left, rect.top, rect.width, rect.height });
					}
				}
			}
		}
	}
}

void sdlgf::Map::Draw(const Camera& camera)
{
	if (graphicsDevice == nullptr || layers.empty()) return;

	const auto [screenWidth, screenHeight] = camera.GetScreenResolution();
	const auto [corner00X, corner00Y] = Utils::WindowToWorldCoords(camera, 0.0f, 0.0f);
	const auto [corner10X, corner10Y] = Utils::WindowToWorldCoords(camera, static_cast<float>(screenWidth), 0.0f);
	const auto [corner01X, corner01Y] = Utils::WindowToWorldCoords(camera, 0.0f, static_cast<float>(screenHeight));
	const auto [corner11X, corner11Y] = Utils::WindowToWorldCoords(camera, static_cast<float>(screenWidth), static_cast<float>(screenHeight));
	MapLayer::ViewBounds viewBounds = {
		(std::min)({ corner00X, corner10X, corner01X, corner11X }),
		(std::min)({ corner00Y, corner10Y, corner01Y, corner11Y }),
		(std::max)({ corner00X, corner10X, corner01X, corner11X }),
		(std::max)({ corner00Y, corner10Y, corner01Y, corner11Y })
	};

	for (auto& layer : layers) {
		layer->Draw(camera, viewBounds);
	}
}

void sdlgf::Map::UpdateAreas(float pointX, float pointY)
{
   for (const auto& [layerName, areas] : objectAreasByLayer) {
		auto handlerIt = areaUpdateHandlers.find(layerName);
		if (handlerIt == areaUpdateHandlers.end()) {
			continue;
		}
		const auto& handler = handlerIt->second;
		if (!handler) {
			continue;
		}
		for (const auto& area : areas) {
			if (Utils::IsWithinRectangle(pointX, pointY, area.x, area.y, area.width, area.height)) {
				handler(area);
			}
		}
	}
}

void sdlgf::Map::SetAreaUpdateHandler(const std::string& layerName, std::function<void(const ObjectArea&)> handler)
{
 areaUpdateHandlers[layerName] = std::move(handler);
}

const std::unordered_set<std::int64_t>& sdlgf::Map::GetTileCells(const std::string& lowerLayerName) const
{
	static const std::unordered_set<std::int64_t> empty;
	auto it = tileCells.find(lowerLayerName);
	return it != tileCells.end() ? it->second : empty;
}

const std::vector<sdlgf::Map::ObjectArea>& sdlgf::Map::GetAreas(const std::string& layerName) const
{
	static const std::vector<ObjectArea> empty;
	auto it = objectAreasByLayer.find(layerName);
	return it != objectAreasByLayer.end() ? it->second : empty;
}

std::vector<std::shared_ptr<sdlgf::Texture>>& sdlgf::Map::GetTextures()
{
	return textures;
}

std::vector<std::shared_ptr<sdlgf::MapLayer>>& sdlgf::Map::GetLayers()
{
	return layers;
}

std::vector<std::shared_ptr<sdlgf::ICollider>>& sdlgf::Map::GetColliders()
{
	return colliders;
}
