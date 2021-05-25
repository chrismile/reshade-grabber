include(FindPackageHandleStandardArgs)

find_path(
		RESHADE_SOURCE_DIR
		PATHS ${ReShade_DIR} ../reshade/
		PATH_SUFFIXES source
		NAMES dll_config.hpp)
find_path(
		RESHADE_INCLUDE_DIR
		PATHS ${ReShade_DIR} ../reshade/
		PATH_SUFFIXES include
		NAMES reshade.hpp)
find_path(
		IMGUI_INCLUDE_DIR
		PATHS ${ReShade_DIR}/deps/ ../reshade/deps/
		PATH_SUFFIXES imgui
		NAMES imgui.h)

find_package_handle_standard_args(ReShade DEFAULT_MSG RESHADE_INCLUDE_DIR)
mark_as_advanced(RESHADE_INCLUDE_DIR)
