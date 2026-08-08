<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="ru_RU">
<context>
    <name></name>
    <!-- Device log frame-trace columns (modules/watch). -->
    <message>
        <source>Type ID</source>
        <translation>Тип</translation>
    </message>
    <message>
        <source>Cause</source>
        <translation>Причина</translation>
    </message>
    <message>
        <source>IOA</source>
        <!-- Short on purpose: it is a column header beside Тип and Причина,
             where "Адрес" is unambiguous, and the full "Адрес объекта" clips. -->
        <translation>Адрес</translation>
    </message>
    <!-- Device log filter bar (modules/watch). "I-format" / "S/U-format" are
         the IEC 60870-5-104 §5.1 APCI format names and stay as-is. -->
    <message>
        <source>All frames</source>
        <translation>Все кадры</translation>
    </message>
    <message>
        <source>I-format</source>
        <translation>Формат I</translation>
    </message>
    <message>
        <source>S/U-format</source>
        <translation>Формат S/U</translation>
    </message>
    <message>
        <source>Errors only</source>
        <translation>Только ошибки</translation>
    </message>
    <message>
        <source>Filter by IOA, type or cause</source>
        <translation>Фильтр по адресу, типу или причине</translation>
    </message>
    <!-- Table view: what a row is bound to — a NodeId, or the expression for
         a computed row (modules/table). -->
    <message>
        <source>Source</source>
        <translation>Источник</translation>
    </message>
    <!-- Status strip: an armed frame capture (main_window/status_bar). -->
    <message>
        <source>Capturing</source>
        <translation>Идёт захват</translation>
    </message>
    <!-- Status strip: the ping cell when the round trip crosses the stall
         threshold, and the local events that announce the stall and its end
         (main_window/status_bar/session_status_provider.cpp). -->
    <message>
        <source>no response</source>
        <translation>нет ответа</translation>
    </message>
    <message>
        <source>The server has not answered a ping for </source>
        <translation>Сервер не отвечает на опрос уже </translation>
    </message>
    <message>
        <source> ms. Either the connection is degraded, or the client itself has stopped running: the operating system can suspend a client whose window is not visible, which halts data and events, not only the display.</source>
        <translation> мс. Либо ухудшилось соединение, либо остановился сам клиент: операционная система может приостановить клиент, окно которого не видно, и тогда прекращается приём данных и событий, а не только обновление экрана.</translation>
    </message>
    <message>
        <source>The server is answering again.</source>
        <translation>Сервер снова отвечает.</translation>
    </message>
    <!-- APCI columns (IEC 60870-5-104 §5.1). N(S)/N(R) is the standard's own
         notation for the send/receive sequence numbers and is not translated,
         like RX/TX below. -->
    <message>
        <source>Fmt</source>
        <translation>Форм.</translation>
    </message>
    <message>
        <source>N(S)/N(R)</source>
        <translation>N(S)/N(R)</translation>
    </message>
    <!-- Device log frame-trace mode (modules/watch). RX/TX are the
         protocol-standard direction abbreviations and are kept as-is
         in Russian, matching the drivers' own #RX:/$TX: markers. -->
    <message>
        <source>Frame trace</source>
        <translation>Трассировка кадров</translation>
    </message>
    <message>
        <source>Dir</source>
        <translation>Напр.</translation>
    </message>
    <message>
        <source>RX</source>
        <translation>RX</translation>
    </message>
    <message>
        <source>TX</source>
        <translation>TX</translation>
    </message>
    <!-- Frame-decode pane (modules/watch). APCI, ASDU and the type/cause
         mnemonics are the standard's own notation and stay as they are; the
         field labels around them are ordinary UI text. -->
    <message>
        <source>APCI</source>
        <translation>APCI</translation>
    </message>
    <message>
        <source>ASDU</source>
        <translation>ASDU</translation>
    </message>
    <message>
        <source>Start</source>
        <translation>Начало</translation>
    </message>
    <message>
        <source>APDU length</source>
        <translation>Длина APDU</translation>
    </message>
    <message>
        <source>N(S) send</source>
        <translation>N(S) передача</translation>
    </message>
    <message>
        <source>N(R) recv</source>
        <translation>N(R) приём</translation>
    </message>
    <message>
        <source>I-format</source>
        <translation>Формат I</translation>
    </message>
    <message>
        <source>S-format</source>
        <translation>Формат S</translation>
    </message>
    <message>
        <source>U-format</source>
        <translation>Формат U</translation>
    </message>
    <message>
        <source>SQ / count</source>
        <translation>SQ / кол-во</translation>
    </message>
    <message>
        <source>Cause (COT)</source>
        <translation>Причина (COT)</translation>
    </message>
    <message>
        <source>Originator</source>
        <translation>Инициатор</translation>
    </message>
    <message>
        <source>Common address</source>
        <translation>Общий адрес</translation>
    </message>
    <message>
        <source>Information objects</source>
        <translation>Объекты информации</translation>
    </message>
    <message>
        <source>Command</source>
        <translation>Команда</translation>
    </message>
    <message>
        <source>Qualifier</source>
        <translation>Квалификатор</translation>
    </message>
    <message>
        <source>Sequence</source>
        <translation>Последовательность</translation>
    </message>
    <message>
        <source>Field</source>
        <translation>Поле</translation>
    </message>
    <message>
        <source>Offset</source>
        <translation>Смещение</translation>
    </message>
    <message>
        <source>bytes</source>
        <translation>байт</translation>
    </message>
    <message>
        <source>Mapped node</source>
        <translation>Привязка</translation>
    </message>
    <message>
        <source>Not in the device&apos;s address map</source>
        <translation>Нет в карте адресов устройства</translation>
    </message>
    <message>
        <source>No frame selected</source>
        <translation>Кадр не выбран</translation>
    </message>
    <message>
        <source>No octets were captured for this line.</source>
        <translation>Для этой строки октеты не записаны.</translation>
    </message>
    <message>
        <source>Not an IEC 60870-5-104 APDU.</source>
        <translation>Не APDU МЭК 60870-5-104.</translation>
    </message>
    <!-- File-store message-box titles (modules/filesystem). -->
    <message>
        <source>Open File</source>
        <translation>Открыть файл</translation>
    </message>
    <message>
        <source>Add File</source>
        <translation>Добавить файл</translation>
    </message>
    <!-- Export/import and resource-error messages. All reach the operator
         through RunMessageBox (ResourceError text via ShowResourceError).
         Translate() looks up the empty context and lupdate never sees it. -->
    <message>
        <source>Error</source>
        <translation>Ошибка</translation>
    </message>
    <message>
        <source>No item</source>
        <translation>Нет элемента</translation>
    </message>
    <message>
        <source>Export failed. Please check that Microsoft Excel is installed correctly.</source>
        <translation>Не удалось выполнить экспорт. Проверьте, что Microsoft Excel установлен корректно.</translation>
    </message>
    <message>
        <source>Failed to open Notepad</source>
        <translation>Не удалось открыть Блокнот</translation>
    </message>
    <message>
        <source>Failed to open report</source>
        <translation>Не удалось открыть отчёт</translation>
    </message>
    <!-- modules/write/write_model.cpp — the control-command review an
         operator answers before an irreversible field action
         (docs/ux/principles.md §7). Goes through Translate(), so it
         lives in the empty context and lupdate never sees it. -->
    <message>
        <source>The remote device is ready to execute the command.</source>
        <translation>Устройство готово выполнить команду.</translation>
    </message>
    <message>
        <source>Present:</source>
        <translation>Текущее:</translation>
    </message>
    <message>
        <source>Command:</source>
        <translation>Команда:</translation>
    </message>
    <message>
        <source>This control command is sent to physical equipment and cannot be undone remotely. Send it?</source>
        <translation>Эта команда управления передаётся на оборудование и не может быть отменена дистанционно. Отправить?</translation>
    </message>
    <message>
        <source>Search tags, objects, commands…</source>
        <translation>Поиск объектов, сигналов, команд…</translation>
    </message>
    <!-- common/common/format.cpp default value-formatting text: the fallback
         state labels for a two-state item without its own TsFormat labels, and
         the placeholder for an unresolvable display name. -->
    <message>
        <source>On</source>
        <translation>Вкл</translation>
    </message>
    <message>
        <source>Off</source>
        <translation>Откл</translation>
    </message>
    <message>
        <source>#NAME?</source>
        <translation>#ИМЯ?</translation>
    </message>
    <!-- main_window/activity_bar section labels + overview_page.cpp -->
    <message>
        <source>Overview</source>
        <translation>Обзор</translation>
    </message>
    <message>
        <source>Alarms</source>
        <translation>Тревоги</translation>
    </message>
    <message>
        <source>Trends</source>
        <translation>Тренды</translation>
    </message>
    <message>
        <source>Substations</source>
        <translation>Подстанции</translation>
    </message>
    <message>
        <source>Tables</source>
        <translation>Таблицы</translation>
    </message>
    <message>
        <source>Administration</source>
        <translation>Администрирование</translation>
    </message>
    <!-- main_window/command_palette_qt.cpp + tag search -->
    <message>
        <source>Type a command…</source>
        <translation>Введите команду…</translation>
    </message>
    <message>
        <source>tag</source>
        <translation>тег</translation>
    </message>
    <!-- main_window/main_window_qt.cpp (alarm-flood pill) -->
    <message>
        <source>Alarm flood</source>
        <translation>Поток тревог</translation>
    </message>
    <!-- modules/graph series inspector + limit markers (2.4 trend workspace) -->
    <message>
        <source>Series</source>
        <translation>Серия</translation>
    </message>
    <message>
        <source>Colour</source>
        <translation>Цвет</translation>
    </message>
    <message>
        <source>Appearance</source>
        <translation>Вид</translation>
    </message>
    <message>
        <source>Own pane</source>
        <translation>Отдельная область</translation>
    </message>
    <message>
        <source>Show dots</source>
        <translation>Показывать точки</translation>
    </message>
    <message>
        <source>Stepped</source>
        <translation>Ступенчато</translation>
    </message>
    <message>
        <source>Y-axis</source>
        <translation>Ось Y</translation>
    </message>
    <message>
        <source>Auto</source>
        <translation>Авто</translation>
    </message>
    <message>
        <source>Limits &amp; annotations</source>
        <translation>Уставки и аннотации</translation>
    </message>
    <message>
        <source>No limits configured</source>
        <translation>Уставки не заданы</translation>
    </message>
    <message>
        <source>Node</source>
        <translation>Узел</translation>
    </message>
    <message>
        <source>Alarm low</source>
        <translation>Аварийная нижняя</translation>
    </message>
    <message>
        <source>Warning low</source>
        <translation>Предупредительная нижняя</translation>
    </message>
    <message>
        <source>Warning high</source>
        <translation>Предупредительная верхняя</translation>
    </message>
    <message>
        <source>Alarm high</source>
        <translation>Аварийная верхняя</translation>
    </message>
    <!-- main_window/main_window_qt.cpp (Settings menu) -->
    <message>
        <source>Settings</source>
        <translation>Настройки</translation>
    </message>
    <!-- main_window/main_window_qt.cpp + pages/page_commands.cpp: the activity
         rail's page context menu. "Open page", "Move up"/"Move down" and
         "Delete page" are rail-only; "Duplicate" is also a Page-menu command. -->
    <message>
        <source>Open page</source>
        <translation>Открыть страницу</translation>
    </message>
    <message>
        <source>Duplicate</source>
        <translation>Дублировать</translation>
    </message>
    <message>
        <source>Move up</source>
        <translation>Переместить вверх</translation>
    </message>
    <message>
        <source>Move down</source>
        <translation>Переместить вниз</translation>
    </message>
    <message>
        <source>Delete page</source>
        <translation>Удалить страницу</translation>
    </message>
    <!-- Suffix appended to a duplicated page's title: "Обзор — копия". -->
    <message>
        <source>copy</source>
        <translation>копия</translation>
    </message>
    <!-- main_window/main_menu/main_menu_model.cpp: the single Settings menu
         item that opens the preferences dialog (SettingsDialog). The ellipsis
         is the platform convention for "this opens a dialog" and is kept in
         the translation. -->
    <message>
        <source>Settings...</source>
        <translation>Настройки...</translation>
    </message>
    <!-- main_window/main_menu/main_menu_model.cpp: Settings -> Colour scheme,
         the experimental UX design-token themes. Deliberately not "Appearance",
         which Translate() already maps to "Вид" for the series inspector. -->
    <message>
        <source>Colour scheme</source>
        <translation>Цветовая схема</translation>
    </message>
    <!-- main_window/main_window_module.cpp: Settings -> Language. Both entries
         were missing entirely, so a Russian client listed its own languages as
         "English" and "Russian" - the settings dialog rendered "Язык: Russian"
         beside a form of Russian labels. Named in Russian rather than as
         autonyms ("English"/"Русский") to match every other row of the dialog,
         which is translated rather than shown in its own locale. -->
    <message>
        <source>English</source>
        <translation>Английский</translation>
    </message>
    <message>
        <source>Russian</source>
        <translation>Русский</translation>
    </message>
    <message>
        <source>Classic</source>
        <translation>Классическое</translation>
    </message>
    <message>
        <source>Follow system</source>
        <translation>Как в системе</translation>
    </message>
    <message>
        <source>Dark</source>
        <translation>Тёмное</translation>
    </message>
    <message>
        <source>Light</source>
        <translation>Светлое</translation>
    </message>
    <message>
        <source>High contrast</source>
        <translation>Высокая контрастность</translation>
    </message>
    <message>
        <source>The colours have changed. The workbench layout — activity bar, context bar and Inspector — follows when the client is restarted.</source>
        <translation>Цвета изменены. Компоновка рабочей среды — панель разделов, контекстная панель и Инспектор — изменится после перезапуска клиента.</translation>
    </message>
    <!-- main_window/status_bar/event_status_provider.cpp
         + modules/events/qt/severity_tile_strip.cpp (KPI severity tiles) -->
    <message>
        <source>Critical</source>
        <translation>Критично</translation>
    </message>
    <message>
        <source>Warning</source>
        <translation>Предупреждение</translation>
    </message>
    <message>
        <source>Unacknowledged</source>
        <translation>Не квитировано</translation>
    </message>
    <!-- modules/login/login_controller.cpp (security_mode_list) -->
    <message>
        <source>No security</source>
        <translation>Нет безопасности</translation>
    </message>
    <message>
        <source>Most secure available</source>
        <translation>Максимальная безопасность</translation>
    </message>
    <message>
        <source>Sign and encrypt</source>
        <translation>Подпись и шифрование</translation>
    </message>
    <!-- modules/events/event_table_model.cpp (journal alarm surface) -->
    <message>
        <source>— pending —</source>
        <translation>— ожидает —</translation>
    </message>
    <!-- modules/table (reshell quality marks) -->
    <message>
        <source>Quality</source>
        <translation>Достоверность</translation>
    </message>
    <!-- modules/table, modules/timed_data: grid column headers. These used to
         be raw u"..." literals in app/string_const.h that never reached the
         Translate() seam. -->
    <message>
        <source>Source Timestamp</source>
        <translation>Метка времени источника</translation>
    </message>
    <message>
        <source>Server Timestamp</source>
        <translation>Метка времени сервера</translation>
    </message>
    <!-- modules/inspector: the quality pill's third band, for a signal whose
         value has never been delivered. Shared with modules/user_access, whose
         role pill uses it for a user whose AccessRights could not be read. -->
    <message>
        <source>No data</source>
        <translation>Нет данных</translation>
    </message>
    <!-- main_menu / modules/modus: Settings-menu entries. -->
    <message>
        <source>Language</source>
        <translation>Язык</translation>
    </message>
    <message>
        <source>Show Modus topology</source>
        <translation>Показывать топологию Modus</translation>
    </message>
    <message>
        <source>Use Modus runtime renderer</source>
        <translation>Использовать модуль отображения Modus</translation>
    </message>
    <!-- modules/vds_runtime: operator-facing display document errors. -->
    <message>
        <source>No display document is assigned to this window.</source>
        <translation>Для этого окна не задана мнемосхема.</translation>
    </message>
    <message>
        <source>Cannot open document</source>
        <translation>Не удалось открыть документ</translation>
    </message>
    <message>
        <source>Cannot read document info</source>
        <translation>Не удалось прочитать сведения о документе</translation>
    </message>
    <message>
        <source>Cannot render document</source>
        <translation>Не удалось отобразить документ</translation>
    </message>
    <message>
        <source>Cannot render document: invalid size.</source>
        <translation>Не удалось отобразить документ: неверный размер.</translation>
    </message>
    <message>
        <source>VDS runtime is not available.</source>
        <translation>Модуль отображения мнемосхем недоступен.</translation>
    </message>
    <message>
        <source>Good</source>
        <translation>Достоверно</translation>
    </message>
    <message>
        <source>Uncertain</source>
        <translation>Неопределённо</translation>
    </message>
    <message>
        <source>Bad</source>
        <translation>Недостоверно</translation>
    </message>
    <!-- main_window/status_bar/user_status_provider.cpp -->
    <message>
        <source>Administrator</source>
        <translation>Администратор</translation>
    </message>
    <message>
        <source>Operator</source>
        <translation>Оператор</translation>
    </message>
    <message>
        <source>Observer</source>
        <translation>Наблюдатель</translation>
    </message>
    <!-- modules/user_access/qt/user_access_panel.cpp -->
    <message>
        <source>Access rights</source>
        <translation>Права доступа</translation>
    </message>
    <message>
        <source>Permissions</source>
        <translation>Разрешения</translation>
    </message>
    <message>
        <source>Select a user to see its access rights</source>
        <translation>Выберите пользователя, чтобы увидеть его права доступа</translation>
    </message>
    <message>
        <source>View &amp; monitor</source>
        <translation>Просмотр и наблюдение</translation>
    </message>
    <message>
        <source>Control &amp; manual input</source>
        <translation>Управление и ручной ввод</translation>
    </message>
    <message>
        <source>Configure &amp; administer</source>
        <translation>Настройка и администрирование</translation>
    </message>
    <message>
        <source>Add user</source>
        <translation>Добавить пользователя</translation>
    </message>
    <message>
        <source>Reset password</source>
        <translation>Сбросить пароль</translation>
    </message>
    <message>
        <source>Editing requires the Administrator role</source>
        <translation>Редактирование требует роли администратора</translation>
    </message>
    <message>
        <source>Role</source>
        <translation>Роль</translation>
    </message>
    <message>
        <source>Sessions</source>
        <translation>Сеансы</translation>
    </message>
    <message>
        <source>Single</source>
        <translation>Один</translation>
    </message>
    <message>
        <source>Multiple</source>
        <translation>Несколько</translation>
    </message>
    <!-- modules/transmission_rules/qt/transmission_rule_inspector.cpp -->
    <message>
        <source>Transmission rule</source>
        <translation>Правило ретрансляции</translation>
    </message>
    <message>
        <source>Select a transmission rule to edit it</source>
        <translation>Выберите правило ретрансляции для редактирования</translation>
    </message>
    <message>
        <source>Source</source>
        <translation>Источник</translation>
    </message>
    <message>
        <source>Destination</source>
        <translation>Назначение</translation>
    </message>
    <message>
        <source>Information object address (IOA)</source>
        <translation>Адрес объекта информации (IOA)</translation>
    </message>
    <!-- modules/bulk_create/qt/bulk_create_preview_panel.cpp -->
    <message>
        <source>Name template</source>
        <translation>Шаблон имени</translation>
    </message>
    <message>
        <source>NodeId template</source>
        <translation>Шаблон NodeId</translation>
    </message>
    <message>
        <source>Start index</source>
        <translation>Начальный индекс</translation>
    </message>
    <message>
        <source>Index step</source>
        <translation>Шаг индекса</translation>
    </message>
    <message>
        <source>IOA start</source>
        <translation>Начальный IOA</translation>
    </message>
    <message>
        <source>IOA step</source>
        <translation>Шаг IOA</translation>
    </message>
    <message>
        <source>Status</source>
        <translation>Статус</translation>
    </message>
    <message>
        <source>new</source>
        <translation>новый</translation>
    </message>
    <message>
        <source>exists</source>
        <translation>существует</translation>
    </message>
    <message>
        <source>conflict</source>
        <translation>конфликт</translation>
    </message>
    <!-- modules/table: reshell Trend (sparkline) column -->
    <message>
        <source>Trend</source>
        <translation>Тренд</translation>
    </message>
    <!-- modules/table/qt/table_toolbar.cpp -->
    <message>
        <source>Add signal</source>
        <translation>Добавить сигнал</translation>
    </message>
    <message>
        <source>To graph</source>
        <translation>На график</translation>
    </message>
    <!-- modules/inspector/qt/inspector_panel.cpp (event card) -->
    <message>
        <source>Acknowledged</source>
        <translation>Квитировано</translation>
    </message>
    <!-- modules/events/event_severity.cpp + qt/alarm_footer.cpp -->
    <message>
        <source>No unacknowledged events</source>
        <translation>Нет неквитированных событий</translation>
    </message>
    <message>
        <source>highest</source>
        <translation>наивысшая</translation>
    </message>
    <!-- modules/events/qt/event_filter_bar.cpp -->
    <message>
        <source>Unacknowledged only</source>
        <translation>Только неквитированные</translation>
    </message>
    <message>
        <source>Min. severity</source>
        <translation>Мин. важность</translation>
    </message>
    <message>
        <source>Area</source>
        <translation>Зона</translation>
    </message>
    <message>
        <source>All areas</source>
        <translation>Все зоны</translation>
    </message>
    <message>
        <source>All</source>
        <translation>Все</translation>
    </message>
    <!-- client_utils.cpp -->
    <message>
        <source>Local</source>
        <translation>Локальный</translation>
    </message>
    <message>
        <source>Value</source>
        <translation>Значение</translation>
    </message>
    <message>
        <source>Time</source>
        <translation>Время</translation>
    </message>
    <message>
        <source>Updated</source>
        <translation>Обновлен</translation>
    </message>
    <message>
        <source>events</source>
        <translation>событий</translation>
    </message>
    <message>
        <source>Telecontrol</source>
        <translation>Телеконтроль</translation>
    </message>
    <message>
        <source>Vidicon</source>
        <translation>Видикон</translation>
    </message>
    <message>
        <source>Loading</source>
        <translation>Загрузка</translation>
    </message>
    <!-- action_manager.cpp: category titles -->
    <message>
        <source>New</source>
        <translation>Создание</translation>
    </message>
    <message>
        <source>Open</source>
        <translation>Открыть</translation>
    </message>
    <message>
        <source>Item</source>
        <translation>Сигнал</translation>
    </message>
    <message>
        <source>Object</source>
        <translation>Объект</translation>
    </message>
    <message>
        <source>Device</source>
        <translation>Устройство</translation>
    </message>
    <message>
        <source>Options</source>
        <translation>Настройки</translation>
    </message>
    <message>
        <source>Export</source>
        <translation>Экспорт</translation>
    </message>
    <message>
        <source>Misc</source>
        <translation>Прочее</translation>
    </message>
    <message>
        <source>Window</source>
        <translation>Окно</translation>
    </message>
    <message>
        <source>Period</source>
        <translation>Период</translation>
    </message>
    <!-- modules/display_frame/qt/display_frame.cpp (reshell display toolbar) -->
    <message>
        <source>Live</source>
        <translation>Онлайн</translation>
    </message>
    <message>
        <source>Zoom in</source>
        <translation>Увеличить</translation>
    </message>
    <message>
        <source>Zoom out</source>
        <translation>Уменьшить</translation>
    </message>
    <message>
        <source>Fit</source>
        <translation>По размеру</translation>
    </message>
    <message>
        <source>Fit to window</source>
        <translation>Вписать в окно</translation>
    </message>
    <message>
        <source>Actual size</source>
        <translation>Реальный размер</translation>
    </message>
    <message>
        <source>Export image</source>
        <translation>Экспорт изображения</translation>
    </message>
    <!-- modules/inspector/qt/inspector_panel.cpp + main_window (Inspector dock) -->
    <message>
        <source>Inspector</source>
        <translation>Инспектор</translation>
    </message>
    <message>
        <source>Select an element to inspect it</source>
        <translation>Выберите элемент для просмотра</translation>
    </message>
    <message>
        <source>Measurements</source>
        <translation>Измерения</translation>
    </message>
    <message>
        <source>Limits</source>
        <translation>Уставки</translation>
    </message>
    <message>
        <source>HiHi</source>
        <translation>Верхняя аварийная</translation>
    </message>
    <!-- modules/events/event_timeline.cpp (Inspector alarm-card History) -->
    <message>
        <source>History</source>
        <translation>История</translation>
    </message>
    <message>
        <source>Raised</source>
        <translation>Возникло</translation>
    </message>
    <message>
        <source>Received by the server</source>
        <translation>Принято сервером</translation>
    </message>
    <message>
        <source>Acknowledged</source>
        <translation>Квитировано</translation>
    </message>
    <message>
        <source>Awaiting acknowledgement</source>
        <translation>Ожидает квитирования</translation>
    </message>
    <message>
        <source>Hi</source>
        <translation>Верхняя</translation>
    </message>
    <message>
        <source>Lo</source>
        <translation>Нижняя</translation>
    </message>
    <message>
        <source>LoLo</source>
        <translation>Нижняя аварийная</translation>
    </message>
    <message>
        <source>This object cannot be controlled</source>
        <translation>Объектом нельзя управлять</translation>
    </message>
    <message>
        <source>The signal has no output channel</source>
        <translation>У сигнала нет канала управления</translation>
    </message>
    <message>
        <source>Controlling requires the Control privilege</source>
        <translation>Для управления требуется право «Управление»</translation>
    </message>
    <!-- modules/events/event_view.cpp (disabled-command reasons) -->
    <message>
        <source>Select an event to acknowledge</source>
        <translation>Выберите событие для квитирования</translation>
    </message>
    <message>
        <source>The selected events are already acknowledged</source>
        <translation>Выбранные события уже квитированы</translation>
    </message>
    <message>
        <source>Nothing is waiting to be acknowledged</source>
        <translation>Нет неквитированных событий</translation>
    </message>
    <!-- modules/display_frame/qt/display_frame.cpp (bay strips) -->
    <message>
        <source>Recent events</source>
        <translation>Последние события</translation>
    </message>
    <message>
        <source>Control…</source>
        <translation>Управление…</translation>
    </message>
    <message>
        <source>Good</source>
        <translation>Достоверно</translation>
    </message>
    <message>
        <source>Bad</source>
        <translation>Недостоверно</translation>
    </message>
    <!-- modules/device_diagnostics/qt/device_diagnostics_panel.cpp + main_window (Device diagnostics dock) -->
    <message>
        <source>Device diagnostics</source>
        <translation>Диагностика устройства</translation>
    </message>
    <message>
        <source>Select a device to see its diagnostics</source>
        <translation>Выберите устройство для просмотра диагностики</translation>
    </message>
    <message>
        <source>Link up</source>
        <translation>Связь есть</translation>
    </message>
    <message>
        <source>Link down</source>
        <translation>Нет связи</translation>
    </message>
    <message>
        <source>Disabled</source>
        <translation>Отключено</translation>
    </message>
    <message>
        <source>no response from device</source>
        <translation>устройство не отвечает</translation>
    </message>
    <message>
        <source>device is not polled</source>
        <translation>устройство не опрашивается</translation>
    </message>
    <message>
        <source>Counters</source>
        <translation>Счётчики</translation>
    </message>
    <message>
        <source>Actions</source>
        <translation>Действия</translation>
    </message>
    <message>
        <source>Metrics trend</source>
        <translation>График метрик</translation>
    </message>
    <message>
        <source>Filter</source>
        <translation>Фильтр</translation>
    </message>
    <message>
        <source>Revert</source>
        <translation>Откатить</translation>
    </message>
    <message>
        <source>Apply</source>
        <translation>Применить</translation>
    </message>
    <message>
        <source>General</source>
        <translation>Общие</translation>
    </message>
    <message>
        <source>Unsaved changes</source>
        <translation>Несохранённые изменения</translation>
    </message>
    <message>
        <source>Address map</source>
        <translation>Карта адресов</translation>
    </message>
    <message>
        <source>Limits</source>
        <translation>Уставки</translation>
    </message>
    <message>
        <source>Reconnect</source>
        <translation>Переподключить</translation>
    </message>
    <message>
        <source>Reconnect now</source>
        <translation>Переподключить сейчас</translation>
    </message>
    <message>
        <source>Your account cannot issue control commands on this link</source>
        <translation>Учётная запись не имеет права управления этой связью</translation>
    </message>
    <message>
        <source>Not available for the current selection</source>
        <translation>Недоступно для текущего выделения</translation>
    </message>
    <message>
        <source>Open log</source>
        <translation>Открыть журнал</translation>
    </message>
    <message>
        <source>Pause</source>
        <translation>Пауза</translation>
    </message>
    <message>
        <source>Resume</source>
        <translation>Продолжить</translation>
    </message>
    <message>
        <source>Clear</source>
        <translation>Очистить</translation>
    </message>
    <message>
        <source>Save trace</source>
        <translation>Сохранить трассу</translation>
    </message>
    <message>
        <source>Filter title or id</source>
        <translation>Фильтр по имени или ID</translation>
    </message>
    <message>
        <source>Signal</source>
        <translation>Сигнал</translation>
    </message>
    <message>
        <source>Type</source>
        <translation>Тип</translation>
    </message>
    <message>
        <source>Messages RX</source>
        <translation>Сообщений принято</translation>
    </message>
    <message>
        <source>Messages TX</source>
        <translation>Сообщений передано</translation>
    </message>
    <message>
        <source>Bytes RX</source>
        <translation>Байт принято</translation>
    </message>
    <message>
        <source>Bytes TX</source>
        <translation>Байт передано</translation>
    </message>
    <message>
        <source>Interrogations</source>
        <translation>Опросов</translation>
    </message>
    <message>
        <source>Clock syncs</source>
        <translation>Синхронизаций часов</translation>
    </message>
    <message>
        <source>Opens the two-stage command confirm. Actions are logged.</source>
        <translation>Открывает двухэтапное подтверждение команды. Действия журналируются.</translation>
    </message>
    <message>
        <source>Create</source>
        <translation>Создать</translation>
    </message>
    <message>
        <source>Edit</source>
        <translation>Редактирование</translation>
    </message>
    <message>
        <source>Function</source>
        <translation>Функция</translation>
    </message>
    <message>
        <source>Interval</source>
        <translation>Интервал</translation>
    </message>
    <!-- actions.cpp: create actions -->
    <message>
        <source>Multiple Create...</source>
        <translation>Серийное создание...</translation>
    </message>
    <message>
        <source>Service Items...</source>
        <translation>Сервисные объекты...</translation>
    </message>
    <message>
        <source>IEC 60870-101 Link</source>
        <translation>Канал IEC 60870-101</translation>
    </message>
    <message>
        <source>IEC 60870-104 Link</source>
        <translation>Канал IEC 60870-104</translation>
    </message>
    <message>
        <source>Folder</source>
        <translation>Папка</translation>
    </message>
    <message>
        <source>Create Folder...</source>
        <translation>Создать папку...</translation>
    </message>
    <message>
        <source>File</source>
        <translation>Файл</translation>
    </message>
    <message>
        <source>Add File...</source>
        <translation>Добавить файл...</translation>
    </message>
    <!-- actions.cpp: open/view actions -->
    <message>
        <source>Graph</source>
        <translation>График</translation>
    </message>
    <message>
        <source>Data</source>
        <translation>Данные</translation>
    </message>
    <message>
        <source>Display</source>
        <translation>Мнемосхема</translation>
    </message>
    <message>
        <source>Table</source>
        <translation>Таблица</translation>
    </message>
    <message>
        <source>Summary</source>
        <translation>Сводка</translation>
    </message>
    <message>
        <source>Events</source>
        <translation>События</translation>
    </message>
    <message>
        <source>Group Table</source>
        <translation>Групповая таблица</translation>
    </message>
    <!-- actions.cpp: item actions -->
    <message>
        <source>Acknowledge</source>
        <translation>Квитирование</translation>
    </message>
    <message>
        <source>Unlock</source>
        <translation>Разблокировать</translation>
    </message>
    <message>
        <source>Control...</source>
        <translation>Управление...</translation>
    </message>
    <message>
        <source>Control</source>
        <translation>Управление</translation>
    </message>
    <message>
        <source>Manual Input...</source>
        <translation>Ручной ввод...</translation>
    </message>
    <message>
        <source>Manual Input</source>
        <translation>Ручной ввод</translation>
    </message>
    <message>
        <source>Limits...</source>
        <translation>Уставки...</translation>
    </message>
    <message>
        <source>Limits</source>
        <translation>Уставки</translation>
    </message>
    <!-- actions.cpp: device actions -->
    <message>
        <source>Poll Device</source>
        <translation>Опрос устройства</translation>
    </message>
    <message>
        <source>Synchronize Clock</source>
        <translation>Синхронизация часов</translation>
    </message>
    <!-- actions.cpp: setup actions -->
    <message>
        <source>Print</source>
        <translation>Печать</translation>
    </message>
    <!-- actions.cpp: export actions -->
    <message>
        <source>Export to CSV</source>
        <translation>Экспорт в CSV</translation>
    </message>
    <message>
        <source>Export to Excel</source>
        <translation>Экспорт в Excel</translation>
    </message>
    <!-- actions.cpp: specific actions -->
    <message>
        <source>Watch</source>
        <translation>Монитор</translation>
    </message>
    <message>
        <source>Metrics</source>
        <translation>Метрики</translation>
    </message>
    <message>
        <source>Set Password...</source>
        <translation>Задать пароль...</translation>
    </message>
    <message>
        <source>Password</source>
        <translation>Пароль</translation>
    </message>
    <message>
        <source>Enable</source>
        <translation>Включить</translation>
    </message>
    <message>
        <source>Disable</source>
        <translation>Отключить</translation>
    </message>
    <message>
        <source>Pause</source>
        <translation>Пауза</translation>
    </message>
    <!-- actions.cpp: view actions -->
    <message>
        <source>Acknowledge All</source>
        <translation>Квитировать все</translation>
    </message>
    <message>
        <source>Severity...</source>
        <translation>Важность...</translation>
    </message>
    <message>
        <source>Severity</source>
        <translation>Важность</translation>
    </message>
    <message>
        <source>Unacknowledged Only</source>
        <translation>Только неквитированные</translation>
    </message>
    <message>
        <source>Add Web Page...</source>
        <translation>Добавить веб-страницу...</translation>
    </message>
    <message>
        <source>Add Web Page</source>
        <translation>Добавить веб-страницу</translation>
    </message>
    <message>
        <source>Toolbar</source>
        <translation>Панель инструментов</translation>
    </message>
    <message>
        <source>Status Bar</source>
        <translation>Строка состояния</translation>
    </message>
    <message>
        <source>Event Panel</source>
        <translation>Панель событий</translation>
    </message>
    <message>
        <source>Save</source>
        <translation>Сохранить</translation>
    </message>
    <message>
        <source>Save As...</source>
        <translation>Сохранить как...</translation>
    </message>
    <!-- actions.cpp: period actions -->
    <message>
        <source>Current</source>
        <translation>Текущие</translation>
    </message>
    <message>
        <source>15 min</source>
        <translation>15 мин</translation>
    </message>
    <message>
        <source>Hour</source>
        <translation>Час</translation>
    </message>
    <message>
        <source>Day</source>
        <translation>Сутки</translation>
    </message>
    <message>
        <source>Week</source>
        <translation>Неделя</translation>
    </message>
    <message>
        <source>Month</source>
        <translation>Месяц</translation>
    </message>
    <message>
        <source>Custom...</source>
        <translation>Произвольный...</translation>
    </message>
    <message>
        <source>Custom</source>
        <translation>Произвольный</translation>
    </message>
    <!-- actions.cpp: interval actions -->
    <message>
        <source>1-Minute</source>
        <translation>1-Минутные</translation>
    </message>
    <message>
        <source>5 min</source>
        <translation>5 мин</translation>
    </message>
    <message>
        <source>30 min</source>
        <translation>30 мин</translation>
    </message>
    <message>
        <source>1-Hour</source>
        <translation>1-Часовые</translation>
    </message>
    <message>
        <source>12 hours</source>
        <translation>12 часов</translation>
    </message>
    <message>
        <source>1-Day</source>
        <translation>1-Суточные</translation>
    </message>
    <!-- actions.cpp: aggregation actions -->
    <message>
        <source>First</source>
        <translation>Первое</translation>
    </message>
    <message>
        <source>Last</source>
        <translation>Последнее</translation>
    </message>
    <message>
        <source>Count</source>
        <translation>Количество</translation>
    </message>
    <message>
        <source>Minimum</source>
        <translation>Минимум</translation>
    </message>
    <message>
        <source>Maximum</source>
        <translation>Максимум</translation>
    </message>
    <message>
        <source>Sum</source>
        <translation>Сумма</translation>
    </message>
    <message>
        <source>Average</source>
        <translation>Среднее</translation>
    </message>
    <!-- actions.cpp: edit actions -->
    <message>
        <source>Properties</source>
        <translation>Свойства</translation>
    </message>
    <message>
        <source>Element Properties</source>
        <translation>Свойства элемента</translation>
    </message>
    <message>
        <source>Elements</source>
        <translation>Элементы</translation>
    </message>
    <message>
        <source>Transmission Table</source>
        <translation>Таблица передачи</translation>
    </message>
    <message>
        <source>Transmission</source>
        <translation>Передача</translation>
    </message>
    <message>
        <source>Create Portfolio</source>
        <translation>Создать портфолио</translation>
    </message>
    <message>
        <source>Add Items...</source>
        <translation>Добавить объекты...</translation>
    </message>
    <message>
        <source>Add Items</source>
        <translation>Добавить объекты</translation>
    </message>
    <message>
        <source>Rename</source>
        <translation>Переименовать</translation>
    </message>
    <message>
        <source>Copy</source>
        <translation>Копировать</translation>
    </message>
    <message>
        <source>Paste</source>
        <translation>Вставить</translation>
    </message>
    <message>
        <source>Delete</source>
        <translation>Удалить</translation>
    </message>
    <message>
        <source>Clear</source>
        <translation>Очистить</translation>
    </message>
    <!-- main_menu_model.cpp -->
    <message>
        <source>&lt;No displays&gt;</source>
        <translation>&lt;Нет мнемосхем&gt;</translation>
    </message>
    <message>
        <source>&lt;No favourites&gt;</source>
        <translation>&lt;Нет избранного&gt;</translation>
    </message>
    <message>
        <source>The specified page is open in another window.</source>
        <translation>Указанная страница открыта в другом окне.</translation>
    </message>
    <message>
        <source>Restore</source>
        <translation>Восстановить</translation>
    </message>
    <message>
        <source>&lt;Trash is empty&gt;</source>
        <translation>&lt;Корзина пуста&gt;</translation>
    </message>
    <message>
        <source>New Table</source>
        <translation>Новая таблица</translation>
    </message>
    <message>
        <source>New Custom Table</source>
        <translation>Новая произвольная таблица</translation>
    </message>
    <message>
        <source>New Data Table</source>
        <translation>Новая таблица данных</translation>
    </message>
    <message>
        <source>New</source>
        <translation>Новый</translation>
    </message>
    <message>
        <source>Items</source>
        <translation>Объекты</translation>
    </message>
    <message>
        <source>Favourites</source>
        <translation>Избранное</translation>
    </message>
    <message>
        <source>Files</source>
        <translation>Файлы</translation>
    </message>
    <message>
        <source>Portfolio</source>
        <translation>Портфолио</translation>
    </message>
    <message>
        <source>Hardware</source>
        <translation>Оборудование</translation>
    </message>
    <message>
        <source>Event Journal</source>
        <translation>Журнал событий</translation>
    </message>
    <message>
        <source>Nodes</source>
        <translation>Узлы</translation>
    </message>
    <message>
        <source>Objects</source>
        <translation>Объекты</translation>
    </message>
    <message>
        <source>Subsystems</source>
        <translation>Оборудование</translation>
    </message>
    <message>
        <source>Favorites</source>
        <translation>Избранное</translation>
    </message>
    <message>
        <source>CSV Files</source>
        <translation>Файлы CSV</translation>
    </message>
    <message>
        <source>Export completed. Open the file now?</source>
        <translation>Экспорт завершен. Открыть файл сейчас?</translation>
    </message>
    <message>
        <source>Export failed.</source>
        <translation>Ошибка при экспорте.</translation>
    </message>
    <message>
        <source>Unknown data type</source>
        <translation>Неизвестный тип данных</translation>
    </message>
    <message>
        <source>No header row</source>
        <translation>Нет строки заголовка</translation>
    </message>
    <message>
        <source>Invalid column name format</source>
        <translation>Неверный формат имени столбца</translation>
    </message>
    <message>
        <source>Group not found</source>
        <translation>Группа не найдена</translation>
    </message>
    <message>
        <source>Type not found</source>
        <translation>Тип не найден</translation>
    </message>
    <message>
        <source>Property {} not found</source>
        <translation>Свойство {} не найдено</translation>
    </message>
    <message>
        <source>Cannot convert value &apos;{}&apos; to type &apos;{}&apos;</source>
        <translation>Невозможно преобразовать значение &apos;{}&apos; как тип &apos;{}&apos;</translation>
    </message>
    <message>
        <source>Row has fewer cells than expected</source>
        <translation>Количество ячеек в строке меньше ожидаемого</translation>
    </message>
    <message>
        <source>&lt;Group&gt;</source>
        <translation>&lt;Группа&gt;</translation>
    </message>
    <message>
        <source>Formats</source>
        <translation>Форматы</translation>
    </message>
    <message>
        <source>Simulated Signals</source>
        <translation>Имитационные сигналы</translation>
    </message>
    <message>
        <source>Users</source>
        <translation>Пользователи</translation>
    </message>
    <message>
        <source>Description</source>
        <translation>Описание</translation>
    </message>
    <message>
        <source>Roles</source>
        <translation>Роли</translation>
    </message>
    <message>
        <source>Members</source>
        <translation>Участники</translation>
    </message>
    <message>
        <source>Password policy</source>
        <translation>Политика паролей</translation>
    </message>
    <message>
        <source>Audit log</source>
        <translation>Журнал аудита</translation>
    </message>
    <message>
        <source>Length</source>
        <translation>Длина</translation>
    </message>
    <message>
        <source>Any length</source>
        <translation>Любая длина</translation>
    </message>
    <message>
        <source>Must contain</source>
        <translation>Должен содержать</translation>
    </message>
    <message>
        <source>Upper-case letter</source>
        <translation>Заглавную букву</translation>
    </message>
    <message>
        <source>Lower-case letter</source>
        <translation>Строчную букву</translation>
    </message>
    <message>
        <source>Digit</source>
        <translation>Цифру</translation>
    </message>
    <message>
        <source>Special character</source>
        <translation>Специальный символ</translation>
    </message>
    <message>
        <source>The password policy could not be read.</source>
        <translation>Не удалось прочитать политику паролей.</translation>
    </message>
    <message>
        <source>The password is too short</source>
        <translation>Пароль слишком короткий</translation>
    </message>
    <message>
        <source>The password is too long</source>
        <translation>Пароль слишком длинный</translation>
    </message>
    <message>
        <source>The password needs an upper-case letter</source>
        <translation>В пароле нужна заглавная буква</translation>
    </message>
    <message>
        <source>The password needs a lower-case letter</source>
        <translation>В пароле нужна строчная буква</translation>
    </message>
    <message>
        <source>The password needs a digit</source>
        <translation>В пароле нужна цифра</translation>
    </message>
    <message>
        <source>The password needs a special character</source>
        <translation>В пароле нужен специальный символ</translation>
    </message>
    <message>
        <source>Standard role</source>
        <translation>Стандартная</translation>
    </message>
    <message>
        <source>Custom role</source>
        <translation>Произвольная</translation>
    </message>
    <message>
        <source>Enabled</source>
        <translation>Включено</translation>
    </message>
    <message>
        <source>None</source>
        <translation>Нет</translation>
    </message>
    <message>
        <source>Databases</source>
        <translation>Базы данных</translation>
    </message>
    <message>
        <source>Export Configuration to Excel...</source>
        <translation>Экспорт конфигурации в Excel...</translation>
    </message>
    <message>
        <source>Import Configuration from Excel...</source>
        <translation>Импорт конфигурации из Excel...</translation>
    </message>
    <message>
        <source>Connect to Server...</source>
        <translation>Подключиться к серверу...</translation>
    </message>
    <message>
        <source>Disconnect from Server</source>
        <translation>Отключиться от сервера</translation>
    </message>
    <message>
        <source>More</source>
        <translation>Дополнительно</translation>
    </message>
    <message>
        <source>Page</source>
        <translation>Страница</translation>
    </message>
    <message>
        <source>Add to Favourites</source>
        <translation>Добавить в избранное</translation>
    </message>
    <message>
        <source>Close</source>
        <translation>Закрыть</translation>
    </message>
    <message>
        <source>Split Horizontally</source>
        <translation>Разделить горизонтально</translation>
    </message>
    <message>
        <source>Split Vertically</source>
        <translation>Разделить вертикально</translation>
    </message>
    <message>
        <source>Control Confirmation</source>
        <translation>Подтверждение управления</translation>
    </message>
    <message>
        <source>Control Success Message</source>
        <translation>Сообщение об успехе управления</translation>
    </message>
    <message>
        <source>Show Events on Arrival</source>
        <translation>Показывать события при поступлении</translation>
    </message>
    <message>
        <source>Hide Events on Acknowledge</source>
        <translation>Скрывать события при квитировании</translation>
    </message>
    <message>
        <source>Flash Main Window on Event</source>
        <translation>Мигание окна при событии</translation>
    </message>
    <message>
        <source>Sound Alarm on Event</source>
        <translation>Звуковое оповещение при событии</translation>
    </message>
    <message>
        <source>Open Displays Folder</source>
        <translation>Открыть папку мнемосхем</translation>
    </message>
    <message>
        <source>Style</source>
        <translation>Стиль</translation>
    </message>
    <message>
        <source>Settings</source>
        <translation>Настройки</translation>
    </message>
    <message>
        <source>Documentation</source>
        <translation>Документация</translation>
    </message>
    <message>
        <source>Debug Information</source>
        <translation>Отладочная информация</translation>
    </message>
    <message>
        <source>About...</source>
        <translation>О программе...</translation>
    </message>
    <message>
        <source>About Qt...</source>
        <translation>О Qt...</translation>
    </message>
    <message>
        <source>Help</source>
        <translation>Справка</translation>
    </message>
    <!-- main_window_commands.cpp -->
    <message>
        <source>Name:</source>
        <translation>Имя:</translation>
    </message>
    <!-- selection_commands.cpp -->
    <!-- events/event_view.cpp -->
    <message>
        <source>Message</source>
        <translation>Сообщение</translation>
    </message>
    <message>
        <source>User</source>
        <translation>Пользователь</translation>
    </message>
    <message>
        <source>Acknowledged By</source>
        <translation>Квитировал</translation>
    </message>
    <message>
        <source>Acknowledge Time</source>
        <translation>Время квитирования</translation>
    </message>
    <message>
        <source>No data to export.</source>
        <translation>Нет данных для экспорта.</translation>
    </message>
    <message>
        <source>Export error.</source>
        <translation>Ошибка экспорта.</translation>
    </message>
    <message>
        <source>Minimum severity threshold (0 = all events):</source>
        <translation>Минимальный порог важности (0 = все события):</translation>
    </message>
    <!-- events/event_table_model.cpp -->
    <message>
        <source>Current Events</source>
        <translation>Текущие события</translation>
    </message>
    <message>
        <source>Event Journal for Day</source>
        <translation>Журнал событий за сутки</translation>
    </message>
    <message>
        <source>Event Journal for Week</source>
        <translation>Журнал событий за неделю</translation>
    </message>
    <message>
        <source>Event Journal for Month</source>
        <translation>Журнал событий за месяц</translation>
    </message>
    <!-- favorites/favourites_view.cpp -->
    <message>
        <source>URL:</source>
        <translation>URL:</translation>
    </message>
    <!-- filesystem/filesystem_commands.cpp -->
    <message>
        <source>The file has an invalid format.</source>
        <translation>Файл имеет неверный формат.</translation>
    </message>
    <message>
        <source>Unknown file type.</source>
        <translation>Неизвестный тип файла.</translation>
    </message>
    <message>
        <source>Failed to download file from server.</source>
        <translation>Не удалось загрузить файл с сервера.</translation>
    </message>
    <message>
        <source>Failed to read file.</source>
        <translation>Не удалось прочитать файл.</translation>
    </message>
    <!-- services/task_manager_impl.cpp -->
    <message>
        <source>Insert</source>
        <translation>Вставка</translation>
    </message>
    <!-- services/connection_state_reporter.cpp -->
    <message>
        <source>Connection to server established. Login successful.</source>
        <translation>Соединение с сервером установлено. Вход выполнен.</translation>
    </message>
    <!-- profile/profile.cpp -->
    <message>
        <source>Page </source>
        <translation>Страница </translation>
    </message>
    <!-- modus/qt/modus_controller.cpp -->
    <message>
        <source>Built-in Modus diagram rendering is enabled and will be applied to subsequently opened diagrams. To disable, use the Settings menu.</source>
        <translation>Встроенная отрисовка схем Модус включена и будет применена для далее открытых схем. Для отключения функции используйте меню Настройки.</translation>
    </message>
    <message>
        <source>Built-in rendering</source>
        <translation>Встроенная отрисовка</translation>
    </message>
    <!-- app/client_application.cpp -->
    <message>
        <source>The specified username is already in use by another session. Disconnect the open session and continue?</source>
        <translation>Указанное имя пользователя уже используется другой сессией. Разорвать открытую сессию и продолжить?</translation>
    </message>
    <message>
        <source>To disable automatic login, hold Ctrl when launching the application.</source>
        <translation>Чтобы отключить автоматический вход, удерживайте Ctrl при запуске приложения.</translation>
    </message>
    <message>
        <source>Disconnecting from server...</source>
        <translation>Отключение от сервера...</translation>
    </message>
    <!-- export/configuration/excel_configuration_commands.cpp -->
    <message>
        <source>Failed to open file.</source>
        <translation>Не удалось открыть файл.</translation>
    </message>
    <message>
        <source>Export complete. Open the file now?</source>
        <translation>Экспорт завершён. Открыть файл?</translation>
    </message>
    <message>
        <source>No changes found</source>
        <translation>Изменения не найдены</translation>
    </message>
    <message>
        <source>Import</source>
        <translation>Импорт</translation>
    </message>
    <message>
        <source>Apply changes?</source>
        <translation>Применить изменения?</translation>
    </message>
    <!-- portfolio/portfolio_manager.cpp -->
    <!-- (Portfolio already defined above) -->
    <!-- aui/models/property_tree_model.cpp -->
    <message>
        <source>Parameter</source>
        <translation>Параметр</translation>
    </message>
    <!-- configuration/objects/object_tree_model.cpp -->
    <!-- (Name and Value already defined) -->
    <message>
        <source>Browse Name</source>
        <translation>Обозначение</translation>
    </message>
    <message>
        <source>Name</source>
        <translation>Имя</translation>
    </message>
    <!-- modules/debugger/debugger_module.cpp -->
    <message>
        <source>Debugger</source>
        <translation>Отладчик</translation>
    </message>
    <message>
        <source>Debug information copied to clipboard.</source>
        <translation>Отладочная информация скопирована в буфер обмена.</translation>
    </message>
    <!-- modules/table/table_model.cpp -->
    <message>
        <source>Enter expression</source>
        <translation>Введите выражение</translation>
    </message>
    <message>
        <source>Invalid expression.</source>
        <translation>Некорректное выражение.</translation>
    </message>
    <!-- modules/table/table_view.cpp -->
    <message>
        <source>Change Time</source>
        <translation>Время изменения</translation>
    </message>
    <message>
        <source>Event</source>
        <translation>Событие</translation>
    </message>
    <!-- modules/write/write_model.cpp -->
    <message>
        <source>Preparing to control...</source>
        <translation>Подготовка к управлению...</translation>
    </message>
    <message>
        <source>Controlling...</source>
        <translation>Управление...</translation>
    </message>
    <!-- modules/watch/watch_view.cpp -->
    <message>
        <source>Save As</source>
        <translation>Сохранить как</translation>
    </message>
    <!-- modules/watch/watch_model.cpp -->
    <message>
        <source>Subscription interrupted. The device may have been deleted.</source>
        <translation>Подписка прервана. Устройство могло быть удалено.</translation>
    </message>
    <!-- modules/node_properties/node_property_model.cpp -->
    <message>
        <source>Attributes</source>
        <translation>Атрибуты</translation>
    </message>
    <!-- modules/transmission/transmission_view.cpp -->
    <message>
        <source>Address</source>
        <translation>Адрес</translation>
    </message>
    <!-- context menu models (modules/table, modules/sheet) -->
    <message>
        <source>Delete Row</source>
        <translation>Удалить строку</translation>
    </message>
    <message>
        <source>Move Up</source>
        <translation>Сместить вверх</translation>
    </message>
    <message>
        <source>Move Down</source>
        <translation>Сместить вниз</translation>
    </message>
    <message>
        <source>Sort</source>
        <translation>Упорядочить</translation>
    </message>
    <message>
        <source>Channel</source>
        <translation>Канал</translation>
    </message>
    <message>
        <source>Color...</source>
        <translation>Цвет...</translation>
    </message>
    <!-- Status strip: session pane (main_window/status_bar). -->
    <message>
        <source>Connected</source>
        <translation>Подключен</translation>
    </message>
    <message>
        <source>Disconnected</source>
        <translation>Отключен</translation>
    </message>
    <message>
        <source>No response</source>
        <translation>Нет отклика</translation>
    </message>
    <message>
        <source>Server: {} ms</source>
        <!-- {} is the round-trip time; std::format, so the brace pair must
             survive translation intact. -->
        <translation>Сервер: {} мс</translation>
    </message>
    <!-- Status strip: event pane (main_window/status_bar). -->
    <message>
        <source>Events: {}</source>
        <translation>События: {}</translation>
    </message>
    <message>
        <source>No events</source>
        <translation>Нет событий</translation>
    </message>
    <message>
        <source>Severity: {}</source>
        <translation>Важность: {}</translation>
    </message>
    <!-- Workspace tab context menu (main_window/tab_popup_menu.h). Deliberately
         shorter than the Favourites menu's own "Добавить в избранное" — a tab
         context menu is read at a glance, so it keeps the short caption and
         therefore needs a source string of its own. -->
    <message>
        <source>To Favourites</source>
        <translation>В избранное</translation>
    </message>
    <!-- Page rename prompt (main_window/pages/page_commands.cpp). The dialog
         title is the act, not the verb; the Page menu's own ID_PAGE_RENAME
         command keeps the imperative "Переименовать" under "Rename". -->
    <message>
        <source>Rename Page</source>
        <translation>Переименование</translation>
    </message>
    <!-- Page with no windows in it (profile/page.cpp). -->
    <message>
        <source>(Empty)</source>
        <translation>(Пустой)</translation>
    </message>
    <!-- Property editor placeholder choices (properties/). Both are sentinels
         as well as labels: the property's SetText compares the edited text
         against the same call, so the two sides move together. -->
    <message>
        <source>&lt;None&gt;</source>
        <translation>&lt;Нет&gt;</translation>
    </message>
    <message>
        <source>&lt;Default&gt;</source>
        <translation>&lt;Стандартный&gt;</translation>
    </message>
    <!-- Device runtime state in the object tree (services/device_state_notifier.cpp).
         "Disabled" is already translated above, for the device property. -->
    <message>
        <source>Offline</source>
        <translation>Нет связи</translation>
    </message>
    <message>
        <source>Online</source>
        <translation>Есть связь</translation>
    </message>
