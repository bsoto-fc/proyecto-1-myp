project('proyecto-1-myp-server', 'c',
  version : '0.1',
  default_options : ['warning_level=3'])

exe = executable('server', 'src/main.c','src/argparser.c',
  install : true)

test('basic', exe)
