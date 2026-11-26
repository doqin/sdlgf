#include "pch.h"
#include "MapLayer.h"
#include "Map.h"
#include "tmxlite/TileLayer.hpp"
#include <ranges>
#include <algorithm>
#include "../util/Utils.h"

bool sdlgf::MapLayer::IsTileIDInTileSet(unsigned int idx, const std::vector<std::uint32_t>& tileIDs, const tmx::Tileset& tileSet)
{
	return idx < tileIDs.size() && tileIDs[idx] >= tileSet.getFirstGID() && tileIDs[idx] < (tileSet.getFirstGID() + tileSet.getTileCount());
}

void sdlgf::MapLayer::Create(Map* map, std::uint32_t layerIndex)
{
	auto& tmxMap = map->map;
	const auto& layers = tmxMap.getLayers();
	const auto& textures = map->textures;
	assert(layers[layerIndex]->getType() == tmx::Layer::Type::Tile);
	const auto& layer = layers[layerIndex]->getLayerAs<tmx::TileLayer>();
	const auto gridSize = tmxMap.getTileSize(); // returns the size of a grid
	const auto& tileSets = tmxMap.getTilesets();
	const auto tintColour = layer.getTintColour();
	const sdlgf::Color vertexColour = sdlgf::ColorF(
		static_cast<float>(tintColour.r) / 0xFF,
		static_cast<float>(tintColour.g) / 0xFF,
		static_cast<float>(tintColour.b) / 0xFF,
		static_cast<float>(tintColour.a) / 0xFF
	);

	std::vector<tmx::TileLayer::Chunk> chunks;
	if (tmxMap.isInfinite()) {
		chunks = layer.getChunks();
	}
	else {
		tmx::TileLayer::Chunk chunk;
		chunk.position = { 0, 0 };
		auto mapSize = tmxMap.getTileCount();
		chunk.size = { static_cast<int>(mapSize.x), static_cast<int>(mapSize.y) };
		chunk.tiles = layer.getTiles();
		chunks.push_back(chunk);
	}

	for (auto i = 0u; i < tileSets.size(); ++i) {
		const auto& tileSet = tileSets[i];
		const auto& [textureSizeX, textureSizeY] = textures[i]->GetSize();
		const auto& tileSetTileSize = tileSet.getTileSize();
		const auto tileCountX = tileSet.getColumnCount();
		const auto tileSpacing = tileSet.getSpacing();
		const auto tileMargin = tileSet.getMargin();
		const float uNorm = static_cast<float>(tileSetTileSize.x) / textureSizeX;
		const float vNorm = static_cast<float>(tileSetTileSize.y) / textureSizeY;
		Subset subset;
		subset.texture = textures[i];
		std::vector<TileVertex> subsetVertices;

		for (const auto& chunk : chunks) {
			std::vector<TileVertex> vertices;

			for (int y = 0; y < chunk.size.y; ++y) {
				for (int x = 0; x < chunk.size.x; ++x) {
					const auto idx = y * chunk.size.x + x;

					const auto& tile = chunk.tiles[idx];
					std::uint32_t cleanTileID = tile.ID;

					if (cleanTileID == 0 || cleanTileID < tileSet.getFirstGID() || cleanTileID >= (tileSet.getFirstGID() + tileSet.getTileCount())) {
						continue;
					}

					bool flipX = (tile.flipFlags & tmx::TileLayer::FlipFlag::Horizontal);
					bool flipY = (tile.flipFlags & tmx::TileLayer::FlipFlag::Vertical);
					bool flipD = (tile.flipFlags & tmx::TileLayer::FlipFlag::Diagonal);

					auto idIndex = (cleanTileID - tileSet.getFirstGID());
					const auto tileX = idIndex % tileCountX;
					const auto tileY = idIndex / tileCountX;

					// Inset the src rect by 1/64 texel so interpolation error at
					// quad edges can't sample the neighbouring tileset tile.
					const float EPSILON = 1.0f / 64.0f;

					float u0 = (static_cast<float>(tileMargin + tileX * (tileSetTileSize.x + tileSpacing)) + EPSILON) / textureSizeX;
					float v0 = (static_cast<float>(tileMargin + tileY * (tileSetTileSize.y + tileSpacing)) + EPSILON) / textureSizeY;
					float u1 = (static_cast<float>(tileMargin + tileX * (tileSetTileSize.x + tileSpacing) + tileSetTileSize.x) - EPSILON) / textureSizeX;
					float v1 = (static_cast<float>(tileMargin + tileY * (tileSetTileSize.y + tileSpacing) + tileSetTileSize.y) - EPSILON) / textureSizeY;

					// default: 0:TopLeft, 1:TopRight, 2:BottomRight, 3:BottomLeft
					struct UVCoord { float u, v; };
					UVCoord tex[4] = {
						{u0, v0}, // 0
						{u1, v0}, // 1
						{u1, v1}, // 2
						{u0, v1}  // 3
					};

					//diagonal flip
					if (flipD) {
						std::swap(tex[1], tex[3]);
					}
					//horizontal flip
					if (flipX) {
						std::swap(tex[0], tex[1]);
						std::swap(tex[3], tex[2]);
					}
					//vertical flip
					if (flipY) {
						std::swap(tex[0], tex[3]);
						std::swap(tex[1], tex[2]);
					}

					const float tilePosX = static_cast<float>(x + chunk.position.x) * gridSize.x;
					const float tilePosY = (static_cast<float>(y + chunk.position.y) * gridSize.y);

					// Triangle 1: Top-Left (tex[0]), Top-Right (tex[1]), Bottom-Left (tex[3])
					vertices.push_back({ .x = tilePosX, .y = tilePosY, .z = .0f, .color = vertexColour, .u = tex[0].u, .v = tex[0].v });
					vertices.push_back({ .x = tilePosX + tileSetTileSize.x, .y = tilePosY, .z = .0f, .color = vertexColour, .u = tex[1].u, .v = tex[1].v });
					vertices.push_back({ .x = tilePosX, .y = tilePosY + tileSetTileSize.y, .z = .0f, .color = vertexColour, .u = tex[3].u, .v = tex[3].v });

					// Triangle 2: Top-Right (tex[1]), Bottom-Right (tex[2]), Bottom-Left (tex[3])
					vertices.push_back({ .x = tilePosX + tileSetTileSize.x, .y = tilePosY, .z = .0f, .color = vertexColour, .u = tex[1].u, .v = tex[1].v });
					vertices.push_back({ .x = tilePosX + tileSetTileSize.x, .y = tilePosY + tileSetTileSize.y, .z = .0f, .color = vertexColour, .u = tex[2].u, .v = tex[2].v });
					vertices.push_back({ .x = tilePosX, .y = tilePosY + tileSetTileSize.y, .z = .0f, .color = vertexColour, .u = tex[3].u, .v = tex[3].v });
				}
			}

			if (!vertices.empty()) {
				Subset::ChunkRange chunkRange;
				chunkRange.startVertex = static_cast<UINT>(subsetVertices.size());
				chunkRange.primitiveCount = static_cast<UINT>(vertices.size() / 3);
				chunkRange.x = static_cast<float>(chunk.position.x) * gridSize.x;
				chunkRange.y = static_cast<float>(chunk.position.y) * gridSize.y;
				chunkRange.width = static_cast<float>(chunk.size.x) * gridSize.x;
				chunkRange.height = static_cast<float>(chunk.size.y) * gridSize.y;
				subset.chunks.push_back(chunkRange);

				subsetVertices.insert(subsetVertices.end(), vertices.begin(), vertices.end());
			}
		}

		if (!subsetVertices.empty()) {
			subset.vertices = std::move(subsetVertices);
			subsets.emplace_back(std::move(subset));
		}
	}
}

