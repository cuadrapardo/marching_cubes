workspace "Surface_Reconstruction"
	language "C++"
	--cppdialect "C++17"
	cppdialect "C++20"

	platforms { "x64" }
	configurations { "debug", "release" }

	flags "NoPCH"
	flags "MultiProcessorCompile"

	startproject "cw1"

	debugdir "%{wks.location}"
	objdir "_build_/%{cfg.buildcfg}-%{cfg.platform}-%{cfg.toolset}"
	targetsuffix "-%{cfg.buildcfg}-%{cfg.platform}-%{cfg.toolset}"

	-- Default toolset options
	filter "toolset:gcc or toolset:clang"
		linkoptions { "-pthread" }
		buildoptions { "-march=native", "-Wall", "-pthread" }

	filter "toolset:msc-*"
		defines { "_CRT_SECURE_NO_WARNINGS=1" }
		defines { "_SCL_SECURE_NO_WARNINGS=1" }
		buildoptions { "/utf-8" }

	filter "*"

	-- default libraries
	filter "system:linux"
		links "dl"

	filter "system:windows"

	filter "*"

	-- default outputs
	filter "kind:StaticLib"
		targetdir "lib/"

	filter "kind:ConsoleApp"
		targetdir "bin/"
		targetextension ".exe"

	filter "*"

	--configurations
	filter "debug"
		symbols "On"
		defines { "_DEBUG=1" }

	filter "release"
		optimize "On"
		defines { "NDEBUG=1" }

	filter "*"

-- Third party dependencies
include "third_party"

-- GLSLC helpers
dofile( "util/glslc.lua" )

-- Projects
project "cw1"
	local sources = {
		"render/cw1/**.cpp",
		"render/cw1/**.hpp",
		"render/cw1/**.hxx"
	}

	kind "ConsoleApp"
	location "render/cw1"

	files( sources )

	dependson "cw1-shaders"

	links "labutils"
	links "marching-cubes"
	links "x-volk"
	links "x-stb"
	links "x-glfw"
	links "x-vma"
	links "x-imgui"

	dependson "x-glm"
	dependson "x-rapidobj"

project "cw1-shaders"
	local shaders = {
		"render/cw1/shaders/*.vert",
		"render/cw1/shaders/*.frag",
		"render/cw1/shaders/*.comp",
		"render/cw1/shaders/*.geom",
		"render/cw1/shaders/*.tesc",
		"render/cw1/shaders/*.tese"
	}

	kind "Utility"
	location "render/cw1/shaders"

	files( shaders )

	handle_glsl_files( "-O", "assets/cw1/shaders", {} )

project "labutils"
	local sources = {
		"render/labutils/**.cpp",
		"render/labutils/**.hpp",
		"render/labutils/**.hxx"
	}

	kind "StaticLib"
	location "render/labutils"

	files( sources )


project "marching-cubes"
	local sources = {
		"marching_cubes/**.cpp",
		"marching_cubes/**.hpp",
		"marching_cubes/**.hxx"
	}

	kind "StaticLib"
	location "marching_cubes"

	files( sources )

project "marching-cubes-test"
	local sources = {
		"marching_cubes_test/**.cpp",
		"marching_cubes_test/**.hpp",
		"marching_cubes_test/**.hxx"
	}

	kind "StaticLib"
	location "marching_cubes_test"

	files( sources )



--EOF