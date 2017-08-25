#pragma once

#include <algorithm>
#include <cassert>
#include <istream>
#include <list>
#include <map>
#include <string>

namespace xml {

class Error {
};

enum NodeType {
  NodeTypeDocument,
  NodeTypeDeclaration,
  NodeTypeComment,
  NodeTypeElement,
  NodeTypeEndElement,
  NodeTypeAttribute,
  NodeTypeText,
  NodeTypeCDATA,
};

enum Encoding {
  EncodingUnknown,
  EncodingAscii,
  EncodingUtf7,
  EncodingUtf8,
};

typedef std::map<std::string, std::wstring> AttributeMap;

// Reader

class Reader {
 public:
  // Атрибуты текущего узла
  std::string		name;
  std::wstring	value;
  NodeType		node_type;
  bool			empty;
  AttributeMap	attributes;
  Encoding		encoding;

  Reader()
      : encoding(EncodingUnknown) {
  }

  // Атрибуты текущего узла
  bool GetAttribute(const char* attr, std::wstring& val) const;
  std::wstring GetAttribute(const char* attr) const;

  // Внешний интерфейс
  virtual void Read() = 0;
  virtual bool Eof() const = 0;
};

// TextReader

class TextReader : public Reader {
 protected:
  // Внутренние операции
  void SkipSpaces();
  bool IsNameSymbol(int ch, bool NotFirst = true);
  std::string ReadName();
  void ReadUntil(std::string& str, const char* term);

 public:
  // Конструкторы и деструктор
  TextReader()
      : m_stream(NULL) {
  }
  TextReader(std::istream& stream) {
    assert(m_stream);
    m_stream = &stream;
  }

  // Функции инициализации
  void SetStream(std::istream& stream) { m_stream = &stream; }

  // Внешний интерфейс
  virtual void Read();
  virtual bool Eof() const;

  // Атрибуты
  long GetPosition() const {
    assert(m_stream);
    return static_cast<long>(m_stream->tellg());
  }

 protected:
  // Начальные параметры
  std::istream*	m_stream;
};

class Node;

// TextWriter

class TextWriter {
 protected:
  void WriteAttributes(Reader& reader);

 public:
  TextWriter();
  explicit TextWriter(std::ostream& stream);

  void SetStream(std::ostream& stream) { m_stream = &stream; }

  void Write(Reader& reader);

  Encoding encoding;
  bool line_breaks;

 protected:
  std::ostream*	m_stream;

  int node_level_;
};

// Node

class Node {
 public:
  std::string		name;
  std::wstring	value;
  NodeType		type;
  Node*			parent;
  Node*			next;
  Node*			first_child;
  Node*			last_child;
  AttributeMap	attributes;

  explicit Node(NodeType type)
      : type(type),
        parent(0),
        next(0),
        first_child(0),
        last_child(0) {
  }
  ~Node() { clear(); }

  Node& AddChild(NodeType type);
  Node& AddChildFirst(NodeType type);
  Node& AddElement(const char* name);
  Node& AddElement(const std::string& name);
  void clear();

  bool GetAttribute(const char* attr, std::wstring& val) const;
  bool GetAttribute(const char* attr, std::string& val) const;
  
  std::wstring GetAttribute(const char* attr) const;
  std::string GetAttributeA(const char* attr) const;
  
  void SetAttribute(const char* attr, const wchar_t* val);
  void SetAttribute(const char* attr, const char* val);
  void SetAttribute(const char* attr, const std::string& val);
  void SetAttribute(const char* attr, const std::wstring& val);

  Node* select(const char* name);
  const Node* select(const char* name) const;

  // returns value of first text child node
  std::wstring get_text() const;
  // sets value of first text child node, adds one if not exists
  void set_text(const wchar_t* text);
  // returns value of first child node
  const std::wstring& get_value() const;
  // sets value of first child node, adds one if not exists
  void set_value(const wchar_t* val);

  void to_stream(std::ostream& stream) const;
  std::string to_string() const;
};

// Document

class Document : public Node {
 public:
  Document();

  Node* GetDocumentElement();
  Node* GetDeclaration(bool create = false);
  Encoding GetEncoding() const;
  void SetEncoding(Encoding enc);
  void SetVersion(const wchar_t* ver);

  void Load(TextReader& Reader);
  void Save(TextWriter& Writer);
};

// NodeReader

class NodeReader : public Reader {
 public:
  // Конструкторы и деструктор
  void Start(Node& parent);

  // Внешний интерфейс
  virtual void Read();
  virtual bool Eof() const;

 protected:
  // Параметры чтения
  typedef std::list<Node*> NodePath;
  NodePath	path;
  Node*		current;
};

inline bool Node::GetAttribute(const char* attr, std::wstring& val) const {
  AttributeMap::const_iterator i = attributes.find(attr);
  if (i == attributes.end())
    return false;
  val = i->second;
  return true;
}

inline std::wstring Node::GetAttribute(const char* attr) const {
  std::wstring val;
  return GetAttribute(attr, val) ? val : std::wstring();
}

inline std::string Node::GetAttributeA(const char* attr) const {
  std::string val;
  return GetAttribute(attr, val) ? val : std::string();
}

inline void Node::SetAttribute(const char* attr, const wchar_t* val) {
  attributes[attr] = val;
}

inline void Node::SetAttribute(const char* attr, const std::string& val) {
  SetAttribute(attr, val.c_str());
}

inline void Node::SetAttribute(const char* attr, const std::wstring& val) {
  SetAttribute(attr, val.c_str());
}

} // namespace xml
