#ifndef __filesystem_h__
#define __filesystem_h__

#include <chrono>
#include <cstdint>
#include <locale>
#include <stdexcept>
#include <string>
#include <system_error>
#include <memory>

#include <dirent.h>

/*
 *
 * light-weight implementation of n3803
 *
 *
 */

namespace filesystem {

	class path;
	class filesystem_error;

	using std::error_code;
	using std::system_error;

	class path {

	public:

		typedef char value_type;
		typedef std::basic_string<value_type> string_type;
		static constexpr value_type preferred_separator = '/';


		// constructors and destructor
		path() = default;
		path(const path& p) = default;
		path(path&& p) noexcept :
			_path(std::move(p._path)) {
			p.invalidate();
		}

		template<class Source>
		path(Source const& source) :
			_path(source) {}

		template <class InputIterator>
		path(InputIterator begin, InputIterator end) :
			_path(begin, end) {}

		template <class Source>
		path(Source const& source, const std::locale& loc);

		template <class InputIterator>
		path(InputIterator begin, InputIterator end, const std::locale& loc);

		~path() = default;

		// assignments
		path& operator=(const path& p) {
			if (this != &p)	{
				_path = p._path;
				_info = p._info;
			}
			return *this;
		}
		path& operator=(path&& p) noexcept {
			if (this != &p) {
				_path = std::move(p._path);
				_info = p._info;
				p.invalidate();
			}
			return *this;
		}

		template <class Source>
		path& operator=(Source const& source) {
			invalidate();
			_path = source;
			return *this;
		}

		path& assign(const path &p) {
			return (*this = p);
		}

		template <class Source>
		path& assign(Source const& source) {
			invalidate();
			_path.assign(source);
			return *this;
		}

		template <class InputIterator>
		path& assign(InputIterator begin, InputIterator end) {
			invalidate();
			_path.assign(begin, end);
			return *this;
		}


		// appends
		path& operator/=(const path& p) {
			return append(p);
		}



		template <class Source>
		path& operator/=(Source const& source) {
			return append(source);
		}



		// consider this a specialization of append.
		path& append(const path &p);

		path& append(const string_type &s);


		template <class Source>
		path& append(Source const& source) {
			return append(path(source));
		}

		template <class InputIterator>
		path& append(InputIterator begin, InputIterator end) {
			return append(path(begin, end));
		}


		// concatenation
		path& operator+=(const path& x) {
			invalidate();
			_path += x._path;
			return *this;
		}

		path& operator+=(const string_type& x) {
			invalidate();
			_path += x;			
			return *this;
		}

		path& operator+=(const value_type* x) {
			invalidate();
			_path += x;
			return *this;
		}

		path& operator+=(value_type x) {
			invalidate();
			_path += x;			
			return *this;
		}

		template <class Source>
		path& operator+=(Source const& x) {
			invalidate();
			_path += x;
			return *this;	
		}

		template <class charT>
		path& operator+=(charT x){
			invalidate();
			_path += x;			
			return *this;
		}

		template <class Source>
		path& concat(Source const& x) {
			invalidate();
			_path += x;
			return *this;
		}

		template <class InputIterator>
		path& concat(InputIterator begin, InputIterator end) {
			invalidate();
			_path.append(begin, end);
			return *this;
		}


		// modifiers
		void  clear() noexcept {
			invalidate();
			_path.clear();
		}

		path& make_preferred() {
			return *this;
		}
		path& remove_filename();
		path& replace_filename(const path& replacement);
		path& replace_extension(const path& replacement = path());

		void  swap(path& rhs) noexcept {
			std::swap(_path, rhs._path);
			std::swap(_info, rhs._info);
		}

		// native format observers
		const string_type& native() const noexcept {
			return _path;
		}

		const value_type* c_str() const noexcept {
			return _path.c_str();
		}

		operator string_type() const {
			return _path;
		}

		template <class charT, class traits = std::char_traits<charT>,
			class Allocator = std::allocator<charT> >
		std::basic_string<charT, traits, Allocator> string(const Allocator& a = Allocator()) const;

		std::string string() const {
			return _path;
		}

		std::wstring wstring() const;
		std::string u8string() const;
		std::u16string u16string() const;
		std::u32string u32string() const;

