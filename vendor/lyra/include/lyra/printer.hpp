// Copyright René Ferdinand Rivera Morell
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LYRA_PRINTER_HPP
#define LYRA_PRINTER_HPP

#include "lyra/option_style.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <ostream>
#include <string>

namespace lyra {

/* tag::reference[]

[#lyra_printer]
= `lyra::printer`

A `printer` is an interface to manage formatting for output. Mostly the output
is for the help text of lyra cli parsers. The interface abstracts away specifics
of the output device and any visual arrangement, i.e. padding, coloring, etc.

[source]
----
virtual printer & printer::heading(
	const option_style & style,
	const std::string & txt) = 0;
virtual printer & printer::paragraph(
	const option_style & style,
	const std::string & txt) = 0;
virtual printer & printer::option(
	const option_style & style,
	const std::string & opt,
	const std::string & description) = 0;
----

Indenting levels is implemented at the base and the indent is available for
concrete implementations to apply as needed.

[source]
----
virtual printer & indent(int levels = 1);
virtual printer & dedent(int levels = 1);
virtual int get_indent_level() const;
----

You can customize the printing output by implementing a subclass of
`lyra::printer` and implementing a corresponding `make_printer` factory
function which matches the output to the printer. For example:

[source]
----
inline std::unique_ptr<my_printer> make_printer(my_output & os_)
{
	return std::unique_ptr<my_printer>(new my_output(os_));
}
----

*/ // end::reference[]
class printer
{
	public:
	virtual ~printer() = default;
	virtual printer & heading(
		const option_style & style, const std::string & txt)
		= 0;
	virtual printer & paragraph(
		const option_style & style, const std::string & txt,
		const std::string &title = {})
		= 0;
	virtual printer & option(const option_style & style,
		const std::string & opt,
		const std::string & description)
		= 0;
	virtual printer & indent(int levels = 1)
	{
		indent_level += levels;
		return *this;
	}
	virtual printer & dedent(int levels = 1)
	{
		indent_level -= levels;
		return *this;
	}
	virtual int get_indent_level() const
	{
		if (indent_level < 0) return 0;
		return indent_level;
	}

	protected:
	int indent_level = 0;
};

/* tag::reference[]

[#lyra_ostream_printer]
= `lyra::ostream_printer`

A <<lyra_printer>> that uses `std::ostream` for output. This is the one used in
the case of printing to the standard output channels of `std::cout` and
`std::cerr`.

*/ // end::reference[]
class ostream_printer : public printer
{
	public:
	explicit ostream_printer(std::ostream & os_)
		: os(os_)
	{}
	printer & heading(const option_style &, const std::string & txt) override
	{
		os << txt << "\n";
		return *this;
	}
	printer & paragraph(
		const option_style & style, const std::string & txt,
		const std::string &title) override
	{
		const std::string indent_str(
			get_indent_level() * style.indent_size, ' ');
		print_wrapped(txt, indent_str + std::string(title.size(), ' '),
			style.max_width, indent_str + title);
		os << "\n";
		return *this;
	}
	printer & option(const option_style & style,
		const std::string & opt,
		const std::string & description) override
	{
		const std::string indent_str(
			get_indent_level() * style.indent_size, ' ');
		const std::string opt_pad(
			26 - get_indent_level() * style.indent_size - 1, ' ');
		if (opt.size() > opt_pad.size())
			os << indent_str << opt << "\n"
			   << indent_str << opt_pad << " ";
		else
			os << indent_str << opt
			   << opt_pad.substr(0, opt_pad.size() - opt.size()) << " ";

		if (description.empty())
		{
			os << "\n";
		}
		else
		{
			std::string desc_indent(26, ' ');
			print_wrapped(description, desc_indent, style.max_width);
		}
		return *this;
	}

	protected:
	void print_wrapped(const std::string& text, const std::string& indent_str,
		size_t max_width, const std::string& first_indent_str = {})
	{
		size_t indent_len = indent_str.length();
		size_t width = max_width > indent_len ? max_width - indent_len : 1;

		size_t pos = 0;
		bool first = true;
		while (pos < text.length())
		{
			if (first)
			{
				os << first_indent_str;
			}
			else
			{
				os << indent_str;
				while (pos < text.length() && text[pos] == ' ') pos++;
				if (pos >= text.length()) break;
			}

			size_t len = std::min(width, text.length() - pos);

			// If there's an explicit newline within the current window,
			// break at that newline (forced line break).
			size_t next_nl = text.find('\n', pos);
			if (next_nl != std::string::npos && next_nl - pos < len)
				len = next_nl - pos;
			else if (pos + len < text.length())
			{
				size_t last_space = text.rfind(' ', pos + len);
				if (last_space != std::string::npos && last_space > pos)
					len = last_space - pos;
			}

			os << text.substr(pos, len) << "\n";
			pos += len;
			// If we broke at an explicit newline, skip that character.
			if (pos < text.length() && text[pos] == '\n') ++pos;
			first = false;
		}
	}

	std::ostream & os;
};

inline std::unique_ptr<printer> make_printer(std::ostream & os_)
{
	return std::unique_ptr<printer>(new ostream_printer(os_));
}

} // namespace lyra

#endif
