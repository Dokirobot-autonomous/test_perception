# generated from catkin/cmake/template/pkg.context.pc.in
CATKIN_PACKAGE_PREFIX = ""
PROJECT_PKG_CONFIG_INCLUDE_DIRS = "${prefix}/include".split(';') if "${prefix}/include" != "" else []
PROJECT_CATKIN_DEPENDS = "roscpp;tf;geometry_msgs;visualization_msgs;vector_map_msgs".replace(';', ' ')
PKG_CONFIG_LIBRARIES_WITH_PREFIX = "-lvector_map".split(';') if "-lvector_map" != "" else []
PROJECT_NAME = "vector_map"
PROJECT_SPACE_DIR = "/usr/local"
PROJECT_VERSION = "1.11.0"
