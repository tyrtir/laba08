## Laboratory work V

Данная лабораторная работа посвещена изучению фреймворков для тестирования на примере **GTest**

## Создаем banking/CMakeLists.txt
```
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
```
## Создаем CMakeLists.txt (корневой)
```
cmake_minimum_required(VERSION 3.5)

set(COVERAGE OFF CACHE BOOL "Coverage")
set(CMAKE_CXX_COMPILER "/usr/bin/g++")

project(TestRunning)


add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/googletest" "gtest")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/banking")
add_executable(RunTest
    ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp
)

if(COVERAGE)
    target_compile_options(RunTest PRIVATE --coverage)
    target_link_libraries(RunTest PRIVATE --coverage)
endif()
target_include_directories(RunTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/banking
)
target_link_libraries(RunTest PRIVATE
    gtest
    gtest_main
    gmock_main
    banking
)
```
## Добавляем googletest
```
git submodule add https://github.com/google/googletest.git
```
## Создаем тестовый фалй
test.cpp
```
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <iostream>
#include <Account.h>
#include <Transaction.h>

using ::testing::_;
using ::testing::Expectation;

class MockBankAccount : public Account {
public:
    MockBankAccount(int account_id, int initial_balance) : Account(account_id, initial_balance) {}
    
    MOCK_METHOD(int, GetCurrentBalance, ());
    MOCK_METHOD(void, UpdateBalance, (int amount));
    MOCK_METHOD(int, GetAccountId, (), (const));
    MOCK_METHOD(void, SecureLock, ());
    MOCK_METHOD(void, SecureUnlock, ());
};

class MockMoneyTransfer : public Transaction {
public:
    MOCK_METHOD(int, GetTransactionFee, ());
    MOCK_METHOD(void, SetTransactionFee, (int fee_amount));
    MOCK_METHOD(bool, ExecuteTransfer, (Account& sender, Account& recipient, int transfer_amount));
};

namespace BankAccountTests {
    TEST(BankAccountTest, ShouldThrowWhenAccountNotLocked) {
        Account client_account(123, 500);
        
        EXPECT_THROW(client_account.ChangeBalance(100), std::runtime_error);
        
        client_account.Lock();
        client_account.ChangeBalance(100);
        EXPECT_EQ(client_account.GetBalance(), 600);
        
        EXPECT_THROW(client_account.Lock(), std::runtime_error);
        client_account.Unlock();
    }

    TEST(BankAccountTest, ShouldHandleAccountOperations) {
        MockBankAccount client_account(456, 1000);
        
        Expectation balance_check = EXPECT_CALL(client_account, GetCurrentBalance()).Times(3);
        Expectation lock_op = EXPECT_CALL(client_account, SecureLock()).Times(1).After(balance_check);
        Expectation unlock_op = EXPECT_CALL(client_account, SecureUnlock()).Times(1);
        EXPECT_CALL(client_account, UpdateBalance(_)).Times(2);
        EXPECT_CALL(client_account, GetAccountId()).Times(1);

        client_account.GetCurrentBalance();
        client_account.GetAccountId();
        client_account.SecureUnlock();
        client_account.UpdateBalance(500);
        client_account.GetCurrentBalance();
        client_account.UpdateBalance(-200);
        client_account.GetCurrentBalance();
        client_account.SecureLock();
    }
}

namespace MoneyTransferTests {
    TEST(MoneyTransferTest, ShouldProcessFinancialTransaction) {
        MockMoneyTransfer money_transfer;
        MockBankAccount sender_account(1, 500);
        MockBankAccount receiver_account(2, 300);
        MockBankAccount company_account(3, 10000);
        MockBankAccount charity_account(4, 2000);

        EXPECT_CALL(money_transfer, GetTransactionFee()).Times(1);
        EXPECT_CALL(money_transfer, SetTransactionFee(_)).Times(1);
        EXPECT_CALL(money_transfer, ExecuteTransfer(_, _, _)).Times(2);
        EXPECT_CALL(sender_account, GetCurrentBalance()).Times(1);
        EXPECT_CALL(receiver_account, GetCurrentBalance()).Times(1);

        money_transfer.SetTransactionFee(150);
        money_transfer.ExecuteTransfer(sender_account, receiver_account, 2000);
        money_transfer.GetTransactionFee();
        sender_account.GetCurrentBalance();
        receiver_account.GetCurrentBalance();
        money_transfer.ExecuteTransfer(company_account, charity_account, 5000);
    }
}
```

## .github/workflows/main.yml
```
name: CMake with Coverage

on:
  push:
    branches: [master]
  pull_request:
    branches: [master]

env:
  BUILD_TYPE: Release
  COMPILER: g++-9
  COVERAGE_TOOL: lcov

jobs:
  build-and-test:
    name: Build and Test with Coverage
    runs-on: ubuntu-24.04
    
    steps:
    - name: Checkout repository
      uses: actions/checkout@v4
      with:
        submodules: recursive
        
    - name: Install dependencies
      run: |
        sudo apt-get update -qq
        sudo apt-get install -y \
          ${{env.COMPILER}} \
          ${{env.COVERAGE_TOOL}} \
          cmake \
          make
        
    - name: Create and prepare build directory
      run: |
        mkdir -p _build
        cd _build
        
    - name: Configure CMake
      working-directory: _build
      run: |
        cmake \
          -DCOVERAGE=ON \
          -DCMAKE_BUILD_TYPE=${{env.BUILD_TYPE}} \
          -DCMAKE_CXX_COMPILER=${{env.COMPILER}} \
          ..
        
    - name: Build project
      working-directory: _build
      run: cmake --build . --config ${{env.BUILD_TYPE}} --verbose
      
    - name: Execute tests
      working-directory: _build
      run: ./RunTest
      
    - name: Generate coverage data
      working-directory: _build
      run: |
        lcov --capture \
          --directory . \
          --output-file coverage.info \
          --ignore-errors mismatch,unused \
          --rc branch_coverage=1 \
          --rc geninfo_unexecuted_blocks=1
        
    - name: Filter coverage results
      working-directory: _build
      run: |
        lcov --remove coverage.info \
          '/usr/*' \
          '*/external/*' \
          '*/test/*' \
          '*/gtest/*' \
          '*/gmock/*' \
          --output-file filtered.info \
          --ignore-errors unused
        
    - name: Generate HTML report
      working-directory: _build
      run: |
        genhtml --branch-coverage \
          --title "Test Coverage Report" \
          --legend \
          --output-directory coverage \
          filtered.info \
          --ignore-errors unmapped,unused
        
    - name: Upload coverage report
      uses: actions/upload-artifact@v4
      with:
        name: coverage-report
        path: _build/coverage
        retention-days: 7
```
