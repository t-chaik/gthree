examples = [
  'cairo',
  'cubes',
  'effects',
  'envmap',
  'gtklogo',
  'interactive',
  'materials',
  'model',
  'morphtargets',
  'multi',
  'normals',
  'performance',
  'points',
  'properties',
  'rendertarget',
  'shader',
  'shadow',
  'skinning',
  'sprites',
  'testanimation',
  'clipping',
  'decals',
  'skeleton',
  'toon',
]

install_subdir('textures',
               install_dir : get_option('datadir') / 'gthree-examples',
               exclude_files: ['snowflake.png'])
install_subdir('models',
               install_dir : get_option('datadir') / 'gthree-examples')

example_resources = gnome.compile_resources('gthree-example-resources',
                                            'examples.gresource.xml',
                                            source_dir: [
                                                # List in order of preference
                                                meson.current_build_dir(),
                                                meson.current_source_dir(),
                                            ],
                                            c_name: 'gthree_example')

libexample_sources = [
  'prop-editor.c',
  'utils.c',
  'orbitcontrols.c',
] + example_resources

example_c_args = []

if cc.get_argument_syntax() == 'msvc'
  example_c_args += '-D_USE_MATH_DEFINES'
endif

if get_option('gtk3')
  example_gtk3_c_args = example_c_args + [ '-DUSE_GTK3' ]

  libexample_gtk3 = static_library(
    'libexample-gtk3',
    libexample_sources,
    c_args: example_gtk3_c_args,
    dependencies: [
      libgthree_gtk3_dep, libm ],
    install: false,
  )

  libexample_gk3_dep = declare_dependency(
    link_whole: libexample_gtk3,
    dependencies: [
      libgthree_gtk3_dep, libm ],
  )

  foreach example : examples
    executable(
      '@0@-gtk3'.format(example),
      '@0@.c'.format(example),
      c_args: example_gtk3_c_args,
      dependencies: [
        libexample_gk3_dep ],
      install: false)
  endforeach
endif

if get_option('gtk4')
  example_gtk4_c_args = example_c_args + [ '-DUSE_GTK4' ]

  libexample_gtk4 = static_library(
    'libexample-gtk4',
    libexample_sources,
    c_args: example_gtk4_c_args,
    dependencies: [
      libgthree_gtk4_dep, libm ],
    install: false,
  )

  libexample_gk4_dep = declare_dependency(
    link_whole: libexample_gtk4,
    dependencies: [
      libgthree_gtk4_dep, libm ],
  )

  foreach example : examples
    executable(
      '@0@-gtk4'.format(example),
      '@0@.c'.format(example),
      c_args: example_gtk4_c_args,
      dependencies: [
        libexample_gk4_dep],
      install: false)
  endforeach
endif
