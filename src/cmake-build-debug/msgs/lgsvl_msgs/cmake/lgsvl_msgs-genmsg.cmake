# generated from genmsg/cmake/pkg-genmsg.cmake.em

message(STATUS "lgsvl_msgs: 6 messages, 0 services")

set(MSG_I_FLAGS "-Ilgsvl_msgs:/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg;-Igeometry_msgs:/opt/ros/kinetic/share/geometry_msgs/cmake/../msg;-Isensor_msgs:/opt/ros/kinetic/share/sensor_msgs/cmake/../msg;-Istd_msgs:/opt/ros/kinetic/share/std_msgs/cmake/../msg")

# Find all generators
find_package(gencpp REQUIRED)
find_package(geneus REQUIRED)
find_package(genlisp REQUIRED)
find_package(gennodejs REQUIRED)
find_package(genpy REQUIRED)

add_custom_target(lgsvl_msgs_generate_messages ALL)

# verify that message/service dependencies have not changed since configure



get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox2D.msg" NAME_WE)
add_custom_target(_lgsvl_msgs_generate_messages_check_deps_${_filename}
  COMMAND ${CATKIN_ENV} ${PYTHON_EXECUTABLE} ${GENMSG_CHECK_DEPS_SCRIPT} "lgsvl_msgs" "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox2D.msg" ""
)

get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection3DArray.msg" NAME_WE)
add_custom_target(_lgsvl_msgs_generate_messages_check_deps_${_filename}
  COMMAND ${CATKIN_ENV} ${PYTHON_EXECUTABLE} ${GENMSG_CHECK_DEPS_SCRIPT} "lgsvl_msgs" "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection3DArray.msg" "lgsvl_msgs/BoundingBox3D:geometry_msgs/Twist:std_msgs/Header:geometry_msgs/Quaternion:geometry_msgs/Point:geometry_msgs/Vector3:geometry_msgs/Pose:lgsvl_msgs/Detection3D"
)

get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection2D.msg" NAME_WE)
add_custom_target(_lgsvl_msgs_generate_messages_check_deps_${_filename}
  COMMAND ${CATKIN_ENV} ${PYTHON_EXECUTABLE} ${GENMSG_CHECK_DEPS_SCRIPT} "lgsvl_msgs" "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection2D.msg" "lgsvl_msgs/BoundingBox2D:geometry_msgs/Twist:std_msgs/Header:geometry_msgs/Vector3"
)

get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection3D.msg" NAME_WE)
add_custom_target(_lgsvl_msgs_generate_messages_check_deps_${_filename}
  COMMAND ${CATKIN_ENV} ${PYTHON_EXECUTABLE} ${GENMSG_CHECK_DEPS_SCRIPT} "lgsvl_msgs" "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection3D.msg" "lgsvl_msgs/BoundingBox3D:geometry_msgs/Twist:std_msgs/Header:geometry_msgs/Quaternion:geometry_msgs/Vector3:geometry_msgs/Point:geometry_msgs/Pose"
)

get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection2DArray.msg" NAME_WE)
add_custom_target(_lgsvl_msgs_generate_messages_check_deps_${_filename}
  COMMAND ${CATKIN_ENV} ${PYTHON_EXECUTABLE} ${GENMSG_CHECK_DEPS_SCRIPT} "lgsvl_msgs" "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection2DArray.msg" "geometry_msgs/Vector3:lgsvl_msgs/BoundingBox2D:geometry_msgs/Twist:std_msgs/Header:lgsvl_msgs/Detection2D"
)

get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox3D.msg" NAME_WE)
add_custom_target(_lgsvl_msgs_generate_messages_check_deps_${_filename}
  COMMAND ${CATKIN_ENV} ${PYTHON_EXECUTABLE} ${GENMSG_CHECK_DEPS_SCRIPT} "lgsvl_msgs" "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox3D.msg" "geometry_msgs/Quaternion:geometry_msgs/Pose:geometry_msgs/Point:geometry_msgs/Vector3"
)

