set_inc = include_directories('set/src')

set_lib = static_library(
  'set',
  'set/src/set.c',
  include_directories: set_inc,
)

set_dep = declare_dependency(
  link_with: set_lib,
  include_directories: set_inc,
)
