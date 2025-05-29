## Laboratory work VI
Данная лабораторная работа посвещена изучению средств пакетирования на примере CPack
```
$ open https://cmake.org/Wiki/CMake:CPackPackageGenerators
```
## Меняем корневой CMakeLists.txt
```
cmake_minimum_required(VERSION 3.10...3.28)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(COVERAGE "Enable code coverage" OFF)
option(BUILD_TESTS "Build tests" ON)

project(TestRunning
    VERSION 0.1.0
    DESCRIPTION "Banking application test framework"
    LANGUAGES CXX
)

add_subdirectory(banking)
if(BUILD_TESTS)
    find_package(Git REQUIRED)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/googletest/CMakeLists.txt")
        message(STATUS "Cloning googletest submodule...")
        execute_process(
            COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE GIT_SUBMOD_RESULT
        )
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "Failed to initialize submodules")
        endif()
    endif()

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    add_subdirectory(googletest)
    add_executable(RunTest test.cpp)

    target_include_directories(RunTest PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/banking
    )

    target_link_libraries(RunTest PRIVATE
        banking
        gtest
        gmock
        gtest_main
    )

    if(COVERAGE)
        target_compile_options(RunTest PRIVATE --coverage)
        target_link_options(RunTest PRIVATE --coverage)
    endif()
    include(GoogleTest)
    gtest_discover_tests(RunTest)
endif()
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "tyrtir")
set(CPACK_PACKAGE_CONTACT "mihailokrivov@yandex.ru")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

if(WIN32)
    set(CPACK_GENERATOR "WIX")
    set(CPACK_WIX_UPGRADE_GUID "D9D9F7B2-3C4D-4E2F-9B1A-0E1D2C3B4A5F") 
    set(CPACK_WIX_PRODUCT_GUID "B8F9F7A1-3C4D-4E2F-9B1A-0E1D2C3B4A5F")
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.rtf")
        file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.rtf" " ")
    endif()
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.rtf")
else()
    set(CPACK_GENERATOR "TGZ;DEB;RPM")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "tyrtir <mihailokrivov@yandex.ru>")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
endif()

include(InstallRequiredSystemLibraries)
include(CPack)
```
## Добавляем CHangeLog.md
```
* Sat May 26 2025 tyrtir <mihailokrivov@yandex.ru> 0.1.0.0
- Initial RPM release
```
## Изменям main.yml
```
name: Build and Test

on:
  push:
    branches: [ master ]
  pull_request:
    branches: [ master ]

jobs:
  linux-build:
    runs-on: ubuntu-latest
    steps:
    - name: Checkout code
      uses: actions/checkout@v4
      with:
        submodules: recursive
        
    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y build-essential cmake dpkg-dev rpm lcov
    
    - name: Configure project
      run: cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
      
    - name: Build project
      run: cmake --build build --config Release --parallel $(nproc)
      
    - name: Run tests
      run: |
        cd build
        ctest --output-on-failure
        
    - name: Generate coverage report
      if: success() && matrix.coverage == 'ON'
      run: |
        lcov --capture --directory . --output-file coverage.info
        lcov --remove coverage.info '/usr/*' --output-file coverage.info
        genhtml coverage.info --output-directory coverage_report
        
    - name: Create packages
      run: |
        cd build
        cpack -G TGZ
        cpack -G DEB
        cpack -G RPM
        
    - name: Upload artifacts
      uses: actions/upload-artifact@v4
      with:
        name: linux-packages
        path: |
          build/*.tar.gz
          build/*.deb
          build/*.rpm
          build/coverage_report/**


  windows-build:
    runs-on: windows-latest
    steps:
    - uses: actions/checkout@v4
    
    - name: Setup WiX
      run: |
        choco install wixtoolset -y
        echo "WIX=C:\Program Files (x86)\WiX Toolset v3.11\bin" >> $GITHUB_ENV
        
    - name: Prepare license
      shell: pwsh
      run: |
        if (!(Test-Path "LICENSE.rtf")) {" " | Out-File -Encoding ASCII "LICENSE.rtf"}
    
    - name: Configure and build
      run: |
        cmake -B build -DCMAKE_BUILD_TYPE=Release
        cmake --build build --config Release
        
    - name: Generate MSI
      run: |
        cd build
        cpack -G WIX -C Release -V --debug
        
    - name: List Windows packages
      shell: pwsh
      run: |
        ls build/*.msi -File | % { $_.FullName }
        if (!(Test-Path "build/*.msi")) { Write-Output "No MSI package found" }
        
    - name: Upload Windows packages
      uses: actions/upload-artifact@v4
      with:
        name: windows-packages
        path: build/*.msi
        if-no-files-found: warn

  macos-build:
    runs-on: macos-latest
    steps:
    - name: Checkout code
      uses: actions/checkout@v4
      with:
        submodules: recursive
    - name: Configure project
      run: cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
    - name: Build project
      run: cmake --build build --config Release --parallel 2
    - name: Run tests
      run: |
        cd build
        ctest --output-on-failure
    - name: Create DMG package
      run: |
        cd build
        cpack -G DragNDrop
        
    - name: List macOS packages
      run: |
        ls -la build/*.dmg || echo "No DMG package found"
        
    - name: Upload macOS packages
      uses: actions/upload-artifact@v4
      with:
        name: macos-packages
        path: build/*.dmg
        if-no-files-found: warn
```
