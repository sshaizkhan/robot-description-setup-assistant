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

#include "robot_description_setup_framework/urdf_loader.hpp"
#include "robot_description_setup_framework/process_utils.hpp"

#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <urdf_parser/urdf_parser.h>

namespace robot_description
{
namespace
{
// Split a xacro argument string into tokens, honoring single and double quotes
// so values containing spaces (e.g. initial_positions:="0 0 1.57") stay intact.
std::vector<std::string> tokenizeArgs(const std::string& args)
{
  std::vector<std::string> tokens;
  std::string cur;
  bool in_token = false;
  char quote = 0;
  for (char c : args)
  {
    if (quote)
    {
      if (c == quote)
        quote = 0;
      else
        cur.push_back(c);
    }
    else if (c == '"' || c == '\'')
    {
      quote = c;
      in_token = true;
    }
    else if (std::isspace(static_cast<unsigned char>(c)))
    {
      if (in_token)
      {
        tokens.push_back(cur);
        cur.clear();
        in_token = false;
      }
    }
    else
    {
      cur.push_back(c);
      in_token = true;
    }
  }
  if (in_token)
  {
    tokens.push_back(cur);
  }
  return tokens;
}
}  // namespace

bool URDFLoader::isXacro(const std::filesystem::path& p)
{
  return p.extension() == ".xacro";
}

std::string URDFLoader::readFile(const std::filesystem::path& p)
{
  std::ifstream in(p);
  if (!in)
  {
    throw std::runtime_error("Unable to open URDF file: " + p.string());
  }
  std::stringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

std::string URDFLoader::runXacro(const std::filesystem::path& p, const std::string& args)
{
  std::vector<std::string> argv{ "xacro", p.string() };
  // split args, honoring quotes ("name:=x ur_type:=ur5", or values with spaces)
  for (std::string& tok : tokenizeArgs(args))
  {
    argv.push_back(std::move(tok));
  }

  std::string out, err;
  int code = runProcess(argv, out, err);
  if (code != 0)
  {
    throw std::runtime_error("xacro failed (exit " + std::to_string(code) + "): " + err);
  }
  return out;
}

URDFModel URDFLoader::parse(const std::string& xml)
{
  urdf::ModelInterfaceSharedPtr model = urdf::parseURDF(xml);
  if (!model)
  {
    throw std::runtime_error("URDF parse failed");
  }

  URDFModel out;
  out.xml = xml;
  out.robot_name = model->getName();
  if (model->getRoot())
  {
    out.root_link = model->getRoot()->name;
  }
  for (const auto& [name, joint] : model->joints_)
  {
    // Skip FIXED/UNKNOWN, and mimic joints (their value is derived from the
    // joint they mimic, not an independent DOF to publish).
    if (joint && joint->type != urdf::Joint::FIXED && joint->type != urdf::Joint::UNKNOWN && !joint->mimic)
    {
      out.movable_joints.push_back(name);
    }
  }
  return out;
}

URDFModel URDFLoader::load(const std::filesystem::path& urdf_path, const std::string& xacro_args)
{
  if (!std::filesystem::is_regular_file(urdf_path))
  {
    throw std::runtime_error("URDF file does not exist: " + urdf_path.string());
  }
  const std::string xml = isXacro(urdf_path) ? runXacro(urdf_path, xacro_args) : readFile(urdf_path);
  return parse(xml);
}
}  // namespace robot_description