#
#  langs = gencpp;geneus;genlisp;gennodejs;genpy
#

### Section generating for lang: gencpp
### Generating Messages
_generate_msg_cpp(lgsvl_msgs
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox2D.msg"
  "${MSG_I_FLAGS}"
  ""
  ${CATKIN_DEVEL_PREFIX}/${gencpp_INSTALL_DIR}/lgsvl_msgs
)
_generate_msg_cpp(lgsvl_msgs
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection3DArray.msg"
  "${MSG_I_FLAGS}"
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox3D.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Twist.msg;/opt/ros/kinetic/share/std_msgs/cmake/../msg/Header.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Quaternion.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Point.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Vector3.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Pose.msg;/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection3D.msg"
  ${CATKIN_DEVEL_PREFIX}/${gencpp_INSTALL_DIR}/lgsvl_msgs
)
_generate_msg_cpp(lgsvl_msgs
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection2D.msg"
  "${MSG_I_FLAGS}"
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox2D.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Twist.msg;/opt/ros/kinetic/share/std_msgs/cmake/../msg/Header.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Vector3.msg"
  ${CATKIN_DEVEL_PREFIX}/${gencpp_INSTALL_DIR}/lgsvl_msgs
)
_generate_msg_cpp(lgsvl_msgs
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection3D.msg"
  "${MSG_I_FLAGS}"
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox3D.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Twist.msg;/opt/ros/kinetic/share/std_msgs/cmake/../msg/Header.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Quaternion.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Vector3.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Point.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Pose.msg"
  ${CATKIN_DEVEL_PREFIX}/${gencpp_INSTALL_DIR}/lgsvl_msgs
)
_generate_msg_cpp(lgsvl_msgs
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection2DArray.msg"
  "${MSG_I_FLAGS}"
  "/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Vector3.msg;/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox2D.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Twist.msg;/opt/ros/kinetic/share/std_msgs/cmake/../msg/Header.msg;/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection2D.msg"
  ${CATKIN_DEVEL_PREFIX}/${gencpp_INSTALL_DIR}/lgsvl_msgs
)
_generate_msg_cpp(lgsvl_msgs
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox3D.msg"
  "${MSG_I_FLAGS}"
  "/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Quaternion.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Pose.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Point.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Vector3.msg"
  ${CATKIN_DEVEL_PREFIX}/${gencpp_INSTALL_DIR}/lgsvl_msgs
)

### Generating Services

### Generating Module File
_generate_module_cpp(lgsvl_msgs
  ${CATKIN_DEVEL_PREFIX}/${gencpp_INSTALL_DIR}/lgsvl_msgs
  "${ALL_GEN_OUTPUT_FILES_cpp}"
)

add_custom_target(lgsvl_msgs_generate_messages_cpp
  DEPENDS ${ALL_GEN_OUTPUT_FILES_cpp}
)
add_dependencies(lgsvl_msgs_generate_messages lgsvl_msgs_generate_messages_cpp)

# add dependencies to all check dependencies targets
get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox2D.msg" NAME_WE)
add_dependencies(lgsvl_msgs_generate_messages_cpp _lgsvl_msgs_generate_messages_check_deps_${_filename})
get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection3DArray.msg" NAME_WE)
add_dependencies(lgsvl_msgs_generate_messages_cpp _lgsvl_msgs_generate_messages_check_deps_${_filename})
get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection2D.msg" NAME_WE)
add_dependencies(lgsvl_msgs_generate_messages_cpp _lgsvl_msgs_generate_messages_check_deps_${_filename})
get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection3D.msg" NAME_WE)
add_dependencies(lgsvl_msgs_generate_messages_cpp _lgsvl_msgs_generate_messages_check_deps_${_filename})
get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection2DArray.msg" NAME_WE)
add_dependencies(lgsvl_msgs_generate_messages_cpp _lgsvl_msgs_generate_messages_check_deps_${_filename})
get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox3D.msg" NAME_WE)
add_dependencies(lgsvl_msgs_generate_messages_cpp _lgsvl_msgs_generate_messages_check_deps_${_filename})

