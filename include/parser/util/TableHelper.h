#ifndef __ARCB_UTIL_TABLE_HELPER__H__
#define __ARCB_UTIL_TABLE_HELPER__H__


///////////////////////////////////////////////////////////////////////////////
// INCLUDES
///////////////////////////////////////////////////////////////////////////////

#include <map>
#include <set>
#include <regex>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <filesystem>

#include "Core.h"
#include "Support.h"
#include "Defines.h"





//    ███████╗ ██████╗ ██████╗ ██╗    ██╗ █████╗ ██████╗ ██████╗ ███████╗
//    ██╔════╝██╔═══██╗██╔══██╗██║    ██║██╔══██╗██╔══██╗██╔══██╗██╔════╝
//    █████╗  ██║   ██║██████╔╝██║ █╗ ██║███████║██████╔╝██║  ██║███████╗
//    ██╔══╝  ██║   ██║██╔══██╗██║███╗██║██╔══██║██╔══██╗██║  ██║╚════██║
//    ██║     ╚██████╔╝██║  ██║╚███╔███╔╝██║  ██║██║  ██║██████╔╝███████║
//    ╚═╝      ╚═════╝ ╚═╝  ╚═╝ ╚══╝╚══╝ ╚═╝  ╚═╝╚═╝  ╚═╝╚═════╝ ╚══════╝
//                                                                                                                                                                                                                                                                                                       


/**
 * @file TableHelper.h
 * @brief Table helpers for semantic attribute queries and profile/OS alignment.
 *
 * This header provides generic utilities to query and extract values from
 * map-like tables used across Arcb (e.g. vtables/ftables/ctables), with
 * support for:
 * - semantic attribute filtering
 * - profile-aware key mangling lookup
 * - in-place alignment of profile/OS-specialized entries
 */

/**
 * @defgroup Table Table Utilities
 * @brief Generic helper utilities for Arcb table-like containers.
 *
 * Provides attribute-aware accessors and alignment helpers for map-like
 * containers used by Arcb semantic/runtime layers.
 */

/**
 * @addtogroup Table
 * @{
 */

BEGIN_MODULE(Semantic::Attr)

enum class Type;

END_MODULE(Semantic::Attr)







BEGIN_MODULE(Table)




//    ██████╗ ██████╗ ██╗██╗   ██╗ █████╗ ████████╗███████╗    ███████╗██╗   ██╗███╗   ██╗ ██████╗███████╗
//    ██╔══██╗██╔══██╗██║██║   ██║██╔══██╗╚══██╔══╝██╔════╝    ██╔════╝██║   ██║████╗  ██║██╔════╝██╔════╝
//    ██████╔╝██████╔╝██║██║   ██║███████║   ██║   █████╗      █████╗  ██║   ██║██╔██╗ ██║██║     ███████╗
//    ██╔═══╝ ██╔══██╗██║╚██╗ ██╔╝██╔══██║   ██║   ██╔══╝      ██╔══╝  ██║   ██║██║╚██╗██║██║     ╚════██║
//    ██║     ██║  ██║██║ ╚████╔╝ ██║  ██║   ██║   ███████╗    ██║     ╚██████╔╝██║ ╚████║╚██████╗███████║
//    ╚═╝     ╚═╝  ╚═╝╚═╝  ╚═══╝  ╚═╝  ╚═╝   ╚═╝   ╚══════╝    ╚═╝      ╚═════╝ ╚═╝  ╚═══╝ ╚═════╝╚══════╝
//                                                                                                        


/**
 * @brief Checks whether a mapped value exposes a given semantic attribute.
 *
 * This overload expects the mapped type to provide:
 * `bool hasAttribute(Arcb::Semantic::Attr::Type) const;`
 *
 * @tparam T Mapped type.
 * @param value Mapped value.
 * @param attr Attribute to query.
 * @return true if the attribute is present.
 */
template <typename T>
inline static bool HasAttrOnMapped(const T& value, const Arcb::Semantic::Attr::Type attr)
{
    return value.hasAttribute(attr);
}



/**
 * @brief Checks whether a pointer mapped value exposes a given semantic attribute.
 *
 * @tparam T Pointee type.
 * @param ptr Pointer to mapped value (may be null).
 * @param attr Attribute to query.
 * @return true if ptr is non-null and the attribute is present.
 */