</context>
<context>
    <name>AboutDialog</name>
    <message>
        <location filename="../../modules/about/qt/about_dialog.ui" line="14"/>
        <source>About</source>
        <translation>О программе</translation>
    </message>
    <message>
        <location filename="../../modules/about/qt/about_dialog.ui" line="84"/>
        <source>OK</source>
        <translation>ОК</translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation type="vanished">Отмена</translation>
    </message>
    <message>
        <location filename="../../modules/about/qt/about_dialog.cpp" line="22"/>
        <source>Version %1</source>
        <translation>Версия %1</translation>
    </message>
    <message>
        <location filename="../../modules/about/qt/about_dialog.cpp" line="23"/>
        <source>Telecontrol</source>
        <translation>Телеконтроль</translation>
    </message>
</context>
<context>
    <name>AddFavouritesDialog</name>
        <!-- Accept button: names the action, not the assent
             (docs/ux/dialogs.md §3). -->
    <message>
        <source>Add</source>
        <translation>Добавить</translation>
    </message>
    <message>
        <location filename="../../modules/favourites/qt/add_favourites_dialog.cpp" line="21"/>
        <source>(No Folder)</source>
        <translation>(Нет группы)</translation>
    </message>
    <message>
        <location filename="../../modules/favourites/qt/add_favourites_dialog.ui" line="14"/>
        <source>Add to Favourites</source>
        <translation>Добавить в избранное</translation>
    </message>
    <message>
        <location filename="../../modules/favourites/qt/add_favourites_dialog.ui" line="25"/>
        <source>Name:</source>
        <translation>Имя:</translation>
    </message>
    <message>
        <location filename="../../modules/favourites/qt/add_favourites_dialog.ui" line="35"/>
        <source>Folder:</source>
        <translation>Группа:</translation>
    </message>
    <message>
        <location filename="../../modules/favourites/qt/add_favourites_dialog.ui" line="84"/>
        <source>OK</source>
        <translation>ОК</translation>
    </message>
    <message>
        <location filename="../../modules/favourites/qt/add_favourites_dialog.ui" line="91"/>
        <source>Cancel</source>
        <translation>Отмена</translation>
    </message>
