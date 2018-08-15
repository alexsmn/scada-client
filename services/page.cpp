#include "services/page.h"

#include "base/base64.h"
#include "base/files/file_util.h"
#include "base/json/json_string_value_serializer.h"
#include "base/stl_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "base/utils.h"
#include "base/xml.h"
#include "services/profile_utils.h"
#include "window_info.h"

#define MAX_TITLE 30

namespace {

static const base::char16 kPageFileExtension[] = L"page";

void LoadWinItem(WindowItem& item, const xml::Node& node) {
  for (xml::AttributeMap::const_iterator i = node.attributes.begin();
       i != node.attributes.end(); ++i) {
    item.SetString(i->first.c_str(), i->second);
  }
}

void LoadWinItems(WindowItems& items, const xml::Node& node) {
  for (const xml::Node* child = node.first_child; child; child = child->next) {
    items.push_back(WindowItem());
    WindowItem& item = items.back();
    item.set_name(child->name);
    LoadWinItem(item, *child);
  }
}

// load win item attributes
void SaveWinItem(const WindowItem& item, xml::Node& node) {
  const base::DictionaryValue& attrs = item.attributes();
  for (base::DictionaryValue::Iterator i(attrs); !i.IsAtEnd(); i.Advance()) {
    base::string16 s;
    i.value().GetAsString(&s);
    node.SetAttribute(i.key().c_str(), s);
  }
}

void SaveWinItems(const WindowItems& items, xml::Node& elem) {
  for (WindowItems::const_iterator i = items.begin(); i != items.end(); i++) {
    const WindowItem& item = *i;
    assert(!item.name().empty());
    xml::Node& child = elem.AddElement(item.name());
    SaveWinItem(item, child);
  }
}

void LoadLayoutBlock(PageLayoutBlock& block, const xml::Node& node) {
  assert(block.type == PageLayoutBlock::PANE);
  assert(block.wins.empty());
  assert(!block.left && !block.right);

  const xml::Node* splite = node.select("Split");
  if (splite) {
    bool horz = splite->GetAttribute("orientation").compare(L"Vertical") != 0;
    block.split(horz);
    block.pos =
        ParseWithDefault<int>(splite->GetAttribute("position").c_str(), -1);
    const xml::Node* pane1e = splite->select("Pane1");
    const xml::Node* pane2e = splite->select("Pane2");
    if (pane1e)
      LoadLayoutBlock(*block.left, *pane1e);
    if (pane2e)
      LoadLayoutBlock(*block.right, *pane2e);

  } else {
    assert(block.type == PageLayoutBlock::PANE);
    for (const xml::Node* child = node.first_child; child;
         child = child->next) {
      if (child->type != xml::NodeTypeElement)
        continue;
      int id = ParseWithDefault<int>(child->GetAttribute("id").c_str(), 0);
      if (!id)
        continue;
      block.wins.push_back(id);
    }

    block.active_window =
        ParseWithDefault(node.GetAttribute("active").c_str(), -1);
  }

  block.central = ParseWithDefault<bool>(node.GetAttribute("central"), false);
}

const char* dock_names[4] = {"DockBottom", "DockTop", "DockLeft", "DockRight"};

void SaveLayoutBlock(const PageLayoutBlock& block, xml::Node& node) {
  if (block.type == PageLayoutBlock::SPLIT) {
    xml::Node& node2 = node.AddElement("Split");
    node2.SetAttribute("orientation", block.horz ? L"Horizontal" : L"Vertical");
    node2.SetAttribute("position", WideFormat(block.pos).c_str());
    SaveLayoutBlock(*block.left, node2.AddElement("Pane1"));
    SaveLayoutBlock(*block.right, node2.AddElement("Pane2"));

  } else {
    for (int i = 0; i < (int)block.wins.size(); ++i) {
      int id = block.wins[i];
      xml::Node& node2 = node.AddElement("Window");
      node2.SetAttribute("id", WideFormat(id).c_str());
    }

    if (block.active_window != -1)
      node.SetAttribute("active", WideFormat(block.active_window).c_str());
  }

  if (block.central)
    node.SetAttribute("central", WideFormat(true));
}

}  // namespace

