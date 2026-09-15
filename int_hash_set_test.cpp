#include "int_hash_set.hpp"

#include <catch2/catch_test_macros.hpp>
#include <sstream>


// returns true if `value` is stored in the bucket it belongs to
static bool is_in_correct_bucket(const ds::IntHashSet& set, int64_t value) {
  for (const auto* node = set.bucket(set.bucket_index(value)); node != nullptr;
       node = node->next) {
    if (node->data == value) {
      return true;
    }
  }
  return false;
}


TEST_CASE("constructors", "[constructors]") {

  SECTION("default_constructor_creates_8_buckets") {
    ds::IntHashSet set;
    CHECK(set.bucket_count() == 8);
    CHECK(set.size() == 0);
    CHECK(set.empty());
    CHECK(set.load_factor() == 0.0);
  }

  SECTION("custom_number_of_buckets") {
    ds::IntHashSet set{3};
    CHECK(set.bucket_count() == 3);
    CHECK(set.empty());
  }

  SECTION("all_buckets_of_a_new_set_are_empty") {
    ds::IntHashSet set;
    for (uint64_t i(0); i < set.bucket_count(); ++i) {
      CHECK(set.bucket(i) == nullptr);
    }
  }

  SECTION("zero_buckets_are_not_allowed") {
    CHECK_THROWS(ds::IntHashSet{0});
  }

  SECTION("bucket_throws_if_index_is_out_of_bounds") {
    ds::IntHashSet set{4};
    CHECK_NOTHROW(set.bucket(3));
    CHECK_THROWS(set.bucket(4));
    CHECK_THROWS(set.bucket(100));
  }
}


TEST_CASE("hash_function", "[hash]") {
  ds::IntHashSet set{10};

  SECTION("positive_values") {
    CHECK(set.bucket_index(0) == 0);
    CHECK(set.bucket_index(7) == 7);
    CHECK(set.bucket_index(10) == 0);
    CHECK(set.bucket_index(13) == 3);
    CHECK(set.bucket_index(12345) == 5);
  }

  SECTION("negative_values") {
    CHECK(set.bucket_index(-1) == 9);
    CHECK(set.bucket_index(-10) == 0);
    CHECK(set.bucket_index(-13) == 7);
    CHECK(set.bucket_index(-12345) == 5);
  }

  SECTION("index_depends_on_the_number_of_buckets") {
    ds::IntHashSet eight;
    CHECK(eight.bucket_index(13) == 5);
    CHECK(eight.bucket_index(-1) == 7);
    ds::IntHashSet three{3};
    CHECK(three.bucket_index(13) == 1);
    CHECK(three.bucket_index(-1) == 2);
  }
}


TEST_CASE("insert_values", "[insert]") {
  ds::IntHashSet set{4};

  SECTION("insert_into_empty_set") {
    CHECK(set.insert(6));
    CHECK(set.size() == 1);
    CHECK(not set.empty());
    CHECK(set.bucket(2)->data == 6);
    CHECK(set.bucket(2)->next == nullptr);
  }

  SECTION("values_are_stored_in_their_bucket") {
    set.insert(4);
    set.insert(1);
    set.insert(-2);
    CHECK(set.bucket(0)->data == 4);
    CHECK(set.bucket(1)->data == 1);
    CHECK(set.bucket(2)->data == -2);
    CHECK(set.bucket(3) == nullptr);
  }

  SECTION("new_values_are_added_to_the_front_of_the_bucket") {
    set.insert(1);
    set.insert(5);
    set.insert(9);
    CHECK(set.bucket(1)->data == 9);
    CHECK(set.bucket(1)->next->data == 5);
    CHECK(set.bucket(1)->next->next->data == 1);
    CHECK(set.bucket(1)->next->next->next == nullptr);
    CHECK(set.size() == 3);
  }

  SECTION("duplicates_are_rejected") {
    CHECK(set.insert(1));
    CHECK(set.insert(5));
    CHECK(not set.insert(1));
    CHECK(not set.insert(5));
    CHECK(set.size() == 2);
    CHECK(set.bucket(1)->data == 5);
    CHECK(set.bucket(1)->next->data == 1);
    CHECK(set.bucket(1)->next->next == nullptr);
  }
}


TEST_CASE("contains_values", "[contains]") {
  ds::IntHashSet set;

  SECTION("empty_set_contains_nothing") {
    CHECK(not set.contains(0));
    CHECK(not set.contains(1));
  }

  SECTION("find_stored_and_missing_values") {
    set.insert(3);
    set.insert(11);
    set.insert(-5);
    CHECK(set.contains(3));
    CHECK(set.contains(11));
    CHECK(set.contains(-5));
    CHECK(not set.contains(19));  // same bucket as 3, 11 and -5
    CHECK(not set.contains(5));
    CHECK(not set.contains(0));
  }
}


