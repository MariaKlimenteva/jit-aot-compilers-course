#pragma once

#include <cstdint>
#include <iostream>
#include <cassert>

std::ostream& operator<<(std::ostream& os, const Opcode& op) {
    return os << static_cast<int>(op);
}

#define ASSERT_EQ(left, right) \
    t.AssertEqual(left, right, #left " == " #right " at " __FILE__ ":" + std::to_string(__LINE__))
#define ASSERT_NOT_EQ(left, right) \
    t.AssertNotEqual(left, right, #left " != " #right " at " __FILE__ ":" + std::to_string(__LINE__))
