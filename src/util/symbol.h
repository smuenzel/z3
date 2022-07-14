/*++
Copyright (c) 2006 Microsoft Corporation

Module Name:

    symbol.h

Abstract:

    Lisp-like symbols.

Author:

    Leonardo de Moura (leonardo) 2006-09-11.

Revision History:

--*/
#pragma once
#include <climits>
#include <string>
#include <ostream>

#include "util/hash.h"
#include "util/util.h"
#include "util/tptr.h"
#include "util/string_buffer.h"

template<typename T>
class symbol_table;

class symbol {
    union { 
      char const * m_data;
      size_t const * m_data_as_size_t;
    };

    template<typename T>
    friend class symbol_table;

    void initialize_add_to_table() const;

    explicit symbol(void const * data):
        m_data(reinterpret_cast<char const *>(data)) {
    }

    constexpr explicit symbol(size_t const* raw_data) : m_data_as_size_t(raw_data + 1) {

    }

    bool is_marked() const {
        return GET_TAG(m_data) > 1;
    }
    static symbol mark(symbol s) {
        SASSERT(!s.is_marked());
        return symbol(TAG(void *, UNTAG(void *, s.m_data), GET_TAG(s.m_data) + 2));
    }
    static symbol unmark(symbol s) {
        SASSERT(s.is_marked());
        return symbol(TAG(void *, UNTAG(void *, s.m_data), GET_TAG(s.m_data) - 2));
    }
    static symbol m_dummy;
public:
    symbol():
        m_data(nullptr) {
    }
    explicit symbol(char const * d);
    explicit symbol(const std::string & str) : symbol(str.c_str()) {}
    explicit symbol(unsigned idx):
        m_data(BOXTAGINT(char const *, idx, 1)) {
#if !defined(__LP64__) && !defined(_WIN64)
        SASSERT(idx < (SIZE_MAX >> PTR_ALIGNMENT));
#endif
    }
    static symbol dummy() { return m_dummy; }
    static const symbol null;
    symbol & operator=(char const * d);
    friend bool operator==(symbol const & s1, symbol const & s2) { return s1.m_data == s2.m_data; }
    friend bool operator!=(symbol const & s1, symbol const & s2) { return s1.m_data != s2.m_data; }
    bool is_numerical() const { return GET_TAG(m_data) == 1; }
    bool is_null() const { return m_data == nullptr; }
    bool is_non_empty_string() const { return !is_null() && !is_numerical() && 0 != bare_str()[0]; }
    unsigned int get_num() const { SASSERT(is_numerical()); return UNBOXINT(m_data); }
    std::string str() const;
    friend bool operator==(symbol const & s1, char const * s2) {
        if (s1.m_data == nullptr && s2 == nullptr)
            return true;
        if (s1.m_data == nullptr || s2 == nullptr)
            return false;
        if (!s1.is_numerical())
            return strcmp(s1.bare_str(), s2) == 0;
        return s1.str() == s2;
    }
    friend bool operator!=(symbol const & s1, char const * s2) { return !operator==(s1, s2); }
    
    // C-API only functions
    void * c_api_symbol2ext() const { return const_cast<char*>(m_data); }
    static symbol c_api_ext2symbol(void const * ptr) { 
        return symbol(ptr);
    }
    unsigned hash() const { 
        if (m_data == nullptr) return 0x9e3779d9;
        else if (is_numerical()) return get_num(); 
        else return static_cast<unsigned>(m_data_as_size_t[-1]);
    }
    bool contains(char c) const;
    unsigned display_size() const;
    char const * bare_str() const { SASSERT(!is_numerical()); return m_data; }
    friend std::ostream & operator<<(std::ostream & target, symbol s) {
        SASSERT(!s.is_marked());
        if (GET_TAG(s.m_data) == 0) {
            if (s.m_data) {
                target << s.m_data;
            }
            else {
                target << "null";
            }
        }
        else {
            target << "k!" << UNBOXINT(s.m_data);
        }
        return target;
    }
    template<unsigned SZ>
    friend string_buffer<SZ> & operator<<(string_buffer<SZ> & target, symbol s) {
        SASSERT(!s.is_marked());
        if (GET_TAG(s.m_data) == 0) {
            if (s.m_data) {
                target << s.m_data;
            }
            else {
                target << "null";
            }
        }
        else {
            target << "k!" << UNBOXINT(s.m_data);
        }
        return target;
    }

    template<char ...> friend struct static_symbol;
};

struct symbol_hash_proc {
    unsigned operator()(symbol const & s) const { 
        return s.hash();
    }
};

struct symbol_eq_proc {
    bool operator()(symbol const & s1, symbol const & s2) const {
        return s1 == s2;
    }
};

void initialize_symbols();
void finalize_symbols();
/*
  ADD_INITIALIZER('initialize_symbols();')
  ADD_FINALIZER('finalize_symbols();')
*/

// total order on symbols... I did not overloaded '<' to avoid misunderstandings.
// numerical symbols are smaller than non numerical symbols.
// two numerical symbols are compared using get_num.
// two non-numerical symbols are compared using string comparison.
bool lt(symbol const & s1, symbol const & s2);

template<char ...S> struct static_symbol {
    static_assert(sizeof(size_t) == 8);
    struct hash_c { size_t hash; char c[sizeof...(S) + 1]; };
    static constexpr char raw_string[] = { S..., 0};
    static constexpr size_t hash =  (size_t)string_hash(raw_string ,sizeof...(S),17);
    static constexpr hash_c value { hash, {S...,0} } ;
    static constexpr size_t const * x = &value.hash;
    static constexpr symbol sym = symbol(x);
    static void __attribute__((constructor)) construct(){
      sym.initialize_add_to_table();
    }
};

template<typename charT, charT ...S> constexpr const symbol& operator""_symbol() {
    static_assert(std::is_same<charT, char>(), "string char type mismatch");
    return static_symbol<S...>::sym;
};

static symbol qxxx = "hello"_symbol;

