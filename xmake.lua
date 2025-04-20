set_version("0.0.1")

add_rules("plugin.vsxmake.autoupdate")
add_rules("mode.debug", "mode.release")

set_languages("c++23")
set_allowedplats("windows")
set_allowedarchs("x64")
set_defaultplat("windows")
set_defaultarchs("x64")

-- Warning As Errors
add_cxxflags("/WX")
-- Disable RTTI
add_cxxflags("/GR-")

-- Debug
if is_mode("debug") then
    set_symbols("debug")
    set_optimize("none")
end

-- Release
if is_mode("release") then
    set_symbols("hidden")
    set_strip("all")
    set_optimize("fastest")
end

includes("ZenithEngine/xmake.lua")
includes("ZenithApp/xmake.lua")