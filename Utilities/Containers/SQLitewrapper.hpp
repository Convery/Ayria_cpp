/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-11-15
    License: MIT
*/

#pragma once
#include <memory>
#include <cstdint>
#include <cassert>
#include <concepts>
#include <optional>
#include <algorithm>
#include <functional>
#include <type_traits>
#include <string_view>

#include <sqlite3.h>
#pragma comment(lib, "sqlite3.lib")

namespace sqlite
{
    // Helpers for type deduction.
    template <class, template <class...> class> inline constexpr bool isDerived = false;
    template< template <class...> class T, class... Args> inline constexpr bool isDerived<T<Args...>, T> = true;

    template <typename T> concept SQLBlob = isDerived<T, std::vector>;
    template <typename T> concept SQLOptional = isDerived<T, std::optional>;
    template <typename T> concept SQLString = isDerived<T, std::basic_string>;
    template <typename T> concept SQLNumber = std::is_integral_v<T> || std::is_floating_point_v<T>;
    template <typename T> concept SQLValue = SQLNumber<T> || SQLString<T> || SQLOptional<T> || SQLBlob<T> || std::is_same_v<T, nullptr_t>;

    // Helper to get the arguments to a function.
    template <typename> struct Functiontraits;
    template <typename F> struct Functiontraits : public Functiontraits<decltype(&std::remove_reference_t<F>::operator())> {};
    template <typename R, typename C, typename ...Args> struct Functiontraits<R(C::*)(Args...)> : Functiontraits<R(*)(Args...)> {};
    template <typename R, typename C, typename ...Args> struct Functiontraits<R(C::*)(Args...) const> : Functiontraits<R(*)(Args...)> {};
    template <typename R, typename ...Args> struct Functiontraits<R(*)(Args...)>
    {
        static const size_t argcount = sizeof...(Args);
        using type = std::function<R>(Args...);
        using return_type = R;

        template <size_t N> using arg = typename std::tuple_element_t<N, std::tuple<Args...>>;
    };

    // Convert between SQL and C++.
    template <SQLValue T> T getResult(sqlite3_stmt *Statement, int Index)
    {
        switch (sqlite3_column_type(Statement, Index))
        {
            case SQLITE_NULL: return {};

            case SQLITE_INTEGER:
                if constexpr (std::is_integral_v<T> && sizeof(T) == sizeof(uint64_t)) return sqlite3_column_int64(Statement, Index);
                if constexpr (std::is_integral_v<T> && sizeof(T) != sizeof(uint64_t)) return sqlite3_column_int(Statement, Index);
                break;

            case SQLITE_FLOAT:
                if constexpr (std::is_floating_point_v<T>) return sqlite3_column_double(Statement, Index);
                break;

            case SQLITE_TEXT:
                if constexpr (SQLString<T>) return T((typename T::value_type *)sqlite3_column_text(Statement, Index));
                break;

            case SQLITE_BLOB:
                if constexpr (SQLBlob<T>)
                {
                    const auto Size = sqlite3_column_bytes(Statement, Index);
                    const auto Buffer = (typename T::value_type *)sqlite3_column_blob(Statement, Index);
                    return T(Buffer, Buffer + Size / sizeof(typename T::value_type));
                }
                break;
        }

        // No acceptible conversion possible.
        assert(false);
        return {};
    }
    template <SQLValue T> void getResult(sqlite3_stmt *Statement, int Index, T &Output)
    {
        Output = getResult<T>(Statement, Index);
    }
    template <SQLValue T> void bindValue(sqlite3_stmt *Statement, int Index, const T &Value)
    {
        const auto Result = [&]() -> int
        {
            if constexpr (SQLBlob<T>) return sqlite3_bind_blob(Statement, Index, Value.data(), int(Value.size() * sizeof(typename T::value_type)), SQLITE_TRANSIENT);
            if constexpr (SQLString<T>) return sqlite3_bind_text(Statement, Index, (const char *)Value.data(), (int)Value.size(), SQLITE_TRANSIENT);
            if constexpr (std::is_integral_v<T> && sizeof(T) == sizeof(uint64_t)) return sqlite3_bind_int64(Statement, Index, Value);
            if constexpr (std::is_integral_v<T> && sizeof(T) != sizeof(uint64_t)) return sqlite3_bind_int(Statement, Index, Value);
            if constexpr (std::is_floating_point_v<T>) return sqlite3_bind_double(Statement, Index, Value);
            if constexpr (std::is_same_v<T, nullptr_t>) return sqlite3_bind_null(Statement, Index);

            if constexpr (SQLOptional<T>)
            {
                if (Value) bindValue(Statement, Index, *Value);
                else bindValue(Statement, Index, nullptr);
                return SQLITE_OK;
            }
        }();

        assert(SQLITE_OK == Result);
    }