template <typename T>
inline static bool HasAttrOnMapped(T* ptr, const Arcb::Semantic::Attr::Type attr)
{
    return ptr && ptr->hasAttribute(attr);
}



/**
 * @brief Checks whether a shared_ptr mapped value exposes a given semantic attribute.
 *
 * @tparam T Pointee type.
 * @param ptr Shared pointer (may be null).
 * @param attr Attribute to query.
 * @return true if ptr is non-null and the attribute is present.
 */
template <typename T>
inline static bool HasAttrOnMapped(const std::shared_ptr<T>& ptr, const Arcb::Semantic::Attr::Type attr)
{
    return ptr && ptr->hasAttribute(attr);
}



/**
 * @brief Checks whether a unique_ptr mapped value exposes a given semantic attribute.
 *
 * @tparam T Pointee type.
 * @param ptr Unique pointer (may be null).
 * @param attr Attribute to query.
 * @return true if ptr is non-null and the attribute is present.
 */
template <typename T>
inline static bool HasAttrOnMapped(const std::unique_ptr<T>& ptr, const Arcb::Semantic::Attr::Type attr)
{
    return ptr && ptr->hasAttribute(attr);
}







//    ██████╗ ███████╗███████╗██╗  ██╗     ██████╗ ██████╗  ██████╗ ██╗   ██╗██████╗ 
//    ██╔══██╗██╔════╝██╔════╝██║ ██╔╝    ██╔════╝ ██╔══██╗██╔═══██╗██║   ██║██╔══██╗
//    ██████╔╝█████╗  █████╗  █████╔╝     ██║  ███╗██████╔╝██║   ██║██║   ██║██████╔╝
//    ██╔═══╝ ██╔══╝  ██╔══╝  ██╔═██╗     ██║   ██║██╔══██╗██║   ██║██║   ██║██╔═══╝ 
//    ██║     ███████╗███████╗██║  ██╗    ╚██████╔╝██║  ██║╚██████╔╝╚██████╔╝██║     
//    ╚═╝     ╚══════╝╚══════╝╚═╝  ╚═╝     ╚═════╝ ╚═╝  ╚═╝ ╚═════╝  ╚═════╝ ╚═╝     
//                                                                                   



/**
 * @brief Returns the first table entry whose mapped value has a given attribute.
 *
 * @tparam TABLE Map-like container type.
 * @param table Table to scan.
 * @param attr Attribute to match.
 * @return reference to the mapped value if found, std::nullopt otherwise.
 */
template <typename TABLE>
std::optional<std::reference_wrapper<typename TABLE::mapped_type>>
GetValue(TABLE& table, const Arcb::Semantic::Attr::Type attr)
{
    for (auto& [k, v] : table)
    {
        if (HasAttrOnMapped(v, attr))
        {
            return std::ref(v);
        }
    }

    return std::nullopt;
}



/**
 * @brief Returns all table entries whose mapped values have a given attribute.
 *
 * @tparam TABLE Map-like container type.
 * @param table Table to scan.
 * @param attr Attribute to match.
 * @return vector of references if at least one match exists, std::nullopt otherwise.
 */
template <typename TABLE>
std::optional<std::vector<std::reference_wrapper<typename TABLE::mapped_type>>>
GetValues(TABLE& table, const Arcb::Semantic::Attr::Type attr)
{
    std::vector<std::reference_wrapper<typename TABLE::mapped_type>> vec;

    for (auto& [k, v] : table)
    {
        if (HasAttrOnMapped(v, attr))
        {
            vec.push_back(std::ref(v));
        }
    }

    if (vec.empty())
    {
        return std::nullopt;
    }

    return vec;
}



/**
 * @brief Looks up a key in a table, with optional profile-based mangling fallback.
 *
 * If `key` is not present, attempts lookup using a mangled key for each profile
 * in the provided list (via Support::generate_mangling()).
 *
 * @tparam TABLE Map-like container type.
 * @param table Table to query.
 * @param key Base key.
 * @param profiles Ordered list of profiles to try.
 * @return reference to the mapped value if found, std::nullopt otherwise.
 */
