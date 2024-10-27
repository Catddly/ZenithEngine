set_version("0.0.1")

set_objectdir("$(projectdir)/Intermediates")
set_targetdir("$(projectdir)/Target")

target("ZenithApp")
    set_kind("binary")
    add_defines("ENGINE_DLL_EXPORT=0")

    add_deps("ZenithEngine")

    add_includedirs("$(projectdir)/ZenithEngine/")

    add_files("Main.cpp")
target_end()