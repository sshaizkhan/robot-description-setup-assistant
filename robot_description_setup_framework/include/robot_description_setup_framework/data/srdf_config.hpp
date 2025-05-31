/*********************************************************************
 * Software License Agreement (BSD License)
 *
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
 *   * Neither the name of Willow Garage nor the names of its
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

#pragma once

#include <filesystem>
#include <memory>
#include <srdfdom/srdf_writer.h>
#include <rclcpp/node.hpp>

namespace robot_description::setup_framework
{

class RobotModel {
public:
    ///\brief Construct a kinematic model from a parsed description */
    RobotModel(const urdf::ModelInterfaceSharedPtr& urdf_model, const srdf::ModelConstSharedPtr& srdf_model);

    ~ RobotModel();

    ///\brief Get the model name
    const std::string& getName() const
    {
        return model_name_;
    }
    /** \brief Get the frame in which the transforms for this model are
    computed (when using a RobotState). This frame depends on the
    root joint. As such, the frame is either extracted from SRDF, or
    it is assumed to be the name of the root link */
    const std::string& getModelFrame() const
    {
        return model_frame_;
    }

    /** \brief Get the parsed URDF model */
    const urdf::ModelInterfaceSharedPtr& getURDF() const
    {
        return urdf_;
    }

    /** \brief Get the parsed SRDF model */
    const srdf::ModelConstSharedPtr& getSRDF() const
    {
        return srdf_;
    }

    /** \brief Print information about the constructed model */
    void printModelInfo(std::ostream& out) const;
protected:
    std::string model_name_;
    std::string model_frame_;

    srdf::ModelConstSharedPtr srdf_;

    urdf::ModelInterfaceSharedPtr urdf_;
};
typedef std::shared_ptr<RobotModel> RobotModelPtr;

class SRDFConfig
{
public:
    void onInit();

    bool isConfigured() const
    {
        return robot_model_ != nullptr;
    }

    /// Update the robot model with the new SRDF, AND mark the changes that have been made to the model
    void updateRobotModel(long changed_information = 0L);

    void clearCollisionData()
    {
        srdf_.no_default_collision_links_.clear();
        srdf_.enabled_collision_pairs_.clear();
        srdf_.disabled_collision_pairs_.clear();
    }

    std::vector<srdf::Model::CollisionPair>& getDisabledCollisions()
    {
        return srdf_.disabled_collision_pairs_;
    }

    std::vector<srdf::Model::EndEffector>& getEndEffectors()
    {
        return srdf_.end_effectors_;
    }

    std::vector<srdf::Model::Group>& getGroups()
    {
        return srdf_.groups_;
    }

    std::vector<std::string> getGroupNames() const
    {
        std::vector<std::string> group_names;
        group_names.reserve(srdf_.groups_.size());
        for (const srdf::Model::Group& group : srdf_.groups_)
        {
        group_names.push_back(group.name_);
        }
        return group_names;
    }

    std::vector<srdf::Model::GroupState>& getGroupStates()
    {
        return srdf_.group_states_;
    }

    std::vector<srdf::Model::VirtualJoint>& getVirtualJoints()
    {
        return srdf_.virtual_joints_;
    }

    std::vector<srdf::Model::PassiveJoint>& getPassiveJoints()
    {
        return srdf_.passive_joints_;
    }

protected:
    void getRelativePath();
    void loadURDFModel();

    std::filesystem::path srdf_path_;
    std::filesystem::path srdf_pkg_relative_path_;
    srdf::SRDFWriter srdf_;
    std::shared_ptr<urdf::Model> urdf_model_;

    RobotModelPtr robot_model_;
    unsigned long changes_;

    rclcpp::Node::SharedPtr parent_node_;
    std::shared_ptr<rclcpp::Logger> logger_;
};
} // namespace robot_description::setup_framework