</context>
<context>
    <name>ChangePasswordDialog</name>
    <message>
        <location filename="../../modules/change_password/qt/change_password_dialog.ui" line="14"/>
        <source>Change Password</source>
        <translation>Смена пароля</translation>
    </message>
    <message>
        <location filename="../../modules/change_password/qt/change_password_dialog.ui" line="25"/>
        <source>Current:</source>
        <translation>Текущий:</translation>
    </message>
    <message>
        <location filename="../../modules/change_password/qt/change_password_dialog.ui" line="39"/>
        <source>New:</source>
        <translation>Новый:</translation>
    </message>
    <message>
        <location filename="../../modules/change_password/qt/change_password_dialog.ui" line="53"/>
        <source>Repeat:</source>
        <translation>Повтор:</translation>
    </message>
    <message>
        <location filename="../../modules/change_password/qt/change_password_dialog.ui" line="99"/>
        <source>OK</source>
        <translation>ОК</translation>
    </message>
    <message>
        <location filename="../../modules/change_password/qt/change_password_dialog.ui" line="106"/>
        <source>Cancel</source>
        <translation>Отмена</translation>
    </message>
    <message>
        <location filename="../../modules/change_password/qt/change_password_dialog.cpp" line="38"/>
        <source>New and repeated password do not match.</source>
        <translation>Новый и повторенный пароль не совпадают.</translation>
    </message>
