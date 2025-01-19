#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include <unordered_map>
#include <stack>
#include <random>

extern "C" {
#include "cstack.h"
}

TEST(AllAPITest, BadStackHandler)
{
    std::vector<int> stacks = {-1, 0, 1, 1000000};
    for (auto stack : stacks)
    {
        stack_free(stack);
        EXPECT_EQ(stack_valid_handler(stack), 1);
        EXPECT_EQ(stack_size(stack), 0u);
        const int data_in = 1;
        stack_push(stack, &data_in, sizeof(data_in));
        int data_out = 18;
        EXPECT_EQ(stack_pop(stack, &data_out, sizeof(data_out)), 0u);
        EXPECT_EQ(data_out, 18);
    }
}

TEST(AllocationTests, SingleAllocation)
{
    const hstack_t stack = stack_new();
    EXPECT_EQ(stack_valid_handler(stack), 0);
    EXPECT_EQ(stack_size(stack), 0u);
    stack_free(stack);
    EXPECT_EQ(stack_valid_handler(stack), 1);
}

TEST(AllocationTests, SeveralAllocations)
{
    const size_t count = 10;
    hstack_t stacks[count] = {-1};
    for (size_t i = 0; i < count; ++i)
    {
        stacks[i] = stack_new();
        EXPECT_EQ(stack_valid_handler(stacks[i]), 0);
        EXPECT_EQ(stack_size(stacks[i]), 0u);
    }
    for (size_t i = 0; i < count; ++i)
    {
        stack_free(stacks[i]);
        EXPECT_EQ(stack_valid_handler(stacks[i]), 1);
    }
}

TEST(AllocationTests, SeveralAllocationsWithDoubleFree)
{
    const size_t count = 10;
    hstack_t stacks[count] = {-1};
    for (size_t i = 1; i < count; ++i)
    {
        stacks[i] = stack_new();
        EXPECT_EQ(stack_valid_handler(stacks[i]), 0);
        EXPECT_EQ(stack_size(stacks[i]), 0u);
    }
    for (size_t i = 1; i < count; ++i)
    {
        stack_free(stacks[i-1]);
        EXPECT_EQ(stack_valid_handler(stacks[i-1]), 1);
        stack_free(stacks[i]);
        EXPECT_EQ(stack_valid_handler(stacks[i]), 1);
    }
}

struct ModifyTests : ::testing::Test
{
    void SetUp()
    {
        stack = stack_new();
        ASSERT_EQ(stack_valid_handler(stack), 0);
    }
    void TearDown()
    {
        stack_free(stack);
        ASSERT_EQ(stack_valid_handler(stack), 1);
    }
    hstack_t stack = -1;
};

TEST_F(ModifyTests, PushBadArgs)
{
    stack_push(stack, nullptr, 0u);
    EXPECT_EQ(stack_size(stack), 0u);

    const int data_out = 1;
    stack_push(stack, &data_out, 0u);
    EXPECT_EQ(stack_size(stack), 0u);

    stack_push(stack, nullptr, sizeof(data_out));
    EXPECT_EQ(stack_size(stack), 0u);
}

TEST_F(ModifyTests, PopBadArgs)
{
    const size_t size = 5;
    const int data_in[size] = {1};
    stack_push(stack, &data_in, sizeof(data_in));
    ASSERT_EQ(stack_size(stack), 1u);

    EXPECT_EQ(stack_pop(stack, nullptr, 0u), 0u);
    ASSERT_EQ(stack_size(stack), 1u);

    int data_out[size - 1] = {0};
    EXPECT_EQ(stack_pop(stack, data_out, sizeof(data_out)), 0u);
    EXPECT_THAT(data_out, ::testing::Each(0));
    ASSERT_EQ(stack_size(stack), 1u);

    EXPECT_EQ(stack_pop(stack, nullptr, sizeof(data_in)), 0u);
    EXPECT_THAT(data_out, ::testing::Each(0));
    ASSERT_EQ(stack_size(stack), 1u);
}