TEST_CASE("remove_values", "[remove]") {
  ds::IntHashSet set{4};
  set.insert(1);
  set.insert(5);
  set.insert(9);  // bucket 1: 9 -> 5 -> 1

  SECTION("remove_first_element_of_a_bucket") {
    CHECK(set.remove(9));
    CHECK(set.size() == 2);
    CHECK(set.bucket(1)->data == 5);
    CHECK(set.bucket(1)->next->data == 1);
  }

  SECTION("remove_middle_element_of_a_bucket") {
    CHECK(set.remove(5));
    CHECK(set.bucket(1)->data == 9);
    CHECK(set.bucket(1)->next->data == 1);
    CHECK(set.bucket(1)->next->next == nullptr);
  }

  SECTION("remove_last_element_of_a_bucket") {
    CHECK(set.remove(1));
    CHECK(set.bucket(1)->next->data == 5);
    CHECK(set.bucket(1)->next->next == nullptr);
  }

  SECTION("remove_all_values") {
    CHECK(set.remove(5));
    CHECK(set.remove(1));
    CHECK(set.remove(9));
    CHECK(set.empty());
    CHECK(set.bucket(1) == nullptr);
  }

  SECTION("remove_missing_value") {
    CHECK(not set.remove(13));  // same bucket, but not stored
    CHECK(not set.remove(2));   // empty bucket
    CHECK(not set.remove(-3));
    CHECK(set.size() == 3);
  }

  SECTION("remove_from_empty_set") {
    ds::IntHashSet empty;
    CHECK(not empty.remove(1));
    CHECK(empty.empty());
  }

  SECTION("removed_values_can_be_inserted_again") {
    set.remove(5);
    CHECK(not set.contains(5));
    CHECK(set.insert(5));
    CHECK(set.contains(5));
    CHECK(set.bucket(1)->data == 5);
  }
}


TEST_CASE("rehashing", "[rehash]") {
  ds::IntHashSet set;

  SECTION("load_factor") {
    set.insert(1);
    CHECK(set.load_factor() == 0.125);
    set.insert(2);
    set.insert(3);
    CHECK(set.load_factor() == 0.375);
  }

  SECTION("no_rehash_up_to_a_load_factor_of_0.75") {
    for (int64_t i(0); i < 6; ++i) {
      set.insert(i);
    }
    CHECK(set.bucket_count() == 8);
    CHECK(set.load_factor() == 0.75);
  }

  SECTION("7th_value_doubles_the_number_of_buckets") {
    for (int64_t i(0); i < 7; ++i) {
      set.insert(i);
    }
    CHECK(set.bucket_count() == 16);
    CHECK(set.size() == 7);
    CHECK(set.load_factor() == 0.4375);
  }

  SECTION("all_values_are_moved_into_the_correct_bucket") {
    for (int64_t i(-20); i < 20; ++i) {
      set.insert(i * 7);
    }
    CHECK(set.size() == 40);
    CHECK(set.bucket_count() == 64);
    for (int64_t i(-20); i < 20; ++i) {
      CHECK(is_in_correct_bucket(set, i * 7));
    }
  }

  SECTION("duplicates_do_not_trigger_a_rehash") {
    for (int64_t i(0); i < 6; ++i) {
      set.insert(i);
    }
    set.insert(5);
    set.insert(0);
    CHECK(set.bucket_count() == 8);
    CHECK(set.size() == 6);
  }

  SECTION("removing_values_does_not_reduce_the_number_of_buckets") {
    for (int64_t i(0); i < 7; ++i) {
      set.insert(i);
    }
    for (int64_t i(0); i < 7; ++i) {
      set.remove(i);
    }
    CHECK(set.empty());
    CHECK(set.bucket_count() == 16);
  }

  SECTION("rehash_with_custom_number_of_buckets") {
    ds::IntHashSet small{1};
    small.insert(10);
    CHECK(small.bucket_count() == 2);
    small.insert(11);
    CHECK(small.bucket_count() == 4);
    small.insert(12);
    CHECK(small.bucket_count() == 4);
    CHECK(is_in_correct_bucket(small, 10));
    CHECK(is_in_correct_bucket(small, 11));
    CHECK(is_in_correct_bucket(small, 12));
  }
}


TEST_CASE("clear_the_set", "[clear]") {
  ds::IntHashSet set;
  for (int64_t i(0); i < 10; ++i) {
    set.insert(i);
  }

  SECTION("set_is_empty_after_clear") {
    set.clear();
    CHECK(set.empty());
    CHECK(set.size() == 0);
    CHECK(set.bucket_count() == 16);
    CHECK(not set.contains(3));
    for (uint64_t i(0); i < set.bucket_count(); ++i) {
      CHECK(set.bucket(i) == nullptr);
    }
  }

  SECTION("set_can_be_filled_again_after_clear") {
    set.clear();
    CHECK(set.insert(3));
    CHECK(set.size() == 1);
    CHECK(set.contains(3));
  }
}


