/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2021-11-15
    License: MIT

    Largely inspired by https://github.com/SqliteModernCpp/sqlite_modern_cpp

    NOTES:
    On errors, an assert is triggered.
    If a query fails, no output is written.
    If a query returns incompatible data, output is default constructed.
    If a lambda is supplied as output, it will be called for each result but can return false to terminate.
    If an output is not provided, the query will be executed when the object goes out of scope but results ignored.

    Database_t(*Ptr) << "SELECT * FROM Table WHERE Input = ?;" << myInput
        >> [](Results...) {};
        >> std::tie(a, b);
        >> myOutput;
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

// For UTF8 encoding.
#include "../Encoding/Stringconv.hpp"

#include <sqlite3.h>
#pragma comment(lib, "sqlite3.lib")

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

                    if constexpr (std::is_same_v<typename T::value_type, char>) return Encoding::toNarrow(Temp);
                    else if constexpr (std::is_same_v<typename T::value_type, wchar_t>) return Encoding::toWide(Temp);
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

        template <typename Function, typename ...Values, size_t Boundry = Count>
        static R<Function> Run(sqlite3_stmt *Statement, Function &&Func, Values&& ...va) requires(sizeof...(Values) < Boundry)
        {
            std::remove_cv_t<std::remove_reference_t<Argtype<Function, sizeof...(Values)>>> value{};

            getResult(Statement, sizeof...(Values), value);

            return Run<Function>(Statement, Func, std::forward<Values>(va)..., std::move(value));
        }

        template <typename Function, typename ...Values, size_t Boundry = Count>
        static R<Function> Run(sqlite3_stmt *, Function &&Func, Values&& ...va) requires(sizeof...(Values) == Boundry)
        {
            return Func(std::move(va)...);
        }
    };

    // Holds the prepared statement that we append values to.
    class Statement_t
    {
        std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> Statement;
        bool isStarted{};
        int32_t Index{};

        // Intial state for the bindings.
        inline void setStarted()
        {
            isStarted = true;
            Index = 0;
        }

        // Step through the query and extract anything interesting.
        void Extractsingle(std::function<void()> Callback)
        {
            setStarted();

            // Extract a row.
            int Result = sqlite3_step(Statement.get());
            if (SQLITE_ROW == Result) Callback();

            // Verify that there's indeed only one row.
            Result = sqlite3_step(Statement.get());
            assert(SQLITE_DONE == Result);
        }
        void Extractmultiple(std::function<void()> Callback)
        {
            int Result;
            setStarted();

            while (SQLITE_ROW == (Result = sqlite3_step(Statement.get())))
            {
                Callback();
            }

            // If there was an error, report it.
            assert(SQLITE_DONE == Result);
        }
        void Extractmultiple(std::function<bool()> Callback)
        {
            int Result;
            setStarted();

            while (SQLITE_ROW == (Result = sqlite3_step(Statement.get())))
            {
                if (!Callback())
                {
                    return;
                }
            }

            // If there was an error, report it.
            assert(SQLITE_DONE == Result);
        }

    public:
        // Result-extraction opterator, lambdas return false to terminate execution or void for complete queries.
        template <typename Function> requires (!Value_t<Function>) void operator>>(Function &&Callback)
        {
            // Interruptable queries.
            Extractmultiple([&Callback, this]()
            {
                return Functionbinder_t<Functiontraits<Function>::argcount>::Run(Statement.get(), Callback);
            });
        }
        template <typename... T> void operator>>(std::tuple<T...> &&Value)
        {
            Extractsingle([&Value, this]
            {
                Tuple_t<std::tuple<T...>>::iterate(Statement.get(), Value);
            });
        }
        template <Value_t T> void operator>>(T &Value)
        {
            Extractsingle([&Value, this]
            {
                getResult(Statement.get(), 0, Value);
            });
        }

        // Input operator, sequential propagating of the '?' placeholders in the query.
        template <Value_t T> Statement_t &operator<<(const T &Value)
        {
            // Ensure a clean state,
            if (isStarted && Index == 0)
            {
                sqlite3_reset(Statement.get());
                sqlite3_clear_bindings(Statement.get());
            }

            bindValue(Statement.get(), Index++, Value);
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

            bindValue(Statement.get(), Index++, Value);
            return *this;
        }

        // Reset can also be used to avoid RTTI evaluation.
        void Reset() { setStarted(); }

        explicit Statement_t(std::shared_ptr<sqlite3> Connection, std::string_view SQL) : Statement(nullptr, sqlite3_finalize)
        {
            const char *Remaining{};
            sqlite3_stmt *Temp{};

            // Prepare the statement.
            const auto Result = sqlite3_prepare_v2(Connection.get(), SQL.data(), (int)SQL.size(), &Temp, &Remaining);
            assert(SQLITE_OK == Result);
            (void)Result;

            // Save the statement and finalize when we go out of scope.
            Statement = { Temp, sqlite3_finalize };
        }
        Statement_t(Statement_t &&Other) noexcept : Statement(std::move(Other.Statement)), isStarted(Other.isStarted), Index(Other.Index) {}
        ~Statement_t()
        {
            // Need to ensure that the statement was evaluated.
            if (!isStarted) Extractmultiple([]() {});
        }
    };

    // Holds the connection and creates the prepared statement.
    struct Database_t
    {
        std::shared_ptr<sqlite3> Connection;

        Database_t(std::shared_ptr<sqlite3> Ptr) : Connection(Ptr) {};
        Statement_t operator<<(std::string_view SQL) const
        {
            // For simplicity, we don't accept more than one statement, i.e. only one ';' allowed.
            assert(1 == std::ranges::count(SQL, ';'));
            return Statement_t(Connection, SQL);
        }
    };

    // Restore warnings.
    #pragma warning(pop)
}