# target for backward compatibility
add_custom_target(lgsvl_msgs_gencpp)
add_dependencies(lgsvl_msgs_gencpp lgsvl_msgs_generate_messages_cpp)

# register target for catkin_package(EXPORTED_TARGETS)
list(APPEND ${PROJECT_NAME}_EXPORTED_TARGETS lgsvl_msgs_generate_messages_cpp)

### Section generating for lang: geneus
### Generating Messages
_generate_msg_eus(lgsvl_msgs
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox2D.msg"
  "${MSG_I_FLAGS}"
  ""
  ${CATKIN_DEVEL_PREFIX}/${geneus_INSTALL_DIR}/lgsvl_msgs
)
_generate_msg_eus(lgsvl_msgs
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection3DArray.msg"
  "${MSG_I_FLAGS}"
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox3D.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Twist.msg;/opt/ros/kinetic/share/std_msgs/cmake/../msg/Header.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Quaternion.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Point.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Vector3.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Pose.msg;/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection3D.msg"
  ${CATKIN_DEVEL_PREFIX}/${geneus_INSTALL_DIR}/lgsvl_msgs
)
_generate_msg_eus(lgsvl_msgs
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection2D.msg"
  "${MSG_I_FLAGS}"
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox2D.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Twist.msg;/opt/ros/kinetic/share/std_msgs/cmake/../msg/Header.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Vector3.msg"
  ${CATKIN_DEVEL_PREFIX}/${geneus_INSTALL_DIR}/lgsvl_msgs
)
_generate_msg_eus(lgsvl_msgs
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection3D.msg"
  "${MSG_I_FLAGS}"
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox3D.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Twist.msg;/opt/ros/kinetic/share/std_msgs/cmake/../msg/Header.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Quaternion.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Vector3.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Point.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Pose.msg"
  ${CATKIN_DEVEL_PREFIX}/${geneus_INSTALL_DIR}/lgsvl_msgs
)
_generate_msg_eus(lgsvl_msgs
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection2DArray.msg"
  "${MSG_I_FLAGS}"
  "/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Vector3.msg;/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox2D.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Twist.msg;/opt/ros/kinetic/share/std_msgs/cmake/../msg/Header.msg;/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection2D.msg"
  ${CATKIN_DEVEL_PREFIX}/${geneus_INSTALL_DIR}/lgsvl_msgs
)
_generate_msg_eus(lgsvl_msgs
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox3D.msg"
  "${MSG_I_FLAGS}"
  "/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Quaternion.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Pose.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Point.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Vector3.msg"
  ${CATKIN_DEVEL_PREFIX}/${geneus_INSTALL_DIR}/lgsvl_msgs
)

### Generating Services

### Generating Module File
_generate_module_eus(lgsvl_msgs
  ${CATKIN_DEVEL_PREFIX}/${geneus_INSTALL_DIR}/lgsvl_msgs
  "${ALL_GEN_OUTPUT_FILES_eus}"
)

add_custom_target(lgsvl_msgs_generate_messages_eus
  DEPENDS ${ALL_GEN_OUTPUT_FILES_eus}
)
add_dependencies(lgsvl_msgs_generate_messages lgsvl_msgs_generate_messages_eus)

