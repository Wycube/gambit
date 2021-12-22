# Version information file
configure_file(Version.hpp.in ${PROJECT_SOURCE_DIR}/src/common/Version.hpp)

# Logging 
add_library(common_logging Log.cpp)