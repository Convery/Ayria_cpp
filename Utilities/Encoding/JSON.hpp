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
#include "Stringconv.hpp"
#include "Utilities/Internal/Misc.hpp"

#pragma warning(push)
#pragma warning(disable: 4702)
namespace JSON
{
    namespace Internal
    {
        // Helpers for type deduction.
        template <class T, template <class...> class Template>
        struct isDerived : std::false_type {};
        template <template <class...> class Template, class... Args>
        struct isDerived<Template<Args...>, Template> : std::true_type {};
    }

    enum class Type_t { Null, Bool, Float, Signedint, Unsignedint, String, Object, Array };
    using Object_t = std::unordered_map<std::string, struct Value_t>;
    using Array_t = std::vector<struct Value_t>;

    // Helper for type deduction.
    template<typename T> constexpr Type_t toType()
    {
        if constexpr (Internal::isDerived<T, std::basic_string_view>{}) return Type_t::String;
        if constexpr (Internal::isDerived<T, std::basic_string>{}) return Type_t::String;

        if constexpr (Internal::isDerived<T, std::unordered_set>{}) return Type_t::Array;
        if constexpr (Internal::isDerived<T, std::vector>{}) return Type_t::Array;
        if constexpr (Internal::isDerived<T, std::set>{}) return Type_t::Array;

        if constexpr (Internal::isDerived<T, std::unordered_map>{}) return Type_t::Object;
        if constexpr (Internal::isDerived<T, std::map>{}) return Type_t::Object;

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
    #define asPtr(Type) std::static_pointer_cast<Type>(Value)
    struct Value_t
    {
        Type_t Type{};
        std::shared_ptr<void> Value{};

        //
        [[nodiscard]] std::string dump() const
        {
            switch (Type)
            {
                case Type_t::String: return va("\"%s\"", Encoding::toNarrow(*asPtr(std::u8string)).c_str());
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
                    const auto Delta = std::static_pointer_cast<Object_t>(Input.Value);
                    const auto Current = asPtr(Object_t);

                    for (const auto &[lKey, lValue] : *Delta)
                    {
                        (*Current)[lKey].update(lValue);
                    }

                    break;
                }
                case Type_t::Array:
                {
                    const auto Delta = std::static_pointer_cast<Array_t>(Input.Value);
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
        [[nodiscard]] bool contains(std::string_view Key) const
        {
            if (Type != Type_t::Object) return false;
            return asPtr(Object_t)->contains(Key.data());
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
        template<typename T, size_t N> auto value(std::string_view Key, const std::array<T, N> &Defaultvalue)
        {
            return value(Key, std::basic_string<std::remove_const_t<T>>(Defaultvalue, N));
        }
        template<typename T, size_t N> auto value(std::string_view Key, T(&Defaultvalue)[N])
        {
            return value(Key, std::basic_string<std::remove_const_t<T>>(Defaultvalue, N));
        }
        template<typename T> T value(std::string_view Key, T Defaultvalue) const
        {
            if constexpr (!std::is_convertible_v<Value_t, T>) return Defaultvalue;
            if (Type != Type_t::Object) return Defaultvalue;

            if (!asPtr(Object_t)->contains(Key.data())) return Defaultvalue;
            return asPtr(Object_t)->at(Key.data());
        }
        template<typename T = Value_t> T value(std::string_view Key) const
        {
            if constexpr (!std::is_convertible_v<Value_t, T>) return {};
            if (Type != Type_t::Object) return {};

            if (!asPtr(Object_t)->contains(Key.data())) return {};
            return asPtr(Object_t)->at(Key.data());
        }

        // NOTE(tcn): std::basic_string variants can't deduce T, use value().
        template<typename T> operator T() const
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
                    if constexpr (Internal::isDerived<T, std::basic_string>{})
                    {
                        if constexpr (std::is_same_v<typename T::value_type, wchar_t>)
                            return Encoding::toWide(*asPtr(std::u8string));

                        if constexpr (std::is_same_v<typename T::value_type, char>)
                            return Encoding::toNarrow(*asPtr(std::u8string));

                        if constexpr (std::is_same_v<typename T::value_type, char8_t>)
                            return *asPtr(std::u8string);
                    }
                    break;
                }
                case Type_t::Array:
                {
                    if constexpr (Internal::isDerived<T, std::set>{} || Internal::isDerived<T, std::unordered_set>{} ||
                                  Internal::isDerived<T, std::vector>{})
                    {
                        if constexpr (std::is_same_v<T, Array_t>)
                        return *asPtr(Array_t);

                        const auto Array = asPtr(Array_t);
                        T Result; Result.reserve(Array->size());

                        if constexpr (Internal::isDerived<T, std::set>{} || Internal::isDerived<T, std::unordered_set>{})
                            for (const auto &Item : *Array)
                                Result.insert(Item);

                        if constexpr (Internal::isDerived<T, std::vector>{})
                            std::ranges::copy(*Array, Result.begin());
                    }
                    break;
                }
            }

            // Allow NULL-ness to be checked in if-statements.
            if constexpr (std::is_same_v<T, bool>) return !isNull();
            else return {};
        }
        template<typename T> T get() const
        {
            return *this;
        }
        template<typename T> std::remove_pointer_t<T> *get()
        {
            if (isNull()) return {};
            return *asPtr(std::remove_pointer_t<T>);
        }

        //
        template<size_t N> Value_t &operator[](const char(&Key)[N])
        {
            if (Type != Type_t::Object) return *this;
            return asPtr(Object_t)->at({ Key, N });
        }
        template<size_t N> Value_t operator[](const char(&Key)[N]) const
        {
            if (Type != Type_t::Object) return *this;
            return *asPtr(Object_t)->at({ Key, N });
        }

        //
        template<typename T, size_t N> Value_t(const std::array<T, N> &Input)
        {
            *this = std::basic_string<std::remove_const_t<T>>(Input.data(), N);
            return;
        }
        template<typename T, size_t N> [[nodiscard]] Value_t(T(&Input)[N])
        {
            *this = std::basic_string<std::remove_const_t<T>>(Input, N);
            return;
        }
        template<typename T> Value_t(const T &Input)
        {
            // Let's not bother with pointers.
            static_assert(!std::is_pointer_v<T>);

            // Safety-check, we do not convert arrays to strings.
            static_assert(toType<T>() != Type_t::Null);

            if constexpr (std::is_same_v<T, Value_t>) *this = Input;
            else
            {
                Type = toType<T>();

                if constexpr (toType<T>() == Type_t::String) Value = std::make_shared<std::u8string>(Encoding::toUTF8(Input));
                if constexpr (toType<T>() == Type_t::Unsignedint) Value = std::make_shared<uint64_t>(Input);
                if constexpr (toType<T>() == Type_t::Signedint) Value = std::make_shared<int64_t>(Input);
                if constexpr (toType<T>() == Type_t::Float) Value = std::make_shared<double>(Input);
                if constexpr (toType<T>() == Type_t::Bool) Value = std::make_shared<bool>(Input);

                if constexpr (Internal::isDerived<T, std::unordered_map>{} || Internal::isDerived<T, std::map>{})
                {
                    static_assert(std::is_convertible_v<typename T::key_type, std::string>);

                    Object_t Object(Input.size());
                    for (const auto &[Key, _Value] : Input)
                        Object[Key] = _Value;

                    Type = Type_t::Object;
                    Value = std::make_shared<Object_t>(std::move(Object));
                }
                if constexpr (Internal::isDerived<T, std::set>{} || Internal::isDerived<T, std::unordered_set>{} ||
                              Internal::isDerived<T, std::vector>{})
                {
                    Array_t Array(Input.size());
                    for (auto Items = Enumerate(Input); const auto &[Index, _Value] : Items)
                        Array[Index] = _Value;

                    Type = Type_t::Array;
                    Value = std::make_shared<Array_t>(std::move(Array));
                }
            }
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

            try
            {
                Result = Parse(nlohmann::json::parse(JSONString.data(), nullptr, true, true));
            }
            catch (const std::exception &e)
            {
                (void)e; Debugprint(e.what());
            };
            #else
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
    inline Value_t Parse(const char *JSONString, size_t Length = 0)
    {
        if (!JSONString) return {};
        if (Length == 0) return Parse(std::string(JSONString));
        else return Parse(std::string_view(JSONString, Length));
    }
}
#pragma warning(pop)
