# reshade-grabber

An add-on for [ReShade](https://reshade.me/) that acts as a frame grabber extracting both color frames and
auxiliary G-buffer data (at the moment only depth and screen space normals).


## Building the Add-On

This program has two dependencies.

- A modified version of [ReShade](https://github.com/chrismile/reshade) needs to be used together with reshade-grabber.
  Currently, the program will not work with the upstream version of ReShade. When compiling reshade-grabber, the path
  to the repository needs to be set using `ReShade_DIR`.
  
- [libpng](http://www.libpng.org/pub/png/libpng.html) is necessary for saving 16-bit depth images. All other formats
  are supported by stb_image_write bundled with this project. You can compile and install libpng on your own or use,
  e.g., the package version that can be obtained from the package manager [vcpkg](https://github.com/Microsoft/vcpkg).

To install libpng using vcpkg, the following commands can be used to install both the 32-bit and 64-bit version.

```
./vcpkg install libpng
./vcpkg install libpng --triplet=x64-windows
```

The build process was tested on MSVC 2019. To build the add-on, the following commands can be used.

```
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=<path-to-vcpkg>/scripts/buildsystems/vcpkg.cmake -DReShade_DIR=<path-to-reshade> ..
cmake --build . --parallel
```

Optionally, if you want the add-on to be installed to a special directory, `CMAKE_INSTALL_PREFIX` needs to be set when
calling CMake. Then, the following command can be used after compiling the program.

```
cmake --build . --target install
```


## How to Use

As a first step, the modified version of ReShade (https://github.com/chrismile/reshade) needs to be compiled according
to the compilation instructions of the project. Then, ReShade needs to be set up like normally for an application.

In order for add-on to be loaded when launching the program, the following two lines need to be added to the ReShade
`.ini` file storing the configuration of the application.

```
[INSTALL]
AddonPath=<path-to-reshade-grabber-addon>
```

The program will store saved screenshots in the directory `C:\Users\<user-name>\Pictures\reshade-grabber\<app-name>`.
This directory can be changed at runtime using the ImGui user interface. At the moment, it will save color, linearized
depth and screen space normal data computed from the linearized depth.
