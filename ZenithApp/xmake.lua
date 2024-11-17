set_version("0.0.1")

set_objectdir("$(projectdir)/Intermediates")
set_targetdir("$(projectdir)/Target")

add_packagedirs("$(projectdir)/ZenithEngine/")

target("ZenithApp")
    set_kind("binary")
    add_defines("ZENITH_ENGINE_DLL_EXPORT=0")

    add_packages("ZenithEngine", "taskflow")
    add_deps("ZenithEngine")

    add_includedirs("$(projectdir)/ZenithEngine/")

    add_files("Main.cpp")
target_end()