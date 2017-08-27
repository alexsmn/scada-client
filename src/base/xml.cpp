#include "base/xml.h"

#include <cassert>
#include <atlconv.h>
#include <sstream>
#include <windows.h>

namespace xml {

template<typename CharType>
class StringBuilder {
 public:
  typedef std::basic_string<CharType> StringType;
  StringType& str;

  CharType buf[64];
  int nbuf;

  explicit StringBuilder(StringType& str) : str(str), nbuf(0) { }

  StringBuilder(const StringBuilder&) = delete;
  StringBuilder& operator=(const StringBuilder&) = delete;

  void append(CharType ch) {
    buf[nbuf++] = ch;
    if (nbuf == _countof(buf))
      flush();
  }

  void flush() {
    if (nbuf) {
      str.append(buf, nbuf);
      nbuf = 0;
    }
  }

  StringBuilder& operator+=(CharType ch) {
    append(ch);
    return *this;
  }
};

template<int CAPACITY = 64>
struct AutoBuffer {
  char	mem[CAPACITY];
  char*	buf;
  bool	alloc;
  int		size;

  AutoBuffer(int size)
      : size(size) {
    alloc = size > sizeof(mem);
    if (alloc)
      buf = new char[size];
    else
      buf = mem;
  }

  ~AutoBuffer() {
    if (alloc)
      delete[] buf;
  }
};

struct Indent {
  Indent(int level) : level(level) {}
  int level;
};

std::ostream& operator<<(std::ostream& stream, const Indent& indent) {
  for (int i = 0; i < indent.level * 2; ++i)
    stream << ' ';
  return stream;
}

static const wchar_t* ReplaceStringTag(const wchar_t* str, int len) {
  struct Entry {
    const wchar_t* tag;
    const wchar_t* str;
  };

  static const Entry entries[] = {
    { L"lt", L"<" },
    { L"gt", L">" },
    { L"amp", L"&" },
  };

  for (int i = 0; i < _countof(entries); ++i)
    if (wcsncmp(entries[i].tag, str, len) == 0)
      return entries[i].str;
  return NULL;
}

static void ReplaceStringTags(std::wstring& str) {
  std::wstring::size_type amp = std::wstring::npos;
  std::wstring::size_type i = 0;
  while (i < str.length()) {
    wchar_t c = str[i];
    if (c == L'&') {
      if (amp != std::wstring::npos)
        throw std::exception("bad xml string", 1);
      amp = i;
      ++i;
    } else if (c == L';' && amp != -1) {
      std::wstring::size_type len = i - amp - 1;
      const wchar_t* tag = str.data() + amp + 1;
      const wchar_t* s = ReplaceStringTag(tag, len);
      if (!s)
        throw std::exception("bad xml string tag", 1);
      str.replace(amp, len + 2, s);
      i = amp + wcslen(s);
      amp = std::wstring::npos;
    } else
      ++i;
  }
}

// Reader

bool Reader::GetAttribute(const char* attr, std::wstring& val) const {
  AttributeMap::const_iterator i = attributes.find(attr);
  if (i == attributes.end())
    return false;
  val = i->second;
  return true;
}

std::wstring Reader::GetAttribute(const char* attr) const {
  std::wstring str;
  if (GetAttribute(attr, str))
    return str;
  else
    return std::wstring();
}

// TextReader

void TextReader::SkipSpaces()
{
  // m_stream->ipfx();
  while (isspace(m_stream->peek()))
    m_stream->ignore();
}

bool TextReader::IsNameSymbol(int ch, bool NotFirst) {
  switch (ch)
  {
  case '_':
    return true;
  case '-':
    return NotFirst;
  case ':':
    return NotFirst;
  default:
    if (isalpha(ch))
            return true;
    if (isdigit(ch))
      return NotFirst;
    return false;
  }
}

std::string TextReader::ReadName() {
  if (!IsNameSymbol(m_stream->peek(), false))
    throw Error();

  std::string s;
  StringBuilder<char> builder(s);
  int ch;
  while (IsNameSymbol(ch = m_stream->get()))
    builder += static_cast<char>(ch);
  builder.flush();

  if (ch != EOF)
    m_stream->unget();

  return s;
}

void TextReader::ReadUntil(std::string& str, const char* term) {
  int len = (int)strlen(term);
  assert(len > 0);

  char* buf = (char*)_alloca(len);

  // initial fill buffer
  for (int i = 0; i < len; i++) {
    int value = m_stream->get();
    if (value == EOF)
      throw Error();
    buf[i] = static_cast<char>(value);
  }

  StringBuilder<char> builder(str);
  while (memcmp(buf, term, len)) {
    builder += buf[0];
    memmove(buf, buf + 1, len - 1);
    int value = m_stream->get();
    if (value == EOF)
      throw Error();
    buf[len - 1] = static_cast<char>(value);
  }
  builder.flush();
}

UINT ConvertEncodingToCodePage(Encoding encoding) {
  switch (encoding) {
    case EncodingAscii:
      return CP_ACP;
    case EncodingUtf7:
      return CP_UTF7;
    case EncodingUtf8:
      return CP_UTF8;
    default:
      return UINT(-1);
  }
}

static std::wstring DecodeString(UINT code_page, const std::string& str) {
  if (str.empty())
    return std::wstring();

  if (code_page == EncodingUnknown)
    code_page = CP_UTF8;

  int len = MultiByteToWideChar(code_page, MB_ERR_INVALID_CHARS, str.data(),
      (int)str.length(), 0, NULL);
  if (len < 0)
    throw Error();

  AutoBuffer<> buf(len*2);
  int res = MultiByteToWideChar(code_page, MB_ERR_INVALID_CHARS, str.data(),
      (int)str.length(), (LPWSTR)buf.buf, len);
  if (res < 0)
    throw Error();

  return std::wstring((wchar_t*)buf.buf, res);
}

static std::string EncodeString(UINT code_page, const std::wstring& str) {
  if (str.empty())
    return std::string();

  if (code_page == EncodingUnknown)
    code_page = CP_UTF8;

  int len = WideCharToMultiByte(code_page, 0, str.data(), (int)str.length(),
      0, NULL, NULL, NULL);
  if (len <= 0)
    throw Error();

  AutoBuffer<> buf(len);
  int res = WideCharToMultiByte(code_page, 0, str.data(), (int)str.length(),
      buf.buf, len, NULL, NULL);
  if (res <= 0)
    throw Error();

  return std::string(buf.buf, res);
}

void TextReader::Read() {
  assert(m_stream);

  if (m_stream->fail())
    throw Error();

  attributes.clear();
  name.clear();
  value.clear();
  empty = true;

  SkipSpaces();

  int ch = m_stream->get();

  if (ch != '<') {
    // Текстовая область
    std::string val(1, static_cast<char>(ch));
    StringBuilder<char> builder(val);
    while ((ch = m_stream->peek()) != EOF && ch != '<')
      builder += static_cast<char>(m_stream->get());
    builder.flush();
    node_type = NodeTypeText;
    value = DecodeString(encoding, val);
    ReplaceStringTags(value);
    return;
  }

  switch (m_stream->get()) {
    case '/':
      node_type = NodeTypeEndElement;
      break;

    case '!':
      ch = m_stream->get();
      if (ch == '-') {
        // Комментарий, оканичивается на -->
        if (m_stream->get() != '-')
          throw Error();
        node_type = NodeTypeComment;
        assert(value.empty());
        std::string val;
        ReadUntil(val, "-->");
        value = DecodeString(encoding, val);
        return;
      } else if (ch == '[') {
        // <![CDATA[
        static const char* term = "CDATA[";
        static int len = (int)strlen(term);
        const char* p = term;
        while (*p && *p == m_stream->get())
          p++;
        node_type = NodeTypeCDATA;
        std::string val;
        ReadUntil(val, "]]>");
        value = DecodeString(encoding, val);
        return;
      } else
        throw Error();
      break;

    case '?':
      node_type = NodeTypeDeclaration;
      break;

    default:
      node_type = NodeTypeElement;
      m_stream->unget();
      break;
  }

  name = ReadName();
  assert(!name.empty());

  // Читаем атрибуты

  while (TRUE) {
    SkipSpaces();
    
    switch (m_stream->get()) {
      case '>':
        // Конец тега
        empty = false;
        return;

      case '/':
        // Конец пустого тега
        // Проверим что это не закрывающий тег
        if (node_type != NodeTypeElement)
          throw Error();	// должен быть элемент
        if (m_stream->get() != '>')
          throw Error();	// ожидается />
        return;

      case '?':
        if (node_type != NodeTypeDeclaration)
          throw Error();
        if (m_stream->get() != '>')
          throw Error();	// ожидается ?>
        return;

      default: {
        m_stream->unget();
        std::string AttributeName = ReadName();
        assert(!AttributeName.empty());
        SkipSpaces();
        if (m_stream->get() != '=')
          throw Error();	// ожидается знак равенства
        SkipSpaces();
        if ((ch = m_stream->get()) != '"' && ch != '\'')
          throw Error();	// значение атрибута должно быть заключено в кавчыки
        char open = static_cast<char>(ch);
        // Читаем значение атрибута
        std::string AttributeValue;
        StringBuilder<char> builder(AttributeValue);
        while ((ch = m_stream->get()) != open) {
          if (ch == EOF)
            throw Error();	// конец файла при чтении значения атрибута
          builder += static_cast<char>(ch);
        }
        builder.flush();
        std::wstring val = DecodeString(encoding, AttributeValue);
        ReplaceStringTags(val);
        bool ok = attributes.insert(AttributeMap::value_type(AttributeName, val)).second;
        if (!ok)
          throw Error();	// повторяющийся атрибут
        break;
      }
    }
  }
}

bool TextReader::Eof() const {
  return m_stream->peek() == EOF;
}

// NodeReader

void NodeReader::Start(Node& parent) {
  path.clear();
  current = parent.first_child;
}

void NodeReader::Read() {
  attributes.clear();

  if (current) {
    Node& node = *current;
    node_type = node.type;
    name = node.name;
    value = node.value;
    empty = !node.first_child;
    attributes = node.attributes;
    if (empty)
      current = current->next;
    else {
      path.push_back(current);
      current = current->first_child;
    }

  } else {
    assert(!path.empty());

    current = path.back();
    path.pop_back();
    node_type = NodeTypeEndElement;
    name = current->name;
    value.clear();
    current = current->next;
  }
}

bool NodeReader::Eof() const {
  return path.empty() && !current;
}

// TextWriter

TextWriter::TextWriter()
    : encoding(EncodingUtf8),
      line_breaks(true),
      m_stream(NULL),
      node_level_(0) {
}

TextWriter::TextWriter(std::ostream& stream)
    : m_stream(&stream),
      encoding(EncodingUtf8),
      line_breaks(true),
      node_level_(0) {
}

void TextWriter::Write(Reader& reader) {
  switch (reader.node_type) {
    case NodeTypeElement:
      if (line_breaks)
        *m_stream << Indent(node_level_);
      *m_stream << "<" << reader.name;
      WriteAttributes(reader);
      if (reader.empty)
        *m_stream << " /";
      *m_stream << ">";
      if (line_breaks)
        *m_stream << std::endl;
      if (!reader.empty)
        ++node_level_;
      return;

    case NodeTypeEndElement:
      assert(node_level_ > 0);
      --node_level_;
      if (line_breaks)
        *m_stream << Indent(node_level_);
      *m_stream << "</" << reader.name << ">";
      if (line_breaks)
        *m_stream << std::endl;
      return;

    case NodeTypeText:
      *m_stream << EncodeString(encoding, reader.value);
      return;

    case NodeTypeComment:
      if (line_breaks)
        *m_stream << Indent(node_level_);
      *m_stream << "<!--" << EncodeString(encoding, reader.value) << "-->";
      if (line_breaks)
        *m_stream << std::endl;
      return;

    case NodeTypeDeclaration:
      if (line_breaks)
        *m_stream << Indent(node_level_);
      *m_stream << "<?" << reader.name;
      WriteAttributes(reader);
      *m_stream << "?>";
      if (line_breaks)
        *m_stream << std::endl;
      return;

    case NodeTypeCDATA:
      if (line_breaks)
        *m_stream << Indent(node_level_);
      *m_stream << "<![CDATA[" << EncodeString(encoding, reader.value) << "]]>";
      if (line_breaks)
        *m_stream << std::endl;
      return;

    default:
      throw Error();	// неопределенный тип элемента
  }
}

void TextWriter::WriteAttributes(Reader& reader) {
  assert(m_stream);
  for (AttributeMap::const_iterator i = reader.attributes.begin();
       i != reader.attributes.end(); ++i) {
    *m_stream << " " << i->first
              << "=\"" << EncodeString(encoding, i->second) << "\"";
  }
}

// Node

inline Node& alloc_node(NodeType type)
{
  return *(new Node(type));
}

inline void free_node(Node& node)
{
  node.clear();
  delete &node;
}

std::wstring Node::get_text() const
{
  for (const Node* node = first_child; node; node = node->next)
    if (node->type == NodeTypeText)
      return node->value;
  return std::wstring();
}

void Node::set_text(const wchar_t* text) {
  for (Node* node = first_child; node; node = node->next) {
    if (node->type == NodeTypeText) {
      node->value = text;
      return;
    }
  }
  
  AddChild(NodeTypeText).value = text;
}

const std::wstring& Node::get_value() const
{
  static const std::wstring empty_str;
  return first_child ? first_child->value : empty_str;
}

void Node::set_value(const wchar_t* val) {
  Node& node = first_child ? *first_child : AddChild(NodeTypeText);
  node.value = val;
}

bool Node::GetAttribute(const char* attr, std::string& val) const {
  std::wstring wval;
  if (!GetAttribute(attr, wval))
    return false;
    
  USES_CONVERSION;
  val = W2A(wval.c_str());
  return true;
}

void Node::SetAttribute(const char* attr, const char* val) {
  USES_CONVERSION;
  SetAttribute(attr, A2W(val));
}

Node& Node::AddChild(NodeType type) {
  Node& node = alloc_node(type);
  node.parent = this;
  if (last_child) {
    assert(!last_child->next);
    last_child->next = &node;
    last_child = &node;
  } else {
    first_child = &node;
    last_child = &node;
  }
  return node;
}

Node& Node::AddChildFirst(NodeType type)
{
  Node& node = alloc_node(type);
  node.parent = this;
  if (first_child) {
    node.next = first_child;
    first_child = &node;
  } else {
    first_child = &node;
    last_child = &node;
  }
  return node;
}

Node& Node::AddElement(const std::string& name)
{
  Node& node = AddChild(NodeTypeElement);
  node.name = name;
  return node;
}

Node& Node::AddElement(const char* name)
{
  Node& node = AddChild(NodeTypeElement);
  node.name = name;
  return node;
}

Node* Node::select(const char* name)
{
  for (Node* node = first_child; node; node = node->next)
    if (node->type == NodeTypeElement && node->name.compare(name) == 0)
      return node;
  return NULL;
}

const Node* Node::select(const char* name) const
{
  for (Node* node = first_child; node; node = node->next)
    if (node->type == NodeTypeElement && node->name.compare(name) == 0)
      return node;
  return NULL;
}

void Node::clear()
{
  while (first_child) {
    Node& node = *first_child;
    first_child = first_child->next;
    free_node(node);
  }
  last_child = NULL;
}

void Node::to_stream(std::ostream& stream) const
{
  TextWriter writer(stream);
  writer.encoding = EncodingAscii;
  NodeReader reader;
  reader.Start(*const_cast<Node*>(this));
  bool line_break = true;
  int level = 0;
  while (!reader.Eof()) {
    reader.Read();
    // new line before open tag
    if (!line_break && reader.node_type == xml::NodeTypeElement) {
      stream << std::endl;
      line_break = true;
    }
    if (reader.node_type == xml::NodeTypeEndElement)
      level--;
    // indent
    if (line_break) {
      for (int i = 0; i < level; ++i)
        stream.put('\t');
    }
    // tag
    writer.Write(reader);
    line_break = false;
    // level
    if (reader.node_type == xml::NodeTypeElement && !reader.empty)
      level++;
    // new line after close tag
    if ((reader.node_type == xml::NodeTypeElement && reader.empty) ||
        reader.node_type == xml::NodeTypeEndElement) {
      stream << std::endl;
      line_break = true;
    }
  }
}

std::string Node::to_string() const
{
  std::stringstream stream;
  to_stream(stream);
  return stream.str();
}

// Document

struct CodePage {
  const wchar_t* name;
  const wchar_t* alternative_name;
  Encoding enc;
};

static CodePage code_pages[] = {
  { L"utf-7", L"utf7", EncodingUtf7 },
  { L"utf-8", L"utf8", EncodingUtf8 },
};

static Encoding ParseEncoding(const wchar_t* name) {
  for (int i = 0; i < _countof(code_pages); ++i) {
    if (_wcsicmp(name, code_pages[i].name) == 0 ||
        _wcsicmp(name, code_pages[i].alternative_name) == 0)
      return code_pages[i].enc;
  }
  return EncodingUnknown;
}

static const wchar_t* FormatEncoding(Encoding enc) {
  for (int i = 0; i < _countof(code_pages); ++i)
    if (code_pages[i].enc == enc)
      return code_pages[i].name;
  return NULL;
}

Document::Document()
    : Node(NodeTypeDocument) {
}

void Document::Load(TextReader& reader) {
  clear();

  if (reader.encoding == EncodingUnknown)
    reader.encoding = EncodingAscii;

  Node* node = this;
  Node* decl = NULL;

  while (!reader.Eof()) {
    reader.Read();

    if (reader.node_type == NodeTypeEndElement) {
      // Закрывающий тег родительского элемента
      if (reader.name != node->name)
        throw Error();	// закрывающий тег с именем не соответствующим открывающему тегу
      if (!node->parent)
        return;
      node = node->parent;
      continue;
    }

    Node& child = node->AddChild(reader.node_type);
    child.name = reader.name;
    child.value = reader.value;
    child.attributes = reader.attributes;

    // read encoding
    if (!decl && node == this) {
      decl = GetDeclaration();
      if (decl)
        reader.encoding = GetEncoding();
    }

    // Читаем дочерние элементы
    if (!reader.empty)
      node = &child;
  }
}

void Document::Save(TextWriter& writer) {
  NodeReader reader;
  reader.encoding = GetEncoding();
  if (reader.encoding == EncodingUnknown)
    reader.encoding = EncodingAscii;
  reader.Start(*this);
  while (!reader.Eof()) {
    reader.Read();
    writer.Write(reader);
  }
}

Node* Document::GetDocumentElement() {
  for (Node* node = first_child; node; node = node->next)
    if (node->type == NodeTypeElement)
      return node;
  return NULL;
}

Node* Document::GetDeclaration(bool create) {
  if (first_child &&
      first_child->type == NodeTypeDeclaration &&
      first_child->name.compare("xml") == 0)
    return first_child;
    
  if (!create)
    return NULL;
    
  Node& node = AddChild(NodeTypeDeclaration);
  node.name = "xml";
  return &node;
}

Encoding Document::GetEncoding() const {
  const Node* decl = const_cast<Document*>(this)->GetDeclaration();
  if (!decl)
    return EncodingUnknown;
    
  std::wstring encoding = decl->GetAttribute("encoding");
  return ParseEncoding(encoding.c_str());
}

void Document::SetEncoding(Encoding enc) {
  Node* decl = GetDeclaration(true);
  assert(decl);
  const wchar_t* name = FormatEncoding(enc);
  if (name)
    decl->SetAttribute("encoding", name);
  else
    decl->attributes.erase("encoding");
}

void Document::SetVersion(const wchar_t* ver) {
  Node* decl = GetDeclaration(true);
  assert(decl);
  decl->SetAttribute("version", ver);
}

};
