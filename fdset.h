#ifndef __fdset__
#define __fdset__

#include <array>
#include <initializer_list>
#include <string>
#include <vector>

#include <unistd.h>

class fdset;
class fdmask;

/*
 * fdmask does not own the file descriptors and will not close them.
 *
 */
class fdmask {
	public:

	fdmask() = default;
	fdmask(const fdmask &) = default;
	fdmask(fdmask &&) = default;

	fdmask(const std::array<int, 3> &rhs) : _fds(rhs)
	{}

	fdmask(int a, int b, int c) : _fds{{ a, b, c }}
	{} 

#if 0
	fdmask(std::initializer_list<int> rhs) : _fds(rhs)
	{}
#endif

	fdmask &operator=(const fdmask &) = default;
	fdmask &operator=(fdmask &&) = default;

	fdmask &operator=(const std::array<int, 3> &rhs) {
		_fds = rhs;
		return *this;
	}

#if 0
	fdmask &operator=(std::initializer_list<int> rhs) {
		_fds = rhs;
	}
#endif

	void dup() const {
		// dup fds to stdin/stdout/stderr.
		// called after fork, before exec.


		#define __(index, target) \
		if (_fds[index] >= 0 && _fds[index] != target) dup2(_fds[index], target)

		__(0, STDIN_FILENO);
		__(1, STDOUT_FILENO);
		__(2, STDERR_FILENO);

		#undef __
	}


	int operator[](unsigned index) const {
		if (_fds[index] >= 0) return _fds[index];
		return default_fd(index);
	}

	fdmask &operator|=(const fdmask &rhs) {
		for (unsigned i = 0; i < 3; ++i) {
			if (_fds[i] < 0) _fds[i] = rhs._fds[i];
		}
		return *this;
	}

	private:

		static int default_fd(int index) {
			static constexpr std::array<int, 3> _default_fds = {{ STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO }};
			return _default_fds[index];
		}

	friend class fdset;
	std::array<int, 3> _fds = {{ -1, -1, -1 }};
};

/*
 * fd set owns it's descriptors and will close them.
 *
 *
 */

class fdset {
	public:

	fdset() = default;
	fdset(const fdset &) = delete;
	fdset(fdset && rhs) {
		std::swap(rhs._fds, _fds);
	}

	~fdset() {
		close();
	}

	fdset &operator=(const fdset &) = delete;
	fdset &operator=(fdset &&rhs) {
		if (&rhs != this) {
			std::swap(_fds, rhs._fds);
			rhs.close();
		}
		return *this;
	}

	void close(void) {
		for (int &fd : _fds) {
			if (fd >= 0) {
				::close(fd);
				fd = -1;
			}
		}
	}

	void set(int index, int fd) {
		std::swap(fd, _fds[index]);
		if (fd >= 0) ::close(fd);
	}

	fdmask to_mask() const {
		return fdmask(_fds);
	}

	void swap_in_out() {
		std::swap(_fds[0], _fds[1]);
	}

	private:

	void reset() {
		_fds = {{ -1, -1, -1 }};
	}



	std::array<int, 3> _fds = {{ -1, -1, -1 }};

};

inline fdmask operator|(const fdmask &lhs, const fdmask &rhs) {
	fdmask tmp(lhs);
	tmp |= rhs;
	return tmp;
}

inline fdmask operator|(const fdset &lhs, const fdmask &rhs) {
	fdmask tmp(lhs.to_mask());
	tmp |= rhs;
	return tmp;
}

struct process {
	std::vector<std::string> arguments;
	fdset fds;
};


#endif