		// generic format observers
		template <class charT, class traits = std::char_traits<charT>,
			class Allocator = std::allocator<charT> >
			std::basic_string<charT, traits, Allocator>
		generic_string(const Allocator& a = Allocator()) const;
		std::string    generic_string() const {
			return _path;
		}
		std::wstring   generic_wstring() const;
		std::string    generic_u8string() const;
		std::u16string generic_u16string() const;
		std::u32string generic_u32string() const;

		// compare
		int compare(const path& p) const noexcept;
		int compare(const std::string& s) const;
		int compare(const value_type* s) const;

		// decomposition
		path root_name() const;
		path root_directory() const;
		path root_path() const;
		path relative_path() const;
		path parent_path() const;
		path filename() const;
		path stem() const;
		path extension() const;

		// query
		bool empty() const noexcept {
			return _path.empty();
		}

		bool has_root_name() const;
		bool has_root_directory() const;
		bool has_root_path() const;
		bool has_relative_path() const;
		bool has_parent_path() const;
		bool has_filename() const;
		bool has_stem() const;
		bool has_extension() const;

		bool is_absolute() const {
			return !empty() && _path[0] == '/';
		}

		bool is_relative() const {
			return empty() || _path[0] != '/';
		}

		// iterators
		class iterator;
		typedef iterator const_iterator;
		iterator begin() const;
		iterator end() const;

	private:

		void invalidate() const {
			_info.valid = false;
		}

		void study() const;
		path &append_common(const std::string &s);




		string_type _path;

		mutable struct {
			bool valid = false;
			value_type special;
			int stem;
			int extension;
		} _info;

	};


	inline void swap(path& lhs, path& rhs) noexcept {
		lhs.swap(rhs);
	}

/*
	// ugh, this is boost, not stl.
	inline std::size_t hash_value(const path& p) noexcept {
		return std::hash_value(p._path);
	}
*/
	inline bool operator==(const path& lhs, const path& rhs) noexcept {
		return lhs.compare(rhs) == 0;
	}

	inline bool operator!=(const path& lhs, const path& rhs) noexcept {
		return lhs.compare(rhs) != 0;
	}

	inline bool operator< (const path& lhs, const path& rhs) noexcept {
		return lhs.compare(rhs) < 0;
	}
	inline bool operator<=(const path& lhs, const path& rhs) noexcept {
		return lhs.compare(rhs) <= 0;
	}
	inline bool operator> (const path& lhs, const path& rhs) noexcept {
		return lhs.compare(rhs) > 0;		
	}
	inline bool operator>=(const path& lhs, const path& rhs) noexcept {
		return lhs.compare(rhs) >= 0;		
	}
	inline path operator/ (const path& lhs, const path& rhs) {
		path tmp = lhs;
		return tmp.append(rhs);
	}

	//template <class charT, class traits>
	//basic_ostream<charT, traits>&
	//operator<<(basic_ostream<charT, traits>& os, const path& p);
	//template <class charT, class traits>
	//basic_istream<charT, traits>&
	///operator>>(basic_istream<charT, traits>& is, path& p);
	template <class Source>
	 	path u8path(Source const& source);
	template <class InputIterator>
	 	path u8path(InputIterator begin, InputIterator end);


	class filesystem_error : public system_error
	{
		public:
		filesystem_error(const std::string& what_arg, error_code ec) :
			system_error(ec, what_arg)
		{}

		filesystem_error(const std::string& what_arg, const path& p1, error_code ec) : 
			system_error(ec, what_arg), _p1(p1)
		{}

		filesystem_error(const std::string& what_arg, const path& p1, const path& p2, error_code ec) :
			system_error(ec, what_arg), _p1(p1), _p2(p2)
		{}

		const path& path1() const noexcept {
			return _p1;
		}
		const path& path2() const noexcept {
			return _p2;
		}

		//const char* what() const noexcept;

		private:
		path _p1;
		path _p2;
	};



	enum class file_type {

		none = 0,
		not_found = -1,
		regular = 1,
		directory = 2,
		symlink = 3,
		block = 4,
		character = 5,
		fifo = 6,
		socket = 7,
		unknown = 8
	};

