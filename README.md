# Fase

[![Build Status](https://travis-ci.com/denkoken/fase.svg?branch=version2)](https://travis-ci.com/denkoken/fase)

Fase is a editor of pipeline for C++ program.

You can edit pipeline with GUI, and run the editing program without a compiling,
of course, using only C++.

After GUI editing, you can obtain native C++ source code for the deploy.

## Testing Enviroments

* MacOS 10.13.4 & clang++
* Ubuntu 16.04 LTS + clang++ 8 (should be greater than 5)
* Windows10 MSVC2019

## Build Examples

* Requirement
  * cmake
  * glfw (only mac)
  * [NativeFileDialog](https://github.com/mlabbe/nativefiledialog) (optional)

### CMake Options

* `FASE_BUILD_EXAMPLES`
  build examples.

* `FASE_USE_NFD_AT_EXAMPLE`
  use NativeFileDialog, add optinal button.

* `FASE_BUILD_GLFW`
  build glfw, if you use MacOS and GLFW3 is installed, turn OFF.

### Mac

If you installed GLFW3, you need glfw3 separately:  

	> mkdir build
	> cd build
	> cmake -DFASE_BUILD_GLFW=OFF ..
	> make

Else:  

	> mkdir build
	> cd build
	> cmake ..
	> make

### Linux

	> mkdir build
	> cd build
	> cmake ..
	> make

### Windows


## Screenshot ##
<img src="https://raw.githubusercontent.com/denkoken/fase/version2/docs/ss1.jpg">
<img src="https://raw.githubusercontent.com/denkoken/fase/version2/docs/ss2.jpg">
<img src="https://raw.githubusercontent.com/denkoken/fase/version2/docs/ss3.jpg">
<img src="https://raw.githubusercontent.com/denkoken/fase/version2/docs/ss4.jpg">
