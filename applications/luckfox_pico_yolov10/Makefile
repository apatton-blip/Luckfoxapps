APP := luckfox_pico_yolov10
include /home/development/makescripts.mk

build_eval: CMAKE_BUILD_SCRIPT = "cmake -B build -DCOMPILE_EVAL=ON -DCOMPILE_EVAL=ON -DSAMPLE1=ON"
build_eval: build

build_eval2: CMAKE_BUILD_SCRIPT = "cmake -B build -DCOMPILE_EVAL=ON -DCOMPILE_EVAL=ON -DSAMPLE1=OFF"
build_eval2: build