# add dependencies to all check dependencies targets
get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox2D.msg" NAME_WE)
add_dependencies(lgsvl_msgs_generate_messages_eus _lgsvl_msgs_generate_messages_check_deps_${_filename})
get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection3DArray.msg" NAME_WE)
add_dependencies(lgsvl_msgs_generate_messages_eus _lgsvl_msgs_generate_messages_check_deps_${_filename})
get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection2D.msg" NAME_WE)
add_dependencies(lgsvl_msgs_generate_messages_eus _lgsvl_msgs_generate_messages_check_deps_${_filename})
get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection3D.msg" NAME_WE)
add_dependencies(lgsvl_msgs_generate_messages_eus _lgsvl_msgs_generate_messages_check_deps_${_filename})
get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection2DArray.msg" NAME_WE)
add_dependencies(lgsvl_msgs_generate_messages_eus _lgsvl_msgs_generate_messages_check_deps_${_filename})
get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox3D.msg" NAME_WE)
add_dependencies(lgsvl_msgs_generate_messages_eus _lgsvl_msgs_generate_messages_check_deps_${_filename})

# target for backward compatibility
add_custom_target(lgsvl_msgs_geneus)
add_dependencies(lgsvl_msgs_geneus lgsvl_msgs_generate_messages_eus)

# register target for catkin_package(EXPORTED_TARGETS)
list(APPEND ${PROJECT_NAME}_EXPORTED_TARGETS lgsvl_msgs_generate_messages_eus)

### Section generating for lang: genlisp
### Generating Messages
_generate_msg_lisp(lgsvl_msgs
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox2D.msg"
  "${MSG_I_FLAGS}"
  ""
  ${CATKIN_DEVEL_PREFIX}/${genlisp_INSTALL_DIR}/lgsvl_msgs
)
_generate_msg_lisp(lgsvl_msgs
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection3DArray.msg"
  "${MSG_I_FLAGS}"
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox3D.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Twist.msg;/opt/ros/kinetic/share/std_msgs/cmake/../msg/Header.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Quaternion.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Point.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Vector3.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Pose.msg;/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection3D.msg"
  ${CATKIN_DEVEL_PREFIX}/${genlisp_INSTALL_DIR}/lgsvl_msgs
)
_generate_msg_lisp(lgsvl_msgs
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection2D.msg"
  "${MSG_I_FLAGS}"
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox2D.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Twist.msg;/opt/ros/kinetic/share/std_msgs/cmake/../msg/Header.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Vector3.msg"
  ${CATKIN_DEVEL_PREFIX}/${genlisp_INSTALL_DIR}/lgsvl_msgs
)
_generate_msg_lisp(lgsvl_msgs
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection3D.msg"
  "${MSG_I_FLAGS}"
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox3D.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Twist.msg;/opt/ros/kinetic/share/std_msgs/cmake/../msg/Header.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Quaternion.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Vector3.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Point.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Pose.msg"
  ${CATKIN_DEVEL_PREFIX}/${genlisp_INSTALL_DIR}/lgsvl_msgs
)
_generate_msg_lisp(lgsvl_msgs
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection2DArray.msg"
  "${MSG_I_FLAGS}"
  "/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Vector3.msg;/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox2D.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Twist.msg;/opt/ros/kinetic/share/std_msgs/cmake/../msg/Header.msg;/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection2D.msg"
  ${CATKIN_DEVEL_PREFIX}/${genlisp_INSTALL_DIR}/lgsvl_msgs
)
_generate_msg_lisp(lgsvl_msgs
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox3D.msg"
  "${MSG_I_FLAGS}"
  "/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Quaternion.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Pose.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Point.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Vector3.msg"
  ${CATKIN_DEVEL_PREFIX}/${genlisp_INSTALL_DIR}/lgsvl_msgs
)

### Generating Services

### Generating Module File
_generate_module_lisp(lgsvl_msgs
  ${CATKIN_DEVEL_PREFIX}/${genlisp_INSTALL_DIR}/lgsvl_msgs
  "${ALL_GEN_OUTPUT_FILES_lisp}"
)

add_custom_target(lgsvl_msgs_generate_messages_lisp
  DEPENDS ${ALL_GEN_OUTPUT_FILES_lisp}
)
add_dependencies(lgsvl_msgs_generate_messages lgsvl_msgs_generate_messages_lisp)

