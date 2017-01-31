/*
 * Copyright (c) 2017, Benichou Software
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the author nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef STDX_FORMAT_H_INCLUDED
#define STDX_FORMAT_H_INCLUDED

#include <string>
#include <sstream>
#include <iomanip>

namespace stdx
{
	template<typename _CharT, typename Traits = std::char_traits<_CharT> >
	class basic_format
	{
	public:
		typedef _CharT char_type;
		typedef Traits traits_type;
		typedef std::basic_string<_CharT, Traits> string_type;

		explicit basic_format(const string_type& strFormat = string_type())
			: m_oss(m_ossP), m_strFormat(strFormat), m_ulPos(0), m_ioFlags(m_oss.flags()) {}
		basic_format(std::basic_ostream<_CharT, Traits>& oss, const string_type& strFormat = string_type())
			: m_oss(oss), m_strFormat(strFormat), m_ulPos(0), m_ioFlags(m_oss.flags()) {}
		~basic_format() { flush(); }

		template <typename T> basic_format& arg(const T& t) { return operator()<T>(t); }

		operator string_type() const { return str(); }
		inline string_type str() const { flush(); return m_ossP.str(); }

		basic_format& hex(const void* pHex, size_t ulSize,
			const string_type& strDelim = string_type(1, traits_type::to_char_type(' ')))
		{
			parseNextFlag();
			m_oss << std::hex << std::setfill(traits_type::to_char_type('0')) << std::uppercase;

			for (size_t i = 0; i < ulSize; i++)
			{
				if (i > 0)
					m_oss << strDelim;
				m_oss << std::setw(2) << traits_type::to_int_type(static_cast<const char*>(pHex)[i] & 0xFF);
			}

			return *this;
		}

		basic_format& hex(const string_type& strHex,
			const string_type& strDelim = string_type(1, traits_type::to_char_type(' ')))
		{
			return hex(strHex.data(), strHex.size() * sizeof(char_type), strDelim);
		}

		basic_format& mem(const void* pMem, size_t ulSize, size_t ulLine = 16)
		{
			static const string_type spacer(3, traits_type::to_char_type(' '));

			parseNextFlag();
			size_t ulTmpPos = 0;

			while (ulSize)
			{
				m_oss << std::hex << std::setfill(traits_type::to_char_type('0')) << std::setw(sizeof(size_t) * 2) << std::uppercase << reinterpret_cast<long>(static_cast<const char*>(pMem) + ulTmpPos) << ": ";
		//		m_oss << std::hex << std::setfill(traits_type::to_char_type('0')) << std::setw(sizeof(size_t) * 2) << std::uppercase << ulTmpPos << ": ";

				size_t ulTmpSize = ulSize;
				if (ulTmpSize > ulLine)
					ulTmpSize = ulLine;

				string_type strAscii;
				strAscii.reserve(ulTmpSize);

				for (size_t i = 0; i < ulTmpSize; i++)
				{
					char cTmp = static_cast<const char*>(pMem)[ulTmpPos];
					m_oss << std::setw(2) << traits_type::to_int_type(cTmp & 0xFF) << traits_type::to_char_type(' ');

					if (cTmp > 31)
						strAscii += cTmp;
					else
						strAscii += '.';

					ulTmpPos++;
				}

				for (size_t i = ulTmpSize; i < ulLine; i++)
					m_oss << spacer;

				m_oss << strAscii << std::endl;
				ulSize -= ulTmpSize;
			}

			return *this;
		}

		basic_format& mem(const string_type& strMem, size_t ulLine = 16)
		{
			return mem(strMem.data(), strMem.size() * sizeof(char_type), ulLine);
		}

		template <typename InputIterator> basic_format& array(InputIterator first, InputIterator last,
			const string_type& strDelim = string_type(1, traits_type::to_char_type(' ')))
		{
			parseNextFlag();

			for (InputIterator it = first; it != last; ++it)
			{
				if(it != first)
					m_oss << strDelim;
				m_oss << *it;
			}

			return *this;
		}

		template <typename InputIterator>
		basic_format& map(InputIterator first, InputIterator last,
			const string_type& strDelim = string_type(1, traits_type::to_char_type('\n')),
			const string_type& strSeparator = string_type(1, traits_type::to_char_type('=')))
		{
			parseNextFlag();

			for (InputIterator it = first; it != last; ++it)
			{
				if (it != first)
					m_oss << strDelim;
				m_oss << it->first << strSeparator << it->second;
			}

			return *this;
		}

		// Generic operator()
		template <typename T>
		basic_format& operator ()(const T& t)
		{
			int iTruncate = parseNextFlag();
			if (iTruncate != -1)
			{
				std::basic_ostringstream<_CharT, Traits> ossTmp;
				ossTmp << t;
				m_oss << ossTmp.str().substr(0, static_cast<size_t>(iTruncate));
			}
			else
			{
				m_oss << t;
			}

			return *this;
		}


		basic_format(basic_format&) = delete;
		basic_format& operator=(basic_format&) = delete;

	protected:
		void flush() const
		{
			m_oss.flags(m_ioFlags);

			if (m_ulPos != m_strFormat.size())
			{
				m_oss << m_strFormat.substr(m_ulPos);
				m_ulPos = m_strFormat.size();
			}
		}

	private:
		int parseNextFlag() // Return truncate flag value
		{
			static const _CharT szNumeric[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 0 };

			size_t ulPos = m_strFormat.find('%', m_ulPos);
			if (ulPos == string_type::npos)
			{
				flush();
				return -1;
			}

			int iPrecision = -1;
			int iTruncate = -1;

			size_t ulFirstPos = m_ulPos;
			m_ulPos = ulPos;

			m_oss.flags(m_ioFlags);
			m_oss << m_strFormat.substr(ulFirstPos, m_ulPos - ulFirstPos);

			++m_ulPos;

			// Parse Flag
			switch (m_strFormat[m_ulPos])
			{
			case '\'': // no effect yet
				++m_ulPos;
				break;
			case '-':
				m_oss << std::left;
				++m_ulPos;
				break;
			case '_':
				m_oss << std::internal;
				++m_ulPos;
				break;
			case ' ':
				m_oss << std::setfill(traits_type::to_char_type(' '));
				++m_ulPos;
				break;
			case '+':
				m_oss << std::showpos;
				++m_ulPos;
				break;
			case '0':
				m_oss << std::setfill(traits_type::to_char_type('0'));
				++m_ulPos;
				break;
			case '#':
				m_oss << std::showpoint << std::showbase;
				++m_ulPos;
				break;
			case '%':
				m_oss << '%';
				++m_ulPos;
				return parseNextFlag();
			default:
				break;
			}

			// Parse Width
			size_t ulPosWidth = m_strFormat.find_first_not_of(szNumeric, m_ulPos);
			if (ulPosWidth != string_type::npos)
			{
				int iWidth = -1;

				std::basic_istringstream<_CharT, Traits> iss(m_strFormat.substr(m_ulPos, ulPosWidth - m_ulPos));
				iss >> iWidth;

				m_oss << std::setw(iWidth);
				m_ulPos = ulPosWidth;
			}

			// Parse Precision
			if (m_strFormat[m_ulPos] == '.')
			{
				++m_ulPos;
				size_t ulPosPrecision = m_strFormat.find_first_not_of(szNumeric, m_ulPos);
				if (ulPosPrecision != string_type::npos)
				{
					std::basic_istringstream<_CharT, Traits> iss(m_strFormat.substr(m_ulPos, ulPosPrecision - m_ulPos));
					iss >> iPrecision;
					m_oss << std::setprecision(iPrecision);

					m_ulPos = ulPosPrecision;
				}
			}

			// Parse type prefixe
			switch (m_strFormat[m_ulPos])
			{
			case 'l':
			case 'L':
			case 'h':
			case 'I':
				++m_ulPos;
				break;

			default:
				break;
			}

			// Parse Type
			switch (m_strFormat[m_ulPos])
			{
			case 'P':
			case 'X':
				m_oss << std::uppercase;
			case 'p': // pointer => set hex.
			case 'x':
				m_oss << std::hex << std::internal;
				break;

			case 'o':
				m_oss << std::oct;
				break;

			case 'E':
				m_oss << std::uppercase;
			case 'e':
				m_oss << std::scientific;
				m_oss << std::dec;
				break;

			case 'f':
				m_oss << std::fixed;
			case 'u':
			case 'd':
			case 'i':
				m_oss << std::dec << std::noboolalpha;
				break;

			case 'G':
				m_oss << std::uppercase;
			case 'g': // 'g' conversion is default for floats.
				m_oss << std::dec;
				break;

			case 'C':
			case 'c':
				iTruncate = 1;
				break;
			case 'S':
			case 's':
				iTruncate = iPrecision;
				break;

			case 'B': // Boolean
				m_oss << std::uppercase;
			case 'b':
				m_oss << std::boolalpha;
				break;

			case 'Z': // Object
				m_oss << std::uppercase;
			case 'z':
				break;

			default:
				break;
			}

			++m_ulPos;
			return iTruncate;
		}

	private:
		std::basic_ostringstream<_CharT, Traits> m_ossP;
		std::basic_ostream<_CharT, Traits>& m_oss;
		const string_type& m_strFormat;
		mutable size_t m_ulPos;
		std::ios_base::fmtflags m_ioFlags;
	};

	template<typename _CharT, typename Traits = std::char_traits<_CharT> >
	std::basic_ostream<_CharT, Traits>& operator << (std::basic_ostream<_CharT, Traits>& oss, const basic_format<_CharT, Traits>& A)
	{
		oss << A.str();
		return oss;
	}

	typedef basic_format<char>    format;
	typedef basic_format<wchar_t> wformat;
}
#endif // STDX_FORMAT_H_INCLUDED

