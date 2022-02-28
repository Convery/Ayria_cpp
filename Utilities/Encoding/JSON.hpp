/*
    Initial author: Convery (tcn@ayria.se)
    Started: 2020-10-30
    License: MIT

    Each JSON library has their own 'issues'.
    NLohmann does not work well with u8string.
    SIMDJSON treats all but the largest ints as signed.
*/

#pragma once
#include <memory>
#include <Stdinclude.hpp>
#include "Utilities/Internal/Misc.hpp"
#include <Utilities/Encoding/UTF8.hpp>
#include <Utilities/Encoding/Variadicstring.hpp>

#pragma warning(push)
#pragma warning(disable: 4702)
namespace JSON
{
    // Helpers for type deduction.
    template <class, template <class...> class> inline constexpr bool isDerived = false;
    template <template <class...> class T, class... Args> inline constexpr bool isDerived<T<Args...>, T> = true;

    enum class Type_t { Null, Bool, Float, Signedint, Unsignedint, String, Object, Array };
    using Object_t = std::unordered_map<std::string, struct Value_t>;
    using Array_t = std::vector<Value_t>;

    // Helper for type deduction.
    template <typename T> constexpr Type_t toType()
    {
        if constexpr (isDerived<T, std::basic_string_view>) return Type_t::String;
        if constexpr (isDerived<T, std::basic_string>) return Type_t::String;

        if constexpr (isDerived<T, std::unordered_set>) return Type_t::Array;
        if constexpr (isDerived<T, std::vector>) return Type_t::Array;
        if constexpr (isDerived<T, std::set>) return Type_t::Array;

        if constexpr (isDerived<T, std::unordered_map>) return Type_t::Object;
        if constexpr (isDerived<T, std::map>) return Type_t::Object;

        if constexpr (std::is_floating_point_v<T>) return Type_t::Float;
        if constexpr (std::is_same_v<T, bool>) return Type_t::Bool;
        if constexpr (std::is_integral_v<T>)
        {
            if constexpr (std::is_signed_v<T>)
                return Type_t::Signedint;
            return Type_t::Unsignedint;
        }

        return Type_t::Null;
    }

    // Generic value wrapper.
    #define asPtr(Type) std::static_pointer_cast<Type>(Internalstorage)
    struct Value_t
    {
        std::shared_ptr<void> Internalstorage{};
        Type_t Type{};

        //
        [[nodiscard]] std::string dump() const
        {
            switch (Type)
            {
                case Type_t::String: return va("\"%s\"", Encoding::toASCII(*asPtr(std::u8string)).c_str());
                case Type_t::Unsignedint: return va("%llu", *asPtr(uint64_t));
                case Type_t::Signedint: return va("%lli", *asPtr(int64_t));
                case Type_t::Bool: return *asPtr(bool) ? "true" : "false";
                case Type_t::Float: return va("%f", *asPtr(double));
                case Type_t::Null: return "null";

                case Type_t::Object:
                {
                    std::string Result{ "{" };
                    for (const auto Ptr = asPtr(Object_t); const auto &[lKey, lValue] : *Ptr)
                    {
                        Result.append(va("\"%*s\":", lKey.size(), lKey.data()));
                        Result.append(lValue.dump());
                        Result.append(",");
                    }
                    if (!asPtr(Object_t)->empty()) Result.pop_back();
                    Result.append("}");
                    return Result;
                }
                case Type_t::Array:
                {
                    std::string Result{ "[" };
                    for (const auto Ptr = asPtr(Array_t); const auto &lValue : *Ptr)
                    {
                        Result.append(lValue.dump());
                        Result.append(",");
                    }
                    if (!asPtr(Array_t)->empty()) Result.pop_back();
                    Result.append("]");
                    return Result;
                }
            }

            return {};
        }

        //
        bool update(const Value_t &Input)
        {
            if (Type == Type_t::Null)
            {
                *this = Input;
                return true;
            }

            if (Type != Input.Type)
                return false;

            switch (Input.Type)
            {
                case Type_t::Null: break;
                case Type_t::Object:
                {
                    const auto Delta = std::static_pointer_cast<Object_t>(Input.Internalstorage);
                    const auto Current = asPtr(Object_t);

                    for (const auto &[lKey, lValue] : *Delta)
                    {
                        (*Current)[lKey].update(lValue);
                    }

                    break;
                }
                case Type_t::Array:
                {
                    const auto Delta = std::static_pointer_cast<Array_t>(Input.Internalstorage);
                    const auto Current = asPtr(Array_t);

                    Current->reserve(Current->size() + Delta->size());
                    std::ranges::copy(*Delta, Current->end());

                    break;
                }

                default: *this = Input; break;
            }

            return true;
        }

