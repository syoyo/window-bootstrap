# Cross-platform C++ Window/UI bootstrap 

Simple cross-platform C++ windowing bootstrap with ImGui support.

Mac, Linux and Windows are supported.

## Requirements

* OpenGL 2.x
* premake5(optional. required to build example program)

## Build on Mac and Linux

    $ premake5 gmake
    $ make

## Build on Windows

## Use your project.

Please simply copy necessary files into your project.

## License

`window-bootstrap` is just a composed of existing OSS libraries. `main.cc` is public domain.

* ImGui : The MIT license. Copyright (c) 2014-2015 Omar Cornut and ImGui contributors.
* bt3gui : zlib license. 
* nativefiledialog : The MIT license. (`nativefiledialog/LICENSE`)
* glew : BSD/MIT license.