	enum perms {

		none = 0,
		owner_read = 0400,
		owner_write = 0200,
		owner_exec = 0100,
		owner_all = 0700,

		group_read = 040,
		group_write = 020,
		group_exec = 010,
		group_all = 070,

		others_read = 04,
		others_write = 02,
		others_exec = 01,
		others_all = 07,

		all = 0777,
		set_uid = 04000,
		set_gid = 02000,
		sticky_bit = 01000,
		mask = 07777,

		unknown = 0xffff,

		add_perms = 0x10000,
		remove_perms = 0x20000,
		resolve_symlinks = 0x40000,

	};


	class file_status
	{
		public:
		// constructors
		explicit file_status(file_type ft = file_type::none, perms prms = perms::unknown) noexcept:
			_ft(ft), _prms(prms) {}

		file_status(const file_status&) noexcept = default;
		file_status(file_status&&) noexcept = default;
		~file_status() = default;

		file_status& operator=(const file_status&) noexcept = default;
		file_status& operator=(file_status&&) noexcept = default;

		// observers
		file_type type() const noexcept {
			return _ft;
		}

		perms permissions() const noexcept {
			return _prms;
		}

		// modifiers
		void type(file_type ft) noexcept {
			_ft = ft;
		}
		void permissions(perms prms) noexcept {
			_prms = prms;
		}

		private:
		file_type _ft = file_type::none;
		perms _prms = perms::unknown;
	};


	struct space_info  // returned by space function
	{
		uintmax_t capacity;
		uintmax_t free;
		uintmax_t available; // free space available to a non-privileged process
	};

	enum class directory_options
	{
		none,
		follow_directory_symlink,
		skip_permission_denied
	};

	//typedef std::chrono::time_point<trivial-clock> file_time_type;
	typedef std::chrono::time_point<std::chrono::system_clock> file_time_type;

	enum class copy_options
	{
		none = 0,
		skip_existing = 1,
		overwrite_existing = 2,
		update_existing = 4,
		recurive = 8,
		copy_symlinks = 16,
		skip_symlinks = 32,
		directories_only = 64,
		create_symlinks = 128,
		create_hard_links = 256
	};

	path current_path();
	path current_path(error_code& ec);
	void current_path(const path& p);
	void current_path(const path& p, error_code& ec) noexcept;

	path absolute(const path& p, const path& base=current_path());

	path canonical(const path& p, const path& base = current_path());
	path canonical(const path& p, error_code& ec);
	path canonical(const path& p, const path& base, error_code& ec);

	void copy(const path& from, const path& to);
	void copy(const path& from, const path& to, error_code& ec) noexcept;
	void copy(const path& from, const path& to, copy_options options);
	void copy(const path& from, const path& to, copy_options options, error_code& ec) noexcept;

	bool copy_file(const path& from, const path& to);
	bool copy_file(const path& from, const path& to, error_code& ec) noexcept;
	bool copy_file(const path& from, const path& to, copy_options option);
	bool copy_file(const path& from, const path& to, copy_options option, error_code& ec) noexcept;
	void copy_symlink(const path& existing_symlink, const path& new_symlink);

	void copy_symlink(const path& existing_symlink, const path& new_symlink, error_code& ec) noexcept;

	bool create_directories(const path& p);
	bool create_directories(const path& p, error_code& ec) noexcept;

	bool create_directory(const path& p);
	bool create_directory(const path& p, error_code& ec) noexcept;
	bool create_directory(const path& p, const path& attributes);
	bool create_directory(const path& p, const path& attributes, error_code& ec) noexcept;

	void create_directory_symlink(const path& to, const path& new_symlink);
	void create_directory_symlink(const path& to, const path& new_symlink, error_code& ec) noexcept;

	void create_hard_link(const path& to, const path& new_hard_link);
	void create_hard_link(const path& to, const path& new_hard_link, error_code& ec) noexcept;

	void create_symlink(const path& to, const path& new_symlink);
	void create_symlink(const path& to, const path& new_symlink, error_code& ec) noexcept;




	file_status status(const path& p);
	file_status status(const path& p, error_code& ec) noexcept;

	file_status symlink_status(const path& p);
	file_status symlink_status(const path& p, error_code& ec) noexcept;