    // Helper to iterate over tuple members.
    template <typename T, size_t Index = 0, bool Last = (std::tuple_size_v<T> == Index)> struct Tuple_t
    {
        static void iterate(sqlite3_stmt *Statement, T &Tuple)
        {
            getResult(Statement, Index, std::get<Index>(Tuple));
            Tuple_t<T, Index + 1>::iterate(Statement, Tuple);
        }
    };
    template <typename T, size_t Index> struct Tuple_t<T, Index, true> { static void iterate(sqlite3_stmt *Statement, T &Tuple) {} };

    // Helper for callbacks.
    template <size_t Count> struct Functionbinder_t
    {
        template <typename Function, size_t Index>
        using Argtype = typename Functiontraits<Function>::template arg<Index>;

        template <typename Function, typename ...Values, size_t Boundry = Count>
        static void Run(sqlite3_stmt *Statement, Function &&Func, Values&& ...va) requires(sizeof...(Values) < Boundry)
        {
            typename std::remove_cv_t<typename std::remove_reference_t<Argtype<Function, sizeof...(Values)>>> value{};

            getResult(Statement, sizeof...(Values), value);

            Run<Function>(Statement, Func, std::forward<Values>(va)..., std::move(value));
        }

        template <typename Function, typename ...Values, size_t Boundry = Count>
        static void Run(sqlite3_stmt *, Function &&Func, Values&& ...va) requires(sizeof...(Values) == Boundry)
        {
            Func(std::move(va)...);
        }
    };

    // Holds the prepared statement that we append values to.
    struct Statement_t
    {
        std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> Statement;
        bool isStarted{};
        int32_t Index{};

        void Reset()
        {
            //
            if (isStarted && 0 == Index)
            {
                sqlite3_reset(Statement.get());
                sqlite3_clear_bindings(Statement.get());
            }

            // State.
            Index = 0;
            isStarted = true;
        }
        void Extractsingle(std::function<void()> Callback)
        {
            Reset();

            // Extract a row.
            int Result = sqlite3_step(Statement.get());
            assert(SQLITE_ROW == Result);
            Callback();

            // Verify that there's indeed only one row.
            Result = sqlite3_step(Statement.get());
            assert(SQLITE_DONE == Result);
        }
        void Extractmultiple(std::function<void()> Callback)
        {
            int Result;
            Reset();

            while (SQLITE_ROW == (Result = sqlite3_step(Statement.get())))
            {
                Callback();
            }

            // If there was an error, report it.
            assert(SQLITE_DONE == Result);
        }

        template <SQLValue T> void operator>>(T &Value)
        {
            Extractsingle([&Value, this]
            {
                getResult(Statement.get(), 0, Value);
            });
        }
        template <typename... T> void operator>>(std::tuple<T...> &&Value)
        {
            Extractsingle([&Value, this]
            {
                Tuple_t<std::tuple<T...>>::iterate(Statement.get(), Value);
            });
        }
        template <typename Function> requires (!SQLValue<Function>) void operator>>(Function &&Callback)
        {
            Extractmultiple([&Callback, this]()
            {
                Functionbinder_t<Functiontraits<Function>::argcount>::Run(Statement.get(), Callback);
            });
        }

        template <SQLValue T> inline Statement_t &operator<<(const T &Value)
        {
            bindValue(Statement.get(), Index++, Value);
            return *this;
        }
        inline Statement_t &operator<<(const std::string &Value)
        {
            bindValue(Statement.get(), Index++, Value);
            return *this;
        }

        explicit Statement_t(std::shared_ptr<sqlite3> Connection, std::string_view SQL) : Statement(nullptr, sqlite3_finalize)
        {
            const char *Remaining{};
            sqlite3_stmt *Temp{};

            // Prepare the statement.
            const auto Result = sqlite3_prepare_v2(Connection.get(), SQL.data(), (int)SQL.size(), &Temp, &Remaining);
            assert(SQLITE_OK == Result);

            // For simplicity, we don't accept more than one statement, i.e. only one ';' allowed.
            assert(std::all_of(Remaining, SQL.data() + SQL.size(), [](auto Item) { return std::isspace(Item); }));

            // Update the internal state.
            isStarted = false;

            // Save the statement and finalize when we go out of scope.
            Statement = { Temp, sqlite3_finalize };
        }
        Statement_t(Statement_t &&Other) noexcept : Statement(std::move(Other.Statement)), isStarted(Other.isStarted), Index(Other.Index) {}
        ~Statement_t()
        {
            // Need to ensure that the statement was evaluated.
            if (!isStarted)
            {
                int Result;
                Reset();

                while (SQLITE_ROW == (Result = sqlite3_step(Statement.get()))) {};

                // If there was an error, report it.
                assert(SQLITE_DONE == Result);
            }
        }
    };

    // Holds the connection and creates the prepared statement.
    struct Database_t
    {
        std::shared_ptr<sqlite3> Connection;

        Database_t(std::shared_ptr<sqlite3> Ptr) : Connection(Ptr) {};
        Statement_t operator<<(std::string_view SQL) const
        {
            return Statement_t(Connection, SQL);
        }
    };
}
