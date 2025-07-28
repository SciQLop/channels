project('channels', 'cpp',default_options : ['cpp_std=c++23'], license: 'GPL3', version: '0.0.1')

catch2_dep = dependency('catch2-with-main', version:'>3.0.0', required : true)
thread_dep = dependency('threads')

if get_option('compile_time_tests')
    cpp_args = ['-DCHANNELS_COMPILE_TIME_TESTS']
else
    cpp_args = []
endif

channels_dep = declare_dependency(
    include_directories : include_directories('.'),
    compile_args  : cpp_args,
    extra_files : files('channels/concepts.hpp', 'channels/channels.hpp', 'channels/pipelines.hpp', 'channels/tags.hpp', 'channels/traits.hpp'),
)

if (not meson.is_subproject()) or get_option('build_tests_even_if_subproject')
    foreach test:['test1', 'multithreads', 'new_syntax', 'traits', 'channels_concepts']
        exe = executable(test,'tests/'+test+'/main.cpp',
                        dependencies:[catch2_dep, thread_dep, channels_dep],
                        install: false
                        )
        test(test, exe)
    endforeach
endif
