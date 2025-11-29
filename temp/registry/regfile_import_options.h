#pragma once

namespace ngbtools
{
	namespace win32
	{
		namespace registry
		{
			/**
			* Parser options for .REG files
			*/
			enum class REGFILE_IMPORT_OPTIONS : uint32_t
			{
				/**
				* No specific options
				*/
				NONE = 0,

				/**
				* Allow Hashtag-style line comments.
				*/
				ALLOW_HASHTAG_COMMENTS = 1,

				/**
				* Allow Semicolon-style line comments.
				*/
				ALLOW_SEMICOLON_COMMENTS = 2,

				/**
				* If this option is set, the parser is more relaxed about whitespaces in the .REG file.
				* Recommended, especially if you manually edit the file yourself...
				*/
				IGNORE_WHITESPACES = 4,

				/**
				* If this option is set, a .REG file can have a statement like this: 'something'=dword:$$VARIABLE$$, where $$VARIABLE$$ is replaced at runtime with the respective -numeric- variable.
				*/
				ALLOW_VARIABLE_NAMES_FOR_NON_STRING_VARIABLES = 8,

			};
			
			constexpr REGFILE_IMPORT_OPTIONS operator|(REGFILE_IMPORT_OPTIONS a, REGFILE_IMPORT_OPTIONS b)
			{
				return static_cast<REGFILE_IMPORT_OPTIONS>(std::to_underlying(a) | std::to_underlying(b));
			}

			constexpr bool has_flag(REGFILE_IMPORT_OPTIONS set, REGFILE_IMPORT_OPTIONS test)
			{
				return (std::to_underlying(set) & std::to_underlying(test)) == std::to_underlying(test);
			}

		} // namespace registry
	} // namespace win32
} // namespace ngbtools

