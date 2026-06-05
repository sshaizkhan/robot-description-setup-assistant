#include "robot_catalog_core/urdf.hpp"

#include <array>
#include <ctime>
#include <filesystem>
#include <string>
#include <vector>

#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <ament_index_cpp/get_package_share_directory.hpp>

namespace fs = std::filesystem;

namespace robot_catalog {

std::vector<std::string> tokenizeArgs(const std::string& args) {
  std::vector<std::string> out;
  std::string cur;
  bool in_tok = false;
  char quote = 0;
  for (char c : args) {
    if (quote) {
      if (c == quote) {
        quote = 0;
      } else {
        cur += c;
      }
      in_tok = true;
    } else if (c == '"' || c == '\'') {
      quote = c;
      in_tok = true;
    } else if (c == ' ' || c == '\t' || c == '\n') {
      if (in_tok) {
        out.push_back(cur);
        cur.clear();
        in_tok = false;
      }
    } else {
      cur += c;
      in_tok = true;
    }
  }
  if (in_tok) out.push_back(cur);
  return out;
}

// Run argv (argv[0] resolved via PATH). Captures stdout into `out` and stderr
// into `err`, separately (xacro emits XML on stdout, warnings on stderr).
// Returns the child's exit code, or -1 on spawn/wait failure. Kills the child
// after timeout_sec.
static int runProcess(const std::vector<std::string>& args, std::string& out,
                      std::string& err, int timeout_sec) {
  int outpipe[2];
  int errpipe[2];
  if (pipe(outpipe) != 0) return -1;
  if (pipe(errpipe) != 0) {
    close(outpipe[0]);
    close(outpipe[1]);
    return -1;
  }

  pid_t pid = fork();
  if (pid < 0) {
    close(outpipe[0]); close(outpipe[1]);
    close(errpipe[0]); close(errpipe[1]);
    return -1;
  }

  if (pid == 0) {
    // Child: wire stdout/stderr to the pipes and exec.
    dup2(outpipe[1], STDOUT_FILENO);
    dup2(errpipe[1], STDERR_FILENO);
    close(outpipe[0]); close(outpipe[1]);
    close(errpipe[0]); close(errpipe[1]);
    std::vector<char*> argv;
    argv.reserve(args.size() + 1);
    for (const auto& a : args) argv.push_back(const_cast<char*>(a.c_str()));
    argv.push_back(nullptr);
    execvp(argv[0], argv.data());
    _exit(127);  // exec failed
  }

  // Parent.
  close(outpipe[1]);
  close(errpipe[1]);
  fcntl(outpipe[0], F_SETFL, O_NONBLOCK);
  fcntl(errpipe[0], F_SETFL, O_NONBLOCK);

  bool out_open = true;
  bool err_open = true;
  std::array<char, 4096> buf;
  const time_t start = time(nullptr);

  while (out_open || err_open) {
    if (out_open) {
      ssize_t n = read(outpipe[0], buf.data(), buf.size());
      if (n > 0) {
        out.append(buf.data(), static_cast<size_t>(n));
      } else if (n == 0) {
        close(outpipe[0]);
        out_open = false;
      }
    }
    if (err_open) {
      ssize_t n = read(errpipe[0], buf.data(), buf.size());
      if (n > 0) {
        err.append(buf.data(), static_cast<size_t>(n));
      } else if (n == 0) {
        close(errpipe[0]);
        err_open = false;
      }
    }
    if (timeout_sec > 0 && time(nullptr) - start > timeout_sec) {
      kill(pid, SIGKILL);
      err += "\nxacro timed out";
      if (out_open) { close(outpipe[0]); out_open = false; }
      if (err_open) { close(errpipe[0]); err_open = false; }
      break;
    }
    if (out_open || err_open) {
      // Avoid a busy spin while both pipes would block.
      struct timespec ts {0, 2 * 1000 * 1000};  // 2 ms
      nanosleep(&ts, nullptr);
    }
  }

  int status = 0;
  if (waitpid(pid, &status, 0) < 0) return -1;
  if (WIFEXITED(status)) return WEXITSTATUS(status);
  return -1;
}

UrdfResult resolveUrdf(const RobotConfig& robot) {
  UrdfResult res;

  std::string share;
  try {
    share = ament_index_cpp::get_package_share_directory(robot.urdf_package);
  } catch (const std::exception&) {
    res.error = "package not found: " + robot.urdf_package;
    return res;
  }

  fs::path xacro_file = fs::path(share) / robot.urdf_path;
  if (!fs::is_regular_file(xacro_file)) {
    res.error = "xacro file not found: " + xacro_file.string();
    return res;
  }

  std::vector<std::string> argv = {"xacro", xacro_file.string()};
  for (auto& tok : tokenizeArgs(robot.xacro_args)) argv.push_back(tok);

  std::string out;
  std::string err;
  int code = runProcess(argv, out, err, 30);
  if (code != 0) {
    if (!err.empty()) {
      res.error = err;
    } else {
      res.error = "xacro failed (exit " + std::to_string(code) + ")";
    }
    return res;
  }

  res.ok = true;
  res.xml = out;
  return res;
}

}  // namespace robot_catalog
