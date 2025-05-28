#include <iostream>
#include <Account.h>
#include <Transaction.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

class MockAccount : public Account {
public:
    MockAccount(int id, int balance) : Account(id, balance) {}
    MOCK_METHOD0(Unlock, void());
    MOCK_METHOD0(Lock, void());
    MOCK_CONST_METHOD0(id, int());
    MOCK_METHOD1(ChangeBalance, void(int diff));
    MOCK_CONST_METHOD0(GetBalance, int());
};

class MockTransaction : public Transaction {
public:
    MOCK_METHOD3(Make, bool(Account& from, Account& to, int sum));
    MOCK_METHOD1(set_fee, void(int fee));
    MOCK_METHOD0(fee, int());
};

TEST(AccountTest, BasicAccountOperations) {
    MockAccount acc(1, 1000);
    
    EXPECT_CALL(acc, id()).Times(1).WillOnce(testing::Return(1));
    EXPECT_CALL(acc, GetBalance()).Times(1).WillOnce(testing::Return(1000));
    EXPECT_CALL(acc, Lock()).Times(1);
    EXPECT_CALL(acc, Unlock()).Times(1);
    EXPECT_CALL(acc, ChangeBalance(500)).Times(1);
    
    ASSERT_EQ(acc.id(), 1);
    ASSERT_EQ(acc.GetBalance(), 1000);
    acc.Lock();
    acc.ChangeBalance(500);
    acc.Unlock();
}

TEST(AccountTest, AccountThrowsWhenUnlocked) {
    MockAccount acc(1, 1000);
    
    EXPECT_CALL(acc, ChangeBalance(testing::_))
        .Times(1)
        .WillOnce(testing::Throw(std::runtime_error("Error")));
    
    EXPECT_THROW(acc.ChangeBalance(100), std::runtime_error);
}

TEST(AccountTest, AccountLockUnlockBehavior) {
    Account acc(1, 1000);
    
    acc.Lock();
    acc.ChangeBalance(500);
    EXPECT_EQ(acc.GetBalance(), 1500);
    EXPECT_THROW(acc.Lock(), std::runtime_error);
    acc.Unlock();
}

TEST(TransactionTest, TransactionFeeOperations) {
    MockTransaction trans;
    
    EXPECT_CALL(trans, set_fee(100)).Times(1);
    EXPECT_CALL(trans, fee()).Times(1).WillOnce(testing::Return(100));
    
    trans.set_fee(100);
    ASSERT_EQ(trans.fee(), 100);
}

TEST(TransactionTest, SuccessfulTransaction) {
    MockTransaction trans;
    MockAccount acc1(1, 5000);
    MockAccount acc2(2, 3000);
    
    EXPECT_CALL(acc1, GetBalance()).WillOnce(testing::Return(5000));
    EXPECT_CALL(acc2, GetBalance()).WillOnce(testing::Return(3000));
    EXPECT_CALL(acc1, Lock()).Times(1);
    EXPECT_CALL(acc1, Unlock()).Times(1);
    EXPECT_CALL(acc2, Lock()).Times(1);
    EXPECT_CALL(acc2, Unlock()).Times(1);
    EXPECT_CALL(trans, fee()).WillRepeatedly(testing::Return(100));
    EXPECT_CALL(trans, Make(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(true));
    
    ASSERT_TRUE(trans.Make(acc1, acc2, 1000));
}

TEST(TransactionTest, FailedTransaction) {
    MockTransaction trans;
    MockAccount acc1(1, 100);
    MockAccount acc2(2, 3000);
    
    EXPECT_CALL(acc1, GetBalance()).WillOnce(testing::Return(100));
    EXPECT_CALL(trans, fee()).WillOnce(testing::Return(100));
    EXPECT_CALL(trans, Make(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(false));
    
    ASSERT_FALSE(trans.Make(acc1, acc2, 1000));
}
