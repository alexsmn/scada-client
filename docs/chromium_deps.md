# Chromium-Base Dependencies

Remaining chromium-base headers used in the client module.

## Threading / Task Runner
- `base/threading/thread_task_runner_handle.h` — `app/qt/main.cpp`, `app/wt/main.cpp`, `aui/test/qt/app_environment.h`, `aui/test/wt/app_environment.h`, `modus/tester/qt/main.cpp`, `vidicon/display_tester/qt/main.cpp`
- `base/pending_task.h`, `base/single_thread_task_runner.h` — `aui/qt/message_loop_qt.h`, `aui/wt/message_loop_wt.h`

## Task Runner Bridges (moved from core/base)
- `base/bind.h`, `base/task_runner.h` — `base/bind_util.h`
- `base/location.h` — `base/location_util.h`

## Lifecycle
- `base/at_exit.h` — `app/app_init.h`

## Memory
- `base/memory/ref_counted.h` — `base/pool.h`, `aui/os_exchange_data.h`
- `base/bind.h` — `services/connection_state_reporter.cpp`
- `base/memory_istream.h` — `modus/activex/modus_document.cpp`

## Windows COM / GDI
- `base/win/scoped_variant.h` — `base/excel.h`, `components/web/qt/web_view.cpp`, `modus/activex/modus_element.h`
- `base/win/scoped_bstr.h` — `vidicon/teleclient/vidicon_client_unittest.cpp`, `vidicon/display/activex/qt/vidicon_display_activex_view.cpp`, `components/web/qt/web_view.cpp`, `modus/activex/modus_element.h`, `modus/activex/modus_element.cpp`, `modus/activex/modus_document.cpp`
- `base/win/scoped_gdi_object.h` — `aui/qt/tree_model_adapter.cpp`, `aui/wt/tree_model_adapter.cpp`, `aui/qt/image_util.h`, `modus/qt/modus_view2.cpp`, `vidicon/display/native/qt/gdi_widget2.h`
- `base/win/scoped_hdc.h` — `modus/qt/modus_view2.cpp`, `vidicon/display/native/qt/gdi_widget2.h`
- `base/win/scoped_select_object.h` — `vidicon/display/native/qt/gdi_widget2.h`
- `base/win/scoped_hglobal.h` — `aui/os_exchange_data.cpp`
- `base/win/scoped_process_information.h` — `export/configuration/diff_report.cpp`
