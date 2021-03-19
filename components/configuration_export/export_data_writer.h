#pragma once

// Can be used as a signature for CsvReader.
const wchar_t kNodeIdTitle[] = L"Ид";

class CsvWriter;
struct ExportData;

void WriteExportData(const ExportData& data, CsvWriter& writer);
