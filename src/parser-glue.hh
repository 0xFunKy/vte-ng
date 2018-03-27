/*
 * Copyright © 2017, 2018 Christian Persch
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstdint>
#include <algorithm>
#include <string>

#include "parser.hh"

namespace vte {

namespace parser {

class Sequence {
public:

        typedef int number;

        char* ucs4_to_utf8(gunichar const* str,
                           ssize_t len = -1) const noexcept;

        void print() const noexcept;

        /* type:
         *
         *
         * Returns: the type of the sequence, a value from the VTE_SEQ_* enum
         */
        inline constexpr unsigned int type() const noexcept
        {
                return m_seq->type;
        }

        /* command:
         *
         * Returns: the command the sequence codes for, a value
         *   from the VTE_CMD_* enum, or %VTE_CMD_NONE if the command is
         *   unknown
         */
        inline constexpr unsigned int command() const noexcept
        {
                return m_seq->command;
        }

        /* charset:
         *
         * This is the charset to use in a %VTE_CMD_GnDm, %VTE_CMD_GnDMm,
         * %VTE_CMD_CnD or %VTE_CMD_DOCS command.
         *
         * Returns: the charset, a value from the VTE_CHARSET_* enum.
         */
        inline constexpr unsigned int charset() const noexcept
        {
                return m_seq->charset;
        }

        /* intermediates:
         *
         * The intermediate bytes of the ESCAPE, CSI or DCS sequence.
         *
         * Returns: the immediates as flag values from the VTE_SEQ_FLAG_* enum
         */
        inline constexpr unsigned int intermediates() const noexcept
        {
                return m_seq->intermediates;
        }

        /* terminator:
         *
         * This is the character terminating the sequence, or, for a
         * %VTE_SEQ_GRAPHIC sequence, the graphic character.
         *
         * Returns: the terminating character
         */
        inline constexpr uint32_t terminator() const noexcept
        {
                return m_seq->terminator;
        }

        // FIXMEchpe: upgrade to C++17 and use the u32string_view version below, instead
        /*
         * string:
         *
         * This is the string argument of a DCS or OSC sequence.
         *
         * Returns: the string argument
         */
        inline std::u32string string() const noexcept
        {
                size_t len;
                auto buf = vte_seq_string_get(&m_seq->arg_str, &len);
                return std::u32string(reinterpret_cast<char32_t*>(buf), len);
        }

        #if 0
        /*
         * string:
         *
         * This is the string argument of a DCS or OSC sequence.
         *
         * Returns: the string argument
         */
        inline constexpr std::u32string_view string() const noexcept
        {
                size_t len = 0;
                auto buf = vte_seq_string_get(&m_seq->arg_str, &len);
                return std::u32string_view(buf, len);
        }
        #endif

        /*
         * string:
         *
         * This is the string argument of a DCS or OSC sequence.
         *
         * Returns: the string argument
         */
        std::string string_utf8() const noexcept;

        inline char* string_param() const noexcept
        {
                size_t len = 0;
                auto buf = vte_seq_string_get(&m_seq->arg_str, &len);
                return ucs4_to_utf8(buf, len);
        }

        /* size:
         *
         * Returns: the number of parameters
         */
        inline constexpr unsigned int size() const noexcept
        {
                return m_seq->n_args;
        }


        /* size:
         *
         * Returns: the number of parameter blocks, counting runs of subparameters
         *   as only one parameter
         */
        inline constexpr unsigned int size_final() const noexcept
        {
                return m_seq->n_final_args;
        }

        /* capacity:
         *
         * Returns: the number of parameter blocks, counting runs of subparameters
         *   as only one parameter
         */
        inline constexpr unsigned int capacity() const noexcept
        {
                return G_N_ELEMENTS(m_seq->args);
        }

        /* param:
         * @idx:
         * @default_v: the value to use for default parameters
         *
         * Returns: the value of the parameter at index @idx, or @default_v if
         *   the parameter at this index has default value, or the index
         *   is out of bounds
         */
        inline constexpr int param(unsigned int idx,
                                   int default_v = -1) const noexcept
        {
                return __builtin_expect(idx < size(), 1) ? vte_seq_arg_value(m_seq->args[idx], default_v) : default_v;
        }

        /* param:
         * @idx:
         * @default_v: the value to use for default parameters
         * @min_v: the minimum value
         * @max_v: the maximum value
         *
         * Returns: the value of the parameter at index @idx, or @default_v if
         *   the parameter at this index has default value, or the index
         *   is out of bounds. The returned value is clamped to the
         *   range @min_v..@max_v.
         */
        inline constexpr int param(unsigned int idx,
                                   int default_v,
                                   int min_v,
                                   int max_v) const noexcept
        {
                auto v = param(idx, default_v);
                return std::min(std::max(v, min_v), max_v);
        }

        /* param_nonfinal:
         * @idx:
         *
         * Returns: whether the parameter at @idx is nonfinal, i.e.
         * there are more subparameters after it.
         */
        inline constexpr bool param_nonfinal(unsigned int idx) const noexcept
        {
                return __builtin_expect(idx < size(), 1) ? vte_seq_arg_nonfinal(m_seq->args[idx]) : false;
        }

        /* param_default:
         * @idx:
         *
         * Returns: whether the parameter at @idx has default value
         */
        inline constexpr bool param_default(unsigned int idx) const noexcept
        {
                return __builtin_expect(idx < size(), 1) ? vte_seq_arg_default(m_seq->args[idx]) : true;
        }

        /* next:
         * @idx:
         *
         * Returns: the index of the next parameter block
         */
        inline constexpr unsigned int next(unsigned int idx) const noexcept
        {
                /* Find the final parameter */
                while (param_nonfinal(idx))
                        ++idx;
                /* And return the index after that one */
                return ++idx;
        }

        inline constexpr unsigned int cbegin() const noexcept
        {
                return 0;
        }

        inline constexpr unsigned int cend() const noexcept
        {
                return size();
        }

        /* collect:
         *
         * Collects some final parameters.
         *
         * Returns: %true if the sequence parameter list begins with
         *  a run of final parameters that were collected.
         */
        inline constexpr bool collect(unsigned int start_idx,
                                      std::initializer_list<int*> params,
                                      int default_v = -1) const noexcept
        {
                unsigned int idx = start_idx;
                for (auto i : params) {
                        *i = param(idx, default_v);
                        idx = next(idx);
                }

                return (idx - start_idx) == params.size();
        }

        /* collect1:
         * @idx:
         * @default_v:
         *
         * Collects one final parameter.
         *
         * Returns: the parameter value, or @default_v if the parameter has
         *   default value or is not a final parameter
         */
        inline constexpr int collect1(unsigned int idx,
                                      int default_v = -1) const noexcept
        {
                return __builtin_expect(idx < size(), 1) ? vte_seq_arg_value_final(m_seq->args[idx], default_v) : default_v;
        }

        /* collect1:
         * @idx:
         * @default_v:
         * @min_v:
         * @max_v
         *
         * Collects one final parameter.
         *
         * Returns: the parameter value clamped to the @min_v .. @max_v range,
         *   or @default_v if the parameter has default value or is not a final parameter
         */
        inline constexpr int collect1(unsigned int idx,
                                      int default_v,
                                      int min_v,
                                      int max_v) const noexcept
        {
                int v = __builtin_expect(idx < size(), 1) ? vte_seq_arg_value_final(m_seq->args[idx], default_v) : default_v;
                return std::min(std::max(v, min_v), max_v);
        }

        /* collect_subparams:
         *
         * Collects some subparameters.
         *
         * Returns: %true if the sequence parameter list contains enough
         *   subparams at @start_idx
         */
        inline constexpr bool collect_subparams(unsigned int start_idx,
                                                std::initializer_list<int*> params,
                                                int default_v = -1) const noexcept
        {
                unsigned int idx = start_idx;
                for (auto i : params) {
                        *i = param(idx++, default_v);
                }

                return idx <= next(start_idx);
        }

        struct vte_seq** seq_ptr() { return &m_seq; }

        inline explicit operator bool() const { return m_seq != nullptr; }