# add dependencies to all check dependencies targets
get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox2D.msg" NAME_WE)
add_dependencies(lgsvl_msgs_generate_messages_lisp _lgsvl_msgs_generate_messages_check_deps_${_filename})
get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection3DArray.msg" NAME_WE)
add_dependencies(lgsvl_msgs_generate_messages_lisp _lgsvl_msgs_generate_messages_check_deps_${_filename})
get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection2D.msg" NAME_WE)
add_dependencies(lgsvl_msgs_generate_messages_lisp _lgsvl_msgs_generate_messages_check_deps_${_filename})
get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection3D.msg" NAME_WE)
add_dependencies(lgsvl_msgs_generate_messages_lisp _lgsvl_msgs_generate_messages_check_deps_${_filename})
get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection2DArray.msg" NAME_WE)
add_dependencies(lgsvl_msgs_generate_messages_lisp _lgsvl_msgs_generate_messages_check_deps_${_filename})
get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox3D.msg" NAME_WE)
add_dependencies(lgsvl_msgs_generate_messages_lisp _lgsvl_msgs_generate_messages_check_deps_${_filename})

# target for backward compatibility
add_custom_target(lgsvl_msgs_genlisp)
add_dependencies(lgsvl_msgs_genlisp lgsvl_msgs_generate_messages_lisp)

# register target for catkin_package(EXPORTED_TARGETS)
list(APPEND ${PROJECT_NAME}_EXPORTED_TARGETS lgsvl_msgs_generate_messages_lisp)

### Section generating for lang: gennodejs
### Generating Messages
_generate_msg_nodejs(lgsvl_msgs
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox2D.msg"
  "${MSG_I_FLAGS}"
  ""
  ${CATKIN_DEVEL_PREFIX}/${gennodejs_INSTALL_DIR}/lgsvl_msgs
)
_generate_msg_nodejs(lgsvl_msgs
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection3DArray.msg"
  "${MSG_I_FLAGS}"
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox3D.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Twist.msg;/opt/ros/kinetic/share/std_msgs/cmake/../msg/Header.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Quaternion.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Point.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Vector3.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Pose.msg;/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection3D.msg"
  ${CATKIN_DEVEL_PREFIX}/${gennodejs_INSTALL_DIR}/lgsvl_msgs
)
_generate_msg_nodejs(lgsvl_msgs
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection2D.msg"
  "${MSG_I_FLAGS}"
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox2D.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Twist.msg;/opt/ros/kinetic/share/std_msgs/cmake/../msg/Header.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Vector3.msg"
  ${CATKIN_DEVEL_PREFIX}/${gennodejs_INSTALL_DIR}/lgsvl_msgs
)
_generate_msg_nodejs(lgsvl_msgs
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection3D.msg"
  "${MSG_I_FLAGS}"
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox3D.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Twist.msg;/opt/ros/kinetic/share/std_msgs/cmake/../msg/Header.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Quaternion.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Vector3.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Point.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Pose.msg"
  ${CATKIN_DEVEL_PREFIX}/${gennodejs_INSTALL_DIR}/lgsvl_msgs
)
_generate_msg_nodejs(lgsvl_msgs
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection2DArray.msg"
  "${MSG_I_FLAGS}"
  "/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Vector3.msg;/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox2D.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Twist.msg;/opt/ros/kinetic/share/std_msgs/cmake/../msg/Header.msg;/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection2D.msg"
  ${CATKIN_DEVEL_PREFIX}/${gennodejs_INSTALL_DIR}/lgsvl_msgs
)
_generate_msg_nodejs(lgsvl_msgs
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox3D.msg"
  "${MSG_I_FLAGS}"
  "/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Quaternion.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Pose.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Point.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Vector3.msg"
  ${CATKIN_DEVEL_PREFIX}/${gennodejs_INSTALL_DIR}/lgsvl_msgs
)

