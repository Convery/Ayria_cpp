/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-11-15
    License: MIT

    Largely inspired by https://github.com/SqliteModernCpp/sqlite_modern_cpp

    NOTES:
    On errors, an assert is triggered.
    If a query fails, no output is written.
    Bound values per query is limited to 256.
    Only a single statement per query is supported.
    If a query returns incompatible data, output is default constructed or converted (e.g. int <-> string).
    If an output is not provided, the query will be executed when the object goes out of scope but results are discarded.

    Database_t(*Ptr) << "SELECT * FROM Table WHERE Input = ?;" << myInput
    (1)   >> [](int &a, &b, &c) -> bool {};     // Lambda with the expected types, return false to stop evaluation.
    (2)   >> [](int &a, &b, &c) -> void {};     // Lambda with the expected types, executed for each row.
    (3)   >> std::tie(a, b);                    // Tuple with the expected types.
    (4)   >> myOutput;                          // Single variable.
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
#include <Utilities/Encoding/UTF8.hpp>

#include "sqlite3.h"

namespace sqlite
{
    // Ignore warnings about unreachable code.
    #pragma warning(push)
    #pragma warning(disable: 4702)

    // Helpers for type deduction.
    template <class, template <class...> class> inline constexpr bool isDerived = false;
    template <template <class...> class T, class... Args> inline constexpr bool isDerived<T<Args...>, T> = true;

    template <typename T> concept Integer_t = std::is_integral_v<T>;
    template <typename T> concept Float_t = std::is_floating_point_v<T>;
    template <typename T> concept Optional_t = isDerived<T, std::optional>;
    template <typename T> concept String_t = isDerived<T, std::basic_string> || isDerived<T, std::basic_string_view>;
    template <typename T> concept Blob_t = isDerived<T, std::vector> || isDerived<T, std::set> || isDerived<T, std::unordered_set>;
    template <typename T> concept Value_t = Integer_t<T> || String_t<T> || Optional_t<T> || Blob_t<T> || Float_t<T> || std::is_same_v<T, nullptr_t> ;

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