template <typename TABLE>
std::optional<std::reference_wrapper<typename TABLE::mapped_type>>
GetValue(TABLE& table, const typename TABLE::key_type& key, const std::vector<std::string>& profiles)
{
    auto it = table.find(key);

    if (it == table.end())
    {
        for (const auto& profile : profiles)
        {
            const auto mangled = Support::generate_mangling(key, profile);

            it = table.find(mangled);

            if (it != table.end())
            {
                return std::ref(it->second);
            }
        }
    }
    else
    {
        return std::ref(it->second);
    }

    return std::nullopt;
}



/**
 * @brief Looks up a key with profile-based mangling fallback and attribute filtering.
 *
 * @tparam TABLE Map-like container type.
 * @param table Table to query.
 * @param key Base key.
 * @param profiles Ordered list of profiles to try.
 * @param attr Required attribute.
 * @return reference to the mapped value if found, std::nullopt otherwise.
 */
template <typename TABLE>
std::optional<std::reference_wrapper<typename TABLE::mapped_type>>
GetValue(TABLE& table, const typename TABLE::key_type& key, const std::vector<std::string>& profiles, const Arcb::Semantic::Attr::Type attr)
{
    auto it = table.find(key);
    if (it != table.end() && HasAttrOnMapped(it->second, attr))
    {
        return std::ref(it->second);
    }

    for (const auto& profile : profiles)
    {
        const auto mangled = Support::generate_mangling(key, profile);

        it = table.find(mangled);
        if (it != table.end() && HasAttrOnMapped(it->second, attr))
        {
            return std::ref(it->second);
        }
    }

    return std::nullopt;
}



/**
 * @brief Looks up a key in a table, with a single profile mangling fallback.
 *
 * @tparam TABLE Map-like container type.
 * @param table Table to query.
 * @param key Base key.
 * @param profile Profile to use for mangling fallback.
 * @return reference to the mapped value if found, std::nullopt otherwise.
 */
template <typename TABLE>
std::optional<std::reference_wrapper<typename TABLE::mapped_type>>
GetValue(TABLE& table, const typename TABLE::key_type& key, const std::string& profile)
{
    auto it = table.find(key);

    if (it == table.end())
    {
        const auto mangled = Support::generate_mangling(key, profile);

        it = table.find(mangled);

        if (it != table.end())
        {
            return std::ref(it->second);
        }
    }
    else
    {
        return std::ref(it->second);
    }

    return std::nullopt;
}



/**
 * @brief Looks up a key with a single profile mangling fallback and attribute filtering.
 *
 * @tparam TABLE Map-like container type.
 * @param table Table to query.
 * @param key Base key.
 * @param profile Profile to use for mangling fallback.
 * @param attr Required attribute.
 * @return reference to the mapped value if found, std::nullopt otherwise.
 */
template <typename TABLE>
std::optional<std::reference_wrapper<typename TABLE::mapped_type>>
GetValue(TABLE& table, const typename TABLE::key_type& key, const std::string& profile, const Arcb::Semantic::Attr::Type attr)
{
    auto it = table.find(key);

    if (it != table.end() && HasAttrOnMapped(it->second, attr))
    {
        return std::ref(it->second);
    }

    const auto mangled = Support::generate_mangling(key, profile);

    it = table.find(mangled);
    if (it != table.end() && HasAttrOnMapped(it->second, attr))
    {
        return std::ref(it->second);
    }

    return std::nullopt;
}



/**
 * @brief Returns all entries that match an attribute under a given profile.
 *
 * For each key currently in the table, resolves the effective entry for the
 * given profile (with optional mangling fallback) and filters by attribute.
 *
 * @tparam TABLE Map-like container type.
 * @param table Table to query.
 * @param profile Active profile.
 * @param attr Required attribute.
 * @return Vector of mapped value references (possibly empty).
 */
template <typename TABLE>
std::vector<std::reference_wrapper<typename TABLE::mapped_type>>
GetValues(TABLE& table, const std::string& profile, const Arcb::Semantic::Attr::Type attr)
{
    std::vector<std::reference_wrapper<typename TABLE::mapped_type>> vec;

    for (const auto& [k, v] : table)
    {
        auto result = GetValue(table, k, profile, attr);

        if (result)
        {
            vec.push_back(result.value());
        }
    }

    return vec;
}



