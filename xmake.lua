add_requires("boost", {
  system = false,
  configs = {
    python = false,
    fiber = false,
    process = false,
    coroutine = false,
    regex = false,
    graph = false,
    serialization = false,
    random = false,
    wave = false,
    date_time = false,
    locale = false,
    iostreams = false,
    program_options = false,
    test = false,
    chrono = false,
    contract = false,
    graph_parallel = false,
    json = false,
    log = false,
    thread = true,
    filesystem = false,
    math = false,
    mpi = false,
    nowide = false,
    stacktrace = false,
    cmake = true,
    asio = true,
    beast = true,
    system = true,
    header_only = false,
  },
})
function tribute(name, license, url)
  package(name)
  set_license(license)
  set_description(url)
  on_install(function()
  end)
  package_end()
  add_requires(name)
end
tribute("nontype_functional", "BSD-2-Clause", "https://github.com/zhihaoy/nontype_functional")
tribute("concurrentqueue", "BSD-2-Clause", "https://github.com/cameron314/concurrentqueue")
tribute("stack_alloc", "MIT", "https://raw.githubusercontent.com/charles-salvia/charles/master/stack_allocator.hpp")
tribute("system_error2", "Apache2", "https://github.com/ned14/status-code")
tribute("short_alloc", "MIT", "https://howardhinnant.github.io/stack_alloc.html")
tribute("expected", "CC0", "http://tl.tartanllama.xyz/")

