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

hashmap_inc = include_directories('hashmap/')

hashmap_lib = static_library(
  'hashmap',
  'hashmap/hashmap.c',
  include_directories: hashmap_inc,
)

hashmap_dep = declare_dependency(
  link_with: hashmap_lib,
  include_directories: hashmap_inc,
)
