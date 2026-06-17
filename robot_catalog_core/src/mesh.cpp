#include "robot_catalog_core/mesh.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <system_error>
#include <ament_index_cpp/get_package_share_directory.hpp>

namespace fs = std::filesystem;

namespace robot_catalog {

static std::string mediaTypeFor(const std::string& path) {
  auto dot = path.find_last_of('.');
  std::string ext = (dot == std::string::npos) ? "" : path.substr(dot + 1);
  std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
  if (ext == "stl") return "model/stl";
  if (ext == "dae") return "model/vnd.collada+xml";
  if (ext == "obj") return "model/obj";
  if (ext == "mtl") return "model/mtl";
  if (ext == "glb") return "model/gltf-binary";
  if (ext == "gltf") return "model/gltf+json";
  if (ext == "png") return "image/png";
  if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
  if (ext == "gif") return "image/gif";
  if (ext == "svg") return "image/svg+xml";
  return "application/octet-stream";
}

MeshResult readMesh(const std::string& package, const std::string& rel_path) {
  MeshResult res;

  std::string share;
  try {
    share = ament_index_cpp::get_package_share_directory(package);
  } catch (const std::exception&) {
    return res;  // unknown package
  }

  std::error_code ec;
  fs::path base = fs::canonical(share, ec);
  if (ec) return res;

  // Collapse ".." lexically (do NOT resolve symlinks on the final file: merged/
  // symlink colcon installs symlink meshes to their source tree).
  fs::path target = (base / rel_path).lexically_normal();

  const std::string b = base.string();
  const std::string t = target.string();
  // Containment guard: target must live under base.
  if (t.size() < b.size() || t.compare(0, b.size(), b) != 0) return res;
  if (t.size() > b.size() && t[b.size()] != '/') return res;

  if (!fs::is_regular_file(target)) return res;

  // Enforce the size cap before reading anything into memory.
  std::error_code size_ec;
  uintmax_t fsize = fs::file_size(target, size_ec);
  if (size_ec) return res;
  if (fsize > kMeshSizeCap) {
    res.too_large = true;
    res.size = static_cast<uint64_t>(fsize);
    return res;
  }

  std::ifstream f(target, std::ios::binary);
  if (!f) return res;
  res.data.assign(std::istreambuf_iterator<char>(f),
                  std::istreambuf_iterator<char>());
  res.media_type = mediaTypeFor(t);
  res.ok = true;
  return res;
}

}  // namespace robot_catalog
