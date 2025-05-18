set_version("0.0.1")

set_objectdir("$(projectdir)/Intermediates")
set_targetdir("$(projectdir)/Target")
set_configdir("$(projectdir)/")

add_packagedirs("ThirdParty/")
add_requires("taskflow", "spdlog", "glm", "glfw", "assimp")
add_requires("assimp")

target("ZenithEngine")
    set_kind("shared")
    add_defines("ZENITH_ENGINE_DLL_EXPORT=1", "GLFW_VULKAN_STATIC")

    add_packages("taskflow", "spdlog", "glm", "glfw", "assimp")

    add_headerfiles("**.h", "**.hpp")
    add_files("**.cpp|ThirdParty/vulkan/**.c|ThirdParty/vulkan/**.cpp")

    add_includedirs("$(projectdir)/ZenithEngine/")
    add_includedirs("$(projectdir)/ZenithEngine/ThirdParty/refl-cpp/include")
    add_includedirs("$(projectdir)/ZenithEngine/ThirdParty/vulkan/Include/")
    
    -- vulkan
    add_linkdirs("$(projectdir)/ZenithEngine/ThirdParty/vulkan/Lib")
    add_links("vulkan-1.lib")

    -- Debug
    if is_mode("debug") then
        add_defines("ZENITH_ENABLE_RUNTIME_CHECK=1")
    else
        add_defines("ZENITH_ENABLE_RUNTIME_CHECK=0")
    end
target_end()