private:
        struct vte_seq *m_seq{nullptr};

        char const* type_string() const;
        char const* command_string() const;
};

class StringTokeniser {
public:
        using string_type = std::string;
        using char_type = std::string::value_type;

private:
        string_type const& m_string;
        char_type m_separator{';'};

public:
        StringTokeniser(string_type const& s,
                        char_type separator = ';')
                : m_string{s},
                  m_separator{separator}
        {
        }

        StringTokeniser(string_type&& s,
                        char_type separator = ';')
                : m_string{s},
                  m_separator{separator}
        {
        }

        StringTokeniser(StringTokeniser const&) = delete;
        StringTokeniser(StringTokeniser&&) = delete;
        ~StringTokeniser() = default;

        StringTokeniser& operator=(StringTokeniser const&) = delete;
        StringTokeniser& operator=(StringTokeniser&&) = delete;

        /*
         * const_iterator:
         *
         * InputIterator for string tokens.
         */
        class const_iterator {
        public:
                using difference_type = ptrdiff_t;
                using value_type = string_type;
                using pointer = string_type;
                using reference = string_type;
                using iterator_category = std::input_iterator_tag;
                using size_type = string_type::size_type;

        private:
                string_type const* m_string;
                char_type m_separator{';'};
                string_type::size_type m_position;
                string_type::size_type m_next_separator;

