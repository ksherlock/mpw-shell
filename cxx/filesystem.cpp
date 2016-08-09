#include "filesystem.h"

#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <unistd.h>
#include <sys/param.h>
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

namespace filesystem {

	namespace {


		template<class FX, class... Args>
		auto syscall(error_code &ec, FX fx, Args&&... args) -> decltype(fx(std::forward<Args>(args)...))
		{
			auto rv = fx(std::forward<Args>(args)...);
			if (rv < 0) {
				ec = error_code(errno, std::system_category());
			} else {
				ec.clear();
			}

			return rv;
		}

		int fs_stat(const path &p, struct stat *buf, error_code &ec) {
			int rv = stat(p.c_str(), buf);
			if (rv < 0) {
				ec = error_code(errno, std::system_category());
			}
			else {
				ec.clear();
			}
			return rv;
		}

		int fs_lstat(const path &p, struct stat *buf, error_code &ec) {
			int rv = lstat(p.c_str(), buf);
			if (rv < 0) {
				ec = error_code(errno, std::system_category());
			}
			else {
				ec.clear();
			}
			return rv;
		}

		template<class FX>
		file_status status_common(FX fx, const path& p, error_code& ec) noexcept {


			struct stat st;
			int rv = fx(p, &st, ec);
			if (rv < 0)
			{
				switch (ec.value())
				{
				case ENOENT:
				case ENOTDIR:
					return file_status(file_type::not_found);

				case EOVERFLOW:
					return file_status(file_type::unknown);

				//case ENAMETOOLONG: ???
				// case ELOOP ?

				default:
					return file_status(file_type::none);					
				}
			}

			ec.clear();
			perms prms = static_cast<perms>(st.st_mode & perms::mask);

			if (S_ISREG(st.st_mode))
				return file_status(file_type::regular, prms);

			if (S_ISDIR(st.st_mode))
				return file_status(file_type::directory, prms);

			if (S_ISBLK(st.st_mode))
				return file_status(file_type::block, prms);

			if (S_ISFIFO(st.st_mode))
				return file_status(file_type::fifo, prms);

			if (S_ISSOCK(st.st_mode))
				return file_status(file_type::socket, prms);

			return file_status(file_type::unknown, prms);
		}

	}

	file_status status(const path& p) {
		error_code ec;
		file_status result = status(p, ec);
		if (result.type() == file_type::none)
			throw filesystem_error("filesystem::file_status", p, ec);
		return result;
	}

	file_status status(const path& p, error_code& ec) noexcept {

		return status_common(fs_stat, p, ec);
/*
		struct stat st;
		int rv = stat(p.c_str(), &st);
		if (rv < 0) {
			int e = errno;
			ec = error_code(e, std::system_category());

			switch(e){
				case ENOENT:
				case ENOTDIR:
					return file_status(file_type::not_found);

				case EOVERFLOW:
					return file_status(file_type::unknown);

				//case ENAMETOOLONG: ???
				// case ELOOP ?

				default:
					return file_status(file_type::none);
			}
		}

		ec.clear();
		perms prms = static_cast<perms>(st.st_mode & perms::mask);

		if (S_ISREG(st.st_mode))
			return file_status(file_type::regular, prms);

		if (S_ISDIR(st.st_mode))
			return file_status(file_type::directory, prms);

		if (S_ISBLK(st.st_mode))
			return file_status(file_type::block, prms);

		if (S_ISFIFO(st.st_mode))
			return file_status(file_type::fifo, prms);

		if (S_ISSOCK(st.st_mode))
			return file_status(file_type::socket, prms);

		return file_status(file_type::unknown, prms);
*/
	}

	file_status symlink_status(const path& p) {
		error_code ec;
		file_status result = symlink_status(p, ec);
		if (result.type() == file_type::none)
			throw filesystem_error("filesystem::symlink_status", p, ec);
		return result;		
	}

