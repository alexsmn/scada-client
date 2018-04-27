#include "components/modus/qt/modus_view.h"

#include <QUuid>

ModusView::ModusView(ModusDocumentContext&& context)
    : ModusDocumentContext{std::move(context)} {
}

ModusView::~ModusView() {}

void ModusView::Open(const base::FilePath& path) {
  assert(!document_);

  path_ = path;

  if (!setControl("{001F373C-29D3-5C7E-A000-A0FC803D82EE}"))
    setControl("{001F373C-29D3-5F7E-A000-A0FC803D82EE}");

  base::win::ScopedComPtr<htsde2::IHTSDEForm2> sde_form;
  queryInterface(__uuidof(htsde2::IHTSDEForm2), sde_form.ReceiveVoid());
  if (!sde_form)
    return;

  document_ = std::make_unique<modus::ModusDocument>(
      ModusDocumentContext{*this}, *sde_form, path_);
  title_callback_(document_->title());
}

base::FilePath ModusView::GetPath() const {
  return path_;
}

bool ModusView::ShowContainedItem(const scada::NodeId& item_id) {
  return document_ && document_->ShowContainedItem(item_id);
}

htsde2::IHTSDEForm2* ModusView::GetSdeForm() {
  return document_ ? &document_->sde_form() : nullptr;
}
