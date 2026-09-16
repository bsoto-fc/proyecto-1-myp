project('proyecto-1-myp-server', 'c',
  version : '0.1',
  default_options : ['warning_level=3'])

cjson = dependency(
        'libcjson',
        fallback : ['libcjson','libcjson_dep'])

subdir('lib')

exe = executable(
	'server', 
	'src/main.c',
	'src/argparser.c',
	'src/server.c',
        'src/users.c',
	'util/socketutil.c',
        'util/jsonutil.c',
	dependencies: [cjson,set_dep,hashmap_dep],
  install : true)

test('basic', exe)