TEST_F(ModifyTests, PopFromEmptyStack)
{
    ASSERT_EQ(stack_size(stack), 0u);
    int data_out = 0;
    EXPECT_EQ(stack_pop(stack, &data_out, sizeof(data_out)), 0u);
}

TEST_F(ModifyTests, SinglePushPop)
{
    const int data_in[3] = {0, 1, 2};
    {
        int data_out[3] = {2, 1, 0};
        stack_push(stack, data_in, sizeof(data_in));
        EXPECT_EQ(stack_size(stack), 1u);
        EXPECT_EQ(stack_pop(stack, data_out, sizeof(data_out)), sizeof(data_out));
        EXPECT_EQ(stack_size(stack), 0u);
        EXPECT_THAT(data_out, ::testing::ElementsAre(0, 1, 2));
    }
    {
        int data_out[4] = {2, 1, 0, 18};
        stack_push(stack, data_in, sizeof(data_in));
        EXPECT_EQ(stack_size(stack), 1u);
        EXPECT_EQ(stack_pop(stack, data_out, sizeof(data_out)), sizeof(data_in));
        EXPECT_EQ(stack_size(stack), 0u);
        EXPECT_THAT(data_out, ::testing::ElementsAre(0, 1, 2, 18));
    }
}

TEST_F(ModifyTests, SeveralPushPop)
{
    const size_t size = 3;
    const int data_in[size] = {0, 1, 2};
    int data_out[size] = {0, 1, 2};
    for (size_t i = 0; i < size; ++i)
    {
        stack_push(stack, &data_in[i], sizeof(data_in[i]));
        EXPECT_EQ(stack_size(stack), i + 1u);
    }
    for (size_t i = 0; i < size; ++i)
    {
        EXPECT_EQ(stack_pop(stack, &data_out[i], sizeof(data_out[i])), sizeof(data_out[i]));
        EXPECT_EQ(stack_size(stack), size - 1u - i);
    }
    EXPECT_THAT(data_out, ::testing::ElementsAre(2, 1, 0));
}

TEST(Stress, StressPushPop)
{
    constexpr int num_iterations = 1'000'000;
    std::mt19937 gen{735'675};
    std::uniform_int_distribution dist{1, 1000};

    std::unordered_map<int, std::stack<int>> expected;

    hstack_t stack = stack_new();
    EXPECT_EQ(stack_valid_handler(stack), 0);
    expected[stack] = {};
    for (int i = 0; i < num_iterations; ++i) {
        auto code = dist(gen);
        if (code == 1) {
            stack = stack_new();
            EXPECT_EQ(stack_valid_handler(stack), 0);

            expected[stack] = {};
        } else if (code > 900) {
            if (!expected[stack].empty()) {
                int elem_expected = expected[stack].top();
                expected[stack].pop();

                int data_out = -1;
                EXPECT_EQ(stack_pop(stack, &data_out, sizeof(data_out)), sizeof(data_out));
                EXPECT_EQ(data_out, elem_expected);
                EXPECT_EQ(stack_size(stack), expected[stack].size());
            }
        } else {
            expected[stack].push(i);

            stack_push(stack, &i, sizeof(i));
            EXPECT_EQ(stack_size(stack), expected[stack].size());
        }
    }

    std::vector<int> existing_stack;
    existing_stack.reserve(expected.size());
    for (auto it = expected.begin(), it_end = expected.end(); it != it_end; ++it) {
        existing_stack.push_back(it->first);
    }

    for (auto stack : existing_stack) {
        EXPECT_EQ(stack_valid_handler(stack), 0);
        EXPECT_EQ(stack_size(stack), expected[stack].size());

        while (!expected[stack].empty()) {
            int elem_expected = expected[stack].top();
            expected[stack].pop();

            int data_out = -1;
            EXPECT_EQ(stack_pop(stack, &data_out, sizeof(data_out)), sizeof(data_out));
            EXPECT_EQ(data_out, elem_expected);
        }
        ASSERT_EQ(stack_size(stack), 0);
        EXPECT_EQ(stack_valid_handler(stack), 0);
        stack_free(stack);
        EXPECT_EQ(stack_valid_handler(stack), 1);
    }
}