### Generating Services

### Generating Module File
_generate_module_nodejs(lgsvl_msgs
  ${CATKIN_DEVEL_PREFIX}/${gennodejs_INSTALL_DIR}/lgsvl_msgs
  "${ALL_GEN_OUTPUT_FILES_nodejs}"
)

add_custom_target(lgsvl_msgs_generate_messages_nodejs
  DEPENDS ${ALL_GEN_OUTPUT_FILES_nodejs}
)
add_dependencies(lgsvl_msgs_generate_messages lgsvl_msgs_generate_messages_nodejs)

# add dependencies to all check dependencies targets
get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox2D.msg" NAME_WE)
add_dependencies(lgsvl_msgs_generate_messages_nodejs _lgsvl_msgs_generate_messages_check_deps_${_filename})
get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection3DArray.msg" NAME_WE)
add_dependencies(lgsvl_msgs_generate_messages_nodejs _lgsvl_msgs_generate_messages_check_deps_${_filename})
get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection2D.msg" NAME_WE)
add_dependencies(lgsvl_msgs_generate_messages_nodejs _lgsvl_msgs_generate_messages_check_deps_${_filename})
get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection3D.msg" NAME_WE)
add_dependencies(lgsvl_msgs_generate_messages_nodejs _lgsvl_msgs_generate_messages_check_deps_${_filename})
get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection2DArray.msg" NAME_WE)
add_dependencies(lgsvl_msgs_generate_messages_nodejs _lgsvl_msgs_generate_messages_check_deps_${_filename})
get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox3D.msg" NAME_WE)
add_dependencies(lgsvl_msgs_generate_messages_nodejs _lgsvl_msgs_generate_messages_check_deps_${_filename})

# target for backward compatibility
add_custom_target(lgsvl_msgs_gennodejs)
add_dependencies(lgsvl_msgs_gennodejs lgsvl_msgs_generate_messages_nodejs)

# register target for catkin_package(EXPORTED_TARGETS)
list(APPEND ${PROJECT_NAME}_EXPORTED_TARGETS lgsvl_msgs_generate_messages_nodejs)

### Section generating for lang: genpy
### Generating Messages
_generate_msg_py(lgsvl_msgs
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox2D.msg"
  "${MSG_I_FLAGS}"
  ""
  ${CATKIN_DEVEL_PREFIX}/${genpy_INSTALL_DIR}/lgsvl_msgs
)
_generate_msg_py(lgsvl_msgs
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection3DArray.msg"
  "${MSG_I_FLAGS}"
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox3D.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Twist.msg;/opt/ros/kinetic/share/std_msgs/cmake/../msg/Header.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Quaternion.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Point.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Vector3.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Pose.msg;/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection3D.msg"
  ${CATKIN_DEVEL_PREFIX}/${genpy_INSTALL_DIR}/lgsvl_msgs
)
_generate_msg_py(lgsvl_msgs
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection2D.msg"
  "${MSG_I_FLAGS}"
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox2D.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Twist.msg;/opt/ros/kinetic/share/std_msgs/cmake/../msg/Header.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Vector3.msg"
  ${CATKIN_DEVEL_PREFIX}/${genpy_INSTALL_DIR}/lgsvl_msgs
)
_generate_msg_py(lgsvl_msgs
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection3D.msg"
  "${MSG_I_FLAGS}"
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox3D.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Twist.msg;/opt/ros/kinetic/share/std_msgs/cmake/../msg/Header.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Quaternion.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Vector3.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Point.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Pose.msg"
  ${CATKIN_DEVEL_PREFIX}/${genpy_INSTALL_DIR}/lgsvl_msgs
)
_generate_msg_py(lgsvl_msgs
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection2DArray.msg"
  "${MSG_I_FLAGS}"
  "/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Vector3.msg;/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox2D.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Twist.msg;/opt/ros/kinetic/share/std_msgs/cmake/../msg/Header.msg;/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection2D.msg"
  ${CATKIN_DEVEL_PREFIX}/${genpy_INSTALL_DIR}/lgsvl_msgs
)
_generate_msg_py(lgsvl_msgs
  "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox3D.msg"
  "${MSG_I_FLAGS}"
  "/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Quaternion.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Pose.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Point.msg;/opt/ros/kinetic/share/geometry_msgs/cmake/../msg/Vector3.msg"
  ${CATKIN_DEVEL_PREFIX}/${genpy_INSTALL_DIR}/lgsvl_msgs
)

