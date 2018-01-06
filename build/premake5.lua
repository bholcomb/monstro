solution "Monstro"
   location("../")
   configurations { "Debug", "Release" }
   platforms{"x64"}
   startproject "testUi"

   includedirs{"../include", "../3rdParty/include"}
   libdirs{"../lib", "../3rdParty/lib"}

   
  configuration { "Debug" }
    defines { "DEBUG", "TRACE"}
    symbols "On"
    optimize "Off"
 
  configuration { "Release" }
    optimize "Speed"
	
   
project "ui"
	kind "SharedLib"
	language "C++"
	location "ui"
	targetdir("../lib")
   defines("UI_EXPORTS")
	files{"../include/ui/*.h", "../libsrc/ui/*.cxx"}
	vpaths { ["Headers"] = "../include/ui/*.h", ["Source"] = "../libsrc/ui/*.cxx" }
	
project "testUi"
	kind "ConsoleApp"
	language "C++"
	location "testUi"
	targetdir("../bin")
	files{"../libsrc/testUi/*.cxx"}
	links {"ui", "glfw3", "OpenGL32"}
	
project "uiEditor"
	kind "ConsoleApp"
	language "C++"
	location "uiEditor"
	targetdir("../bin")
	files{"../libsrc/uiEditor/*.cxx"}
	links {"ui", "glfw3", "OpenGL32"}