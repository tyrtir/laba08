#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <Account.h>
#include <Transaction.h>

class MockAccount : public Account {
public:
    MockAccount(int id, int balance) : Account(id, balance) {}
    MOCK_METHOD(void, Lock, (), (override));
    MOCK_METHOD(void, Unlock, (), (override));
    MOCK_METHOD(int, GetBalance, (), (const, override));
    MOCK_METHOD(void, ChangeBalance, (int diff), (override));
    MOCK_METHOD(int, id, (), (const, override));
};

class MockTransaction : public Transaction {
public:
    MOCK_METHOD(bool, Make, (Account& from, Account& to, int sum), (override));
    MOCK_METHOD(int, fee, (), (const, override));
    MOCK_METHOD(void, set_fee, (int fee), (override));
};

TEST(AccountTest, BasicOperations) {
    MockAccount acc(1, 100);  
    
    EXPECT_CALL(acc, id()).WillOnce(::testing::Return(1));
    EXPECT_CALL(acc, GetBalance()).WillOnce(::testing::Return(100));
    EXPECT_CALL(acc, Lock()).Times(1);
    EXPECT_CALL(acc, ChangeBalance(50)).Times(1);  
    EXPECT_CALL(acc, Unlock()).Times(1);
    
    EXPECT_EQ(acc.id(), 1);
    acc.Lock();
    acc.ChangeBalance(50);
    acc.Unlock();
}

TEST(AccountTest, LockUnlockBehavior) {
    Account acc(1, 150);  
    
    acc.Lock();
    acc.ChangeBalance(30);  
    EXPECT_EQ(acc.GetBalance(), 180);
    acc.Unlock();
}

TEST(TransactionTest, SmallTransaction) {
    MockTransaction trans;
    MockAccount acc1(1, 200);  
    MockAccount acc2(2, 100);
    
    EXPECT_CALL(acc1, Lock()).Times(1);
    EXPECT_CALL(acc1, Unlock()).Times(1);
    EXPECT_CALL(acc2, Lock()).Times(1);
    EXPECT_CALL(acc2, Unlock()).Times(1);
    EXPECT_CALL(trans, fee()).WillRepeatedly(::testing::Return(5));  
    EXPECT_CALL(trans, Make(::testing::_, ::testing::_, 50))  
        .WillOnce(::testing::Return(true));
    
    EXPECT_TRUE(trans.Make(acc1, acc2, 50));
}

TEST(TransactionTest, InsufficientFunds) {
    MockTransaction trans;
    MockAccount acc1(1, 10);  
    MockAccount acc2(2, 0);
    
    EXPECT_CALL(trans, fee()).WillOnce(::testing::Return(5));
    EXPECT_CALL(trans, Make(::testing::_, ::testing::_, 10))
        .WillOnce(::testing::Return(false));
    
    EXPECT_FALSE(trans.Make(acc1, acc2, 10));
}
