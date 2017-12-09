#pragma once


///
/// Smart pointer for use in conjunction with classes that derive from Object
template <typename T>
class ptr {
public:
	T *p;

	ptr(std::nullptr_t p = nullptr) : p(p) {}

	ptr(T *p) : p(p) {
		if (p != nullptr)
			p->addReference();
	}
	
	ptr(const ptr<T> &p) : p(p.p) {
		if (this->p != nullptr)
			this->p->addReference();	
	}

	ptr(ptr<T> &&p) : p(p.p) {
		p.p = nullptr;
	}

	template <typename U>
	ptr(const ptr<U> &p) : p(p.p) {
		if (this->p != nullptr)
			this->p->addReference();	
	}

	~ptr() {
		if (this->p != nullptr)
			this->p->removeReference();
	}
	
	T *operator ->() const {
		return this->p;
	}
	
	ptr<T> &operator =(std::nullptr_t p) {
		if (this->p != nullptr)
			this->p->removeReference();
		this->p = nullptr;
		return *this;
	}
	
	ptr<T> &operator =(T *p) {
		if (p != nullptr)
			p->addReference();
		if (this->p != nullptr)
			this->p->removeReference();
		this->p = p;
		return *this;
	}

	ptr<T> &operator =(const ptr<T> &p) {
		if (p.p != nullptr)
			p.p->addReference();
		if (this->p != nullptr)
			this->p->removeReference();
		this->p = p.p;
		return *this;
	}

	template <typename U>
	ptr<T> &operator =(const ptr<U> &p) {
		if (p.p != nullptr)
			p.p->addReference();
		if (this->p != nullptr)
			this->p->removeReference();
		this->p = p.p;
		return *this;
	}
	
	bool operator ==(T *p) const {
		return this->p == p;
	}
	
	bool operator !=(T *p) const {
		return this->p != p;
	}

	template <typename U>
	bool operator ==(const ptr<U> &p) const {
		return this->p == p.p;
	}
	
	template <typename U>
	bool operator !=(const ptr<U> &p) const {
		return this->p != p.p;
	}
};