// Page

Page::Page(const Page& source)
    : id{source.id}, title{source.title}, layout{source.layout} {
  windows_.reserve(source.windows_.size());
  for (auto& w : source.windows_)
    windows_.emplace_back(std::make_unique<WindowDefinition>(*w));
}

Page& Page::operator=(const Page& source) {
  if (&source != this) {
    id = source.id;
    title = source.title;
    layout = source.layout;

    windows_.clear();
    windows_.reserve(source.windows_.size());
    for (auto& w : source.windows_)
      windows_.emplace_back(std::make_unique<WindowDefinition>(*w));
  }

  return *this;
}

void Page::Load(const xml::Node& node) {
  std::wstring sid = node.GetAttribute("id");
  if (!sid.empty())
    id = ParseT<int>(sid);
  title = node.GetAttribute("title");

  const xml::Node* winse = node.select("Windows");
  if (winse) {
    for (const xml::Node* node = winse->first_child; node; node = node->next) {
      const xml::Node& win = *node;
      if (win.type != xml::NodeTypeElement)
        continue;

      const std::string& name = win.name;

      UINT type = ParseWindowType(win.GetAttributeA("type").c_str());
      const WindowInfo* info = FindWindowInfo(type);
      if (!info)
        continue;

      // check single window
      bool found = false;
      if (info->is_pane()) {
        for (int i = 0; i < GetWindowCount(); ++i)
          if (&GetWindow(i).window_info() == info) {
            found = true;
            break;
          }
      }
      if (found) {
        assert(false);
        // don't load window
        continue;
      }

      auto w = std::make_unique<WindowDefinition>(*info);
      w->id = ParseWithDefault<int>(win.GetAttribute("id"), 0);
      if (info->flags & WIN_SING)
        w->visible = ParseWithDefault(win.GetAttribute("visible"), true);
      w->title = win.GetAttribute("title");
      w->path = base::FilePath(win.GetAttribute("path"));
      w->size = gfx::Size(ParseT<unsigned>(win.GetAttribute("width")),
                          ParseT<unsigned>(win.GetAttribute("height")));
      LoadWinItems(w->items, win);

      std::string data = base::SysWideToNativeMB(win.get_text());
      JSONStringValueDeserializer serializer(data);
      w->set_storage(serializer.Deserialize(NULL, NULL));

      windows_.emplace_back(std::move(w));
    }
  }

  // fix page ids
  for (int i = 0; i < GetWindowCount(); ++i) {
    WindowDefinition& window = GetWindow(i);
    if (!window.id) {
      window.id = NewWindowId();
      continue;
    }
    for (int j = i + 1; j < GetWindowCount(); ++j)
      if (GetWindow(j).id == window.id)
        GetWindow(j).id = NewWindowId();
  }

  // layout
  const xml::Node* layoute = node.select("Layout");
  if (layoute) {
    if (const xml::Node* maine = layoute->select("Center"))
      LoadLayoutBlock(layout.main, *maine);
    for (int i = 0; i < 4; i++) {
      const xml::Node* docke = layoute->select(dock_names[i]);
      if (!docke)
        continue;
      auto& dock = layout.dock[i];
      dock.size = ParseWithDefault<int>(docke->GetAttribute("size").c_str(), 0);
      dock.place =
          ParseWithDefault<int>(docke->GetAttribute("place").c_str(), 0);
      LoadLayoutBlock(dock, *docke);
    }
    if (auto* blobe = layoute->select("Blob")) {
      auto blob = base::SysWideToNativeMB(blobe->get_text());
      base::Base64Decode(blob, &layout.blob);
    }
  }
}

