#pragma once

#include <atomic>


///
/// Object with reference counting. This is used instead of std::shared_ptr because we have to manually add and remove
/// references while waiting for asynchronous io. There is no known way to do this using std::shared_ptr and
/// std::shared_from_this
class Object {
public:
	
	///
	/// Constructor. Always allocate on the heap
	Object() : referenceCount(0) {}

protected:
	
	///
	/// Destructor. Don't call directly, always use ptr<> or add/removeReference()
	virtual ~Object();

public:

	std::atomic<int> referenceCount;
	
	void addReference() noexcept {
		std::atomic_fetch_add_explicit(&this->referenceCount, 1, std::memory_order_relaxed);
	}

	void removeReference() noexcept {
		if (std::atomic_fetch_sub_explicit(&this->referenceCount, 1, std::memory_order_release) == 1) {
			std::atomic_thread_fence(std::memory_order_acquire);
			delete this;
		}
    }
};