/**
 * @brief Returns all entries that match an attribute under any of the provided profiles.
 *
 * For each key currently in the table, resolves the effective entry by trying
 * profiles in order, then filters by attribute.
 *
 * @tparam TABLE Map-like container type.
 * @param table Table to query.
 * @param profiles Ordered profile list.
 * @param attr Required attribute.
 * @return Vector of mapped value references (possibly empty).
 */
template <typename TABLE>
std::vector<std::reference_wrapper<typename TABLE::mapped_type>>
GetValues(TABLE& table, const std::vector<std::string>& profiles, const Arcb::Semantic::Attr::Type attr)
{
    std::vector<std::reference_wrapper<typename TABLE::mapped_type>> vec;

    for (const auto& [k, v] : table)
    {
        auto result = GetValue(table, k, profiles, attr);

        if (result)
        {
            vec.push_back(result.value());
        }
    }

    return vec;
}







//    ████████╗ █████╗ ██╗  ██╗███████╗     ██████╗ ██████╗  ██████╗ ██╗   ██╗██████╗ 
//    ╚══██╔══╝██╔══██╗██║ ██╔╝██╔════╝    ██╔════╝ ██╔══██╗██╔═══██╗██║   ██║██╔══██╗
//       ██║   ███████║█████╔╝ █████╗      ██║  ███╗██████╔╝██║   ██║██║   ██║██████╔╝
//       ██║   ██╔══██║██╔═██╗ ██╔══╝      ██║   ██║██╔══██╗██║   ██║██║   ██║██╔═══╝ 
//       ██║   ██║  ██║██║  ██╗███████╗    ╚██████╔╝██║  ██║╚██████╔╝╚██████╔╝██║     
//       ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝     ╚═════╝ ╚═╝  ╚═╝ ╚═════╝  ╚═════╝ ╚═╝     
//                                                                                    



/**
 * @brief Removes and returns a value from the table using profile mangling fallback.
 *
 * @tparam TABLE Map-like container type.
 * @param table Table to modify.
 * @param key Base key.
 * @param profile Profile for mangling fallback.
 * @return Extracted mapped value if found, std::nullopt otherwise.
 */
template <typename TABLE>
std::optional<typename TABLE::mapped_type>
TakeValue(TABLE& table, const typename TABLE::key_type& key, const std::string& profile)
{
    auto it = table.find(key);

    if (it != table.end())
    {
        typename TABLE::mapped_type value = std::move(it->second);
        table.erase(it);
        return value;
    }

    const auto mangled = Support::generate_mangling(key, profile);

    it = table.find(mangled);

    if (it != table.end())
    {
        typename TABLE::mapped_type value = std::move(it->second);
        table.erase(it);
        return value;
    }

    return std::nullopt;
}



/**
 * @brief Removes and returns a value from the table using multiple profiles fallback.
 *
 * @tparam TABLE Map-like container type.
 * @param table Table to modify.
 * @param key Base key.
 * @param profiles Ordered profiles list.
 * @return Extracted mapped value if found, std::nullopt otherwise.
 */
template <typename TABLE>
std::optional<typename TABLE::mapped_type>
TakeValue(TABLE& table, const typename TABLE::key_type& key, const std::vector<std::string>& profiles)
{
    auto it = table.find(key);

    if (it != table.end())
    {
        typename TABLE::mapped_type value = std::move(it->second);
        table.erase(it);
        return value;
    }

    for (const auto& profile : profiles)
    {
        const auto mangled = Support::generate_mangling(key, profile);

        it = table.find(mangled);

        if (it != table.end())
        {
            typename TABLE::mapped_type value = std::move(it->second);
            table.erase(it);
            return value;
        }
    }

    return std::nullopt;
}



/**
 * @brief Removes and returns a value matching an attribute using a single profile fallback.
 *
 * @tparam TABLE Map-like container type.
 * @param table Table to modify.
 * @param key Base key.
 * @param profile Profile for mangling fallback.
 * @param attr Required attribute.
 * @return Extracted mapped value if found, std::nullopt otherwise.
 */