        template <size_t N> using arg = std::tuple_element_t<N, std::tuple<Args...>>;
    };

    // Convert between SQL and C++.
    template <Value_t T> T getResult(sqlite3_stmt *Statement, int Index)
    {
        const auto Type = sqlite3_column_type(Statement, Index);
        if (SQLITE_NULL == Type) return {};

        if constexpr (Optional_t<T>)
        {
            return { getResult<typename T::value_type>(Statement, Index) };
        }

        switch (Type)
        {
            case SQLITE_INTEGER:
                if constexpr (Integer_t<T>) return sqlite3_column_int64(Statement, Index);
                break;

            case SQLITE_FLOAT:
                if constexpr (Float_t<T>) return sqlite3_column_double(Statement, Index);
                break;

            case SQLITE_TEXT:
                if constexpr (isDerived<T, std::basic_string>)
                {
                    const std::u8string Temp = (char8_t *)sqlite3_column_text(Statement, Index);

                    if constexpr (std::is_same_v<typename T::value_type, char>) return Encoding::toASCII(Temp);
                    else if constexpr (std::is_same_v<typename T::value_type, wchar_t>) return Encoding::toUNICODE(Temp);
                    else return Temp;
                }
                static_assert(!isDerived<T, std::basic_string_view>, "Can't bind result to string_view.");
                break;

            case SQLITE_BLOB:
                if constexpr (Blob_t<T>)
                {
                    const auto Size = sqlite3_column_bytes(Statement, Index);
                    const auto Buffer = (typename T::value_type *)sqlite3_column_blob(Statement, Index);

                    const std::span Temp(Buffer, Size / sizeof(typename T::value_type));
                    return T(Temp.begin(), Temp.end());
                }
                break;
        }

        // No acceptable conversion possible.
        assert(false);
        return {};
    }
    template <Value_t T> void getResult(sqlite3_stmt *Statement, int Index, T &Output)
    {
        Output = getResult<T>(Statement, Index);
    }
    template <Value_t T> void bindValue(sqlite3_stmt *Statement, int Index, const T &Value)
    {
        const auto Result = [&]() -> int
        {
            if constexpr (Blob_t<T>)
            {
                if constexpr (isDerived<T, std::vector>)
                    return sqlite3_bind_blob(Statement, Index, Value.data(), int(Value.size() * sizeof(typename T::value_type)), SQLITE_TRANSIENT);
                else
                {
                    // Needs contigious memory.
                    const std::vector<typename T::value_type> Temp(Value.begin(), Value.end());
                    return sqlite3_bind_blob(Statement, Index, Temp.data(), int(Temp.size() * sizeof(typename T::value_type)), SQLITE_TRANSIENT);
                }
            }
            if constexpr (Integer_t<T> && sizeof(T) == sizeof(uint64_t)) return sqlite3_bind_int64(Statement, Index, Value);
            if constexpr (Integer_t<T> && sizeof(T) != sizeof(uint64_t)) return sqlite3_bind_int(Statement, Index, Value);
            if constexpr (std::is_same_v<T, nullptr_t>) return sqlite3_bind_null(Statement, Index);
            if constexpr (Float_t<T>) return sqlite3_bind_double(Statement, Index, Value);

            if constexpr (isDerived<T, std::basic_string_view> || isDerived<T, std::basic_string>)
            {
                // Passthrough for std::string and std::u8string.
                const auto Temp = Encoding::toUTF8(Value);

                return sqlite3_bind_text(Statement, Index, (const char *)Temp.data(), (int)Temp.size(), SQLITE_TRANSIENT);
            }

            if constexpr (Optional_t<T>)
            {
                if (Value) bindValue(Statement, Index, *Value);
                else bindValue(Statement, Index, nullptr);
                return SQLITE_OK;
            }

            // Should never happen.
            return SQLITE_ERROR;
        }();

        assert(SQLITE_OK == Result);
        (void)Result;
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
        template <typename Function, size_t Index> using Argtype = typename Functiontraits<Function>::template arg<Index>;
        template <typename Function> using R = typename Functiontraits<Function>::return_type;

        template <typename Function, typename ...Values, size_t Boundary = Count>
        static R<Function> Run(sqlite3_stmt *Statement, Function &&Func, Values&& ...va) requires(sizeof...(Values) < Boundary)
        {
            std::remove_cv_t<std::remove_reference_t<Argtype<Function, sizeof...(Values)>>> value{};

            getResult(Statement, sizeof...(Values), value);

            return Run<Function>(Statement, Func, std::forward<Values>(va)..., std::move(value));
        }

        template <typename Function, typename ...Values, size_t Boundary = Count>
        static R<Function> Run(sqlite3_stmt *, Function &&Func, Values&& ...va) requires(sizeof...(Values) == Boundary)
        {
            return Func(std::move(va)...);
        }
    };

    // Holds the prepared statement that we append values to.
    class Statement_t
    {
        std::shared_ptr<sqlite3_stmt> Statement{};
        uint8_t Argcount{};
        bool isStarted{};
        uint8_t Index{};

        #if !defined (NDEBUG)
        std::shared_ptr<sqlite3> Owner;
        #endif

        // Initial state for the bindings.
        inline void setStarted()
        {
            isStarted = true;
            Index = 0;
        }

        // Step through the query and extract anything interesting.
        void Extractsingle(const std::function<void()> &&Callback)
        {
            // Verify that we didn't forget an argument.
            assert(Argcount == Index);

            // Clear the state.
            setStarted();

            // Extract a row.
            auto Result = sqlite3_step(Statement.get());
            if (SQLITE_ROW == Result) Callback();

            // Verify that there's indeed only one row.
            Result = sqlite3_step(Statement.get());

            // For debugging.
            #if !defined (NDEBUG)
            if (Result != SQLITE_DONE)
            {
                const auto Error = sqlite3_errmsg(Owner.get());
                Errorprint(va("SQL error: %s", Error));
                assert(false);
            }
            #endif
        }
        void Extractmultiple(const std::function<void()> &&Callback)
        {
            // Verify that we didn't forget an argument.
            assert(Argcount == Index);

            // Clear the state.
            setStarted();

            auto Result = sqlite3_step(Statement.get());
            while (SQLITE_ROW == Result)
            {
                Callback();
                Result = sqlite3_step(Statement.get());
            }

            // For debugging.
            #if !defined (NDEBUG)
            if (Result != SQLITE_DONE)
            {
                const auto Error = sqlite3_errmsg(Owner.get());
                Errorprint(va("SQL error: %s", Error));
                assert(false);
            }
            #endif
        }
        void Extractmultiple(const std::function<bool()> &&Callback)
        {
            // Verify that we didn't forget an argument.
            assert(Argcount == Index);

            // Clear the state.
            setStarted();

            auto Result = sqlite3_step(Statement.get());
            while (SQLITE_ROW == Result)
            {
                if (!Callback()) return;
                Result = sqlite3_step(Statement.get());
            }

            // For debugging.
            #if !defined (NDEBUG)
            if (Result != SQLITE_DONE)
            {
                const auto Error = sqlite3_errmsg(Owner.get());
                Errorprint(va("SQL error: %s", Error));
                assert(false);
            }
            #endif
        }

    public:
        // Result-extraction operator, lambdas return false to terminate execution or void for full queries.
        template <typename Function> requires (!Value_t<Function>) void operator>>(Function &&Callback)
        {
            // Interruptible queries.
            Extractmultiple([&Callback, this]()
            {
                return Functionbinder_t<Functiontraits<Function>::argcount>::Run(Statement.get(), Callback);
            });

            sqlite3_reset(Statement.get());
        }
        template <typename... T> void operator>>(std::tuple<T...> &&Value)
        {
            Extractsingle([&Value, this]()
            {
                Tuple_t<std::tuple<T...>>::iterate(Statement.get(), Value);
            });

            sqlite3_reset(Statement.get());
        }
        template <Value_t T> void operator>>(T &Value)
        {
            Extractsingle([&Value, this]()
            {
                getResult(Statement.get(), 0, Value);
            });

            sqlite3_reset(Statement.get());
        }

        // Input operator, sequential propagating of the '?' placeholders in the query.
        template <cmp::Bytealigned_t T, size_t N> Statement_t &operator<<(const cmp::Vector_t<T, N> &Value)
        {
            const std::basic_string_view<T> Temp{ Value.begin(), Value.end() };
            return operator<<(Temp);
        }
        template <Value_t T> Statement_t &operator<<(const T &Value)
        {
            // Ensure a clean state,
            if (isStarted && Index == 0) [[unlikely]]
            {
                sqlite3_reset(Statement.get());
                sqlite3_clear_bindings(Statement.get());
            }

            bindValue(Statement.get(), ++Index, Value);
            return *this;
        }
        Statement_t &operator<<(std::string_view Value)
        {
            // Ensure a clean state,
            if (isStarted && Index == 0) [[unlikely]]
            {
                sqlite3_reset(Statement.get());
                sqlite3_clear_bindings(Statement.get());
            }

            bindValue(Statement.get(), ++Index, Value);
            return *this;
        }

        // Reset can also be used to avoid RTTI evaluation.
        void Execute() { Extractmultiple([]() {}); sqlite3_reset(Statement.get()); }
        void Reset() { setStarted(); sqlite3_reset(Statement.get()); }

        explicit Statement_t(std::shared_ptr<sqlite3> Connection, std::string_view SQL)
        {
            const char *Remaining{};
            sqlite3_stmt *Temp{};

            // For simplicity, we don't accept more than one statement, i.e. only one ';' allowed.
            assert(1 == std::ranges::count(SQL, ';'));
            Argcount = std::ranges::count(SQL, '?');

            // Prepare the statement.
            const auto Result = sqlite3_prepare_v2(Connection.get(), SQL.data(), int(SQL.size()), &Temp, &Remaining);
            (void)Result;

            // For debugging.
            #if !defined (NDEBUG)
            Owner = Connection;

            if (Result != SQLITE_OK)
            {
                const auto Error = sqlite3_errmsg(Owner.get());
                Errorprint(Error);
            }
            #endif

            // Save the statement and finalize when we go out of scope.
            Statement = { Temp, sqlite3_finalize };
        }
        Statement_t(const Statement_t &Other) noexcept : Statement(Other.Statement), Argcount(Other.Argcount), isStarted(Other.isStarted), Index(Other.Index)
        {
            // For debugging.
            #if !defined (NDEBUG)
            Owner = Other.Owner;
            #endif
        }
        Statement_t(Statement_t &&Other) noexcept : Statement(std::move(Other.Statement)), Argcount(Other.Argcount), isStarted(Other.isStarted), Index(Other.Index)
        {
            // For debugging.
            #if !defined (NDEBUG)
            Owner = Other.Owner;
            #endif
        }
        ~Statement_t()
        {
            // Need to ensure that the statement was evaluated.
            if (!isStarted && Argcount == Index) [[unlikely]]
                Extractmultiple([]() {});
        }
    };

    // Holds the connection and creates the prepared statement(s).
    struct Database_t
    {
        std::shared_ptr<sqlite3> Connection;

        Database_t(std::shared_ptr<sqlite3> Ptr) : Connection(Ptr) {};
        Statement_t operator<<(std::string_view SQL) const
        {
            return Statement_t(Connection, SQL);
        }
    };

    // Restore warnings.
    #pragma warning(pop)
}
