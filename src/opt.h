#pragma once


template <typename T>
class opt {
public:
  bool valid;
  T value;

  opt(std::nullptr_t p = nullptr) : valid(false), value() {}

  opt(T value) : valid(true), value(value) {
  }

  const T & operator *() const {
     return this->value;
  }

  T & operator *() {
     return this->value;
  }

  const T * operator ->() const {
     return &this->value;
  }

  T * operator ->() {
     return &this->value;
  }

  opt<T> & operator =(std::nullptr_t p) {
     this->valid = false;
     return *this;
  }

  opt<T> & operator =(T value) {
     this->valid = true;
     this->value = value;
     return *this;
  }

  operator bool () const {
     return this->valid;
  }
};