	inline bool status_known(file_status s) noexcept {
		return s.type() != file_type::none;
	}

	inline bool exists(file_status s) noexcept {
		return status_known(s) && s.type() != file_type::not_found;
	}

	inline bool exists(const path& p) {
		return exists(status(p));
	}
	inline bool exists(const path& p, error_code& ec) noexcept {
		return exists(status(p, ec));
	}


	bool         equivalent(const path& p1, const path& p2);
	bool         equivalent(const path& p1, const path& p2, error_code& ec) noexcept;

	uintmax_t    file_size(const path& p);
	uintmax_t    file_size(const path& p, error_code& ec) noexcept;

	uintmax_t    hard_link_count(const path& p);
	uintmax_t    hard_link_count(const path& p, error_code& ec) noexcept;

	inline bool is_block_file(file_status s) noexcept {
		return s.type() == file_type::block;
	}
	inline bool is_block_file(const path& p) {
		return is_block_file(status(p));
	}
	inline bool is_block_file(const path& p, error_code& ec) noexcept {
		return is_block_file(status(p, ec));		
	}

	inline bool is_character_file(file_status s) noexcept {
		return s.type() == file_type::character;
	}
	inline bool is_character_file(const path& p) {
		return is_character_file(status(p));
	}
	inline bool is_character_file(const path& p, error_code& ec) noexcept {
		return is_character_file(status(p, ec));		
	}

	inline bool is_directory(file_status s) noexcept {
		return s.type() == file_type::directory;
	}
	inline bool is_directory(const path& p) {
		return is_directory(status(p));
	}
	inline bool is_directory(const path& p, error_code& ec) noexcept {
		return is_directory(status(p, ec));		
	}

	bool is_empty(const path& p);
	bool is_empty(const path& p, error_code& ec) noexcept;


	inline bool is_fifo(file_status s) noexcept {
		return s.type() == file_type::fifo;
	}
	inline bool is_fifo(const path& p) {
		return is_fifo(status(p));
	}
	inline bool is_fifo(const path& p, error_code& ec) noexcept {
		return is_fifo(status(p, ec));		
	}

	inline bool is_socket(file_status s) noexcept {
		return s.type() == file_type::socket;
	}
	inline bool is_socket(const path& p) {
		return is_socket(status(p));
	}
	inline bool is_socket(const path& p, error_code& ec) noexcept {
		return is_socket(status(p, ec));		
	}


	inline bool is_symlink(file_status s) noexcept {
		return s.type() == file_type::symlink;
	}
	inline bool is_symlink(const path& p) {
		return is_symlink(status(p));
	}
	inline bool is_symlink(const path& p, error_code& ec) noexcept {
		return is_symlink(status(p, ec));		
	}


	inline bool is_regular_file(file_status s) noexcept {
		return s.type() == file_type::regular;
	}

	inline bool is_regular_file(const path& p) {
		return is_regular_file(status(p));
	}

	inline bool is_regular_file(const path& p, error_code& ec) noexcept {
		return is_regular_file(status(p, ec));
	}

	inline bool is_other(file_status s) noexcept {
		return exists(s) && !is_regular_file(s) && ! is_directory(s) && !is_symlink(s);	
	}

	inline bool is_other(const path& p) {
		return is_other(status(p));
	}

	inline bool is_other(const path& p, error_code& ec) noexcept {
		return is_other(status(p, ec));		
	}


	

	file_time_type last_write_time(const path& p);
	file_time_type last_write_time(const path& p, error_code& ec) noexcept;
	void last_write_time(const path& p, file_time_type new_time);
	void last_write_time(const path& p, file_time_type new_time, error_code& ec) noexcept;

	void permissions(const path& p, perms prms);
	void permissions(const path& p, perms prms, error_code& ec) noexcept;

