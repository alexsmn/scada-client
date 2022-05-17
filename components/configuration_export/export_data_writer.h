#pragma once

// Can be used as a signature for CsvReader.
const char16_t kNodeIdTitle[] = u"Ид";

class CsvWriter;
struct ExportData;

void WriteExportData(const ExportData& data, CsvWriter& writer);