</context>
<context>
    <name>CreateServiceItemDialog</name>
        <!-- Accept button: names the action, not the assent
             (docs/ux/dialogs.md §3). -->
    <message>
        <source>Create</source>
        <translation>Создать</translation>
    </message>
    <message>
        <location filename="../../modules/create_service_item/qt/create_service_item.ui" line="14"/>
        <source>Create Service Items</source>
        <translation>Создание сервисных объектов</translation>
    </message>
    <message>
        <location filename="../../modules/create_service_item/qt/create_service_item.ui" line="25"/>
        <source>Device:</source>
        <translation>Устройство:</translation>
    </message>
    <message>
        <location filename="../../modules/create_service_item/qt/create_service_item.ui" line="74"/>
        <source>OK</source>
        <translation>ОК</translation>
    </message>
    <message>
        <location filename="../../modules/create_service_item/qt/create_service_item.ui" line="81"/>
        <source>Cancel</source>
        <translation>Отмена</translation>
    </message>
</context>
<context>
    <name>CsvExportDialog</name>
        <!-- Accept button: names the action, not the assent
             (docs/ux/dialogs.md §3). -->
    <message>
        <source>Export</source>
        <translation>Экспортировать</translation>
    </message>
    <!-- The dialog's strings belong to this context: uic emits
         QCoreApplication::translate("CsvExportDialog", ...) for a .ui, and the
         QMessageBox calls in csv_export_dialog.cpp use tr(). They previously
         sat under the empty context as "vanished", so none of them shipped.
         The literal delimiter (",") and quote ("\"") combo items are
         deliberately absent: accept() parses them as single characters, so a
         translation would break the dialog. -->
    <message>
        <source>CSV parameters</source>
        <translation>Параметры CSV</translation>
    </message>
    <message>
        <source>Encoding:</source>
        <translation>Кодировка:</translation>
    </message>
    <message>
        <source>Delimiter:</source>
        <translation>Разделитель:</translation>
    </message>
    <message>
        <source>Quote:</source>
        <translation>Кавычки:</translation>
    </message>
    <message>
        <source>System</source>
        <translation>Системная</translation>
    </message>
    <message>
        <source>Unicode (UTF-8)</source>
        <translation>Юникод (UTF-8)</translation>
    </message>
    <message>
        <source>Tab</source>
        <translation>Табуляция</translation>
    </message>
    <message>
        <source>Space</source>
        <translation>Пробел</translation>
    </message>
    <message>
        <source>OK</source>
        <translation>ОК</translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation>Отмена</translation>
    </message>
    <message>
        <source>Please enter a symbol for the delimiter or choose one from the drop-down list.</source>
        <translation>Пожалуйста, укажите одиночный символ разделителя или выберите из списка.</translation>
    </message>
    <message>
        <source>Please enter a symbol for the quote or choose one from the drop-down list.</source>
        <translation>Пожалуйста, укажите одиночный символ кавычки или выберите из списка.</translation>
    </message>
    <!-- modules/export/csv/qt/csv_export_dialog.ui (flood-group expansion) -->
    <message>
        <source>Expand grouped rows</source>
        <translation>Развернуть сгруппированные строки</translation>
    </message>
    <message>
        <source>Write one row per event. When the journal collapses repeated alarms during a flood, an unchecked box exports the collapsed rows as shown, with their counts.</source>
        <translation>Записывать по одной строке на событие. Когда журнал сворачивает повторяющиеся тревоги при их потоке, снятый флажок выгружает свёрнутые строки как показано, вместе со счётчиками.</translation>
    </message>
    <message>
        <source>New and repeated password do not match.</source>
        <translation type="obsolete">Новый и повторенный пароль не совпадают.</translation>
    </message>
    <message>
        <source>Change Password</source>
        <translation type="obsolete">Смена пароля</translation>
    </message>
    <message>
        <source>Current:</source>
        <translation type="obsolete">Текущий:</translation>
    </message>
    <message>
        <source>New:</source>
        <translation type="obsolete">Новый:</translation>
    </message>
    <message>
        <source>Repeat:</source>
        <translation type="obsolete">Повтор:</translation>
    </message>
    <message>
        <source>CSV parameters</source>
        <translation type="vanished">Параметры CSV</translation>
    </message>
    <message>
        <source>Encoding:</source>
        <translation type="vanished">Кодировка:</translation>
    </message>
    <message>
        <source>Delimiter:</source>
        <translation type="vanished">Разделитель:</translation>
    </message>
    <message>
        <source>Quote:</source>
        <translation type="vanished">Кавычки:</translation>
    </message>
    <message>
        <source>System</source>
        <translation type="vanished">Системная</translation>
    </message>
    <message>
        <source>Unicode (UTF-8)</source>
        <translation type="vanished">Юникод (UTF-8)</translation>
    </message>
    <message>
        <source>Tab</source>
        <translation type="vanished">Табуляция</translation>
    </message>
    <message>
        <source>Space</source>
        <translation type="vanished">Пробел</translation>
    </message>
    <message>
        <source>OK</source>
        <translation type="vanished">ОК</translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation type="vanished">Отмена</translation>
    </message>
    <message>
        <source>Please enter a symbol for the delimiter or choose one from the drop-down list.</source>
        <translation type="vanished">Пожалуйста, укажите одиночный символ разделителя или выберите из списка.</translation>
    </message>
    <message>
        <source>Please enter a symbol for the quote or choose one from the drop-down list.</source>
        <translation type="vanished">Пожалуйста, укажите одиночный символ кавычки или выберите из списка.</translation>
    </message>
