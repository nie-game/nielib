#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <nie/threadsafe_map.hpp>

TEST_CASE("simple") {
  nie::threadsafe_map<int, int> m;
  CHECK(m.validate());
  m.insert_or_assign(1, 2);
  CHECK(m.validate());
  CHECK(!m.get(0));
  CHECK(!m.get(2));
  CHECK(m.get(1));
  if (m.get(1))
    CHECK(*m.get(1) == 2);
  size_t cnt = 0;
  for (auto [a, b] : m.pairs()) {
    CHECK(a == 1);
    CHECK(b == 2);
    cnt++;
  }
  CHECK(cnt == 1);
}
TEST_CASE("simple") {
  nie::threadsafe_map<int, int> m;
  CHECK(m.validate());
  m.insert_or_assign(3, 3);
  CHECK(m.validate());
  m.insert_or_assign(1, 1);
  m.print_tree();
  CHECK(m.validate());
  m.insert_or_assign(2, 2);
  m.print_tree();
  CHECK(m.validate());
  m.insert_or_assign(4, 4);
  CHECK(m.validate());
  m.print_tree();
  for (auto [a, b] : m.pairs())
    std::println("VAL {} {}", a, b);
  CHECK(!m.get(0));
  CHECK(!m.get(5));
  CHECK(m.get(1));
  if (m.get(1))
    CHECK(*m.get(1) == 1);
  CHECK(m.get(2));
  if (m.get(2))
    CHECK(*m.get(2) == 2);
  CHECK(m.get(3));
  if (m.get(3))
    CHECK(*m.get(3) == 3);
  CHECK(m.get(4));
  if (m.get(4))
    CHECK(*m.get(4) == 4);
  size_t cnt = 0;
  for (auto [a, b] : m.pairs()) {
    if (cnt == 0) {
      CHECK(a == 1);
      CHECK(b == 1);
    } else if (cnt == 1) {
      CHECK(a == 2);
      CHECK(b == 2);
    } else if (cnt == 2) {
      CHECK(a == 3);
      CHECK(b == 3);
    } else if (cnt == 3) {
      CHECK(a == 4);
      CHECK(b == 4);
    }
    cnt++;
  }
  CHECK(cnt == 4);
}
TEST_CASE("simple") {
  nie::threadsafe_map<int, int> m;
  CHECK(m.validate());
  m.insert_or_assign(3, 3);
  CHECK(m.validate());
  m.insert_or_assign(1, 1);
  m.print_tree();
  CHECK(m.validate());
  m.insert_or_assign(2, 2);
  m.print_tree();
  CHECK(m.validate());
  m.insert_or_assign(4, 4);
  m.print_tree();
  CHECK(m.validate());
  m.insert_or_assign(5, 5);
  m.print_tree();
  CHECK(m.validate());
  m.insert_or_assign(6, 6);
  m.print_tree();
  CHECK(m.validate());
  m.insert_or_assign(7, 7);
  m.print_tree();
  CHECK(m.validate());
  m.insert_or_assign(8, 8);
  m.print_tree();
  CHECK(m.validate());
  m.insert_or_assign(9, 9);
  CHECK(m.validate());
  m.print_tree();
  for (auto [a, b] : m.pairs())
    std::println("VAL {} {}", a, b);
  CHECK(!m.get(0));
  CHECK(!m.get(10));
  CHECK(m.get(1));
  if (m.get(1))
    CHECK(*m.get(1) == 1);
  CHECK(m.get(2));
  if (m.get(2))
    CHECK(*m.get(2) == 2);
  CHECK(m.get(3));
  if (m.get(3))
    CHECK(*m.get(3) == 3);
  CHECK(m.get(4));
  if (m.get(4))
    CHECK(*m.get(4) == 4);
  CHECK(m.get(5));
  if (m.get(5))
    CHECK(*m.get(5) == 5);
  CHECK(m.get(6));
  if (m.get(6))
    CHECK(*m.get(6) == 6);
  CHECK(m.get(7));
  if (m.get(7))
    CHECK(*m.get(7) == 7);
  CHECK(m.get(8));
  if (m.get(8))
    CHECK(*m.get(8) == 8);
  CHECK(m.get(9));
  if (m.get(9))
    CHECK(*m.get(9) == 9);
  size_t cnt = 0;
  for (auto [a, b] : m.pairs()) {
    if (cnt == 0) {
      CHECK(a == cnt + 1);
      CHECK(b == cnt + 1);
    }
    cnt++;
  }
  CHECK(cnt == 9);
}

