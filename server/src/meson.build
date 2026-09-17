threads_dep = dependency('threads')

server_lib = static_library(
  'server-core',
  ['argparser.c', 'server.c', 'users.c',],
  include_directories: [project_inc, util_inc,],
  dependencies: [
    util_dep,
    hashmap_dep,
    set_dep,
    threads_dep,
  ],
)

server_dep = declare_dependency(
  link_with: server_lib,
  include_directories: [project_inc,util_inc,],
  dependencies: [
    util_dep,
    hashmap_dep,
    set_dep,
  ],
)

server_exe = executable(
  'xerces_server',
  'main.c',
  dependencies: [
    server_dep,
    threads_dep,
  ],
  install: true,
)
