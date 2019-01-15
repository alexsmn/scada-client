#include "components/statistics/views/statistics_view.h"

#include "controller_factory.h"
#include "ui/views/controls/table/table_view.h"

static const base::char16* kStatisticTitles[] = {
    L"Состояние связи",        L"Принято байт",         L"Отправлено байт",
    L"Принято сообщений",      L"Отправлено сообщений", L"Сообщений в очереди",
    L"Байт в буфере приема",   L"Обновляется таблиц",   L"Выполнено транзакций",
    L"Выполняется транзакций", L"Обновлено записей"};

// StatisticsView

const WindowInfo kWindowInfo = {ID_STATISTICS_VIEW, "Stat", L"Статус",
                                WIN_SING,           300,    400};

REGISTER_CONTROLLER(StatisticsView, kWindowInfo);

StatisticsView::StatisticsView(const ControllerContext& context)
    : Controller{context} {
  ui::TableColumn columns[] = {
      {0, L"Параметр", 150, ui::TableColumn::LEFT},
      {1, L"Значение", 100, ui::TableColumn::RIGHT},
  };

  table_.reset(new views::TableView(*this));
  table_->SetColumns(std::size(columns), columns);

  update_timer_.Start(FROM_HERE, base::TimeDelta::FromMilliseconds(500), this,
                      &StatisticsView::UpdateStatistics);
}

views::View* StatisticsView::Init(const WindowDefinition& definition) {
  return table_->CreateParentIfNecessary();
}

void StatisticsView::UpdateStatistics() {
  NotifyItemsChanged(0, _countof(kStatisticTitles));
}

int StatisticsView::GetRowCount() {
  return _countof(kStatisticTitles);
}

void StatisticsView::GetCell(ui::TableCell& cell) {
  DCHECK(cell.column_id >= 0 && cell.column_id < 2);
  DCHECK(cell.row >= 0 && cell.row < _countof(kStatisticTitles));

  if (cell.column_id == 0) {
    cell.text = kStatisticTitles[cell.row];

  } else {
    // switch (cell.row) {
    //  case 0:
    //	  cell.text = GetServerConnectionStatusString();
    //	  break;
    //  case 1:
    //	  cell.text =
    // base::IntToString16(g_session->GetStatistics(SessionProxy::kNumBytesReceived));
    //	  break;
    //  case 2:
    //	  cell.text =
    // base::IntToString16(g_session->GetStatistics(SessionProxy::kNumBytesSent));
    //	  break;
    //  case 3:
    //	  cell.text =
    // base::IntToString16(g_session->GetStatistics(SessionProxy::kNumMessagesReceived));
    //	  break;
    //  case 4:
    //	  cell.text =
    // base::IntToString16(g_session->GetStatistics(SessionProxy::kNumMessagesSent));
    //	  break;
    //  case 5:
    //	  cell.text =
    // base::IntToString16(g_session->GetStatistics(SessionProxy::kNumBytesQueued));
    //	  break;
    //  case 6:
    //	  /*{
    //		  u_long nbytes;
    //		  if (!ioctlsocket(g_client_session.sock, FIONREAD, &nbytes))
    //			  cell.text.Format(_T("%lu"), nbytes);
    //	  }*/
    //	  break;
    //  case 7:
    //	  // cell.text = format(g_session->refr_map.size());
    //	  break;
    //  case 8:
    //	  // cell.text =
    // Format(g_session->GetStatistics(SessionProxy::kNumCompletedRequests));
    //	  break;
    //  case 9:
    //	  cell.text =
    // base::IntToString16(g_session->GetStatistics(SessionProxy::kNumRunningRequests));
    //	  break;
    //  case 10:
    //	  cell.text =
    // base::IntToString16(g_session->GetStatistics(SessionProxy::kNumUpdatedRecords));
    //	  break;
    //}
  }
}
