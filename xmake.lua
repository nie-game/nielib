set_config("vs", "2022")
add_requires("fmt", "boost", "nie-breakpad", "bzip2")
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
package("nie-breakpad")
do
  set_sourcedir(path.join(os.scriptdir(), "breakpad"))
  add_includedirs("include/breakpad")

  on_install(function(package)
    os.vrun("rm -rf \"%s\"/src/third_party/lss", package:sourcedir())
    os.vrun("git clone https://chromium.googlesource.com/linux-syscall-support/ \"%s\"/src/third_party/lss",
      package:sourcedir())
    local configs = {}
    -- if package:debug() then
    -- table.insert(configs, "--enable-debug")
    table.insert(configs, "ac_cv_func_arc4random=no")
    -- end
    package:add("toolchains", "myclang")
    import("package.tools.autoconf").install(package, configs, {
      envs = {
        CXX = "clang++",
        LDFLAGS = "-fuse-ld=lld -gdwarf-4",
        CC = "clang",
        SHFLAGS = "-fuse-ld=lld",
        CXXFLAGS = "-include cstdint -gdwarf-4",
      },
    })
  end)
end
package_end()
target("nielib")
do
  add_packages("stack_alloc")
  add_packages("system_error2")
  add_packages("nontype_functional")
  add_packages("concurrentqueue")
  add_packages("short_alloc")
  add_packages("skia-ref_ptr")
  add_packages("expected")
  add_defines("GLM_FORCE_RADIANS", "GLM_ENABLE_EXPERIMENTAL", "GLM_FORCE_DEPTH_ZERO_TO_ONE", {public = true})
  set_kind("object")
  add_includedirs("include/", {public = true})
  add_cxxflags("-std=c++2c", {public = true})
  add_packages("fmt", {public = true})
  add_packages("boost", {public = true, links = {"boost_atomic-mt", "boost_filesystem-mt"}})
  add_packages("bzip2", {public = true, links = {"bz2"}})
  add_packages("nie-breakpad", {links = "breakpad_client", public = true})
  add_files("src/*.cpp")
  add_defines("NIELIB_FULL", {public = true})

  on_load(function(target)
    target.after_build_breakpad = (function(target)
      if not is_mode("release") then
        -- if false then
        local dumper =
          path.join(assert(target:pkg("nie-breakpad"), "missing breakpad"):installdir(), "bin", "dump_syms")
        local function dump_syms(f)
          local out = os.iorunv(dumper, {"-i", f})
          local platform, architecture, hash, name = out:match("^MODULE (%g+) (%g+) (%g+) (%g+)\n")
          local fn = "symbols/" .. platform .. "/" .. architecture .. "/" .. name .. "/" .. hash .. "/" .. name ..
                       ".sym"
          if not os.exists(fn) then
            print("dumping " .. f .. " into " .. fn)
            os.exec("mkdir -p symbols/" .. platform .. "/" .. architecture .. "/" .. name .. "/" .. hash)
            os.exec("bash -c '%s -d -v %s 2>%s > %s'", dumper, f, fn .. ".err", fn)
          end
        end
        dump_syms(target:targetfile())
        local fl = table.join({"-d", "-r", target:targetfile()} --[[,
        (os.match("/usr/lib/x86_64-linux-gnu/libvulkan*.so*", false)),
        (os.match("/usr/lib/x86_64-linux-gnu/nvidia/current/*.so*", false)),
        (os.match("/usr/lib/x86_64-linux-gnu/libVk*.so*", false)),
        (os.match("/usr/lib/x86_64-linux-gnu/libnvidia-*.so*", false))]] )
        local ldd = os.iorunv("ldd", is_mode("release") and {"-d", "-r", target:targetfile()} or fl)
        for file in ldd:gmatch("=> (.-) %(0x[0-9a-fA-F]*%)") do
          dump_syms(file)
        end
        dump_syms(
          "/opt/nvidia/nsight-graphics-for-linux/nsight-graphics-for-linux-2024.2.0.0/target/linux-desktop-nomad-x64/libNvda.Graphics.Interception.so")
      end
      local t2 = target:targetfile() .. "_dist"
      print(target:targetfile(), t2, os.filesize(t2))
      os.cp(target:targetfile(), t2)
      os.execv("llvm-strip", {"-sx", t2})
      os.exec("llvm-objcopy --localize-hidden --discard-all --strip-all --strip-unneeded \"%s\"", t2)
      os.exec("llvm-strip -sx \"%s\"", t2)
      -- end
    end)
  end)

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
target("nielib_slim")
do
  set_kind("object")
  add_packages("stack_alloc")
  add_packages("boost", {public = true, links = {"boost_atomic-mt", "boost_filesystem-mt"}})
  add_defines("GLM_FORCE_DEPTH_ZERO_TO_ONE", {public = true})
  add_includedirs("include/", {public = true})
  --add_cxxflags("-std=c++2c", {public = true})
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
  add_packages("stack_alloc")
  set_kind("headeronly")
  add_defines("GLM_FORCE_DEPTH_ZERO_TO_ONE", {public = true})
  add_includedirs("include/", {public = true})
  add_cxxflags("-std=c++2c", {public = true})
  -- add_files("src/nie.cpp")
  add_cxflags("-fno-rtti", {public = true})
end
target_end()