</context>
<context>
    <name>Dialog</name>
    <message>
        <source>OK</source>
        <translation type="obsolete">ОК</translation>
    </message>
    <message>
        <source>Cancel</source>
        <translation type="obsolete">Отмена</translation>
    </message>
</context>
<context>
    <name>LimitDialog</name>
        <!-- Accept button: names the action, not the assent
             (docs/ux/dialogs.md §3). -->
    <message>
        <source>Apply</source>
        <translation>Применить</translation>
    </message>
    <message>
        <location filename="../../modules/limits/qt/limit_dialog.ui" line="33"/>
        <source>Critical Limits</source>
        <translation>Аварийные уставки</translation>
    </message>
    <message>
        <location filename="../../modules/limits/qt/limit_dialog.ui" line="41"/>
        <location filename="../../modules/limits/qt/limit_dialog.ui" line="82"/>
        <source>High:</source>
        <translation>Верхняя:</translation>
    </message>
    <message>
        <location filename="../../modules/limits/qt/limit_dialog.ui" line="55"/>
        <location filename="../../modules/limits/qt/limit_dialog.ui" line="89"/>
        <source>Low:</source>
        <translation>Нижняя:</translation>
    </message>
    <message>
        <location filename="../../modules/limits/qt/limit_dialog.ui" line="14"/>
        <location filename="../../modules/limits/qt/limit_dialog.ui" line="74"/>
        <source>Limits</source>
        <translation>Уставки</translation>
    </message>
    <message>
        <location filename="../../modules/limits/qt/limit_dialog.ui" line="23"/>
        <source>(Description)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../modules/limits/qt/limit_dialog.ui" line="145"/>
        <source>OK</source>
        <translation>ОК</translation>
    </message>
    <message>
        <location filename="../../modules/limits/qt/limit_dialog.ui" line="152"/>
        <source>Cancel</source>
        <translation>Отмена</translation>
    </message>
