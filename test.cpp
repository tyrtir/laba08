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
