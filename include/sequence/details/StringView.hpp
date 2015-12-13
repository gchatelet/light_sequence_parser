#pragma once

#include <cstring>
#include <string>
#include <stdexcept>
#include <algorithm>

template <typename T> struct View {
private:
  typedef View<const char> CView;
  typedef typename std::conditional<std::is_const<T>::value, const std::string,
                                    std::string>::type string;

public:
  T *ptr_ = nullptr;
  size_t size_ = 0;

  View() = default;
  View(const View &) = default;
  View(T *str, size_t size) : ptr_(str), size_(size) {}
  View(T *str) : ptr_(str), size_(strlen(str)) {}
  View(string &str) : View(&str[0], str.size()) {}

  enum : size_t { npos = std::string::npos };

  bool operator==(CView other) const;
  bool operator!=(CView other) const;
  bool operator<(CView other) const;

  inline operator CView() const { return CView(ptr_, size_); }

  inline bool empty() const { return size_ == 0; }
  inline size_t size() const { return size_; }

  inline const T *ptr() const { return ptr_; }
  inline const T *begin() const { return ptr_; }
  inline const T *end() const { return ptr_ + size_; }
  inline T operator[](size_t at) const { return ptr_[at]; }

  inline T *ptr() { return ptr_; }
  inline T *begin() { return ptr_; }
  inline T *end() { return ptr_ + size_; }
  inline T &operator[](size_t at) { return ptr_[at]; }

  size_t indexOf(char c) const;
  size_t lastIndexOf(char c) const;
  size_t indexOf(CView other) const;

  inline bool contains(char c) const { return indexOf(c) != npos; }
  inline bool contains(CView o) const { return indexOf(o) != npos; }

  std::string toString() const { return std::string(ptr_, size_); }

  View substr(size_t pos = 0, size_t len = npos) const;
};

///////////////////////////////////////////////////////////////////////////////////////////

typedef View<const char> CStringView;
typedef View<char> StringView;

///////////////////////////////////////////////////////////////////////////////////////////

template <typename T> bool View<T>::operator==(CView other) const {
  return size_ == other.size_ &&
         (ptr_ == other.ptr_ || memcmp(ptr_, other.ptr_, size_) == 0);
}

template <typename T> bool View<T>::operator!=(CView other) const {
  return !(*this == other);
}

template <typename T> bool View<T>::operator<(CView other) const {
  const auto result = memcmp(ptr_, other.ptr_, std::min(size_, other.size_));
  return result == 0 ? size_ < other.size_ : result < 0;
}

template <typename T> size_t View<T>::indexOf(char c) const {
  const char *const ptr = (const char *)memchr(ptr_, c, size_);
  return ptr ? ptr - ptr_ : npos;
}

#ifdef _GNU_SOURCE
template <typename T> size_t View<T>::lastIndexOf(char c) const {
  const char *const ptr = (const char *)memrchr(ptr_, c, size_);
  return ptr ? ptr - ptr_ : npos;
}
#else
template <typename T> size_t View<T>::lastIndexOf(const char c) const {
  if (size_) {
    for (const char *ptr = ptr_ + size_; ptr >= ptr_; --ptr) {
      if (*ptr == c) {
        return ptr - ptr_;
      }
    }
  }
  return npos;
}
#endif

template <typename T> size_t View<T>::indexOf(const CView other) const {
  if (other.size() > size() || other.empty())
    return npos;
  const char first = other[0];
  for (size_t index = indexOf(first); index != npos; ++index) {
    if (substr(index, other.size()) == other) {
      return index;
    }
  }
  return npos;
}

template <typename T> View<T> View<T>::substr(size_t pos, size_t len) const {
  if (pos > size_)
    throw std::out_of_range("StringView::substr");
  return View<T>(ptr_ + pos, std::min(size_ - pos, len));
}
