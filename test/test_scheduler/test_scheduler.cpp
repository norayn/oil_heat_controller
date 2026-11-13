#include <unity.h>

#include "services/scheduler.h"

void test_periodic_timer_fires_once_per_period() {
    PeriodicTimer timer;
    timer.start(1000, 20);

    TEST_ASSERT_FALSE(timer.due(1019));
    TEST_ASSERT_TRUE(timer.due(1020));
    TEST_ASSERT_FALSE(timer.due(1021));
    TEST_ASSERT_FALSE(timer.due(1039));
    TEST_ASSERT_TRUE(timer.due(1040));
}

void test_missed_ticks_are_not_replayed_as_batch() {
    PeriodicTimer timer;
    timer.start(0, 20);

    TEST_ASSERT_TRUE(timer.due(20));
    // Следующий срок был 40; пришли в 100. Пропущено 3 тика (40,60,80).
    TEST_ASSERT_TRUE(timer.due(100));
    TEST_ASSERT_EQUAL_UINT32(3, timer.missed());

    // Следующий срок назначен от now, а не от 40: 120.
    TEST_ASSERT_FALSE(timer.due(119));
    TEST_ASSERT_TRUE(timer.due(120));
}

void test_timer_is_wrap_safe() {
    PeriodicTimer timer;
    const uint32_t start = 0xFFFFFFF0u;
    timer.start(start, 20);   // next = 0x00000004 (wrap)

    TEST_ASSERT_FALSE(timer.due(0xFFFFFFFAu));
    TEST_ASSERT_TRUE(timer.due(0x00000004u));
    TEST_ASSERT_FALSE(timer.due(0x00000005u));
    TEST_ASSERT_TRUE(timer.due(0x00000018u));
}

void test_stopped_timer_never_fires() {
    PeriodicTimer timer;
    timer.start(0, 10);
    timer.stop();
    TEST_ASSERT_FALSE(timer.due(1000));
}

void test_scheduler_add_and_due() {
    Scheduler scheduler;
    scheduler.begin(0);

    TEST_ASSERT_TRUE(scheduler.addTask(0, 20, 0));
    TEST_ASSERT_TRUE(scheduler.addTask(1, 100, 0));
    TEST_ASSERT_FALSE(scheduler.addTask(Scheduler::kMaxTasks, 10, 0));

    TEST_ASSERT_FALSE(scheduler.due(0, 19));
    TEST_ASSERT_TRUE(scheduler.due(0, 20));
    TEST_ASSERT_FALSE(scheduler.due(1, 99));
    TEST_ASSERT_TRUE(scheduler.due(1, 100));

    TEST_ASSERT_FALSE(scheduler.due(3, 1000));
    TEST_ASSERT_EQUAL_UINT32(0, scheduler.missedTicks(3));
}

int main(int, char**) {
    UNITY_BEGIN();

    RUN_TEST(test_periodic_timer_fires_once_per_period);
    RUN_TEST(test_missed_ticks_are_not_replayed_as_batch);
    RUN_TEST(test_timer_is_wrap_safe);
    RUN_TEST(test_stopped_timer_never_fires);
    RUN_TEST(test_scheduler_add_and_due);

    return UNITY_END();
}
