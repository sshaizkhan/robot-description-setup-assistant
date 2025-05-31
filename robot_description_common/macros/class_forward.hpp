#pragma once

#include "declare_ptr.hpp"

/**
 * \def ROBOT_DESCRIPTION_CLASS_FORWARD
 * 
 * Forward declares a class and creates its smart pointer types.
 * Combines forward declaration with smart pointer type creation.
 * 
 * Example:
 *   ROBOT_DESCRIPTION_CLASS_FORWARD(RobotModel)
 *   // Creates: class RobotModel; and RobotModelPtr, RobotModelConstPtr, etc.
 */
#define ROBOT_DESCRIPTION_CLASS_FORWARD(ClassName)                                   \
  class ClassName;                                                                   \
  ROBOT_DESCRIPTION_DECLARE_PTR(ClassName, ClassName)

/**
 * \def ROBOT_DESCRIPTION_STRUCT_FORWARD  
 * 
 * Forward declares a struct and creates its smart pointer types.
 * 
 * Example:
 *   ROBOT_DESCRIPTION_STRUCT_FORWARD(RobotConfig)
 *   // Creates: struct RobotConfig; and RobotConfigPtr, etc.
 */
#define ROBOT_DESCRIPTION_STRUCT_FORWARD(StructName)                                 \
  struct StructName;                                                                 \
  ROBOT_DESCRIPTION_DECLARE_PTR(StructName, StructName)

/**
 * \def ROBOT_DESCRIPTION_FORWARD_PTR
 * 
 * Creates only smart pointer aliases for an already forward-declared type.
 * Use when the forward declaration exists but you need the pointer types.
 * 
 * Example:
 *   class RobotModel;  // Already declared elsewhere
 *   ROBOT_DESCRIPTION_FORWARD_PTR(Robot, RobotModel)
 *   // Creates: RobotPtr, RobotConstPtr, etc. (no forward declaration)
 */
#define ROBOT_DESCRIPTION_FORWARD_PTR(Name, Type)                                    \
  ROBOT_DESCRIPTION_DECLARE_PTR(Name, Type)