TEST_CASE("simple") {
  nie::threadsafe_map<int, int> m;
  CHECK(m.validate());
  m.insert_or_assign(3, 3);
  CHECK(m.validate());
  m.insert_or_assign(1, 1);
  m.print_tree();
  CHECK(m.validate());
  m.insert_or_assign(2, 2);
  m.print_tree();
  CHECK(m.validate());
  m.insert_or_assign(4, 4);
  m.print_tree();
  CHECK(m.validate());
  m.insert_or_assign(5, 5);
  m.print_tree();
  CHECK(m.validate());
  m.insert_or_assign(8, 8);
  m.print_tree();
  CHECK(m.validate());
  m.insert_or_assign(9, 9);
  m.print_tree();
  CHECK(m.validate());
  m.insert_or_assign(6, 6);
  m.print_tree();
  CHECK(m.validate());
  m.insert_or_assign(7, 7);
  m.print_tree();
  CHECK(m.validate());
  for (auto [a, b] : m.pairs())
    std::println("VAL {} {}", a, b);
  CHECK(!m.get(0));
  CHECK(!m.get(10));
  CHECK(m.get(1));
  if (m.get(1))
    CHECK(*m.get(1) == 1);
  CHECK(m.get(2));
  if (m.get(2))
    CHECK(*m.get(2) == 2);
  CHECK(m.get(3));
  if (m.get(3))
    CHECK(*m.get(3) == 3);
  CHECK(m.get(4));
  if (m.get(4))
    CHECK(*m.get(4) == 4);
  CHECK(m.get(5));
  if (m.get(5))
    CHECK(*m.get(5) == 5);
  CHECK(m.get(6));
  if (m.get(6))
    CHECK(*m.get(6) == 6);
  CHECK(m.get(7));
  if (m.get(7))
    CHECK(*m.get(7) == 7);
  CHECK(m.get(8));
  if (m.get(8))
    CHECK(*m.get(8) == 8);
  CHECK(m.get(9));
  if (m.get(9))
    CHECK(*m.get(9) == 9);
  size_t cnt = 0;
  for (auto [a, b] : m.pairs()) {
    if (cnt == 0) {
      CHECK(a == cnt + 1);
      CHECK(b == cnt + 1);
    }
    cnt++;
  }
  CHECK(cnt == 9);
}
TEST_CASE("fat") {
  size_t count = 1000;
  nie::threadsafe_map<int, int> m;
  CHECK(m.validate());
  for (size_t i = 0; i < count; i++) {
    m.insert_or_assign(i + 1, i + 1);
    CHECK(m.validate());
  }
  CHECK(!m.get(0));
  CHECK(!m.get(count + 1));
  for (size_t i = 0; i < count; i++) {
    CHECK(m.get(i + 1));
    if (m.get(i + 1))
      CHECK(*m.get(i + 1) == i + 1);
  }
  size_t cnt = 0;
  for (auto [a, b] : m.pairs()) {
    if (cnt == 0) {
      CHECK(a == cnt + 1);
      CHECK(b == cnt + 1);
    }
    cnt++;
  }
  CHECK(cnt == count);
}
TEST_CASE("ransom") {
  std::vector<size_t> elements;
  elements.append_range(std::views::iota(0, 1000));
  std::mt19937 mt(0);
  std::shuffle(elements.begin(), elements.end(), mt);
  nie::threadsafe_map<int, int> m;
  CHECK(m.validate());
  for (auto i : elements) {
    m.insert_or_assign(i + 1, i + 1);
    CHECK(m.validate());
  }
  CHECK(!m.get(0));
  CHECK(!m.get(elements.size() + 1));
  for (auto i : elements) {
    CHECK(m.get(i + 1));
    if (m.get(i + 1))
      CHECK(*m.get(i + 1) == i + 1);
  }
  size_t cnt = 0;
  for (auto [a, b] : m.pairs()) {
    if (cnt == 0) {
      CHECK(a == cnt + 1);
      CHECK(b == cnt + 1);
    }
    cnt++;
  }
  CHECK(cnt == elements.size());
}