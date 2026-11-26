#pragma once
#include <vector>
#include "ISaveable.h"
#include <memory>

namespace sdlgf {
	class SaveManager {
	private:
		std::vector<ISaveable*> saveables;
	public:
		void Clear();
		void Register(ISaveable* saveable);
		void Save(const std::string& filepath);
		void Load(const std::string& filepath);
	};
}