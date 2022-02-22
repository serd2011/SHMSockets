workspace "SHMSockets"
    configurations { 
        "Debug",
        "Release"
    }
    platforms { "x86", "x86_64" }
    
include "Sockets/"
include "samples/Client-Server/"