void sdlgf::MapLayer::Draw(const Camera& camera, const ViewBounds& viewBounds)
{
	if (graphicsDevice == nullptr || subsets.empty()) return;

	const float overlapEpsilon = 0.01f;
	const auto intersectsView = [&](float x, float y, float width, float height) {
		const float rectMaxX = x + width;
		const float rectMaxY = y + height;
		return rectMaxX > viewBounds.minX + overlapEpsilon
			&& x < viewBounds.maxX - overlapEpsilon
			&& rectMaxY > viewBounds.minY + overlapEpsilon
			&& y < viewBounds.maxY - overlapEpsilon;
	};
	const sdlgf::Mat4 matCamera = camera.GetTransformMatrix();

	for (const auto& subset : subsets) {
		if (subset.chunks.empty() || subset.texture == nullptr || !subset.texture->IsLoaded()) continue;

		visibleVertices.clear();
		for (const auto& chunk : subset.chunks) {
			if (!intersectsView(chunk.x, chunk.y, chunk.width, chunk.height)) continue;

			// A chunk can be a whole map, so also skip the tiles inside it that are off screen.
			// Vertices 0 and 4 of a tile (6 vertices) are its top-left and bottom-right corners.
			for (UINT tile = 0; tile < chunk.primitiveCount / 2; ++tile) {
				const TileVertex* quad = &subset.vertices[chunk.startVertex + tile * 6];
				const TileVertex& topLeft = quad[0];
				const TileVertex& bottomRight = quad[4];
				if (!intersectsView(topLeft.x, topLeft.y, bottomRight.x - topLeft.x, bottomRight.y - topLeft.y)) continue;

				for (int i = 0; i < 6; ++i) {
					sdlgf::Vec4 p = sdlgf::Transform(sdlgf::Vec4(quad[i].x, quad[i].y, quad[i].z, 1.0f), matCamera);
					if (p.w != 0.0f) {
						p.x /= p.w;
						p.y /= p.w;
					}
					visibleVertices.push_back({ p.x, p.y, quad[i].u, quad[i].v, quad[i].color });
				}
			}
		}
		graphicsDevice->DrawTexturedTriangles(subset.texture.get(), visibleVertices.data(), static_cast<int>(visibleVertices.size()));
	}
}