</context>
<context>
    <name>LoginDialog</name>
    <!-- modules/login/qt/login_dialog.{ui,cpp} — TLS client-authentication
         fields. "Закрытый ключ" is the standard Russian PKI term for a private
         key. The browse buttons' "…" is a glyph and is deliberately left
         untranslated (see ALLOWED_UNTRANSLATED in check_ui_translations.py). -->
    <!-- "Sign in" labels the accept button: the dialog names the action rather
         than the assent (docs/ux/dialogs.md §3). It used to be an in-content
         heading duplicating the window title, which the native title bar
         already carries. The subtitle below it went with that heading and is
         kept only so an older .qm does not lose the string. -->
    <message>
        <source>Sign in</source>
        <translation>Вход в систему</translation>
    </message>
    <message>
        <source>Telecontrol SCADA operator client</source>
        <translation>Клиент оператора Telecontrol SCADA</translation>
    </message>
    <message>
        <source>Connecting to: %1</source>
        <translation>Подключение к: %1</translation>
    </message>
    <message>
        <source>Security:</source>
        <translation>Безопасность:</translation>
    </message>
    <message>
        <source>Certificate:</source>
        <translation>Сертификат:</translation>
    </message>
    <message>
        <source>Private key:</source>
        <translation>Закрытый ключ:</translation>
    </message>
    <message>
        <source>Client certificate (.pem)</source>
        <translation>Сертификат клиента (.pem)</translation>
    </message>
    <message>
        <source>Client private key (.pem)</source>
        <translation>Закрытый ключ клиента (.pem)</translation>
    </message>
    <message>
        <source>Select client certificate</source>
        <translation>Выберите сертификат клиента</translation>
    </message>
    <message>
        <source>Select client private key</source>
        <translation>Выберите закрытый ключ клиента</translation>
    </message>
    <message>
        <source>PEM files (*.pem);;All files (*)</source>
        <translation>Файлы PEM (*.pem);;Все файлы (*)</translation>
    </message>
    <message>
        <location filename="../../modules/login/qt/login_dialog.ui" line="20"/>
        <source>Login</source>
        <translation>Вход в систему</translation>
    </message>
    <message>
        <location filename="../../modules/login/qt/login_dialog.ui" line="34"/>
        <source>User name:</source>
        <translation>Имя:</translation>
    </message>
    <message>
        <location filename="../../modules/login/qt/login_dialog.ui" line="51"/>
        <source>Password:</source>
        <translation>Пароль:</translation>
    </message>
    <message>
        <location filename="../../modules/login/qt/login_dialog.ui" line="65"/>
        <source>Auto-Login:</source>
        <translation>Авто:</translation>
    </message>
    <message>
        <location filename="../../modules/login/qt/login_dialog.ui" line="75"/>
        <source>Server:</source>
        <translation>Сервер:</translation>
    </message>
    <message>
        <location filename="../../modules/login/qt/login_dialog.cpp" line="76"/>
        <source>You can remove the highlighted user from list by pressing Delete.</source>
        <translation>Отмеченного пользователя можно удалить из списка нажатием Delete.</translation>
    </message>