### Generating Services

### Generating Module File
_generate_module_py(lgsvl_msgs
  ${CATKIN_DEVEL_PREFIX}/${genpy_INSTALL_DIR}/lgsvl_msgs
  "${ALL_GEN_OUTPUT_FILES_py}"
)

add_custom_target(lgsvl_msgs_generate_messages_py
  DEPENDS ${ALL_GEN_OUTPUT_FILES_py}
)
add_dependencies(lgsvl_msgs_generate_messages lgsvl_msgs_generate_messages_py)

# add dependencies to all check dependencies targets
get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox2D.msg" NAME_WE)
add_dependencies(lgsvl_msgs_generate_messages_py _lgsvl_msgs_generate_messages_check_deps_${_filename})
get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection3DArray.msg" NAME_WE)
add_dependencies(lgsvl_msgs_generate_messages_py _lgsvl_msgs_generate_messages_check_deps_${_filename})
get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection2D.msg" NAME_WE)
add_dependencies(lgsvl_msgs_generate_messages_py _lgsvl_msgs_generate_messages_check_deps_${_filename})
get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection3D.msg" NAME_WE)
add_dependencies(lgsvl_msgs_generate_messages_py _lgsvl_msgs_generate_messages_check_deps_${_filename})
get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/Detection2DArray.msg" NAME_WE)
add_dependencies(lgsvl_msgs_generate_messages_py _lgsvl_msgs_generate_messages_check_deps_${_filename})
get_filename_component(_filename "/home/ohashi/test_perception/src/msgs/lgsvl_msgs/msg/BoundingBox3D.msg" NAME_WE)
add_dependencies(lgsvl_msgs_generate_messages_py _lgsvl_msgs_generate_messages_check_deps_${_filename})

# target for backward compatibility
add_custom_target(lgsvl_msgs_genpy)
add_dependencies(lgsvl_msgs_genpy lgsvl_msgs_generate_messages_py)

# register target for catkin_package(EXPORTED_TARGETS)
list(APPEND ${PROJECT_NAME}_EXPORTED_TARGETS lgsvl_msgs_generate_messages_py)



if(gencpp_INSTALL_DIR AND EXISTS ${CATKIN_DEVEL_PREFIX}/${gencpp_INSTALL_DIR}/lgsvl_msgs)
  # install generated code
  install(
    DIRECTORY ${CATKIN_DEVEL_PREFIX}/${gencpp_INSTALL_DIR}/lgsvl_msgs
    DESTINATION ${gencpp_INSTALL_DIR}
  )
endif()
if(TARGET geometry_msgs_generate_messages_cpp)
  add_dependencies(lgsvl_msgs_generate_messages_cpp geometry_msgs_generate_messages_cpp)
endif()
if(TARGET sensor_msgs_generate_messages_cpp)
  add_dependencies(lgsvl_msgs_generate_messages_cpp sensor_msgs_generate_messages_cpp)
endif()
if(TARGET std_msgs_generate_messages_cpp)
  add_dependencies(lgsvl_msgs_generate_messages_cpp std_msgs_generate_messages_cpp)
endif()

