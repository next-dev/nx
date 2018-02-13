-- NX build script

rootdir = path.join(path.getdirectory(_SCRIPT), "..")

filter { "platforms:Win64" }
	system "Windows"
	architecture "x64"


-- Solution
solution "nx"
	language "C++"
	configurations { "Debug", "Release" }
	platforms { "Win64" }
	location "../_build"
    --debugdir "../data"
    characterset "MBCS"

	defines {
		"_CRT_SECURE_NO_WARNINGS",
	}

    --linkoptions "/opt:ref"
    editandcontinue "off"

    rtti "off"
    exceptionhandling "off"

	configuration "Debug"
		defines { "_DEBUG" }
		flags { "FatalWarnings" }
		symbols "on"

	configuration "Release"
		defines { "NDEBUG" }
        flags { "FatalWarnings" }
		optimize "full"

	-- Projects
	project "nx"
		targetdir "../_bin/%{cfg.platform}/%{cfg.buildcfg}/%{prj.name}"
		objdir "../_obj/%{cfg.platform}/%{cfg.buildcfg}/%{prj.name}"
        kind "WindowedApp"
		files {
            "../src/**.h",
			"../src/**.cc",
            "../src/**.c",
            "../src/**.ico",
            "../src/**.rc",
            "../README.md",
            "../etc/keys.txt",
		}
        includedirs {
            "../include",
            "../src",
        }
        links {
            "flac.lib",
            "freetype.lib",
            "ogg.lib",
            "openal32.lib",
            "vorbis.lib",
            "vorbisenc.lib",
            "vorbisfile.lib",
            "opengl32.lib",
            "winmm.lib",
            "gdi32.lib",
        }
        defines {
            "SFML_STATIC",
        }

        libdirs {
            "../lib/msvc",
            "../lib/Exts/msvc"
        }

        configuration "Debug"
            links {
                "sfml-audio-s-d.lib",
                "sfml-graphics-s-d.lib",
                "sfml-main-d.lib",
                "sfml-network-s-d.lib",
                "sfml-system-s-d.lib",
                "sfml-window-s-d.lib",
                "portaudio_static_x64-d.lib",
            }

        configuration "Release"
            links {
                "sfml-audio-s.lib",
                "sfml-graphics-s.lib",
                "sfml-main.lib",
                "sfml-network-s.lib",
                "sfml-system-s.lib",
                "sfml-window-s.lib",
                "portaudio_static_x64.lib",
            }

        -- postbuildcommands {
        --     "copy \"" .. path.translate(path.join(rootdir, "data", "*.*")) .. '" "' ..
        --         path.translate(path.join(rootdir, "_Bin", "%{cfg.platform}", "%{cfg.buildcfg}", "%{prj.name}")) .. '"'
        -- }

		configuration "Win*"
			defines {
				"WIN32",
			}
			flags {
				--"StaticRuntime",
				--"NoMinimalRebuild",
				--"NoIncrementalLink",
			}
            linkoptions {
                "/ignore:4099"
            }
