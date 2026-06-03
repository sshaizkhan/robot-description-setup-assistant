/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2026, Shahwaz Khan
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the copyright holder nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/
/* Author: Shahwaz Khan */

#include "robot_description_setup_framework/process_utils.hpp"

#include <array>
#include <poll.h>
#include <sys/wait.h>
#include <unistd.h>

namespace robot_description
{
namespace
{
void drain(int fd, std::string& sink)
{
  std::array<char, 4096> buf;
  ssize_t n;
  while ((n = ::read(fd, buf.data(), buf.size())) > 0)
  {
    sink.append(buf.data(), static_cast<size_t>(n));
  }
}
}  // namespace

int runProcess(const std::vector<std::string>& argv, std::string& out, std::string& err)
{
  out.clear();
  err.clear();
  if (argv.empty())
  {
    return -1;
  }

  int out_pipe[2];
  int err_pipe[2];
  if (::pipe(out_pipe) != 0 || ::pipe(err_pipe) != 0)
  {
    return -1;
  }

  pid_t pid = ::fork();
  if (pid < 0)
  {
    return -1;
  }

  if (pid == 0)
  {
    // child
    ::dup2(out_pipe[1], STDOUT_FILENO);
    ::dup2(err_pipe[1], STDERR_FILENO);
    ::close(out_pipe[0]);
    ::close(out_pipe[1]);
    ::close(err_pipe[0]);
    ::close(err_pipe[1]);

    std::vector<char*> c_argv;
    c_argv.reserve(argv.size() + 1);
    for (const std::string& a : argv)
    {
      c_argv.push_back(const_cast<char*>(a.c_str()));
    }
    c_argv.push_back(nullptr);

    ::execvp(c_argv[0], c_argv.data());
    ::_exit(127);  // exec failed
  }

  // parent
  ::close(out_pipe[1]);
  ::close(err_pipe[1]);

  // Read both fds concurrently via poll to avoid pipe-buffer deadlock.
  std::array<pollfd, 2> fds{ { { out_pipe[0], POLLIN, 0 }, { err_pipe[0], POLLIN, 0 } } };
  int open_fds = 2;
  while (open_fds > 0)
  {
    int ready = ::poll(fds.data(), fds.size(), -1);
    if (ready < 0)
    {
      break;
    }
    for (auto& pfd : fds)
    {
      if (pfd.fd < 0)
      {
        continue;
      }
      if (pfd.revents & (POLLIN | POLLHUP))
      {
        std::array<char, 4096> buf;
        ssize_t n = ::read(pfd.fd, buf.data(), buf.size());
        if (n > 0)
        {
          (pfd.fd == out_pipe[0] ? out : err).append(buf.data(), static_cast<size_t>(n));
        }
        else  // EOF or error
        {
          ::close(pfd.fd);
          pfd.fd = -1;
          --open_fds;
        }
      }
    }
  }
  // flush any straggler bytes
  if (fds[0].fd >= 0)
  {
    drain(out_pipe[0], out);
    ::close(out_pipe[0]);
  }
  if (fds[1].fd >= 0)
  {
    drain(err_pipe[0], err);
    ::close(err_pipe[0]);
  }

  int status = 0;
  ::waitpid(pid, &status, 0);
  if (WIFEXITED(status))
  {
    int code = WEXITSTATUS(status);
    return (code == 127) ? -1 : code;  // 127 == exec failed
  }
  return -1;
}
}  // namespace robot_description