TEST_CASE("copy", "[copy]") {
  ds::IntHashSet set{4};
  set.insert(1);
  set.insert(5);
  set.insert(2);

  SECTION("copy_constructor") {
    ds::IntHashSet other(set);
    CHECK(other.bucket_count() == 4);
    CHECK(other.size() == 3);
    CHECK(other.contains(1));
    CHECK(other.contains(5));
    CHECK(other.contains(2));
    CHECK(other.bucket(1) != set.bucket(1));
    CHECK(is_in_correct_bucket(other, 1));
    CHECK(is_in_correct_bucket(other, 5));
    CHECK(is_in_correct_bucket(other, 2));
  }

  SECTION("copy_constructor_creates_an_independent_copy") {
    ds::IntHashSet other(set);
    other.remove(1);
    other.insert(3);
    CHECK(set.contains(1));
    CHECK(not set.contains(3));
    CHECK(set.size() == 3);
    set.remove(2);
    CHECK(other.contains(2));
  }

  SECTION("copy_constructor_of_empty_set") {
    ds::IntHashSet empty{2};
    ds::IntHashSet other(empty);
    CHECK(other.empty());
    CHECK(other.bucket_count() == 2);
    CHECK(other.bucket(0) == nullptr);
    CHECK(other.bucket(1) == nullptr);
  }

  SECTION("copy_assignment") {
    ds::IntHashSet other;
    other.insert(100);
    other = set;
    CHECK(other.bucket_count() == 4);
    CHECK(other.size() == 3);
    CHECK(not other.contains(100));
    CHECK(other.contains(5));
    CHECK(other.bucket(1) != set.bucket(1));
    other.clear();
    CHECK(set.size() == 3);
  }

  SECTION("copy_assignment_to_a_set_with_more_buckets") {
    ds::IntHashSet other{64};
    other = set;
    CHECK(other.bucket_count() == 4);
    CHECK(is_in_correct_bucket(other, 5));
    other.insert(3);  // 4 values in 4 buckets: rehash to 8 buckets
    CHECK(other.bucket_count() == 8);
  }

  SECTION("self_assignment") {
    ds::IntHashSet& same = set;
    set = same;
    CHECK(set.size() == 3);
    CHECK(set.contains(1));
    CHECK(set.contains(5));
    CHECK(set.contains(2));
  }

  SECTION("chained_assignment") {
    ds::IntHashSet a;
    ds::IntHashSet b;
    a = b = set;
    CHECK(a.size() == 3);
    CHECK(b.size() == 3);
    CHECK(a.bucket(1) != b.bucket(1));
  }
}


TEST_CASE("non_member_functions", "[ostream]") {
  ds::IntHashSet set{4};

  SECTION("print_empty_set") {
    std::stringstream ss;
    ss << set;
    CHECK(ss.str() == "hs[]");
  }

  SECTION("print_example_from_the_readme") {
    set.insert(1);
    set.insert(5);
    set.insert(2);
    std::stringstream ss;
    ss << set;
    CHECK(ss.str() == "hs[5, 1, 2]");
  }

  SECTION("print_bucket_by_bucket") {
    set.insert(3);
    set.insert(-4);
    set.insert(6);
    std::stringstream ss;
    ss << set;
    CHECK(ss.str() == "hs[-4, 6, 3]");
  }

  SECTION("operator_returns_the_stream") {
    set.insert(7);
    std::stringstream ss;
    ss << set << " " << set;
    CHECK(ss.str() == "hs[7] hs[7]");
  }
}


TEST_CASE("const_correctness", "[const]") {
  ds::IntHashSet set{4};
  set.insert(1);
  set.insert(5);
  const ds::IntHashSet& c = set;

  SECTION("read_only_member_functions_are_const") {
    CHECK(c.bucket(1)->data == 5);
    CHECK(c.bucket_count() == 4);
    CHECK(c.bucket_index(-1) == 3);
    CHECK(c.size() == 2);
    CHECK(not c.empty());
    CHECK(c.load_factor() == 0.5);
    CHECK(c.contains(1));
  }

  SECTION("copy_from_const_set") {
    ds::IntHashSet other(c);
    CHECK(other.size() == 2);
  }
}


TEST_CASE("performance", "[performance]") {
  ds::IntHashSet set;
  const int64_t n = 1000000;
  for (int64_t i(0); i < n; ++i) {
    set.insert(i * 3 - n);
  }

  SECTION("insert_1,000,000_values") {
    CHECK(set.size() == 1000000);
    CHECK(set.load_factor() <= 0.75);
  }

  SECTION("find_and_remove_1,000,000_values") {
    uint64_t found = 0;
    for (int64_t i(0); i < n; ++i) {
      if (set.contains(i * 3 - n)) {
        ++found;
      }
    }
    CHECK(found == 1000000);
    for (int64_t i(0); i < n; ++i) {
      set.remove(i * 3 - n);
    }
    CHECK(set.empty());
  }
}
