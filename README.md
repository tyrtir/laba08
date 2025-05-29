## Laboratory work VIII
Данная лабораторная работа посвещена изучению систем автоматизации развёртывания и управления приложениями на примере **Docker**

```sh
$ open https://docs.docker.com/get-started/
```
## Создаем Dockerfile
```
FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    lcov \
    g++ \
    && rm -rf /var/lib/apt/lists/*

COPY . /app
WORKDIR /app


RUN git submodule init && git submodule update

RUN mkdir -p build


RUN cd build && \
    cmake -DCOVERAGE=ON -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build . --config Release --parallel $(nproc)


RUN cd build && ./RunTest


RUN cd build && \
    lcov --capture --directory . --output-file coverage.info \
    --rc geninfo_unexecuted_blocks=1 \
    --ignore-errors mismatch,unused && \
    lcov --remove coverage.info \
    '/usr/*' \
    '*/googletest/*' \
    '*/test/*' \
    --output-file coverage.info \
    --ignore-errors unused && \
    genhtml coverage.info --output-directory coverage_report \
    --ignore-errors unmapped,unused

CMD ["bash"]
```
## Добавляем .dockerignore
```
build
*.swp
*.swo
.DS_Store
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
  docker-build:
    runs-on: ubuntu-latest
    steps:
    - name: Checkout code
      uses: actions/checkout@v4
      with:
        submodules: recursive
        
    - name: Build Docker image
      run: docker build -t banking-app .
      
    - name: Run tests in container
      run: |
        docker run --rm banking-app sh -c "cd /app/build && ./RunTest"
        
    - name: Extract coverage report
      run: |
        docker create --name banking-container banking-app
        docker cp banking-container:/app/build/coverage_report ./docker_coverage
        docker rm banking-container
        
    - name: Upload coverage report
      uses: actions/upload-artifact@v4
      with:
        name: docker-coverage-report
        path: ./docker_coverage
        if-no-files-found: error
```