template <typename TABLE>
std::optional<typename TABLE::mapped_type>
TakeValue(TABLE& table, const typename TABLE::key_type& key, const std::string& profile, const Arcb::Semantic::Attr::Type attr)
{
    auto it = table.find(key);

    if (it != table.end() && HasAttrOnMapped(it->second, attr))
    {
        typename TABLE::mapped_type value = std::move(it->second);
        table.erase(it);
        return value;
    }

    const auto mangled = Support::generate_mangling(key, profile);

    it = table.find(mangled);

    if (it != table.end() && HasAttrOnMapped(it->second, attr))
    {
        typename TABLE::mapped_type value = std::move(it->second);
        table.erase(it);
        return value;
    }

    return std::nullopt;
}



/**
 * @brief Removes and returns a value matching an attribute using multiple profiles fallback.
 *
 * @tparam TABLE Map-like container type.
 * @param table Table to modify.
 * @param key Base key.
 * @param profiles Ordered profiles list.
 * @param attr Required attribute.
 * @return Extracted mapped value if found, std::nullopt otherwise.
 */
template <typename TABLE>
std::optional<typename TABLE::mapped_type>
TakeValue(TABLE& table, const typename TABLE::key_type& key, const std::vector<std::string>& profiles, const Arcb::Semantic::Attr::Type attr)
{
    auto it = table.find(key);

    if (it != table.end() && HasAttrOnMapped(it->second, attr))
    {
        typename TABLE::mapped_type value = std::move(it->second);
        table.erase(it);
        return value;
    }

    for (const auto& profile : profiles)
    {
        const auto mangled = Support::generate_mangling(key, profile);

        it = table.find(mangled);

        if (it != table.end() && HasAttrOnMapped(it->second, attr))
        {
            typename TABLE::mapped_type value = std::move(it->second);
            table.erase(it);
            return value;
        }
    }

    return std::nullopt;
}



/**
 * @brief Removes and returns all values matching an attribute under a given profile.
 *
 * @tparam TABLE Map-like container type.
 * @param table Table to modify.
 * @param profile Active profile.
 * @param attr Required attribute.
 * @return Extracted mapped values (possibly empty).
 */
template <typename TABLE>
std::vector<typename TABLE::mapped_type>
TakeValues(TABLE& table, const std::string& profile, const Arcb::Semantic::Attr::Type attr)
{
    std::vector<typename TABLE::mapped_type> vec;

    for (auto it = table.begin(); it != table.end(); )
    {
        auto key = it->first;
        ++it;
        auto result = TakeValue(table, key, profile, attr);

        if (result)
        {
            vec.push_back(result.value());
        }
    }

    return vec;
}



/**
 * @brief Removes and returns all values matching an attribute under any of the provided profiles.
 *
 * @tparam TABLE Map-like container type.
 * @param table Table to modify.
 * @param profiles Ordered profiles list.
 * @param attr Required attribute.
 * @return Extracted mapped values (possibly empty).
 */
template <typename TABLE>
std::vector<typename TABLE::mapped_type>
TakeValues(TABLE& table, const std::vector<std::string>& profiles, const Arcb::Semantic::Attr::Type attr)
{
    std::vector<typename TABLE::mapped_type> vec;

    for (auto it = table.begin(); it != table.end(); )
    {
        auto key = it->first;
        ++it;
        auto result = TakeValue(table, key, profiles, attr);

        if (result)
        {
            vec.push_back(result.value());
        }
    }

    return vec;
}






//    ██████╗ ██╗   ██╗██████╗ ██╗     ██╗ ██████╗    ███████╗██╗   ██╗███╗   ██╗ ██████╗███████╗
//    ██╔══██╗██║   ██║██╔══██╗██║     ██║██╔════╝    ██╔════╝██║   ██║████╗  ██║██╔════╝██╔════╝
//    ██████╔╝██║   ██║██████╔╝██║     ██║██║         █████╗  ██║   ██║██╔██╗ ██║██║     ███████╗
//    ██╔═══╝ ██║   ██║██╔══██╗██║     ██║██║         ██╔══╝  ██║   ██║██║╚██╗██║██║     ╚════██║
//    ██║     ╚██████╔╝██████╔╝███████╗██║╚██████╗    ██║     ╚██████╔╝██║ ╚████║╚██████╗███████║
//    ╚═╝      ╚═════╝ ╚═════╝ ╚══════╝╚═╝ ╚═════╝    ╚═╝      ╚═════╝ ╚═╝  ╚═══╝ ╚═════╝╚══════╝
//                                                                                               




