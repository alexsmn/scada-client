#include "explorer_selection.h"

#include "screenshot_wait.h"
#include "view_capture.h"

#include "aui/translation.h"
#include "aui/tree.h"
#include "main_window/main_window.h"
#include "main_window/opened_view/opened_view.h"
#include "node_service/node_service.h"

#include <QAbstractItemModel>
#include <QAbstractProxyModel>
#include <QApplication>
#include <QDockWidget>
#include <QItemSelectionModel>
#include <QLabel>
#include <QModelIndex>
#include <QStackedWidget>
#include <QString>
#include <QStringList>
#include <gtest/gtest.h>

#include <array>
#include <span>

namespace scada::screenshot_generator {
namespace {

// Walks the Explorer tree from its visible root down `path` (one display name
// per level), fetching and expanding each level on the way, and returns the
// index of the last name. The rows are lazily loaded, so a level has to be
// fetched and settled before the next name can be looked for.
//
// An invalid index means a step was not found, and the failure names the level
// and the rows it did see: the caller uses this to put a selection on screen,
// and a silently missed row would publish the state the selection exists to
// replace.
QModelIndex FindTreeRowByPath(scada::aui::Tree& tree,
                              AnyExecutor executor,
                              NodeService& node_service,
                              std::span<const QString> path) {
  QModelIndex parent = tree.rootIndex();
  for (size_t level = 0; level < path.size(); ++level) {
    const QString& name = path[level];
    QModelIndex found;
    const bool level_has_name = WaitUntil([&] {
      if (tree.model()->canFetchMore(parent))
        tree.model()->fetchMore(parent);
      for (int row = 0; row < tree.model()->rowCount(parent); ++row) {
        const QModelIndex index = tree.model()->index(row, 0, parent);
        if (index.data(Qt::DisplayRole).toString() == name) {
          found = index;
          return true;
        }
      }
      return false;
    });

    if (!level_has_name) {
      QStringList seen;
      for (int row = 0; row < tree.model()->rowCount(parent); ++row) {
        seen << tree.model()
                    ->index(row, 0, parent)
                    .data(Qt::DisplayRole)
                    .toString();
      }
      ADD_FAILURE() << "Explorer row not found: " << name.toStdString()
                    << " | siblings="
                    << seen.join(QLatin1String(", ")).toStdString();
      return {};
    }

    // Expanding fetches the next level; the leaf is left as the operator would
    // leave it, so selecting a signal does not open a branch under it.
    if (level + 1 < path.size()) {
      tree.expand(found);
      if (!WaitForPendingNodeLoads(executor, node_service))
        return {};
    }
    parent = found;
  }
  return parent;
}

}  // namespace

bool MaterializeExplorerTree(scada::aui::Tree& tree,
                             QDockWidget* tree_dock,
                             AnyExecutor executor,
                             NodeService& node_service) {
  tree.show();

  for (int i = 0; i < 10; ++i)
    QApplication::processEvents();

  // The Explorer tree already roots itself at the synthetic tree root, so its
  // rootIndex IS the node whose children are the object rows — do not descend
  // another level or the capture would show one branch instead of the tree.
  // The fallback covers a tree that has not had its root index applied.
  QModelIndex root_index = tree.rootIndex();
  if (!root_index.isValid())
    root_index = tree.model()->index(0, 0);
  if (!root_index.isValid()) {
    ADD_FAILURE() << "the Explorer tree has no valid root index";
    return false;
  }

  if (tree.model()->canFetchMore(root_index))
    tree.model()->fetchMore(root_index);
  if (!WaitForPendingNodeLoads(executor, node_service)) {
    ADD_FAILURE() << "the Explorer tree's root never finished loading";
    return false;
  }

  // Re-asserted rather than assumed: this also sidesteps the "expanded root
  // with empty child viewport" state that Qt sometimes gets into here.
  tree.setRootIndex(root_index);
  tree.setRootIsDecorated(true);
  tree.expand(tree.rootIndex());
  if (!WaitUntil([&] { return tree.isExpanded(tree.rootIndex()); })) {
    ADD_FAILURE() << "the Explorer tree's root never expanded";
    return false;
  }
  tree.expandRecursively(tree.rootIndex(), 3);
  tree.resizeColumnToContents(0);
  tree.doItemsLayout();
  tree.viewport()->update();

  auto* proxy_model = qobject_cast<QAbstractProxyModel*>(tree.model());
  QModelIndex materialized_source_root;
  const bool first_child_visible = WaitUntil(
      [&] {
        const auto& visible_root = tree.rootIndex();
        if (tree.model()->canFetchMore(visible_root))
          tree.model()->fetchMore(visible_root);

        auto first_child = tree.model()->index(0, 0, visible_root);
        if (!first_child.isValid() && proxy_model) {
          materialized_source_root = proxy_model->mapToSource(visible_root);
          if (materialized_source_root.isValid()) {
            const auto source_first_child = proxy_model->sourceModel()->index(
                0, 0, materialized_source_root);
            if (source_first_child.isValid())
              first_child = proxy_model->mapFromSource(source_first_child);
          }

          if (!first_child.isValid() && materialized_source_root.isValid() &&
              proxy_model->sourceModel()->rowCount(materialized_source_root) >
                  0) {
            tree.model()->sort(0);
            tree.collapse(visible_root);
            QApplication::processEvents();
            tree.expand(visible_root);
            first_child = tree.model()->index(0, 0, visible_root);

            if (!first_child.isValid()) {
              const auto source_first_child = proxy_model->sourceModel()->index(
                  0, 0, materialized_source_root);
              if (source_first_child.isValid())
                first_child = proxy_model->mapFromSource(source_first_child);
            }
          }
        }

        if (!first_child.isValid())
          return false;

        tree.scrollTo(first_child);
        tree.doItemsLayout();
        tree.viewport()->update();
        return !tree.visualRect(first_child).isEmpty();
      },
      2000);
  int source_child_count = -1;
  if (proxy_model && materialized_source_root.isValid()) {
    source_child_count =
        proxy_model->sourceModel()->rowCount(materialized_source_root);
  }

  if (!first_child_visible) {
    const auto& visible_root = tree.rootIndex();
    const auto proxy_child_count = tree.model()->rowCount(visible_root);
    const auto first_child = tree.model()->index(0, 0, visible_root);
    const auto first_child_rect =
        first_child.isValid() ? tree.visualRect(first_child) : QRect{};
    const auto root_rect = tree.visualRect(visible_root);
    const auto viewport_size = tree.viewport()->size();

    ADD_FAILURE() << "Struct tree did not materialize visible rows before "
                  << "capture"
                  << " | proxy_child_count=" << proxy_child_count
                  << " | source_child_count=" << source_child_count
                  << " | viewport=" << viewport_size.width() << "x"
                  << viewport_size.height() << " | root_rect=" << root_rect.x()
                  << "," << root_rect.y() << " " << root_rect.width() << "x"
                  << root_rect.height()
                  << " | first_child_valid=" << first_child.isValid()
                  << " | first_child_rect=" << first_child_rect.x() << ","
                  << first_child_rect.y() << " " << first_child_rect.width()
                  << "x" << first_child_rect.height()
                  << " | tree_visible=" << tree.isVisible()
                  << " | viewport_visible=" << tree.viewport()->isVisible()
                  << " | dock_visible="
                  << (tree_dock ? tree_dock->isVisible() : true);
  }
  if (!first_child_visible)
    return false;
  const bool rows_settled = WaitUntil([&] {
    const auto loading_suffix =
        QString::fromStdU16String(u"[" + Translate("Loading") + u"]");
    for (int row = 0; row < tree.model()->rowCount(tree.rootIndex()); ++row) {
      const auto index = tree.model()->index(row, 0, tree.rootIndex());
      if (index.data(Qt::DisplayRole).toString().contains(loading_suffix))
        return false;
    }
    return true;
  });
  if (!rows_settled) {
    ADD_FAILURE() << "the Explorer tree still shows [Loading] rows";
    return false;
  }
  for (int i = 0; i < 10; ++i)
    QApplication::processEvents();
  return true;
}

void SelectSignalForInspector(MainWindow& main_window,
                              OpenedView& explorer_view,
                              scada::aui::Tree& tree,
                              AnyExecutor executor,
                              NodeService& node_service,
                              QWidget& window) {
  // Активная мощность is an analog item under the fixture's telemetry folder,
  // and it is the signal two of the journal rows are about — so the window
  // shows one story rather than two.
  const std::array<QString, 3> inspected_path = {
      QStringLiteral("ЭСТРА-ПС"), QStringLiteral("ТИ"),
      QStringLiteral("Активная мощность")};
  const QModelIndex inspected =
      FindTreeRowByPath(tree, executor, node_service, inspected_path);
  ASSERT_TRUE(inspected.isValid());

  main_window.ActivateView(explorer_view);
  tree.scrollTo(inspected);
  tree.selectionModel()->setCurrentIndex(
      inspected,
      QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

  // The card fills in two steps — ShowSelection switches to the element page
  // synchronously, and the readout arrives with the value — so wait for the
  // second one. A "—" readout is the failure this guards: it means the panel is
  // on screen holding nothing, which is the state the selection was made to
  // avoid.
  auto* stack = window.findChild<QStackedWidget*>("inspectorStack");
  ASSERT_NE(stack, nullptr);
  auto* value = window.findChild<QLabel*>("inspectorValue");
  ASSERT_NE(value, nullptr);
  const bool filled = WaitUntil([&] {
    return stack->currentIndex() == 1 && !value->text().isEmpty() &&
           value->text() != QStringLiteral("—");
  });
  EXPECT_TRUE(filled) << "Inspector did not fill for the Explorer selection"
                      << " | page=" << stack->currentIndex()
                      << " | value=" << value->text().toStdString();

  // The Measurements limits block arrives on its own schedule: the bands are
  // property children, so the panel asks the shell to fetch them
  // (InspectorPanelContext::load_limits) and redraws when they land. Asserting
  // it is the point — a card that renders a live value with no limits beside it
  // is what principles.md §2 exists to prevent, and it is exactly what this
  // capture published until the fetch was wired.
  auto* limits = window.findChild<QWidget*>("inspectorLimits");
  ASSERT_NE(limits, nullptr);
  EXPECT_TRUE(WaitUntil([&] { return limits->isVisible(); }))
      << "Inspector limits block stayed hidden: the node's limit bands never "
         "became readable";
}

}  // namespace scada::screenshot_generator
