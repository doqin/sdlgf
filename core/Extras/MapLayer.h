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
		// info about the vertex for the graphics device
		const DWORD D3DFVF_TILEVERTEX = (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1);
		struct TileVertex {
			float x, y, z;
			DWORD color;
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
			IDirect3DVertexBuffer9* vertexBuffer = nullptr;
		};
		
		std::vector<Subset> subsets;

		bool IsTileIDInTileSet(
			unsigned int idx, 
			const std::vector<std::uint32_t>& tileIDs, 
			const tmx::Tileset& tileSet
		);
		GraphicsDevice* graphicsDevice;
	public:
		MapLayer(GraphicsDevice* graphicsDevice) : graphicsDevice(graphicsDevice) {}
		~MapLayer();
		void Create(Map* map, std::uint32_t layerIndex);
		
		struct ViewBounds { float minX, minY, maxX, maxY; };
		void Draw(const Camera& camera, const ViewBounds& viewBounds);
	};
}