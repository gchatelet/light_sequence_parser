#include <iostream>
#include <sstream>

namespace json {

struct JsonNull {};

class JsonStreamWriter {
public:
  JsonStreamWriter() : first(true) {}
  virtual ~JsonStreamWriter() {}
  virtual std::string build() {
    finish();
    return stream.str();
  }

protected:
  friend class JsonObjectStreamWriter;
  friend class JsonArrayStreamWriter;
  std::ostringstream stream;
  bool first;

  template <typename T> JsonStreamWriter &operator<<(T value) {
    stream << value;
    return *this;
  }
  JsonStreamWriter &addSep() {
    if (first) {
      first = false;
    } else {
      stream << ',';
    }
    return *this;
  }
  virtual std::ostream &finish() = 0;
};

template <> JsonStreamWriter &JsonStreamWriter::operator<<(bool value) {
  stream << (value ? "true" : "false");
  return *this;
}

template <> JsonStreamWriter &JsonStreamWriter::operator<<(const char *string) {
  stream << '"' << string << '"';
  return *this;
}

template <> JsonStreamWriter &JsonStreamWriter::operator<<(JsonNull) {
  stream << "null";
  return *this;
}

class JsonObjectStreamWriter : public JsonStreamWriter {
public:
  JsonObjectStreamWriter() { stream << '{'; }

  template <typename T>
  JsonObjectStreamWriter &operator<<(const std::pair<const char *, T> pair) {
    addSep() << pair.first << ':' << pair.second;
    return *this;
  }

protected:
  virtual std::ostream &finish() { return stream << '}'; }
};

class JsonArrayStreamWriter : public JsonStreamWriter {
public:
  JsonArrayStreamWriter() { stream << '['; }
  template <typename T> JsonArrayStreamWriter &operator<<(T value) {
    addSep() << value;
    return *this;
  }

protected:
  virtual std::ostream &finish() { return stream << ']'; }
};

} // namespace json
