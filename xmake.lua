add_rules("mode.debug", "mode.release")
add_requires("nlohmann_json")

target("updater")
  set_kind("binary")
  set_languages("cxx17")
  set_warnings("all")
  add_files("main.cpp")
  add_packages("nlohmann_json")
