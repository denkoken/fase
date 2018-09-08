# Fase

Fase is a editor of pipeline for C++ program.

You can edit pipeline with GUI, and run the editing program without a compiling,
of course, using only C++.

After GUI editing, you can obtain native C++ source code for the deploy.

## Build Examples

* Requirement
  * cmake
  * glfw (only mac)

### Mac

You need glfw3 separately.  
(for example, type `brew install glfw` to install glfw)  

	> mkdir build
	> cd build
	> cmake -DBUILD_GLFW=OFF ..
	> make

### Linux

	> mkdir build
	> cd build
	> cmake ..
	> make

### Windows


## Screenshot ##
<img src="https://raw.githubusercontent.com/denkoken/fase/master/docs/screenshot1.png">
<img src="https://raw.githubusercontent.com/denkoken/fase/master/docs/screenshot2.png">
