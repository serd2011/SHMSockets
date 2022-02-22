project "Sockets"
    kind "SharedLib"
    language "C"
    cdialect "C11"
    targetname "SHMSockets"
    warnings "Extra"
    
    targetdir ("%{wks.location}/build/bin/%{cfg.buildcfg}-%{cfg.architecture}/%{prj.name}")
    objdir ("%{wks.location}/build/obj/%{cfg.buildcfg}-%{cfg.architecture}/%{prj.name}")

    files { 
        "**.c",
        "**.h"
    }
    
    includedirs{
        "include",
        "src"
    }
    
    links{
        "pthread",
        "rt"
    }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        runtime "Release"
        optimize "On"
