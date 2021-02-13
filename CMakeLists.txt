cmake_minimum_required(VERSION 3.15)
project(convertmdinfo LANGUAGES C)
add_executable(convertmdinfo cmdline.c  errors.c  eval.c  main.c  mdinfo.c  wrappers.c)