        public:
                const_iterator(string_type const* str,
                               char_type separator,
                               size_type position)
                        : m_string{str},
                          m_separator{separator},
                          m_position{position},
                          m_next_separator{m_string->find(m_separator, m_position)}
                {
                }

                const_iterator(string_type const* str,
                               char_type separator)
                        : m_string{str},
                          m_separator{separator},
                          m_position{string_type::npos},
                          m_next_separator{string_type::npos}
                {
                }

                const_iterator(const_iterator const&) = default;
                const_iterator(const_iterator&& o)
                        : m_string{o.m_string},
                          m_separator{o.m_separator},
                          m_position{o.m_position},
                          m_next_separator{o.m_next_separator}
                {
                }

                ~const_iterator() = default;

                const_iterator& operator=(const_iterator const& o)
                {
                        m_string = o.m_string;
                        m_separator = o.m_separator;
                        m_position = o.m_position;
                        m_next_separator = o.m_next_separator;
                        return *this;
                }

                const_iterator& operator=(const_iterator&& o)
                {
                        m_string = std::move(o.m_string);
                        m_separator = o.m_separator;
                        m_position = o.m_position;
                        m_next_separator = o.m_next_separator;
                        return *this;
                }

                inline bool operator==(const_iterator const& o) const noexcept
                {
                        return m_position == o.m_position;
                }

                inline bool operator!=(const_iterator const& o) const noexcept
                {
                        return m_position != o.m_position;
                }

                inline const_iterator& operator++() noexcept
                {
                        if (m_next_separator != string_type::npos) {
                                m_position = ++m_next_separator;
                                m_next_separator = m_string->find(m_separator, m_position);
                        } else
                                m_position = string_type::npos;

                        return *this;
                }

                /*
                 * number:
                 *
                 * Returns the value of the iterator as a number, or -1
                 *   if the string could not be parsed as a number, or
                 *   the parsed values exceeds the uint16_t range.
                 *
                 * Returns: true if a number was parsed
                 */
                bool number(int& v) const noexcept
                {
                        auto const s = size();
                        if (s == 0) {
                                v = -1;
                                return true;
                        }

                        v = 0;
                        size_type i;
                        for (i = 0; i < s; ++i) {
                                char_type c = (*m_string)[m_position + i];
                                if (c < '0' || c > '9')
                                        return false;

                                v = v * 10 + (c - '0');
                                if (v > 0xffff)
                                        return false;
                        }

                        /* All consumed? */
                        return i == s;
                }

                inline size_type size() const noexcept
                {
                        if (m_next_separator != string_type::npos)
                                return m_next_separator - m_position;
                        else
                                return m_string->size() - m_position;
                }

                inline size_type size_remaining() const noexcept
                {
                        return m_string->size() - m_position;
                }

                inline string_type operator*() const noexcept
                {
                        return m_string->substr(m_position, size());
                }

                /*
                 * string_remaining:
                 *
                 * Returns the whole string left, including possibly more separators.
                 */
                inline string_type string_remaining() const noexcept
                {
                        return m_string->substr(m_position);
                }

                inline void append(string_type& str) const noexcept
                {
                        str.append(m_string->substr(m_position, size()));
                }

                inline void append_remaining(string_type& str) const noexcept
                {
                        str.append(m_string->substr(m_position));
                }

        }; // class const_iterator

        inline const_iterator cbegin(char_type c = ';') const noexcept
        {
                return const_iterator(&m_string, m_separator, 0);
        }

        inline const_iterator cend() const noexcept
        {
                return const_iterator(&m_string, m_separator);
        }

        inline const_iterator begin(char_type c = ';') const noexcept
        {
                return cbegin();
        }

        inline const_iterator end() const noexcept
        {
                return cend();
        }

}; // class StringTokeniser

} // namespace parser

} // namespace vte
