test_sources = [
  ['list.c', ['../src/xmalloc.c']],
  ['array.c', ['../src/collections/array.c', '../src/xmalloc.c']],
  ['string.c', ['../src/collections/string.c', '../src/xmalloc.c']],
  ['auth.c', ['../src/auth.c']],
  ['signals.c', ['../src/signals.c']],
  ['cmd_parse.c', ['../src/commands/parse.c', '../src/xmalloc.c', '../src/collections/array.c']],
]


foreach test_arr: test_sources
  test_name = test_arr[0].split('.')[0]
  test_sources = [test_arr[0]] + test_arr[1]
  test_exe = executable(test_name, test_sources,
                        dependencies: dependencies,
                        include_directories: include_dirs)
  test(test_name, test_exe)
endforeach

