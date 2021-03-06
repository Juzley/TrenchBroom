/*
 Copyright 2020 Kristian Duske

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 persons to whom the Software is furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <gtest/gtest.h>

#include "kdl/overload.h"
#include "kdl/result.h"

#include <string>

namespace kdl {
    struct Error1 {};
    struct Error2 {};
    
    inline bool operator==(const Error1&, const Error1&) {
        return true;
    }

    inline bool operator==(const Error2&, const Error2&) {
        return true;
    }

    struct Counter {
        std::size_t copies = 0u;
        std::size_t moves = 0u;
        
        Counter() = default;
        
        Counter(const Counter& c)
        : copies(c.copies + 1u)
        , moves(c.moves) {}
        
        Counter(Counter&& c)
        : copies(c.copies)
        , moves(c.moves + 1u) {}

        Counter& operator=(const Counter& c) {
            this->copies = c.copies + 1u;
            this->moves = c.moves;
            return *this;
        }
        
        Counter& operator=(Counter&& c) {
            this->copies = c.copies;
            this->moves = c.moves + 1u;
            return *this;
        }
    };
        
    /**
     * Tests construction of a successful result.
     */
    template <typename ResultType, typename... V>
    void test_construct_success(V&&... v) {
        auto result = ResultType::success(std::forward<V>(v)...);
        ASSERT_TRUE(result.is_success());
        ASSERT_FALSE(result.is_error());
        ASSERT_EQ(result.is_success(), result);
    }
    
    /**
     * Tests construction of an error result.
     */
    template <typename ResultType, typename E>
    void test_construct_error(E&& e) {
        auto result = ResultType::error(std::forward<E>(e));
        ASSERT_FALSE(result.is_success());
        ASSERT_TRUE(result.is_error());
        ASSERT_EQ(result.is_success(), result);
    }
    
    /**
     * Tests visiting a successful result and passing by const lvalue reference to the visitor.
     */
    template <typename ResultType, typename V>
    void test_visit_success_const_lvalue_ref(V&& v) {
        auto result = ResultType::success(std::forward<V>(v));

        ASSERT_TRUE(visit_result(overload {
            [&] (const auto& x) { return x == v; },
            []  (const Error1&) { return false; },
            []  (const Error2&) { return false; }
        }, result));
    }
    
    /**
     * Tests visiting a successful result and passing by rvalue reference to the visitor.
     */
    template <typename ResultType, typename V>
    void test_visit_success_rvalue_ref(V&& v) {
        auto result = ResultType::success(std::forward<V>(v));

        ASSERT_TRUE(visit_result(overload {
            [&] (auto&&)   { return true; },
            []  (Error1&&) { return false; },
            []  (Error2&&) { return false; }
        }, std::move(result)));
        
        typename ResultType::value_type y;
        visit_result(overload {
            [&] (auto&& x) { y = std::move(x); },
            []  (Error1&&) {},
            []  (Error2&&) {}
        }, std::move(result));
        
        ASSERT_EQ(0u, y.copies);
    }

    /**
     * Tests visiting an error result and passing by const lvalue reference to the visitor.
     */
    template <typename ResultType, typename E>
    void test_visit_error_const_lvalue_ref(E&& e) {
        auto result = ResultType::error(std::forward<E>(e));

        ASSERT_TRUE(visit_result(overload {
            []  (const auto&)   { return false; },
            [&] (const E& x)    { return x == e; },
            []  (const Error2&) { return false; }
        }, result));
    }
    
    /**
     * Tests visiting an error result and passing by rvalue reference to the visitor.
     */
    template <typename ResultType, typename E>
    void test_visit_error_rvalue_ref(E&& e) {
        auto result = ResultType::error(std::forward<E>(e));

        ASSERT_TRUE(visit_result(overload {
            []  (auto&&)   { return false; },
            [&] (E&&)      { return true; },
            []  (Error2&&) { return false; }
        }, std::move(result)));
        
        E y;
        visit_result(overload {
            []  (auto&&)   {},
            [&] (E&& x)    { y = std::move(x); },
            []  (Error2&&) {}
        }, std::move(result));
        
        ASSERT_EQ(0u, y.copies);
    }
    
    /**
     * Tests mapping a successful result and passing by const lvalue reference to the mapping function.
     */
    template <typename FromResult, typename ToValueType, typename V>
    void test_map_const_lvalue_ref(V&& v) {
        auto from = FromResult::success(std::forward<V>(v));
        
        const auto to = map_result([](const typename FromResult::value_type& x) { return static_cast<ToValueType>(x); }, from);
        ASSERT_TRUE(to.is_success());
        ASSERT_FALSE(to.is_error());
        ASSERT_EQ(to.is_success(), to);

        ASSERT_TRUE(visit_result(overload {
            [](const ToValueType&) { return true; },
            [](const auto&) { return false; }
        }, to));
    }
    
    /**
     * Tests mapping a successful result and passing by rvalue reference to the mapping function.
     */
    template <typename FromResult, typename ToValueType, typename V>
    void test_map_rvalue_ref(V&& v) {
        auto from = FromResult::success(std::forward<V>(v));
        const auto to = map_result([](typename FromResult::value_type&& x) { return std::move(static_cast<ToValueType>(x)); }, std::move(from));
        ASSERT_TRUE(to.is_success());
        ASSERT_FALSE(to.is_error());
        ASSERT_EQ(to.is_success(), to);

        ASSERT_TRUE(visit_result(overload {
            [](const ToValueType&) { return true; },
            [](const auto&) { return false; }
        }, to));

        ToValueType y;
        visit_result(overload {
            [&] (ToValueType&& x) { y = x; },
            [] (auto&&) {}
        }, std::move(to));
        
        ASSERT_EQ(0u, y.copies);
    }
    
    /**
     * Tests visiting a successful result when there is no value.
     */
    template <typename ResultType>
    void test_visit_success_with_opt_value() {
        auto result = ResultType::success();
        
        ASSERT_TRUE(visit_result(overload {
            []()            { return true; },
            [](const auto&) { return false; }
        }, result));
    }

    /**
     * Tests visiting a successful result and passing by const lvalue reference to the visitor
     * when the value is optional (or void).
     */
    template <typename ResultType, typename V>
    void test_visit_success_const_lvalue_ref_with_opt_value(V&& v) {
        auto result = ResultType::success(std::forward<V>(v));
        
        ASSERT_TRUE(visit_result(overload {
            []  ()              { return false; },
            [&] (const auto& x) { return x == v; },
            []  (const Error1&) { return false; },
            []  (const Error2&) { return false; }
        }, result));
    }

    /**
     * Tests visiting a successful result and passing by rvalue reference to the visitor
     * when the value is optional (or void).
     */
    template <typename ResultType, typename V>
    void test_visit_success_rvalue_ref_with_opt_value(V&& v) {
        auto result = ResultType::success(std::forward<V>(v));

        ASSERT_TRUE(visit_result(overload {
            []  ()         { return false; },
            [&] (auto&&)   { return true; },
            []  (Error1&&) { return false; },
            []  (Error2&&) { return false; }
        }, std::move(result)));
        
        typename ResultType::value_type y;
        visit_result(overload {
            [] ()         {},
            [&] (auto&& x) { y = std::move(x); },
            []  (Error1&&) {},
            []  (Error2&&) {}
        }, std::move(result));
        
        ASSERT_EQ(0u, y.copies);
    }

    /**
     * Tests visiting an error result and passing by const lvalue reference to the visitor
     * when the value is optional (or void).
     */
    template <typename ResultType, typename E>
    void test_visit_error_const_lvalue_ref_with_opt_value(E&& e) {
        auto result = ResultType::error(std::forward<E>(e));
        
        ASSERT_TRUE(visit_result(overload {
            []  ()            { return false; },
            []  (const auto&) { return false; },
            [&] (const E& x)  { return x == e; }
        }, result));
    }
    
    /**
     * Tests visiting an error result and passing by rvalue reference to the visitor
     * when the value is optional (or void).
     */
    template <typename ResultType, typename E>
    void test_visit_error_rvalue_ref_with_opt_value(E&& e) {
        auto result = ResultType::error(std::forward<E>(e));

        ASSERT_TRUE(visit_result(overload {
            [] ()       { return false; },
            []  (auto&&) { return false; },
            [&] (E&&)    { return true; }
        }, std::move(result)));

        E y;
        visit_result(overload {
            []  ()       {},
            []  (auto&&) {},
            [&] (E&& x)  { y = std::move(x); }
        }, std::move(result));
        
        ASSERT_EQ(0u, y.copies);
    }

    TEST(result_test, constructor) {
        ASSERT_TRUE((result<int, float, std::string>::success(1).is_success()));
        ASSERT_TRUE((result<int, float, std::string>::error(1.0f).is_error()));
        ASSERT_TRUE((result<int, float, std::string>::error("").is_error()));

        test_construct_success<const result<int, Error1, Error2>>(1);
        test_construct_success<result<int, Error1, Error2>>(1);
        test_construct_success<const result<const int, Error1, Error2>>(1);
        test_construct_success<result<const int, Error1, Error2>>(1);

        test_construct_error<const result<int, Error1, Error2>>(Error1{});
        test_construct_error<result<int, Error1, Error2>>(Error1{});
        test_construct_error<const result<const int, Error1, Error2>>(Error1{});
        test_construct_error<result<const int, Error1, Error2>>(Error1{});

        test_construct_error<const result<int, Error1, Error2>>(Error2{});
        test_construct_error<result<int, Error1, Error2>>(Error2{});
        test_construct_error<const result<const int, Error1, Error2>>(Error2{});
        test_construct_error<result<const int, Error1, Error2>>(Error2{});
    }

    TEST(result_test, visit) {
        test_visit_success_const_lvalue_ref<const result<int, Error1, Error2>>(1);
        test_visit_success_const_lvalue_ref<result<int, Error1, Error2>>(1);
        test_visit_success_const_lvalue_ref<const result<const int, Error1, Error2>>(1);
        test_visit_success_const_lvalue_ref<result<const int, Error1, Error2>>(1);
        test_visit_success_rvalue_ref<result<Counter, Error1, Error2>>(Counter{});

        test_visit_error_const_lvalue_ref<const result<int, Error1, Error2>>(Error1{});
        test_visit_error_const_lvalue_ref<result<int, Error1, Error2>>(Error1{});
        test_visit_error_const_lvalue_ref<const result<const int, Error1, Error2>>(Error1{});
        test_visit_error_const_lvalue_ref<result<const int, Error1, Error2>>(Error1{});
        test_visit_error_rvalue_ref<result<int, Counter, Error2>>(Counter{});
    }

    TEST(result_test, map) {
        test_map_const_lvalue_ref<const result<int, Error1, Error2>, float>(1);
        test_map_const_lvalue_ref<result<int, Error1, Error2>, float>(1);
        test_map_const_lvalue_ref<const result<const int, Error1, Error2>, float>(1);
        test_map_const_lvalue_ref<result<const int, Error1, Error2>, float>(1);
        test_map_rvalue_ref<result<Counter, Error1, Error2>, Counter>(Counter{});
    }
    
    TEST(reference_result_test, constructor) {
        int x = 1;

        ASSERT_TRUE((result<int&, float, std::string>::success(x).is_success()));
        ASSERT_TRUE((result<int&, float, std::string>::error(1.0f).is_error()));
        ASSERT_TRUE((result<int&, float, std::string>::error("").is_error()));

        test_construct_success<const result<int&, Error1, Error2>>(x);
        test_construct_success<result<int&, Error1, Error2>>(x);
        test_construct_success<const result<const int&, Error1, Error2>>(x);
        test_construct_success<result<const int&, Error1, Error2>>(x);

        test_construct_error<const result<int&, Error1, Error2>>(Error1{});
        test_construct_error<result<int&, Error1, Error2>>(Error1{});
        test_construct_error<const result<const int&, Error1, Error2>>(Error1{});
        test_construct_error<result<const int&, Error1, Error2>>(Error1{});

        test_construct_error<const result<int&, Error1, Error2>>(Error2{});
        test_construct_error<result<int&, Error1, Error2>>(Error2{});
        test_construct_error<const result<const int&, Error1, Error2>>(Error2{});
        test_construct_error<result<const int&, Error1, Error2>>(Error2{});
    }
    
    TEST(reference_result_test, visit) {
        int x = 1;
        test_visit_success_const_lvalue_ref<const result<int&, Error1, Error2>>(x);
        test_visit_success_const_lvalue_ref<result<int&, Error1, Error2>>(x);
        test_visit_success_const_lvalue_ref<const result<const int&, Error1, Error2>>(x);
        test_visit_success_const_lvalue_ref<result<const int&, Error1, Error2>>(x);
        
        auto c = Counter{};
        test_visit_success_rvalue_ref<result<Counter&, Error1, Error2>>(c);

        test_visit_error_const_lvalue_ref<const result<int&, Error1, Error2>>(Error1{});
        test_visit_error_const_lvalue_ref<result<int&, Error1, Error2>>(Error1{});
        test_visit_error_const_lvalue_ref<const result<const int&, Error1, Error2>>(Error1{});
        test_visit_error_const_lvalue_ref<result<const int&, Error1, Error2>>(Error1{});
        test_visit_error_rvalue_ref<result<int&, Counter, Error2>>(Counter{});
    }
    
    TEST(reference_result_test, map) {
        int x = 1;
        test_map_const_lvalue_ref<const result<int&, Error1, Error2>, float>(x);
        test_map_const_lvalue_ref<result<int&, Error1, Error2>, float>(x);
        test_map_const_lvalue_ref<const result<const int&, Error1, Error2>, float>(x);
        test_map_const_lvalue_ref<result<const int&, Error1, Error2>, float>(x);

        auto c = Counter{};
        test_map_rvalue_ref<result<Counter&, Error1, Error2>, Counter>(c);
    }
    
    TEST(void_result_test, constructor) {
        ASSERT_TRUE((result<void, float, std::string>::success().is_success()));
        ASSERT_TRUE((result<void, float, std::string>::error(1.0f).is_error()));
        ASSERT_TRUE((result<void, float, std::string>::error("").is_error()));

        test_construct_success<const result<void, Error1, Error2>>();
        test_construct_success<result<void, Error1, Error2>>();

        test_construct_error<const result<void, Error1, Error2>>(Error1{});
        test_construct_error<result<void, Error1, Error2>>(Error1{});

        test_construct_error<const result<void, Error1, Error2>>(Error2{});
        test_construct_error<result<void, Error1, Error2>>(Error2{});
    }

    TEST(void_result_test, visit) {
        test_visit_success_with_opt_value<const result<void, Error1, Error2>>();
        test_visit_success_with_opt_value<result<void, Error1, Error2>>();
        
        test_visit_error_const_lvalue_ref_with_opt_value<const result<void, Error1, Error2>>(Error1{});
        test_visit_error_const_lvalue_ref_with_opt_value<result<void, Error1, Error2>>(Error1{});
        test_visit_error_rvalue_ref_with_opt_value<result<void, Counter, Error2>>(Counter{});
    }
    
    TEST(opt_result_test, constructor) {
        ASSERT_TRUE((result<opt<int>, float, std::string>::success().is_success()));
        ASSERT_TRUE((result<opt<int>, float, std::string>::success(1).is_success()));
        ASSERT_TRUE((result<opt<int>, float, std::string>::error(1.0f).is_error()));
        ASSERT_TRUE((result<opt<int>, float, std::string>::error("").is_error()));

        test_construct_success<const result<opt<int>, Error1, Error2>>();
        test_construct_success<result<opt<int>, Error1, Error2>>();
        test_construct_success<const result<opt<const int>, Error1, Error2>>();
        test_construct_success<result<opt<const int>, Error1, Error2>>();

        test_construct_success<const result<opt<int>, Error1, Error2>>(1);
        test_construct_success<result<opt<int>, Error1, Error2>>(1);
        test_construct_success<const result<opt<const int>, Error1, Error2>>(1);
        test_construct_success<result<opt<const int>, Error1, Error2>>(1);

        test_construct_error<const result<opt<int>, Error1, Error2>>(Error1{});
        test_construct_error<result<opt<int>, Error1, Error2>>(Error1{});
        test_construct_error<const result<opt<const int>, Error1, Error2>>(Error1{});
        test_construct_error<result<opt<const int>, Error1, Error2>>(Error1{});

        test_construct_error<const result<opt<int>, Error1, Error2>>(Error2{});
        test_construct_error<result<opt<int>, Error1, Error2>>(Error2{});
        test_construct_error<const result<opt<const int>, Error1, Error2>>(Error2{});
        test_construct_error<result<opt<const int>, Error1, Error2>>(Error2{});
    }

    TEST(opt_result_test, visit) {
        test_visit_success_with_opt_value<const result<opt<int>, Error1, Error2>>();
        test_visit_success_with_opt_value<result<opt<int>, Error1, Error2>>();
        
        test_visit_success_const_lvalue_ref_with_opt_value<const result<opt<int>, Error1, Error2>>(1);
        test_visit_success_const_lvalue_ref_with_opt_value<result<opt<int>, Error1, Error2>>(1);
        test_visit_success_const_lvalue_ref_with_opt_value<const result<opt<const int>, Error1, Error2>>(1);
        test_visit_success_const_lvalue_ref_with_opt_value<result<opt<const int>, Error1, Error2>>(1);
        test_visit_success_rvalue_ref_with_opt_value<result<opt<Counter>, Error1, Error2>>(Counter{});

        test_visit_error_const_lvalue_ref_with_opt_value<const result<opt<int>, Error1, Error2>>(Error1{});
        test_visit_error_const_lvalue_ref_with_opt_value<result<opt<int>, Error1, Error2>>(Error1{});
        test_visit_error_const_lvalue_ref_with_opt_value<const result<opt<const int>, Error1, Error2>>(Error1{});
        test_visit_error_const_lvalue_ref_with_opt_value<result<opt<const int>, Error1, Error2>>(Error1{});
        test_visit_error_rvalue_ref_with_opt_value<result<opt<int>, Counter, Error2>>(Counter{});
    }
}

