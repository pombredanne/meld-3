#include <gtest/gtest.h>
#include <functional>

#include "../meld/task.h"

TEST(TaskTestLifo, test_get_and_remove) {
    LifoScheduler m;

    int var;

    m.add_task([&var] () { var = 1; });
    m.add_task([&var] () { var = 2; });
    m.add_task([&var] () { var = 3; });

    m.get_current_task()();
    m.remove_current_task();
    EXPECT_EQ(3, var);
    m.get_current_task()();
    m.remove_current_task();
    EXPECT_EQ(2, var);
    m.get_current_task()();
    m.remove_current_task();
    EXPECT_EQ(1, var);

    EXPECT_ANY_THROW(m.get_current_task()());
}

TEST(TaskTestLifo, test_iteration) {
    LifoScheduler m;

    int var;

    m.add_task([&var] () { var = 1; });
    m.add_task([&var] () { var = 2; });
    m.add_task([&var] () { var = 3; });

    m.iteration();
    EXPECT_EQ(3, var);
    m.iteration();
    EXPECT_EQ(2, var);
    m.iteration();
    EXPECT_EQ(1, var);

    EXPECT_ANY_THROW(m.get_current_task()());
}

TEST(TaskTestFifo, test_get_and_remove) {
    FifoScheduler m;

    int var;

    m.add_task([&var] () { var = 1; });
    m.add_task([&var] () { var = 2; });
    m.add_task([&var] () { var = 3; });

    m.get_current_task()();
    m.remove_current_task();
    EXPECT_EQ(1, var);
    m.get_current_task()();
    m.remove_current_task();
    EXPECT_EQ(2, var);
    m.get_current_task()();
    m.remove_current_task();
    EXPECT_EQ(3, var);

    EXPECT_ANY_THROW(m.get_current_task()());
}

TEST(TaskTestFifo, test_iteration) {
    FifoScheduler m;

    int var;

    m.add_task([&var] () { var = 1; });
    m.add_task([&var] () { var = 2; });
    m.add_task([&var] () { var = 3; });

    m.iteration();
    EXPECT_EQ(1, var);
    m.iteration();
    EXPECT_EQ(2, var);
    m.iteration();
    EXPECT_EQ(3, var);

    EXPECT_ANY_THROW(m.get_current_task()());
}
