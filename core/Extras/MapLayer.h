#pragma once
#include "../Camera.h"
#include "../GraphicsDevice.h"
#include "../Texture.h"
#include "tmxlite/Tileset.hpp"
#include <memory>
#include <vector>

namespace sdlgf {
	class Map;
	class MapLayer {
	private:
		struct TileVertex {
			float x, y, z;
			sdlgf::Color color;
			float u, v;
		};

		struct Subset {
			struct ChunkRange {
				UINT startVertex;
				UINT primitiveCount;
				float x, y, width, height;
			};
			std::vector<ChunkRange> chunks;
			Texture* texture = nullptr;
			// Two triangles (6 vertices) per tile, in world coordinates; chunks index into this.
			std::vector<TileVertex> vertices;
		};
		
		std::vector<Subset> subsets;

		bool IsTileIDInTileSet(
			unsigned int idx, 
			const std::vector<std::uint32_t>& tileIDs, 
			const tmx::Tileset& tileSet
		);
		GraphicsDevice* graphicsDevice;
		// Scratch space for the tiles of one subset that are on screen, reused every frame
		std::vector<GraphicsDevice::Vertex> visibleVertices;
	public:
		MapLayer(GraphicsDevice* graphicsDevice) : graphicsDevice(graphicsDevice) {}
		~MapLayer() = default;
		void Create(Map* map, std::uint32_t layerIndex);
		
		struct ViewBounds { float minX, minY, maxX, maxY; };
		void Draw(const Camera& camera, const ViewBounds& viewBounds);
	};
}