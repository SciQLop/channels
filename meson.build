project('channels', 'cpp',default_options : ['cpp_std=c++17'], license: 'GPL3', version: '0.0.1')

catch2_dep = dependency('catch2-with-main', version:'>3.0.0', required : true)
thread_dep = dependency('threads')

channels_dep = declare_dependency(
    include_directories : include_directories('.')
)


foreach test:['test1', 'multithreads', 'new_syntax', 'traits']
    exe = executable(test,'tests/'+test+'/main.cpp',
                    dependencies:[catch2_dep, thread_dep, channels_dep],
                    install: false
                    )
    test(test, exe)
endforeach