	path read_symlink(const path& p);
	path read_symlink(const path& p, error_code& ec);
	bool remove(const path& p);
	bool remove(const path& p, error_code& ec) noexcept;
	uintmax_t remove_all(const path& p);
	uintmax_t remove_all(const path& p, error_code& ec) noexcept;
	void rename(const path& from, const path& to);
	void rename(const path& from, const path& to, error_code& ec) noexcept;
	void resize_file(const path& p, uintmax_t size);
	void resize_file(const path& p, uintmax_t size, error_code& ec) noexcept;
	space_info space(const path& p);
	space_info space(const path& p, error_code& ec) noexcept;
	file_status status(const path& p);
	file_status status(const path& p, error_code& ec) noexcept;
	bool status_known(file_status s) noexcept;
	file_status symlink_status(const path& p);
	file_status symlink_status(const path& p, error_code& ec) noexcept;
	path system_complete(const path& p);
	path system_complete(const path& p, error_code& ec);
	path temp_directory_path();
	path temp_directory_path(error_code& ec);




	class directory_entry {

	public:

		// constructors and destructor
		directory_entry() = default;
		directory_entry(const directory_entry&) = default;
		directory_entry(directory_entry&&) noexcept = default;
		explicit directory_entry(const path& p, file_status st=file_status(), file_status symlink_st=file_status());

		~directory_entry() = default;

		// modifiers
		directory_entry& operator=(const directory_entry&) = default;
		directory_entry& operator=(directory_entry&&) noexcept = default;
		void assign(const path& p, file_status st=file_status(),
			file_status symlink_st=file_status());
		void replace_filename(const path& p, file_status st=file_status(),
			file_status symlink_st=file_status());


		// observers
		const filesystem::path&  path() const noexcept {
			return _path;
		}

		file_status  status() const;
		file_status  status(error_code& ec) const noexcept;
		file_status  symlink_status() const;
		file_status  symlink_status(error_code& ec) const noexcept;


		bool operator< (const directory_entry& rhs) const noexcept;
		bool operator==(const directory_entry& rhs) const noexcept;
		bool operator!=(const directory_entry& rhs) const noexcept;
		bool operator<=(const directory_entry& rhs) const noexcept;
		bool operator> (const directory_entry& rhs) const noexcept;
		bool operator>=(const directory_entry& rhs) const noexcept;


	private:
		class path _path;
		mutable file_status _st;
		mutable file_status _lst;
	};


	class directory_iterator : public std::iterator<std::input_iterator_tag, directory_entry>
	{
	public:
		// member functions
		directory_iterator() noexcept = default;
		explicit directory_iterator(const path& p);
		directory_iterator(const path& p, error_code& ec) noexcept;

		directory_iterator(const directory_iterator&) = default;

		directory_iterator(directory_iterator&&) = default;
		~directory_iterator() = default;

		directory_iterator& operator=(const directory_iterator&) = default;
		directory_iterator& operator=(directory_iterator&&) = default;

		const directory_entry& operator*() const {
			return _imp->_entry;
		}
		const directory_entry* operator->() const {
			return &_imp->_entry;
		}
		directory_iterator& operator++();
		directory_iterator& increment(error_code& ec) noexcept;

		bool operator == (const directory_iterator &rhs) const noexcept {
			return _imp == rhs._imp;
		}
		bool operator != (const directory_iterator &rhs) const noexcept {
			return _imp != rhs._imp;
		}

		// other members as required by
		//  C++ Std, 24.1.1 Input iterators [input.iterators]



	private:

		void open_dir(const path &p, error_code &ec) noexcept;

		struct imp {
			path _path;
			directory_entry _entry;
			DIR *_dp = nullptr;

			imp() = default;
			imp(const path &p, DIR *dp) : _path(p), _dp(dp)
			{}

			~imp() {
				if (_dp) closedir(_dp);
			}
		};

		std::shared_ptr<imp> _imp;
	};


	/*
	 directory_iterator non-member functions
	*/
	inline const directory_iterator& begin(const directory_iterator& iter) noexcept {
		return iter;
	}

	inline directory_iterator end(const directory_iterator&) noexcept {
		return directory_iterator();
	}


}


namespace std
{

	inline void swap(filesystem::path& lhs, filesystem::path& rhs) noexcept {
		lhs.swap(rhs);
	}

	template<>
	struct hash<filesystem::path>
	{
		std::size_t operator()(const filesystem::path &p) const
		{
			std::hash<filesystem::path::string_type> hasher;
			return hasher(p.native());
		}
	};
}
 

#endif
