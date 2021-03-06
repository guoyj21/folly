/*
 * Copyright 2015 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#include <folly/futures/Future.h>

using namespace folly;
using std::unique_ptr;
using std::string;

typedef FutureException eggs_t;
static eggs_t eggs("eggs");

TEST(Promise, special) {
  EXPECT_FALSE(std::is_copy_constructible<Promise<int>>::value);
  EXPECT_FALSE(std::is_copy_assignable<Promise<int>>::value);
  EXPECT_TRUE(std::is_move_constructible<Promise<int>>::value);
  EXPECT_TRUE(std::is_move_assignable<Promise<int>>::value);
}

TEST(Promise, getFuture) {
  Promise<int> p;
  Future<int> f = p.getFuture();
  EXPECT_FALSE(f.isReady());
}

TEST(Promise, setValue) {
  Promise<int> fund;
  auto ffund = fund.getFuture();
  fund.setValue(42);
  EXPECT_EQ(42, ffund.value());

  struct Foo {
    string name;
    int value;
  };

  Promise<Foo> pod;
  auto fpod = pod.getFuture();
  Foo f = {"the answer", 42};
  pod.setValue(f);
  Foo f2 = fpod.value();
  EXPECT_EQ(f.name, f2.name);
  EXPECT_EQ(f.value, f2.value);

  pod = Promise<Foo>();
  fpod = pod.getFuture();
  pod.setValue(std::move(f2));
  Foo f3 = fpod.value();
  EXPECT_EQ(f.name, f3.name);
  EXPECT_EQ(f.value, f3.value);

  Promise<unique_ptr<int>> mov;
  auto fmov = mov.getFuture();
  mov.setValue(unique_ptr<int>(new int(42)));
  unique_ptr<int> ptr = std::move(fmov.value());
  EXPECT_EQ(42, *ptr);

  Promise<void> v;
  auto fv = v.getFuture();
  v.setValue();
  EXPECT_TRUE(fv.isReady());
}

TEST(Promise, setException) {
  {
    Promise<void> p;
    auto f = p.getFuture();
    p.setException(eggs);
    EXPECT_THROW(f.value(), eggs_t);
  }
  {
    Promise<void> p;
    auto f = p.getFuture();
    try {
      throw eggs;
    } catch (...) {
      p.setException(exception_wrapper(std::current_exception()));
    }
    EXPECT_THROW(f.value(), eggs_t);
  }
}

TEST(Promise, setWith) {
  {
    Promise<int> p;
    auto f = p.getFuture();
    p.setWith([] { return 42; });
    EXPECT_EQ(42, f.value());
  }
  {
    Promise<int> p;
    auto f = p.getFuture();
    p.setWith([]() -> int { throw eggs; });
    EXPECT_THROW(f.value(), eggs_t);
  }
}

TEST(Promise, isFulfilled) {
  Promise<int> p;

  EXPECT_FALSE(p.isFulfilled());
  p.setValue(42);
  EXPECT_TRUE(p.isFulfilled());
}

TEST(Promise, isFulfilledWithFuture) {
  Promise<int> p;
  auto f = p.getFuture(); // so core_ will become null

  EXPECT_FALSE(p.isFulfilled());
  p.setValue(42); // after here
  EXPECT_TRUE(p.isFulfilled());
}