function nielib_data()
  add_packages("stack_alloc")
  add_packages("system_error2")
  add_packages("nontype_functional")
  add_packages("concurrentqueue")
  add_packages("short_alloc")
  add_packages("skia-ref_ptr")
  add_packages("expected")
  add_packages("fmt", {public = true})
  add_packages("boost", {public = true})
  add_packages("bzip2", {public = true, links = {"bz2"}})
  add_files("src/*.cpp")
  add_includedirs("include/", {public = true})
  add_headerfiles("include/(**)", {public = true})
  add_defines("NIELIB_FULL", {public = true})
  add_cxflags("-fasynchronous-unwind-tables", {public = true})
  add_ldflags("-fasynchronous-unwind-tables", {public = true})

  add_defines("ASIO_HAS_CO_AWAIT=1", "BOOST_ASIO_HAS_CO_AWAIT=1", {public = true, force = true})
  add_defines("ASIO_HAS_STD_COROUTINE=1", "BOOST_ASIO_HAS_STD_COROUTINE=1", {public = true, force = true})
  add_defines("ASIO_HAS_STD_SYSTEM_ERROR=1", "BOOST_ASIO_HAS_STD_SYSTEM_ERROR=1", {public = true, force = true})
  add_defines("ASIO_RECYCLING_ALLOCATOR_CACHE_SIZE=128", "BOOST_ASIO_RECYCLING_ALLOCATOR_CACHE_SIZE=128",
    {public = true, force = true})
  add_defines("ASIO_DISABLE_SERIAL_PORT", "BOOST_ASIO_DISABLE_SERIAL_PORT", {public = true, force = true})
  add_defines("_ASIO_NO_EXCEPTIONS", "BOOST_ASIO_NO_EXCEPTIONS", {public = true, force = true})
  add_defines("GLM_ENABLE_EXPERIMENTAL", "GLM_FORCE_DEPTH_ZERO_TO_ONE", {public = true, force = true})
  add_defines("JSON_HAS_RANGES=1", {public = true, force = true}) -- https://reviews.llvm.org/D149276
  add_defines("BOOST_DISABLE_CURRENT_LOCATION", {public = true, force = true})
  add_defines("JSON_USE_IMPLICIT_CONVERSIONS=0", {public = true, force = true})
  add_defines("KJ_STD_COMPAT", {public = true, force = true})
  add_defines("CXXOPTS_NO_RTTI", {public = true, force = true})
  add_defines("GLM_FORCE_RADIANS", "GLM_ENABLE_EXPERIMENTAL", "GLM_FORCE_DEPTH_ZERO_TO_ONE",
    {public = true, force = true})

  add_defines("NIE_EXPORT=[[gnu::visibility(\"default\"), gnu::dllimport]]", {interface = true})
  add_defines("NIE_EXPORT=[[gnu::visibility(\"default\"), gnu::dllexport]]", {public = false})

  add_cxflags("-fdata-sections", "-ffunction-sections", {public = true, force = true})
  add_ldflags("-Wl,--gc-sections", {public = true, force = true})
  add_cxflags("-march=native", {public = true, force = true})
  add_ldflags("-march=native", {public = true, force = true})

  add_cxxflags("-Werror=inconsistent-missing-override", {public = true, force = true})
  add_cxflags("-fuse-ld=lld", "-Werror=move", "-Werror=unused-result", "-Werror=return-type", "-Werror=switch",
    "-Werror=delete-non-virtual-dtor", "-Werror=return-type", "-Werror=switch",
    "-Werror=call-to-pure-virtual-from-ctor-dtor", "-Werror=defaulted-function-deleted",
    "-Werror=delete-non-virtual-dtor", "-Werror=abstract-final-class", "-ftemplate-backtrace-limit=0",
    "-Werror=ignored-attributes", "-Werror=unused-value", "-Werror=uninitialized",
    "-Werror=tautological-constant-out-of-range-compare", {public = true, force = true})
  if is_os("linux") then
    add_ldflags("-fuse-ld=lld", {public = true, force = true})
    add_shflags("-fuse-ld=lld", {public = true, force = true})
    add_cxflags("-Wall", {public = true, force = true})
    add_defines("BOOST_ASIO_HAS_IO_URING=1", "ASIO_HAS_IO_URING=1", {public = true, force = true})
    add_cxflags("-fPIC", "-fuse-ld=lld", "-fno-strict-aliasing", "-gdwarf-4", "-rdynamic", {public = true, force = true})
    add_shflags("-fPIC", "-fuse-ld=lld", "-fno-strict-aliasing", "-gdwarf-4", "-rdynamic", {public = true, force = true})
    add_ldflags("-fPIC", "-fuse-ld=lld", "-fno-strict-aliasing", "-gdwarf-4", "-rdynamic", {public = true, force = true})
    add_ldflags("-Wl,-z,stack-size=524288", {public = true, force = true})
    add_ldflags("-static-libstdc++", "-static-libgcc", {public = true, force = true})
    add_shflags("-static-libstdc++", "-static-libgcc", {public = true, force = true})
    add_cxflags("-static-libstdc++", "-static-libgcc", {public = true, force = true})
    if is_mode("debug") then
      --[[add_cxflags("-fstack-protector-all", "-mshstk", {public = true, force = true})
      add_ldflags("-fstack-protector-all", "-mshstk", {public = true, force = true})
      add_shflags("-fstack-protector-all", "-mshstk", {public = true, force = true})
      add_cxflags( "-fsanitize=safe-stack", {public = true, force = true})
      add_ldflags("-fsanitize=safe-stack", {public = true, force = true})
      add_shflags("-fsanitize=safe-stack", {public = true, force = true})]]
    else
    end
    add_cxflags("-fno-rtti", {public = true})
  elseif is_os("windows") then
    set_runtimes("MT")
    add_cxflags("/GR-", {public = true})
    add_ldflags("/opt:ref", {public = true, force = true})
    add_shflags("/opt:ref", {public = true, force = true})
  end

  if is_mode("debug") then
    add_defines("_DEBUG", {public = true})
  else
    add_defines("BOOST_DISABLE_CURRENT_LOCATION", {public = true})
    add_defines("NDEBUG", {public = true})
  end
  after_link("linux", function(target)
    local dfile = target:targetfile() .. "_dist"
    os.cp(target:targetfile(), dfile)
    os.execv("strip", {"-sxX", dfile})
    os.exec("objcopy --localize-hidden --discard-all --discard-locals --strip-all --strip-unneeded \"%s\"", dfile)
    os.execv("strip", {"-sxX", dfile})
  end, {public = true})
  before_link("linux", function(target)
    local paths = table.unique(table.join(target:get_from("linkdirs", "*"), {"/usr/lib/x86_64-linux-gnu/"}))
    target:linker():_tool().nf_link = function(self, lib)
      local has_file = false
      if (lib ~= "dl") and (lib ~= "pthread") and (lib ~= "m") and (lib ~= "c") then
        for _, p in ipairs(paths) do
          has_file = has_file or os.exists(path.join(p, "lib" .. lib .. ".a"))
        end
      end
      if has_file and not lib:find("^:") then
        return "-l:lib" .. lib .. ".a"
      else
        if lib == "util" then
          print(paths)
          print(os.exists(path.join("/usr/lib/x86_64-linux-gnu/", "lib" .. lib .. ".a")))
          raise"NO!!!!"
        end
        return "-l" .. lib
      end
    end
    target:linker():_tool().nf_syslink = target:linker():_tool().nf_link
    target:linkflags()
  end, {public = true})
end
target("nielib")
do
  set_kind("shared")
  nielib_data()
end
target_end()
target("nielib_static")
do
  set_kind("static")
  nielib_data()
end
target_end()
