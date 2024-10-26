set_version("0.0.1")

set_objectdir("$(projectdir)/Intermediates")
set_targetdir("$(projectdir)/Target")
set_configdir("$(projectdir)/")

add_requires("taskflow")

target("ZenithEngine")
    set_kind("shared")
    add_defines("ENGINE_DLL_EXPORT=1")

    add_packages("taskflow")

    add_headerfiles("*.h")
    add_files("*.cpp")
target_end()