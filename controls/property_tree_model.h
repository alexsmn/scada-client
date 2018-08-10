#pragma once

#include "controls/property_model.h"
#include "ui/base/models/tree_model.h"

class PropertyTreeModel : public ui::TreeModel {
 public:
  explicit PropertyTreeModel(PropertyModel& property_model);
  ~PropertyTreeModel();

  // ui::TreeModel
  virtual int GetColumnCount() const { return 2; }
  virtual base::string16 GetColumnText(int column_id) const;
  virtual void* GetRoot() override { return this; }
  virtual void* GetParent(void* node) override;
  virtual int GetChildCount(void* parent) override;
  virtual void* GetChild(void* parent, int index) override;
  virtual base::string16 GetText(void* node, int column_id) override;
  virtual void SetText(void* node,
                       int column_id,
                       const base::string16& text) override;
  virtual bool IsEditable(void* node, int column_id) const override;
  virtual ui::EditData GetEditData(void* node, int column_id) override;

 private:
  int NodeToIndex(void* node) const;
  void* IndexToNode(int index) const;

  void PropertiesChanged(int first, int count);

  PropertyModel& property_model_;
};
