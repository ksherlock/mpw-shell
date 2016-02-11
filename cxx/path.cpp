
#include "filesystem.h"


namespace filesystem {

	namespace {
		const path::value_type separator = '/';

		// hmmm... these could be pre-studied since they're constant?
		const path path_dot = ".";
		const path path_dotdot = "..";
		const path path_sep = "/";
	}

	void path::study() const {

		if (_info.valid) return;
		int length = _path.length();
		if (length == 0)
		{
			_info.valid = true;
			_info.special = 0;
			_info.stem = _info.extension = 0;
			return;
		}

		auto back = _path[length - 1];

		// check for special cases (part 1)
		// if the path is all /s, filename is /
		// if the path ends with a / (but contains a non-/ char),
		// the filename is .

		if (back == separator) {
			value_type special = '.';
			if (_path.find_first_not_of(separator) == _path.npos)
				special = separator;

			_info.valid = true;
			_info.extension = length;
			_info.stem = length;
			_info.special = special;
			return;
		}

		int stem = 0;
		int extension = length;
		for (int i = length; i; ) {
			auto c = _path[--i];
			if (c == '.' && extension == length)
				extension = i;
			if (c == '/') {
				stem = i + 1;
				break;
			}
		}


		// check for special cases (part 2)
		// ".." and "." are not extensions.
		if (back == '.') {
			int xlength = length - stem;
			if (xlength == 1) 
				extension = length;
			if (xlength == 2 && _path[stem] == '.')
				extension = length;
		}

		_info.valid = true;
		_info.stem = stem;
		_info.extension = extension;
		_info.special = 0;
	}


#if 0
	path::path(const path &rhs) :
		_path(rhs._path), _info(rhs._info)
	{}

	path::path(path &&rhs) noexcept :
		_path(std::move(rhs._path))
	{
		rhs.invalidate();
	}
#endif


	// private.  neither this->_path nor s are empty.
	path &path::append_common(const std::string &s)
	{
		invalidate();
		if (_path.back() != separator && s.front() != separator)
			_path.push_back(separator);

		_path.append(s);

		return *this;
	}

	path& path::append(const path& p)
	{
		if (p.empty()) return *this;

		if (empty()) {
			return (*this = p); 
		}

		invalidate();

		// check for something stupid like xx.append(xx);
		if (&p == this) {
			return append_common(string_type(p._path));
		}

		return append_common(p._path);
	}

	path& path::append(const string_type &s)
	{
		if (s.empty()) return *this;
		invalidate();

		if (empty()) {
			_path = s;
			return *this;
		}


		if (&s == &_path) {
			string_type tmp(s);
			if (_path.back() != separator && tmp[0] != separator)
				_path.push_back(separator);
			_path.append(tmp);
			return *this;
		}

		if (_path.back() != separator && s[0] != separator)
			_path.push_back(separator);

		_path.append(s);

		return *this;
	}




	path path::filename() const {

		if (empty()) return *this;
		if (!_info.valid) study();

		if (_info.special == separator) return path_sep;
		if (_info.special == '.') return path_dot;

		if (_info.stem == 0) return *this;
		return _path.substr(_info.stem);
	}

	path path::stem() const {

		// filename without the extension.

		if (empty()) return *this;
		if (!_info.valid) study();

		if (_info.special == separator) return path_sep;
		if (_info.special == '.') return path_dot;

		return _path.substr(_info.stem, _info.extension - _info.stem);
	}

	path path::extension() const {

		if (empty()) return *this;
		if (!_info.valid) study();

		return _path.substr(_info.extension);
	}


	bool path::has_parent_path() const
	{
		// if there is a /, it has a parent path.
		// ... unless it's /.

		if (empty()) return false;

		if (!_info.valid) study();

		if (_info.special == '/') return false;

		return _path.find(separator) != _path.npos;
	}

	path path::parent_path() const {

		/*
		 * special cases:
		 * /abc -> /
		 * /abc/ -> /abc
		 * all trailing /s are removed.
		 *
		 */


		if (empty()) return *this;

		if (!_info.valid) study();

		// "/" is a file of "/" with a parent of ""
		if (_info.special == separator) return path();

		// stem starts at 0, eg "abc"
		if (!_info.stem) return path();


		auto tmp = _path.substr(0, _info.stem - 1);

		// remove trailing slashes, but return "/" if nothing BUT /s.
		while (!tmp.empty() && tmp.back() == separator) tmp.pop_back();
		if (tmp.empty()) return path_sep;

		return path(tmp);
	}

	path path::root_directory() const {
		// for unix, root directory is / or "".
		if (empty()) return *this;

		return _path.front() == '/' ? path_sep : path();
	}

	path path::root_name() const {
		/*
		 * boost (unix) considers // or //component 
		 * to be a root name (and only those cases).
		 *
		 * I do not.
		 */ 

		return path();
	}

	path path::root_path() const {
		// root_name + root_directory.
		// since root_name is always empty...

		return root_directory();
	}


	path path::relative_path() const {
		// first pathname *after* the root path
		// root_path is first / in this implementation.

		if (is_relative()) return *this;

		auto pos = _path.find_first_not_of(separator);
		if (pos == _path.npos) return path();

		return path(_path.substr(pos));
	}



	// compare
	int path::compare(const path& p) const noexcept {
		if (&p == this) return 0;
		return _path.compare(p._path);
	}

	int path::compare(const std::string& s) const {
		if (&s == &_path) return 0;
		return _path.compare(s);
	}

	int path::compare(const value_type* s) const {
		return _path.compare(s);
	}


}