</context>
<context>
    <name>MultiCreateDialog</name>
        <!-- Accept button: names the action, not the assent
             (docs/ux/dialogs.md §3). -->
    <message>
        <source>Create</source>
        <translation>Создать</translation>
    </message>
    <message>
        <location filename="../../modules/multi_create/qt/multi_create_dialog.ui" line="20"/>
        <source>Multiple Create</source>
        <translation>Создание серии объектов</translation>
    </message>
    <message>
        <location filename="../../modules/multi_create/qt/multi_create_dialog.ui" line="31"/>
        <source>Type:</source>
        <translation>Тип:</translation>
    </message>
    <message>
        <location filename="../../modules/multi_create/qt/multi_create_dialog.ui" line="53"/>
        <source>Discrete</source>
        <translation>ТС</translation>
    </message>
    <message>
        <location filename="../../modules/multi_create/qt/multi_create_dialog.ui" line="63"/>
        <source>Analog</source>
        <translation>ТИТ</translation>
    </message>
    <message>
        <location filename="../../modules/multi_create/qt/multi_create_dialog.ui" line="73"/>
        <source>Count:</source>
        <translation>Количество:</translation>
    </message>
    <message>
        <location filename="../../modules/multi_create/qt/multi_create_dialog.ui" line="93"/>
        <source>Name prefix:</source>
        <translation>Префикс имени:</translation>
    </message>
    <message>
        <location filename="../../modules/multi_create/qt/multi_create_dialog.ui" line="103"/>
        <source>Starting number:</source>
        <translation>Начальный номер:</translation>
    </message>
    <message>
        <location filename="../../modules/multi_create/qt/multi_create_dialog.ui" line="120"/>
        <source>Device:</source>
        <translation>Устройство:</translation>
    </message>
    <message>
        <location filename="../../modules/multi_create/qt/multi_create_dialog.ui" line="130"/>
        <source>Address prefix:</source>
        <translation>Префикс адреса:</translation>
    </message>
    <message>
        <location filename="../../modules/multi_create/qt/multi_create_dialog.ui" line="140"/>
        <source>Starting address:</source>
        <translation>Начальный адрес:</translation>
    </message>
    <message>
        <location filename="../../modules/multi_create/qt/multi_create_dialog.ui" line="189"/>
        <source>OK</source>
        <translation>ОК</translation>
    </message>
    <message>
        <location filename="../../modules/multi_create/qt/multi_create_dialog.ui" line="196"/>
        <source>Cancel</source>
        <translation>Отмена</translation>
    </message>
</context>
<context>
    <name>TimeRangeDialog</name>
    <message>
        <location filename="../../modules/time_range/qt/time_range_dialog.ui" line="20"/>
        <source>Time Range</source>
        <translation>Период</translation>
    </message>
    <message>
        <location filename="../../modules/time_range/qt/time_range_dialog.ui" line="31"/>
        <source>Date</source>
        <translation>Дата</translation>
    </message>
    <message>
        <location filename="../../modules/time_range/qt/time_range_dialog.ui" line="39"/>
        <location filename="../../modules/time_range/qt/time_range_dialog.ui" line="75"/>
        <source>Start:</source>
        <translation>Начало:</translation>
    </message>
    <message>
        <location filename="../../modules/time_range/qt/time_range_dialog.ui" line="49"/>
        <location filename="../../modules/time_range/qt/time_range_dialog.ui" line="85"/>
        <source>End:</source>
        <translation>Конец:</translation>
    </message>
    <message>
        <location filename="../../modules/time_range/qt/time_range_dialog.ui" line="64"/>
        <source>Time</source>
        <translation>Время</translation>
    </message>
    <message>
        <location filename="../../modules/time_range/qt/time_range_dialog.ui" line="132"/>
        <source>OK</source>
        <translation>ОК</translation>
    </message>
    <message>
        <location filename="../../modules/time_range/qt/time_range_dialog.ui" line="139"/>
        <source>Cancel</source>
        <translation>Отмена</translation>
    </message>
