set_version("0.0.1")

set_objectdir("$(projectdir)/Intermediates")
set_targetdir("$(projectdir)/Target")
set_configdir("$(projectdir)/")

add_requires("taskflow", "spdlog")

target("ZenithEngine")
    set_kind("shared")
    add_defines("ENGINE_DLL_EXPORT=1")

    add_packages("taskflow", "spdlog")

    add_headerfiles("**.h")
    add_files("**.cpp")

    add_includedirs("$(projectdir)/ZenithEngine/")

    -- Debug
    if is_mode("debug") then
        add_defines("ENABLE_ZENITH_RUNTIME_CHECK=1")
    end
target_end()