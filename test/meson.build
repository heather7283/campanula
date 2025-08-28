test_sources = [
  ['list.c', ['../src/xmalloc.c']],
  ['vec.c', ['../src/collections/vec.c', '../src/xmalloc.c']],
  ['string.c', ['../src/collections/string.c', '../src/xmalloc.c']],
  ['auth.c', ['../src/auth.c']],
  ['signals.c', [
    '../src/signals.c', '../src/eventloop.c', '../src/log.c',
    '../src/xmalloc.c', '../src/collections/vec.c'
  ]],
  ['xdg.c', ['../src/xdg.c', '../src/log.c', '../src/xmalloc.c']],
]


foreach test_arr: test_sources
  test_name = test_arr[0].split('.')[0]
  test_sources = [test_arr[0]] + test_arr[1]
  test_exe = executable(test_name, test_sources,
                        dependencies: dependencies,
                        include_directories: include_dirs)
  test(test_name, test_exe)
endforeach