/**
 * @brief Returns the list of keys in a map-like table.
 *
 * @tparam TABLE Map-like container type.
 * @param table Source table.
 * @return Vector of keys.
 */
template <typename TABLE>
std::vector<typename TABLE::key_type>
Keys(const TABLE& table)
{
    std::vector<typename TABLE::key_type> keys;

    for (const auto& [k, v] : table)
    {
        keys.push_back(k);
    }

    return keys;
}



/**
 * @brief Aligns a table to a specific profile, resolving mangled keys in-place.
 *
 * Entries marked with the PROFILE attribute and using a mangled key format
 * (containing "@@") are filtered and reduced to their base key:
 * - keys for non-matching profiles are erased
 * - matching profile entries are moved onto the base key (replacing it if present)
 *
 * @tparam TABLE Map-like container type.
 * @param table Table to modify.
 * @param profile Selected profile name.
 */
template <typename TABLE>
void
AlignOnProfile(TABLE& table, const std::string& profile)
{
    using Key = typename TABLE::key_type;

    std::vector<Key> mangled_keys;
    mangled_keys.reserve(table.size());

    for (const auto& [k, v] : table)
    {
        if (HasAttrOnMapped(v, Arcb::Semantic::Attr::Type::PROFILE))
        {
            auto pos = k.find("@@");
            if (pos != std::string::npos)
            {
                mangled_keys.push_back(k);
            }
        }
    }

    for (const auto& mk : mangled_keys)
    {
        auto it = table.find(mk);

        if (it == table.end()) continue;

        const Key& key = it->first;
        const auto pos = key.find("@@");
        if (pos == std::string::npos) continue;

        const std::string base     = key.substr(0, pos);
        const std::string prof_key = key.substr(pos + 2);

        if (prof_key != profile)
        {
            table.erase(it);
            continue;
        }

        auto base_it = table.find(base);
        if (base_it != table.end())
        {
            base_it->second = std::move(it->second);
            table.erase(it);
        }
        else
        {
            auto node = table.extract(it);
            node.key() = base;
            table.insert(std::move(node));
        }
    }
}



/**
 * @brief Aligns a table to the current operating system, resolving mangled keys in-place.
 *
 * Entries marked with the IFOS attribute and using a mangled key format
 * (containing "@@") are filtered and reduced to their base key:
 * - keys for non-matching OS identifiers are erased
 * - matching OS entries are moved onto the base key (replacing it if present)
 *
 * The current OS identifier is obtained through Core::symbol(Core::SymbolType::OS).
 *
 * @tparam TABLE Map-like container type.
 * @param table Table to modify.
 */
template <typename TABLE>
void
AlignOnOS(TABLE& table)
{
    using Key = typename TABLE::key_type;

    std::vector<Key> mangled_keys;
    mangled_keys.reserve(table.size());

    for (const auto& [k, v] : table)
    {
        if (HasAttrOnMapped(v, Arcb::Semantic::Attr::Type::IFOS))
        {
            auto pos = k.find("@@");
            if (pos != std::string::npos)
            {
                mangled_keys.push_back(k);
            }
        }
    }

    for (const auto& mk : mangled_keys)
    {
        auto it = table.find(mk);

        if (it == table.end()) continue;

        const Key& key = it->first;
        const auto pos = key.find("@@");
        if (pos == std::string::npos) continue;

        const std::string base   = key.substr(0, pos);
        const std::string os_key = key.substr(pos + 2);

        if (os_key != Core::first_symbol(Core::SymbolType::OS))
        {
            table.erase(it);
            continue;
        }

        auto base_it = table.find(base);
        if (base_it != table.end())
        {
            base_it->second = std::move(it->second);
            table.erase(it);
        }
        else
        {
            auto node = table.extract(it);
            node.key() = base;
            table.insert(std::move(node));
        }
    }
}



END_MODULE(Table)

/** @} */

#endif /* __ARCB_UTIL_TABLE_HELPER__H__ */