void Page::Save(xml::Node& node, bool current) const {
  // page attributes
  if (id)
    node.SetAttribute("id", WideFormat(id));
  if (!title.empty())
    node.SetAttribute("title", title);

  xml::Node& winse = node.AddElement("Windows");
  for (int i = 0; i < GetWindowCount(); ++i) {
    WindowDefinition& def = GetWindow(i);
    const WindowInfo& window_info = def.window_info();

    xml::Node& win = winse.AddElement("Window");
    win.SetAttribute("type", window_info.name);
    win.SetAttribute("id", WideFormat(def.id));
    if (!def.visible) {
      assert(window_info.flags & WIN_SING);
      win.SetAttribute("visible", WideFormat(def.visible));
    }
    if (!def.title.empty())
      win.SetAttribute("title", def.title);
    if (!def.path.empty())
      win.SetAttribute("path", def.path.value());
    win.SetAttribute("width", WideFormat(def.size.width()));
    win.SetAttribute("height", WideFormat(def.size.height()));
    win.SetAttribute("locked", WideFormat(def.locked));

    SaveWinItems(def.items, win);

    if (def.storage()) {
      std::string data;
      JSONStringValueSerializer serializer(&data);
      serializer.Serialize(*def.storage());
      win.set_text(base::SysNativeMBToWide(data).c_str());
    }
  }

  // layout
  xml::Node& layoute = node.AddElement("Layout");
  if (!layout.main.empty())
    SaveLayoutBlock(layout.main, layoute.AddElement("Center"));
  for (int i = 0; i < 4; i++) {
    auto& dock = layout.dock[i];
    if (dock.empty())
      continue;
    xml::Node& docke = layoute.AddElement(dock_names[i]);
    docke.SetAttribute("size", WideFormat(dock.size));
    docke.SetAttribute("place", WideFormat(dock.place));
    SaveLayoutBlock(dock, docke);
  }
  if (!layout.blob.empty()) {
    std::string blob;
    base::Base64Encode(layout.blob, &blob);
    layoute.AddElement("Blob").set_text(base::SysNativeMBToWide(blob).c_str());
  }
}

base::string16 Page::GetTitle() const {
  if (!title.empty())
    return title;

  base::string16 title;
  for (int i = 0; i < GetWindowCount(); ++i) {
    WindowDefinition& win = GetWindow(i);
    if (!title.empty())
      title += L", ";
    title += win.title;
    if (title.length() >= MAX_TITLE)
      break;
  }

  if (title.length() > MAX_TITLE) {
    title.erase(MAX_TITLE, title.length() - MAX_TITLE);
    title += L"...";
  }

  if (title.empty())
    title = L"(Пустой)";

  return title;
}

int Page::NewWindowId() {
  static int _id = 1;
  while (FindWindowDef(_id))
    _id++;
  return _id++;
}

WindowDefinition& Page::AddWindow(const WindowDefinition& window) {
  auto w = std::make_unique<WindowDefinition>(window);
  w->id = NewWindowId();
  return *windows_.emplace_back(std::move(w));
}

WindowDefinition* Page::FindWindowDef(int id) {
  for (int i = 0; i < GetWindowCount(); ++i) {
    WindowDefinition& win = GetWindow(i);
    if (win.id == id)
      return &win;
  }
  return NULL;
}

const WindowDefinition* Page::FindWindowDef(int id) const {
  return const_cast<Page*>(this)->FindWindowDef(id);
}

int Page::FindWindowDef(const WindowDefinition& window) const {
  auto i = std::find_if(windows_.begin(), windows_.end(),
                        [&window](auto& p) { return p.get() == &window; });
  return i != windows_.end() ? i - windows_.begin() : -1;
}

void Page::DeleteWindow(int index) {
  DCHECK_GE(index, 0);
  windows_.erase(windows_.begin() + index);
}

void Page::Clear() {
  windows_.clear();
}

void Page::SaveJson(base::DictionaryValue& json) const {}

base::DictionaryValue Page::LoadJson() {
  return {};
}
