util_lib = static_library(
  'server-util',
  ['jsonutil.c', 'socketutil.c',],
  include_directories: [util_inc, project_inc,],
  dependencies: [hashmap_dep,set_dep,],
)

cjson_dep = dependency(
        'libcjson',
        fallback : ['libcjson','libcjson_dep'])

util_dep = declare_dependency(
  link_with: util_lib,
  include_directories: [util_inc,project_inc,],
  dependencies: [hashmap_dep, set_dep, cjson_dep,],
)