</context>
<context>
    <name>WriteDialog</name>
        <!-- Accept button: names the action, not the assent
             (docs/ux/dialogs.md §3). -->
    <message>
        <source>Write</source>
        <translation>Записать</translation>
    </message>
        <!-- Accept button: names the action, not the assent
             (docs/ux/dialogs.md §3). -->
    <message>
        <source>Execute</source>
        <translation>Выполнить</translation>
    </message>
    <message>
        <location filename="../../modules/write/qt/write_dialog.ui" line="20"/>
        <source>Write value</source>
        <translation>Запись значения</translation>
    </message>
    <message>
        <location filename="../../modules/write/qt/write_dialog.ui" line="32"/>
        <source>(Description)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../modules/write/qt/write_dialog.ui" line="44"/>
        <source>Current value:</source>
        <translation>Текущее значение:</translation>
    </message>
    <message>
        <location filename="../../modules/write/qt/write_dialog.ui" line="51"/>
        <source>New value:</source>
        <translation>Новое значение:</translation>
    </message>
    <message>
        <location filename="../../modules/write/qt/write_dialog.ui" line="76"/>
        <source>units</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../modules/write/qt/write_dialog.ui" line="86"/>
        <source>Lock:</source>
        <translation>Блокировка:</translation>
    </message>
    <message>
        <location filename="../../modules/write/qt/write_dialog.ui" line="96"/>
        <source>Condition:</source>
        <translation>Условие:</translation>
    </message>
    <message>
        <location filename="../../modules/write/qt/write_dialog.ui" line="103"/>
        <source>(Condition)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../modules/write/qt/write_dialog.ui" line="110"/>
        <source>(Value)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../modules/write/qt/write_dialog.ui" line="136"/>
        <source>(Status)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../../modules/write/qt/write_dialog.ui" line="156"/>
        <source>OK</source>
        <translation>ОК</translation>
    </message>
    <message>
        <location filename="../../modules/write/qt/write_dialog.ui" line="163"/>
        <source>Cancel</source>
        <translation>Отмена</translation>
    </message>
    <message>
        <location filename="../../modules/write/qt/write_dialog.cpp" line="88"/>
        <source>Satisfied</source>
        <translation>Выполнено</translation>
    </message>
    <message>
        <location filename="../../modules/write/qt/write_dialog.cpp" line="89"/>
        <source>Unsatisfied</source>
        <translation>Нарушено</translation>
    </message>
    <message>
        <location filename="../../modules/write/qt/write_dialog.cpp" line="108"/>
        <source>Incorrect floating point value.</source>
        <translation>Указано некорректное значение с плавающей точкой.</translation>
    </message>
    <!-- Status-code descriptions (core/scada/status.cpp). These moved out of
         the C++ table into this catalog; the English there is the lookup key, so
         a server with no catalog renders the English and the client renders the
         Russian these entries preserve verbatim. -->
    <message>
        <source>Operation completed successfully</source>
        <translation>Операция выполнена успешно</translation>
    </message>
    <message>
        <source>Operation in progress</source>
        <translation>Операция выполняется</translation>
    </message>
    <message>
        <source>The lock was not changed</source>
        <translation>Блокировка не была изменена</translation>
    </message>
    <message>
        <source>Wrong user name or password</source>
        <translation>Неверное имя пользователя или пароль</translation>
    </message>
    <message>
        <source>A session for this user is already open</source>
        <translation>Сессия данного пользователя уже установлена</translation>
    </message>
    <message>
        <source>Protocol version is not supported</source>
        <translation>Версия протокола не поддерживается</translation>
    </message>
    <message>
        <source>Another command is already running</source>
        <translation>В данный момент выполняется другая команда</translation>
    </message>
    <message>
        <source>Wrong node identifier</source>
        <translation>Неправильный идентификатор узла</translation>
    </message>
    <message>
        <source>Wrong device identifier</source>
        <translation>Неправильный идентификатор устройства</translation>
    </message>
    <message>
        <source>Not connected</source>
        <translation>Соединение не установлено</translation>
    </message>
    <message>
        <source>Session closed because this user connected again</source>
        <translation>Сессия разорвана из-за повторного подключения данного пользователя</translation>
    </message>
    <message>
        <source>Operation aborted after the wait timed out</source>
        <translation>Операция прервана по истечении времени ожидания</translation>
    </message>
    <message>
        <source>Cannot delete the object because dependent objects exist</source>
        <translation>Невозможно удалить объект из-за наличия зависимых объектов</translation>
    </message>
    <message>
        <source>Session closed because the server stopped</source>
        <translation>Сессия разорвана из-за остановки сервера</translation>
    </message>
    <message>
        <source>The command is not supported by this object</source>
        <translation>Команда не поддерживается данным объектом</translation>
    </message>
    <message>
        <source>Cannot delete a user from a session that user opened</source>
        <translation>Невозможно удалить пользователя из открытой им сессии</translation>
    </message>
    <message>
        <source>An object with this identifier already exists</source>
        <translation>Объект с таким идентификатором уже существует</translation>
    </message>
    <message>
        <source>File version is not supported</source>
        <translation>Версия файла не поддерживается</translation>
    </message>
    <message>
        <source>Wrong object type</source>
        <translation>Неправильный тип объекта</translation>
    </message>
    <message>
        <source>Wrong parent object identifier</source>
        <translation>Неправильный идентификатор родительского объекта</translation>
    </message>
    <message>
        <source>Not logged on</source>
        <translation>Авторизация не выполнена</translation>
    </message>
    <message>
        <source>Wrong subscription number</source>
        <translation>Неправильный номер подписки</translation>
    </message>
    <message>
        <source>Wrong index</source>
        <translation>Неправильный индекс</translation>
    </message>
    <message>
        <source>Wrong IEC 60870-5 ASDU type</source>
        <translation>Неправильный тип ASDU протокола МЭК-60870</translation>
    </message>
    <message>
        <source>Wrong IEC 60870-5 cause of transmission</source>
        <translation>Неправильная причина передачи протокола МЭК-60870</translation>
    </message>
    <message>
        <source>Wrong IEC 60870-5 device address</source>
        <translation>Неправильный адрес устройства протокола МЭК-60870</translation>
    </message>
    <message>
        <source>Wrong IEC 60870-5 information object address</source>
        <translation>Неправильный адрес объекта протокола МЭК-60870</translation>
    </message>
    <message>
        <source>IEC 60870-5 protocol error</source>
        <translation>Ошибка протокола МЭК-60870</translation>
    </message>
    <message>
        <source>Wrong command arguments</source>
        <translation>Неправильные аргументы команды</translation>
    </message>
    <message>
        <source>Cannot convert the string to a value of this type</source>
        <translation>Невозможно преобразовать строку в значение данного типа</translation>
    </message>
    <message>
        <source>String is too long</source>
        <translation>Слишком длинная строка</translation>
    </message>
    <message>
        <source>Wrong object attribute</source>
        <translation>Неправильный атрибут объекта</translation>
    </message>
    <message>
        <source>Wrong reference type</source>
        <translation>Неправильный тип ссылки</translation>
    </message>
    <message>
        <source>Wrong node class</source>
        <translation>Неправильный класс узла</translation>
    </message>
    <message>
        <source>IEC 61850 protocol error</source>
        <translation>Ошибка протокола МЭК-61850</translation>
    </message>
    <message>
        <source>The request is empty</source>
        <translation>Запрос пуст</translation>
    </message>
    <message>
        <source>Name not found</source>
        <translation>Имя не найдено</translation>
    </message>
    <message>
        <source>Wrong monitored item number</source>
        <translation>Неправильный номер элемента мониторинга</translation>
    </message>
    <message>
        <source>The requested message is no longer available</source>
        <translation>Запрошенное сообщение больше недоступно</translation>
    </message>
    <message>
        <source>Invalid client application signature</source>
        <translation>Неверная подпись приложения клиента</translation>
    </message>
    <message>
        <source>Too many operations in the request</source>
        <translation>Слишком много операций в запросе</translation>
    </message>
    <message>
        <source>Too many monitored items in the request</source>
        <translation>Слишком много элементов мониторинга в запросе</translation>
    </message>
    <message>
        <source>Unknown message sequence number</source>
        <translation>Неизвестный порядковый номер сообщения</translation>
    </message>
    <message>
        <source>The browse continuation point limit is exhausted</source>
        <translation>Исчерпан лимит точек продолжения просмотра</translation>
    </message>
    <message>
        <source>Wrong TimestampsToReturn value</source>
        <translation>Неправильное значение TimestampsToReturn</translation>
    </message>
    <message>
        <source>Unknown view identifier</source>
        <translation>Неизвестный идентификатор представления</translation>
    </message>
    <message>
        <source>Invalid history request parameters</source>
        <translation>Недопустимые параметры запроса истории</translation>
    </message>
    <message>
        <source>The session has no subscriptions</source>
        <translation>Для сессии нет подписок</translation>
    </message>
    <message>
        <source>Not enough rights to perform the operation</source>
        <translation>Недостаточно прав для выполнения операции</translation>
    </message>
    <message>
        <source>Operation is not supported</source>
        <translation>Операция не поддерживается</translation>
    </message>
    <message>
        <source>The license has expired</source>
        <translation>Срок действия лицензии истёк</translation>
    </message>
    <message>
        <source>No value received from the data source yet</source>
        <translation>Значение от источника данных ещё не получено</translation>
    </message>
    <message>
        <source>The value is out of range and will not be stored</source>
        <translation>Значение недопустимо и не будет сохранено</translation>
    </message>
    <!-- Data-quality flags, rendered as a space-separated run
         (core/scada/qualifier.cpp). Kept abbreviated exactly as before: the
         strip sits in a narrow grid cell. -->
    <message>
        <source>Bad quality</source>
        <translation>Недост</translation>
    </message>
    <message>
        <source>Backup</source>
        <translation>Резерв</translation>
    </message>
    <message>
        <source>No link</source>
        <translation>НетСвязи</translation>
    </message>
    <message>
        <source>Manual</source>
        <translation>Ручной</translation>
    </message>
    <message>
        <source>Misconfigured</source>
        <translation>НеСконф</translation>
    </message>
    <message>
        <source>Simulated</source>
        <translation>Эмулирован</translation>
    </message>
    <message>
        <source>Sporadic</source>
        <translation>Спорадика</translation>
    </message>
    <message>
        <source>Stale</source>
        <translation>Устарел</translation>
    </message>
    <message>
        <source>Failed</source>
        <translation>Ошибка</translation>
    </message>
    <!-- Boolean value labels (core/scada/variant.cpp,
         Variant::TrueLabel/FalseLabel). -->
    <message>
        <source>Yes</source>
        <translation>Да</translation>
    </message>
    <message>
        <source>No</source>
        <translation>Нет</translation>
    </message>
</context>
</TS>