	file_status symlink_status(const path& p, error_code& ec) noexcept {

		return status_common(fs_lstat, p, ec);
	}


	uintmax_t file_size(const path& p) {
		error_code ec;

		struct stat st;
		//if (fs_stat(p, &st, ec) < 0)
		if (syscall(ec, ::stat, p.c_str(), &st)  < 0)
			throw filesystem_error("filesystem::file_size", p, ec);

		return st.st_size;

	}

	uintmax_t file_size(const path& p, error_code& ec) noexcept {

		struct stat st;

		//if (fs_stat(p, &st, ec) < 0)
		if (syscall(ec, ::stat, p.c_str(), &st)  < 0)
			return static_cast<uintmax_t>(-1);

		return st.st_size;
	}



	bool create_directory(const path& p) {
		error_code ec;
		bool rv = create_directory(p, ec);
		if (ec)
			throw filesystem_error("filesystem::create_directory", p, ec);

		return rv;
	}

	bool create_directory(const path& p, error_code& ec) noexcept {

		int rv = ::mkdir(p.c_str(), S_IRWXU|S_IRWXG|S_IRWXO);
		if (rv == 0) {
			ec.clear();
			return true;
		}

		int e = errno;
		error_code tmp;

		// special case -- not an error if the directory already exists.

		if (e == EEXIST && is_directory(p, tmp)) {
			ec.clear();
			return false;
		}

		ec = error_code(e, std::system_category());
		return false;
	}

	void resize_file(const path& p, uintmax_t new_size) {
		error_code ec;

		if (syscall(ec, ::truncate, p.c_str(), new_size) < 0)
			throw filesystem_error("filesystem::create_directory", p, ec);
	}

	void resize_file(const path& p, uintmax_t new_size, error_code& ec) noexcept {

		syscall(ec, ::truncate, p.c_str(), new_size);

	}

	bool remove(const path& p) {
		error_code ec;

		if (syscall(ec, ::remove, p.c_str()) < 0)
		{
			throw filesystem_error("filesystem::remove", p, ec);
		}
		return true;
	}

	bool remove(const path& p, error_code& ec) noexcept {
		if (syscall(ec, ::remove, p.c_str()) < 0) return false;
		return true;
	}


	path current_path() {
		error_code ec;
		path p = current_path(ec);

		if (ec) 
			throw filesystem_error("filesystem::current_path", ec);

		return p;
	}

	path current_path(error_code& ec) {

		char *cp;
		char buffer[PATH_MAX+1];

		ec.clear();
		cp = ::getcwd(buffer, PATH_MAX);
		if (cp) return path(cp);

		ec = error_code(errno, std::system_category());
		return path();
	}

	void current_path(const path& p) {
		error_code ec;
		syscall(ec, ::chdir, p.c_str());
		if (ec)
			throw filesystem_error("filesystem::current_path", p, ec);
	}

	void current_path(const path& p, error_code& ec) noexcept {

		syscall(ec, ::chdir, p.c_str());
	}




	path canonical(const path& p, const path& base) {
		error_code ec;
		path rv = canonical(p, base, ec);
		if (ec) 
			throw filesystem_error("filesystem::canonical", p, ec);
		return rv;
	}

	path canonical(const path& p, error_code& ec) {
		char *cp;
		char buffer[PATH_MAX+1];

		ec.clear();
		cp = realpath(p.c_str(), buffer);
		if (cp) return path(cp);
		ec = error_code(errno, std::system_category());
		return path();
	}

	path canonical(const path& p, const path& base, error_code& ec) {

		char *cp;
		char buffer[PATH_MAX+1];
		
		ec.clear();

		if (p.is_absolute()) cp = realpath(p.c_str(), buffer);
		else {
			path tmp = base;
			tmp /= p;
			cp = realpath(tmp.c_str(), buffer);
		}
		if (cp) return path(cp);
		ec = error_code(errno, std::system_category());
		return path();
	}




}