        //
        template <typename ...Args> [[nodiscard]] bool contains_all(Args&&... va) const
        {
            if (Type != Type_t::Object) return false;
            return (contains(va) && ...);
        }
        template <typename ...Args> [[nodiscard]] bool contains_any(Args&&... va) const
        {
            if (Type != Type_t::Object) return false;
            return (contains(va) || ...);
        }
        [[nodiscard]] bool contains(const std::string_view Key) const
        {
            if (Type != Type_t::Object) return false;
            return asPtr(Object_t)->contains(Key.data()) && !asPtr(Object_t)->at(Key.data()).isNull();
        }
        [[nodiscard]] bool isNull() const
        {
            return Type == Type_t::Null;
        }
        [[nodiscard]] bool empty() const
        {
            if (Type == Type_t::String) return asPtr(std::u8string)->empty();
            if (Type == Type_t::Object) return asPtr(Object_t)->empty();
            if (Type == Type_t::Array) return asPtr(Array_t)->empty();
            return true;
        }

        //
        template <typename T, size_t N> auto value(std::string_view Key, const std::array<T, N> &Defaultvalue)
        {
            return value(Key, std::basic_string<std::remove_const_t<T>>(Defaultvalue, N));
        }
        template <typename T, size_t N> auto value(std::string_view Key, T(&Defaultvalue)[N])
        {
            return value(Key, std::basic_string<std::remove_const_t<T>>(Defaultvalue, N));
        }
        template <typename T> T value(const std::string_view Key, T Defaultvalue) const
        {
            if constexpr (!std::is_convertible_v<Value_t, T>) return Defaultvalue;
            if (Type != Type_t::Object) return Defaultvalue;

            if (!asPtr(Object_t)->contains(Key.data())) return Defaultvalue;
            return asPtr(Object_t)->at(Key.data());
        }
        template <typename T = Value_t> T value(const std::string_view Key) const
        {
            if constexpr (!std::is_convertible_v<Value_t, T>) return {};
            if (Type != Type_t::Object) return {};

            if (!asPtr(Object_t)->contains(Key.data())) return {};
            return asPtr(Object_t)->at(Key.data());
        }

        // NOTE(tcn): The std::basic_string variants can't alwas be deduced and needs to be declared explicitly.
        template <typename T> operator T() const
        {
            switch (Type)
            {
                case Type_t::Object: if constexpr (std::is_same_v<T, Object_t>) return *asPtr(Object_t); break;
                case Type_t::Unsignedint: if constexpr (std::is_integral_v<T>) return *asPtr(uint64_t); break;
                case Type_t::Float: if constexpr (std::is_floating_point_v<T>) return *asPtr(double); break;
                case Type_t::Signedint: if constexpr (std::is_integral_v<T>) return *asPtr(int64_t); break;
                case Type_t::Bool: if constexpr (std::is_same_v<T, bool>) return *asPtr(bool); break;
                case Type_t::Null: break;

                case Type_t::String:
                {
                    if constexpr (isDerived<T, std::basic_string>)
                    {
                        if constexpr (std::is_same_v<typename T::value_type, wchar_t>)
                            return Encoding::toUNICODE(*asPtr(std::u8string));

                        if constexpr (std::is_same_v<typename T::value_type, char>)
                            return Encoding::toASCII(*asPtr(std::u8string));

                        if constexpr (std::is_same_v<typename T::value_type, char8_t>)
                            return *asPtr(std::u8string);
                    }
                    else
                    {
                        // While possible in the case of std::u8string_view, better to discourage this.
                        static_assert(!isDerived<T, std::basic_string_view>, "Don't try to cast to a *_view string.");
                    }
                    break;
                }

                case Type_t::Array:
                {
                    if constexpr (isDerived<T, std::set> || isDerived<T, std::unordered_set> || isDerived<T, std::vector>)
                    {
                        if constexpr (std::is_same_v<T, Array_t>) return *asPtr(Array_t);

                        const auto Array = asPtr(Array_t);
                        T Result; Result.reserve(Array->size());

                        if constexpr (isDerived<T, std::set> || isDerived<T, std::unordered_set>)
                            for (const auto &Item : *Array)
                                Result.insert(Item);

                        if constexpr (isDerived<T, std::vector>)
                            std::ranges::copy(*Array, Result.begin());

                        return Result;
                    }
                    break;
                }
            }

            // Allow NULL-ness to be checked in if-statements.
            if constexpr (std::is_same_v<T, bool>) return !isNull();
            else return {};
        }
        template <typename T> T get() const
        {
            return this->operator T();
        }

