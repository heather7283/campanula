test_sources = [
  ['collections_test.c', ['../src/collections.c', '../src/xmalloc.c']],
  ['auth_test.c', ['../src/auth.c']],
]


foreach test_arr: test_sources
  test_name = test_arr[0].split('.')[0]
  test_sources = [test_arr[0]] + test_arr[1]
  test_exe = executable(test_name, test_sources,
                        dependencies: dependencies,
                        include_directories: include_dirs)
  test(test_name, test_exe)
endforeach

