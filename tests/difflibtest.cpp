#include <gtest/gtest.h>

#include "../meld/difflib/src/difflib.h"

TEST(SequenceMatcherTest, testRatio) {
    difflib::SequenceMatcher<std::string> s("abcd", "bcde");
    EXPECT_FLOAT_EQ(0.75, s.ratio());
}

TEST(SequenceMatcherTest, testGetOpcodes) {
    std::string a = "abcd";
    std::string b = "bcde";
    difflib::SequenceMatcher<std::string> s(a, b);
    difflib::chunk_list_t opcodes = s.get_opcodes();
    EXPECT_EQ(3, opcodes.size());

    EXPECT_EQ("delete", std::get<0>(opcodes[0]));
    EXPECT_EQ(0, std::get<1>(opcodes[0]));
    EXPECT_EQ(1, std::get<2>(opcodes[0]));
    EXPECT_EQ(0, std::get<3>(opcodes[0]));
    EXPECT_EQ(0, std::get<4>(opcodes[0]));

    EXPECT_EQ("equal", std::get<0>(opcodes[1]));
    EXPECT_EQ(1, std::get<1>(opcodes[1]));
    EXPECT_EQ(4, std::get<2>(opcodes[1]));
    EXPECT_EQ(0, std::get<3>(opcodes[1]));
    EXPECT_EQ(3, std::get<4>(opcodes[1]));

    EXPECT_EQ("insert", std::get<0>(opcodes[2]));
    EXPECT_EQ(4, std::get<1>(opcodes[2]));
    EXPECT_EQ(4, std::get<2>(opcodes[2]));
    EXPECT_EQ(3, std::get<3>(opcodes[2]));
    EXPECT_EQ(4, std::get<4>(opcodes[2]));
}
