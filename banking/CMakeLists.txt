cmake_minimum_required(VERSION 3.16.3)
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
project(banking)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_library(banking STATIC
Account.cpp
Account.h 
Transaction.cpp
Transaction.h
)
