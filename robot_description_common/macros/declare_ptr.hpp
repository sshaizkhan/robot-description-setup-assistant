#pragma once

#include <memory>
#include <atomic>

/**
 * \def ROBOT_DESCRIPTION_DECLARE_PTR
 * 
 * Enhanced macro that creates smart pointer type aliases for a given type.
 * 
 * Given a Name and Type, this macro declares:
 * - ${Name}Ptr, ${Name}ConstPtr (shared_ptr variants)
 * - ${Name}WeakPtr, ${Name}ConstWeakPtr (weak_ptr variants)  
 * - ${Name}UniquePtr, ${Name}ConstUniquePtr (unique_ptr variants)
 * - make${Name}, make${Name}Unique (factory functions)
 * 
 * Example usage:
 *   ROBOT_DESCRIPTION_DECLARE_PTR(Robot, RobotModel)
 *   // Creates: RobotPtr, RobotConstPtr, makeRobot(), etc.
 */
#define ROBOT_DESCRIPTION_DECLARE_PTR(Name, Type)                                    \
  using Name##Ptr = std::shared_ptr<Type>;                                          \
  using Name##ConstPtr = std::shared_ptr<const Type>;                               \
  using Name##WeakPtr = std::weak_ptr<Type>;                                        \
  using Name##ConstWeakPtr = std::weak_ptr<const Type>;                             \
  using Name##UniquePtr = std::unique_ptr<Type>;                                    \
  using Name##ConstUniquePtr = std::unique_ptr<const Type>;                         \
  \
  template<typename... Args>                                                        \
  inline Name##Ptr make##Name(Args&&... args) {                                     \
    return std::make_shared<Type>(std::forward<Args>(args)...);                     \
  }                                                                                  \
  \
  template<typename... Args>                                                        \
  inline Name##UniquePtr make##Name##Unique(Args&&... args) {                       \
    return std::make_unique<Type>(std::forward<Args>(args)...);                     \
  }

/**
 * \def ROBOT_DESCRIPTION_DECLARE_PTR_MEMBER
 * 
 * Member version that creates smart pointer aliases within a class scope.
 * 
 * Example usage:
 *   class RobotModel {
 *     ROBOT_DESCRIPTION_DECLARE_PTR_MEMBER(RobotModel)
 *     // Creates: RobotModel::Ptr, RobotModel::make(), etc.
 *   };
 */
#define ROBOT_DESCRIPTION_DECLARE_PTR_MEMBER(Type)                                   \
  using Ptr = std::shared_ptr<Type>;                                                \
  using ConstPtr = std::shared_ptr<const Type>;                                     \
  using WeakPtr = std::weak_ptr<Type>;                                              \
  using ConstWeakPtr = std::weak_ptr<const Type>;                                   \
  using UniquePtr = std::unique_ptr<Type>;                                          \
  using ConstUniquePtr = std::unique_ptr<const Type>;                               \
  \
  template<typename... Args>                                                        \
  static Ptr make(Args&&... args) {                                                 \
    return std::make_shared<Type>(std::forward<Args>(args)...);                     \
  }                                                                                  \
  \
  template<typename... Args>                                                        \
  static UniquePtr makeUnique(Args&&... args) {                                     \
    return std::make_unique<Type>(std::forward<Args>(args)...);                     \
  }