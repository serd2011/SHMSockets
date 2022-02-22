project "Server"
    kind "ConsoleApp"
    language "C"
    cdialect "C11"
    
    targetdir ("%{wks.location}/build/bin/%{cfg.buildcfg}-%{cfg.architecture}/samples/Client-Server/%{prj.name}")
    objdir ("%{wks.location}/build/obj/%{cfg.buildcfg}-%{cfg.architecture}/samples/Client-Server/%{prj.name}")

    files { 
        "**.c",
        "**.h"
    }
    
    includedirs{
        "%{wks.location}/Sockets/include",
        "src"
    }
    
    links{
        "Sockets"
    }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        runtime "Release"
        optimize "On"
