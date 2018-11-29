# Fase

[![Build Status](https://travis-ci.com/denkoken/fase.svg?branch=travis-ci-test)](https://travis-ci.com/denkoken/fase)

Fase is a editor of pipeline for C++ program.

You can edit pipeline with GUI, and run the editing program without a compiling,
of course, using only C++.

After GUI editing, you can obtain native C++ source code for the deploy.

## Testing Enviroments

* MacOS 10.13.4 & clang++
* Ubuntu 14.04.5 LTS + clang++ 7.0 (should be greater than 5)

## Build Examples

* Requirement
  * cmake
  * glfw (only mac)

### Mac

You need glfw3 separately.  
(for example, type `brew install glfw` to install glfw)  

	> mkdir build
	> cd build
	> cmake -DFASE_BUILD_GLFW=OFF ..
	> make

### Linux

	> mkdir build
	> cd build
	> cmake ..
	> make

### Windows


## Screenshot ##
<img src="https://raw.githubusercontent.com/denkoken/fase/master/docs/ss1.jpg">
<img src="https://raw.githubusercontent.com/denkoken/fase/master/docs/ss2.jpg">
<img src="https://raw.githubusercontent.com/denkoken/fase/master/docs/ss3.jpg">
