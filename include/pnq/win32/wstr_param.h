#pragma once

#include <string>
#include <pnq/string.h>

namespace pnq
{
    namespace win32
    {
        /// Wrapper class for Win32 API string parameters.
        ///
        /// Background: pnq uses UTF-8 internally. But the Windows API is UTF-16LE, so we need to
        /// convert all strings to wide strings before passing them on to the Windows functions.
        /// This wrapper class will do that on the fly for you.
        class wstr_param final
        {
        public:

            /**
             * \brief   Constructor converts UTF8 input to temporary UTF16 object
             *
             * \param   input   The input string. 
             */

			explicit wstr_param(std::string_view input)
				:
                m_value{ string::encode_as_utf16(input) },
                m_is_null{ string::is_empty(input) }
			{
			}

            ~wstr_param() = default;

            /**
             * \brief   Constructor converts UTF8 char pointer to temporary UTF16 object. Can handle null values
             *
             * \param   input   The input.
             */

            explicit wstr_param(const char* input)
                :
                m_value{ string::encode_as_utf16(input) },
                m_is_null{ input == nullptr }
            {
            }

            /**
             * \brief   Cast that converts the string to a LPCWSTR.
             *
             * \return  The wide char memory
             */

            operator LPCWSTR() const
            {
                return m_is_null ? nullptr : m_value.c_str();
            }

			/**
			 * \brief some APIs require LPWSTR even though they leave the input string unchanged.
			 * 		  Case in point: OpenPrinter().
			 *
			 * \return	The result of the operation.
			 */

			operator LPWSTR()
			{
				return m_is_null ? nullptr : const_cast<LPWSTR>(m_value.c_str());
			}

            wstr_param(const wstr_param&) = delete;
            wstr_param& operator=(const wstr_param&) = delete;
            wstr_param(wstr_param&&) = delete;
            wstr_param& operator=(wstr_param&&) = delete;

        private:
            /** \brief   The actual UTF16 string value. */
            std::wstring m_value;
            const bool m_is_null;
        };
   }
}

/** \brief Convenience: this alias makes wstr_param globally visible once you include this header. */
using pnq::win32::wstr_param;
