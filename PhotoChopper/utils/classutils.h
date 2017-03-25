#pragma once

#define INIT_INHERITANCE(base) \
	typedef base super;

namespace pc
{
namespace utils
{

struct noncopyable
{
	// default
	noncopyable() = default;

	// move 
	noncopyable(noncopyable&&) {};
	noncopyable& operator=(noncopyable&&) {};

	// copy
	noncopyable(const noncopyable&) = delete;
	noncopyable& operator=(const noncopyable&) = delete;

	// destructor
	~noncopyable() = default;
};

}
}