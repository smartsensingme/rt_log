zephyr_include_directories(include)

if(CONFIG_RT_LOG)
  zephyr_library()
  zephyr_library_include_directories(include)
  zephyr_library_sources(src/log.c)
endif()
