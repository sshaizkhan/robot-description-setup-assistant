#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace robot_catalog {

// Hard cap on the size of a mesh/image the service will read and return.
// Files above this are rejected (not read into memory) so the user can shrink
// or decimate the offending asset.
constexpr uint64_t kMeshSizeCap = 5ULL * 1024 * 1024;  // 5 MiB

struct MeshResult {
  bool ok = false;                 // true if a real file inside the package was read
  bool too_large = false;          // true if the file exists but exceeds the cap
  uint64_t size = 0;               // actual file size in bytes (set when too_large)
  std::vector<uint8_t> data;       // raw file bytes on success
  std::string media_type;          // MIME type derived from the extension
};

// Resolve `package`/`rel_path` via ament_index_cpp (rejecting paths that escape
// the package share dir), then read the file into memory if it is within the
// size cap. All filesystem work happens here in C++; callers receive bytes,
// never a path.
MeshResult readMesh(const std::string& package, const std::string& rel_path);

}  // namespace robot_catalog
