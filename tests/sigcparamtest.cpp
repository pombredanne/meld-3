#include <gtest/gtest.h>

#include <sigc++/sigc++.h>

int _first = 0;
int _second = 0;

void callback_test(int first, int second) {
    _first = first;
    _second = second;
}

TEST(SigcParamTest, test_param_order) {
     typedef sigc::signal<void, int> test_signal;
     test_signal instance;

     instance.connect(sigc::bind(sigc::ptr_fun(callback_test), 2));
     instance.emit(1);

     EXPECT_EQ(1, _first);
     EXPECT_EQ(2, _second);

}
