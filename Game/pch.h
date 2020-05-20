#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <array>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <chrono>
#include <cstdint>
#include <d3d11_1.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <dxgi.h>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <functional>
#include <memory>
#include <SDL2/SDL.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_syswm.h>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>
#include <wrl.h>
#include <entt/entt.hpp>

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>

#include "imgui.h"