if(geneus_INSTALL_DIR AND EXISTS ${CATKIN_DEVEL_PREFIX}/${geneus_INSTALL_DIR}/lgsvl_msgs)
  # install generated code
  install(
    DIRECTORY ${CATKIN_DEVEL_PREFIX}/${geneus_INSTALL_DIR}/lgsvl_msgs
    DESTINATION ${geneus_INSTALL_DIR}
  )
endif()
if(TARGET geometry_msgs_generate_messages_eus)
  add_dependencies(lgsvl_msgs_generate_messages_eus geometry_msgs_generate_messages_eus)
endif()
if(TARGET sensor_msgs_generate_messages_eus)
  add_dependencies(lgsvl_msgs_generate_messages_eus sensor_msgs_generate_messages_eus)
endif()
if(TARGET std_msgs_generate_messages_eus)
  add_dependencies(lgsvl_msgs_generate_messages_eus std_msgs_generate_messages_eus)
endif()

if(genlisp_INSTALL_DIR AND EXISTS ${CATKIN_DEVEL_PREFIX}/${genlisp_INSTALL_DIR}/lgsvl_msgs)
  # install generated code
  install(
    DIRECTORY ${CATKIN_DEVEL_PREFIX}/${genlisp_INSTALL_DIR}/lgsvl_msgs
    DESTINATION ${genlisp_INSTALL_DIR}
  )
endif()
if(TARGET geometry_msgs_generate_messages_lisp)
  add_dependencies(lgsvl_msgs_generate_messages_lisp geometry_msgs_generate_messages_lisp)
endif()
if(TARGET sensor_msgs_generate_messages_lisp)
  add_dependencies(lgsvl_msgs_generate_messages_lisp sensor_msgs_generate_messages_lisp)
endif()
if(TARGET std_msgs_generate_messages_lisp)
  add_dependencies(lgsvl_msgs_generate_messages_lisp std_msgs_generate_messages_lisp)
endif()

if(gennodejs_INSTALL_DIR AND EXISTS ${CATKIN_DEVEL_PREFIX}/${gennodejs_INSTALL_DIR}/lgsvl_msgs)
  # install generated code
  install(
    DIRECTORY ${CATKIN_DEVEL_PREFIX}/${gennodejs_INSTALL_DIR}/lgsvl_msgs
    DESTINATION ${gennodejs_INSTALL_DIR}
  )
endif()
if(TARGET geometry_msgs_generate_messages_nodejs)
  add_dependencies(lgsvl_msgs_generate_messages_nodejs geometry_msgs_generate_messages_nodejs)
endif()
if(TARGET sensor_msgs_generate_messages_nodejs)
  add_dependencies(lgsvl_msgs_generate_messages_nodejs sensor_msgs_generate_messages_nodejs)
endif()
if(TARGET std_msgs_generate_messages_nodejs)
  add_dependencies(lgsvl_msgs_generate_messages_nodejs std_msgs_generate_messages_nodejs)
endif()

if(genpy_INSTALL_DIR AND EXISTS ${CATKIN_DEVEL_PREFIX}/${genpy_INSTALL_DIR}/lgsvl_msgs)
  install(CODE "execute_process(COMMAND \"/usr/bin/python\" -m compileall \"${CATKIN_DEVEL_PREFIX}/${genpy_INSTALL_DIR}/lgsvl_msgs\")")
  # install generated code
  install(
    DIRECTORY ${CATKIN_DEVEL_PREFIX}/${genpy_INSTALL_DIR}/lgsvl_msgs
    DESTINATION ${genpy_INSTALL_DIR}
  )
endif()
if(TARGET geometry_msgs_generate_messages_py)
  add_dependencies(lgsvl_msgs_generate_messages_py geometry_msgs_generate_messages_py)
endif()
if(TARGET sensor_msgs_generate_messages_py)
  add_dependencies(lgsvl_msgs_generate_messages_py sensor_msgs_generate_messages_py)
endif()
if(TARGET std_msgs_generate_messages_py)
  add_dependencies(lgsvl_msgs_generate_messages_py std_msgs_generate_messages_py)
endif()
