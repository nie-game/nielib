-- add_requires("stack_alloc", "system_error2", "nontype_functional", "concurrentqueue", "short_alloc", "skia-ref_ptr",
--  "expected")
package("stack_alloc")
do
  set_license("MIT")
  set_description("https://raw.githubusercontent.com/charles-salvia/charles/master/stack_allocator.hpp")
end
package_end()
package("system_error2")
do
  set_license("Apache2")
  set_description("https://github.com/ned14/status-code")
end
package_end()
package("nontype_functional")
do
  set_license("BSD-2-Clause")
  set_description("https://github.com/zhihaoy/nontype_functional")
end
package_end()
package("concurrentqueue")
do
  set_license("BSD-2-Clause")
  set_description("https://github.com/cameron314/concurrentqueue")
end
package_end()
package("short_alloc")
do
  set_license("MIT")
  set_description("https://howardhinnant.github.io/stack_alloc.html")
end
package_end()
package("skia-ref_ptr")
do
  set_license("BSD-3-Clause")
  set_description("https://github.com/google/skia")
end
package_end()
package("expected")
do
  set_license("CC0")
  set_description("http://tl.tartanllama.xyz/")
end
package_end()
package("rangesnext")
do
  set_license("BSL")
  set_description("https://github.com/cor3ntin/rangesnext/blob/master/include/cor3ntin/rangesnext/enumerate.hpp")
end
package_end()
-- add_requires("libxcb", {system = false, configs = {shared = false}})
-- add_requires("libxdmcp", {system = false, configs = {shared = false}})
-- add_requires("libxau", {system = false, configs = {shared = false}})
target("nielib")
do
  set_default(true)
  add_packages("stack_alloc")
  add_packages("system_error2")
  add_packages("nontype_functional")
  add_packages("concurrentqueue")
  add_packages("short_alloc")
  add_packages("skia-ref_ptr")
  add_packages("expected")
  -- add_packages("libxdmpc", {public = true, links = {"xcb", "Xau", "Xdmcp"}})
  -- add_packages("libxau", {public = true, links = {"xcb", "Xau", "Xdmcp"}})
  -- add_packages("libxcb", {public = true, links = {"xcb", "Xau", "Xdmcp"}})
  add_defines("GLM_FORCE_RADIANS", "GLM_ENABLE_EXPERIMENTAL", "GLM_FORCE_DEPTH_ZERO_TO_ONE", {public = true})
  set_kind("object")
  add_includedirs("include/", {public = true})
  add_headerfiles("include/(**)", {public = true})
  add_cxxflags("-std=c++2c", {public = true})
  add_packages("fmt", {public = true})
  add_packages("boost", {public = true, links = {"boost_atomic-mt", "boost_filesystem-mt"}})
  add_packages("bzip2", {public = true, links = {"bz2"}})
  add_files("src/*.cpp")
  add_defines("NIELIB_FULL", {public = true})
  add_cxflags("-fasynchronous-unwind-tables", {public = true})
  add_ldflags("-fasynchronous-unwind-tables", {public = true})

  if false and is_mode("debug") then
    -- set_policy("build.sanitizer.address", true, {public = true})
    -- set_policy("build.sanitizer.undefined", true, {public = true})
    -- set_policy("build.sanitizer.memory", true, {public = true})
    add_cxxflags("-fstack-protector-all", "-mshstk", "-fsanitize=safe-stack", {public = true})
    add_ldflags("-fstack-protector-all", "-mshstk", "-fsanitize=safe-stack", {public = true})
    add_shflags("-fstack-protector-all", "-mshstk", "-fsanitize=safe-stack", {public = true})
  end

  if is_mode("debug") then
    set_symbols("debug", {public = true})
    add_cxflags("-fdata-sections", "-ffunction-sections", {public = true})
    add_ldflags("-Wl,--gc-sections", {public = true})
    add_cxflags("-march=native", {public = true})
    add_ldflags("-march=native", {public = true})
    set_optimize("faster", {public = true})
  else
    add_cxflags("-fdata-sections", "-ffunction-sections", {public = true})
    add_ldflags("-Wl,--gc-sections", {public = true})
    add_cxflags("-march=native", {public = true})
    add_ldflags("-march=native", {public = true})
    set_optimize("fastest", {public = true})
    add_defines("BOOST_DISABLE_CURRENT_LOCATION", {public = true})
  end
  add_cxflags("-fno-rtti", {public = true})
  add_defines("BOOST_ASIO_NO_EXCEPTIONS", {public = true})
end
target_end()
target("nielib_dist")
do
  set_kind("static")
  add_deps("nielib", {public = true})
  set_default(true)
end
target("nielib_slim")
do
  set_default(false)
  set_kind("object")
  add_packages("stack_alloc")
  add_packages("boost", {public = true, links = {"boost_atomic-mt", "boost_filesystem-mt"}})
  add_defines("GLM_FORCE_DEPTH_ZERO_TO_ONE", {public = true})
  add_includedirs("include/", {public = true})
  -- add_cxxflags("-std=c++2c", {public = true})
  add_files("src/*.cpp")
  set_languages("c++latest")

  if is_mode("debug") then
    set_symbols("debug", {public = true})
    add_cxflags("-fdata-sections", "-ffunction-sections", {public = true})
    add_ldflags("-Wl,--gc-sections", {public = true})
    add_cxflags("-march=native", {public = true})
    add_ldflags("-march=native", {public = true})
    set_optimize("faster", {public = true})
  else
    add_cxflags("-fdata-sections", "-ffunction-sections", {public = true})
    add_ldflags("-Wl,--gc-sections", {public = true})
    add_cxflags("-march=native", {public = true})
    add_ldflags("-march=native", {public = true})
    set_optimize("fastest", {public = true})
    add_defines("BOOST_DISABLE_CURRENT_LOCATION", {public = true})
  end
  add_cxflags("-fno-rtti", {public = true})
end
target_end()
target("nielib_slimmer")
do
  set_default(false)
  add_packages("stack_alloc")
  set_kind("headeronly")
  add_defines("GLM_FORCE_DEPTH_ZERO_TO_ONE", {public = true})
  add_includedirs("include/", {public = true})
  add_cxxflags("-std=c++2c", {public = true})
  -- add_files("src/nie.cpp")
  add_cxflags("-fno-rtti", {public = true})
end
target_end()