namespace something {
    TEST(result_test, adl_friend_lookup) {
        auto value_result = kdl::result<int, kdl::Error1, kdl::Error2>::success(1);

        ASSERT_TRUE(kdl::visit_result(kdl::overload {
            [&] (const int&)         { return true; },
            []  (const kdl::Error1&) { return false; },
            []  (const kdl::Error2&) { return false; }
        }, value_result));
        
        ASSERT_TRUE(kdl::visit_result(kdl::overload {
            [&] (int&&)         { return true; },
            []  (kdl::Error1&&) { return false; },
            []  (kdl::Error2&&) { return false; }
        }, std::move(value_result)));
        
        ASSERT_TRUE(kdl::map_result(kdl::overload {
            [&] (const int&) { return true; }
        }, value_result));
        
        ASSERT_TRUE(kdl::map_result(kdl::overload {
            [&] (int&&) { return true; }
        }, std::move(value_result)));

        int x = 1;
        auto ref_result = kdl::result<int&, kdl::Error1, kdl::Error2>::success(x);

        ASSERT_TRUE(kdl::visit_result(kdl::overload {
            [&] (const int&)         { return true; },
            []  (const kdl::Error1&) { return false; },
            []  (const kdl::Error2&) { return false; }
        }, ref_result));
        
        ASSERT_TRUE(kdl::visit_result(kdl::overload {
            [&] (int&&)         { return true; },
            []  (kdl::Error1&&) { return false; },
            []  (kdl::Error2&&) { return false; }
        }, std::move(ref_result)));
        
        ASSERT_TRUE(kdl::map_result(kdl::overload {
            [&] (const int&) { return true; }
        }, ref_result));
        
        ASSERT_TRUE(kdl::map_result(kdl::overload {
            [&] (int&&) { return true; }
        }, std::move(ref_result)));

        auto void_result = kdl::result<void, kdl::Error1, kdl::Error2>::success();
        ASSERT_TRUE(kdl::visit_result(kdl::overload {
            [&] ()                   { return true; },
            []  (const kdl::Error1&) { return false; },
            []  (const kdl::Error2&) { return false; }
        }, void_result));
        
        ASSERT_TRUE(kdl::visit_result(kdl::overload {
            [&] ()              { return true; },
            []  (kdl::Error1&&) { return false; },
            []  (kdl::Error2&&) { return false; }
        }, std::move(void_result)));

        auto opt_result = kdl::result<kdl::opt<int>, kdl::Error1, kdl::Error2>::success();
        ASSERT_TRUE(kdl::visit_result(kdl::overload {
            [&] ()                   { return true; },
            [&] (const int&)         { return true; },
            []  (const kdl::Error1&) { return false; },
            []  (const kdl::Error2&) { return false; }
        }, opt_result));
        
        ASSERT_TRUE(kdl::visit_result(kdl::overload {
            [&] ()              { return true; },
            [&] (int&&)         { return true; },
            []  (kdl::Error1&&) { return false; },
            []  (kdl::Error2&&) { return false; }
        }, std::move(opt_result)));
    }
    
    
}