        //
        template <size_t N> Value_t operator[](const char(&Key)[N]) const
        {
            if (Type != Type_t::Object) { static Value_t Dummyvalue; Dummyvalue = {}; return Dummyvalue; }
            return asPtr(Object_t)->operator[]({ Key, N });
        }
        template <size_t N> Value_t &operator[](const char(&Key)[N])
        {
            if (Type != Type_t::Object) { static Value_t Dummyvalue; Dummyvalue = {}; return Dummyvalue; }
            return asPtr(Object_t)->operator[]({ Key, N });
        }
        Value_t operator[](const size_t N) const
        {
            if (Type != Type_t::Array) { static Value_t Dummyvalue; Dummyvalue = {}; return Dummyvalue; }
            return asPtr(Array_t)->operator[](N);
        }
        Value_t &operator[](const size_t N)
        {
            if (Type != Type_t::Array) { static Value_t Dummyvalue; Dummyvalue = {}; return Dummyvalue; }
            return asPtr(Array_t)->operator[](N);
        }

        //
        template <typename T> Value_t(const T &Input) requires(!std::is_pointer_v<T>)
        {
            // Optional support.
            if constexpr (isDerived<T, std::optional>)
            {
                if (Input) *this = Value_t(*Input);
                else *this = Value_t();
                return;
            }
            else
            {
                // Safety-check..
                static_assert(toType<T>() != Type_t::Null);
            }

            if constexpr (std::is_same_v<T, Value_t>) *this = Input;
            else
            {
                Type = toType<T>();

                if constexpr (toType<T>() == Type_t::String) Internalstorage = std::make_shared<std::u8string>(Encoding::toUTF8(Input));
                if constexpr (toType<T>() == Type_t::Unsignedint) Internalstorage = std::make_shared<uint64_t>(Input);
                if constexpr (toType<T>() == Type_t::Signedint) Internalstorage = std::make_shared<int64_t>(Input);
                if constexpr (toType<T>() == Type_t::Float) Internalstorage = std::make_shared<double>(Input);
                if constexpr (toType<T>() == Type_t::Bool) Internalstorage = std::make_shared<bool>(Input);

                if constexpr (isDerived<T, std::unordered_map> || isDerived<T, std::map>)
                {
                    static_assert(std::is_convertible_v<typename T::key_type, std::string>);

                    Object_t Object(Input.size());
                    for (const auto &[Key, _Value] : Input)
                        Object[Key] = _Value;

                    Type = Type_t::Object;
                    Internalstorage = std::make_shared<Object_t>(std::move(Object));
                }
                if constexpr (isDerived<T, std::set> || isDerived<T, std::unordered_set> || isDerived<T, std::vector>)
                {
                    Array_t Array(Input.size());
                    for (auto Items = Enumerate(Input); const auto &[Index, _Value] : Items)
                        Array[Index] = _Value;

                    Type = Type_t::Array;
                    Internalstorage = std::make_shared<Array_t>(std::move(Array));
                }
            }
        }
        template <typename T, size_t N> Value_t(const std::array<T, N> &Input)
        {
            *this = std::basic_string<std::remove_const_t<T>>(Input.data(), N);
            return;
        }
        template <typename T, size_t N> [[nodiscard]] Value_t(T(&Input)[N])
        {
            *this = std::basic_string<std::remove_const_t<T>>(Input, N);
            return;
        }
        Value_t(const char *Input)
        {
            Type = Type_t::String;
            Internalstorage = std::make_shared<std::u8string>(Encoding::toUTF8(Input));
        }
        Value_t() = default;
    };
    #undef asPtr

    inline std::string Dump(const Value_t &Value) { return Value.dump(); }
    inline std::string Dump(const Array_t &Value) { return Value_t(Value).dump(); }
    inline std::string Dump(const Object_t &Value) { return Value_t(Value).dump(); }

