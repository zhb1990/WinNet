#pragma once

#include <iostream>

#include "NetConf.h"

template <class T>
class NetAllocator 
{
public:
	// type definitions
	typedef T        value_type;
	typedef T*       pointer;
	typedef const T* const_pointer;
	typedef T&       reference;
	typedef const T& const_reference;
	typedef std::size_t    size_type;
	typedef std::ptrdiff_t difference_type;

	// rebind allocator to type U
	template <class U>
	struct rebind 
    {
		typedef NetAllocator<U> other;
	};

	// return address of values
	pointer address(reference value) const 
    {
		return &value;
	}
	const_pointer address(const_reference value) const 
    {
		return &value;
	}

	/* constructors and destructor
	* - nothing to do because the allocator has no state
	*/
    NetAllocator() noexcept 
    {
	}
    NetAllocator(const NetAllocator&) noexcept
    {
	}
	template <class U>
    NetAllocator(const NetAllocator<U>&) noexcept
    {
	}
	~NetAllocator() noexcept
    {
	}

	// return maximum number of elements that can be allocated
	size_type max_size() const noexcept
    {
        return ((size_t)(-1) / sizeof(T));
	}

	// allocate but don't initialize num elements of type T
	pointer allocate(size_type num, const void* = 0) 
    {
		pointer ret = (pointer)(NET_NEW(num * sizeof(T)));
		return ret;
	}

	// initialize elements of allocated storage p with value value
	template<class _Objty,
		class... _Types>
		void construct(_Objty *_Ptr, _Types&&... _Args)
	{	// construct _Objty(_Types...) at _Ptr
		::new ((void *)_Ptr) _Objty(_STD forward<_Types>(_Args)...);
	}

	template<class _Uty>
	void destroy(_Uty *_Ptr)
	{	// destroy object at _Ptr
		_Ptr->~_Uty();
	}

	// deallocate storage p of deleted elements
	void deallocate(pointer p, size_type num) 
    {
        NET_DELETE((void*)p);
	}
};

// return that all specializations of this allocator are interchangeable
template <class T1, class T2>
bool operator== (const NetAllocator<T1>&,
	const NetAllocator<T2>&) noexcept
{
	return true;
}
template <class T1, class T2>
bool operator!= (const NetAllocator<T1>&,
	const NetAllocator<T2>&) noexcept
{
	return false;
}


