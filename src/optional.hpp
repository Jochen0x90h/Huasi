#pragma once


template <typename T>
class optional {
public:
  bool valid;
  T value;

  optional(std::nullptr_t p = nullptr) : valid(false), value() {}

  optional(T value) : valid(true), value(value) {
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

  optional<T> & operator =(std::nullptr_t p) {
     this->valid = false;
     return *this;
  }

  optional<T> & operator =(T value) {
     this->valid = true;
     this->value = value;
     return *this;
  }

  operator bool () const {
     return this->valid;
  }
};