    inline Value_t Parse(std::string_view JSONString)
    {
        Value_t Result{};

        // Malformed statement check. Missing brackets, null-chars messing up C-string parsing.
        const auto C1 = std::ranges::count(JSONString, '{') != std::ranges::count(JSONString, '}');
        const auto C2 = std::ranges::count(JSONString, '[') != std::ranges::count(JSONString, ']');
        const auto C3 = std::ranges::count(JSONString, '\0') > 1;
        if (C1 || C2 || C3)
        {
            Errorprint(va("Trying to parse invalid JSON string, first chars: %*s", std::min(size_t(20), JSONString.size()), JSONString.data()));
            return Result;
        }

        // Implementation dependent.
        if (!JSONString.empty())
        {
            #if defined(HAS_SIMDJSON)
            static simdjson::dom::parser Parser;
            const std::function<Value_t(const simdjson::dom::element &)>
                Parse = [&Parse](const simdjson::dom::element &Item) -> Value_t
                {
                    switch (Item.type())
                    {
                        case simdjson::dom::element_type::STRING: return Encoding::toUTF8(Item.get_string().value());
                        case simdjson::dom::element_type::DOUBLE: return Item.get_double().value();
                        case simdjson::dom::element_type::UINT64: return Item.get_uint64().value();
                        case simdjson::dom::element_type::INT64: return Item.get_int64().value();
                        case simdjson::dom::element_type::BOOL: return Item.get_bool().value();

                        case simdjson::dom::element_type::OBJECT:
                        {
                            Object_t Object; Object.reserve(Item.get_object().value().size());
                            for (const auto Items = Item.get_object(); const auto &[Key, Value] : Items)
                            {
                                Object.emplace(Key, Parse(Value));
                            }
                            return Object;
                        }
                        case simdjson::dom::element_type::ARRAY:
                        {
                            Array_t Array; Array.reserve(Item.get_array().value().size());
                            for (const auto Items = Item.get_array(); const auto &Subitem : Items)
                            {
                                Array.push_back(Parse(Subitem));
                            }
                            return Array;
                        }

                        case simdjson::dom::element_type::NULL_VALUE: return {};
                    }

                    // WTF??
                    assert(false);
                    return {};
                };

            Result = Parse(Parser.parse(JSONString));

            #elif defined(HAS_NLOHMANN)
            const std::function<Value_t(const nlohmann::json &)>
                Parse = [&Parse](const nlohmann::json &Item) -> Value_t
                {
                    if (Item.is_string()) return Encoding::toUTF8(Item.get<std::string>());
                    if (Item.is_number_float()) return Item.get<double>();
                    if (Item.is_number_unsigned()) return Item.get<uint64_t>();
                    if (Item.is_number_integer()) return Item.get<int64_t>();
                    if (Item.is_boolean()) return Item.get<bool>();
                    if (Item.is_object())
                    {
                        Object_t Object; Object.reserve(Item.size());
                        for (auto Items = Item.items(); const auto &[Key, Value] : Items)
                        {
                            Object.emplace(Key, Parse(Value));
                        }
                        return Object;
                    }
                    if (Item.is_array())
                    {
                        Array_t Array; Array.reserve(Item.size());
                        for (const auto &Subitem : Item)
                        {
                            Array.push_back(Parse(Subitem));
                        }
                        return Array;
                    }

                    // WTF??
                    assert(false);
                    return {};
                };

            try { Result = Parse(nlohmann::json::parse(JSONString.data(), nullptr, true, true)); }
            catch (const std::exception &e) { (void)e; Debugprint(e.what()); };
            #else
            //std::function<Value_t(std::string_view &)> Parse = [&Parse](std::string_view &Stream)
            //{
            //    const auto RXString = R"(" ([^"\n\r\t\\\\]* | \\\\ ["\\\\bfnrt\/] | \\\\ u [0-9a-f]{4} )* ")";
            //    const auto RXNumber = R"(-? (?= [1-9]|0(?!\d) ) \d+ (\.\d+)? ([eE] [+-]? \d+)";
            //    const auto RXBool = R"(true | false | null)";
            //};

            static_assert(false, "No JSON parser available.");
            #endif
        }

        return Result;
    }
    inline Value_t Parse(const std::string &JSONString)
    {
        if (JSONString.empty()) return {};
        return Parse(std::string_view(JSONString));
    }
    inline Value_t Parse(const char *JSONString, const size_t Length = 0)
    {
        if (!JSONString) return {};
        if (Length == 0) return Parse(std::string(JSONString));
        return Parse(std::string_view(JSONString, Length));
    }

    template <typename T> requires (sizeof(T) == 1)
    inline Value_t Parse(std::span<T> Span)
    {
        return Parse(std::string_view{ (char *)Span.data(), Span.size() });
    }
}
#pragma warning(pop)
