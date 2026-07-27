#include "modules/watch/frame_decode.h"

#include "aui/translation.h"
#include "base/utf_convert.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <format>
#include <span>

namespace {

using Octets = std::span<const std::uint8_t>;

std::u16string U16(std::string_view text) {
  return UtfConvert<char16_t>(text);
}

// The information element that follows an object address, i.e. what the type
// identification says the object's octets mean. IEC 60870-5-101 §7.3 names the
// elements; §7.2.1.1 assigns them to type identifications.
enum class Element {
  kUnknown,  // shown as raw octets
  kNone,     // no element at all (e.g. C_RD_NA_1 read command)
  kSiq,      // single-point information with quality, §7.2.6.1
  kDiq,      // double-point information with quality, §7.2.6.2
  kVtiQds,   // step position + quality descriptor
  kBsiQds,   // 32-bit string + quality descriptor
  kNva,      // normalised value, no quality (M_ME_ND_1)
  kNvaQds,   // normalised value + quality descriptor
  kSvaQds,   // scaled value + quality descriptor
  kFloatQds, // short floating point + quality descriptor
  kBcr,      // binary counter reading, §7.2.6.9
  kSco,      // single command, §7.2.6.15
  kDco,      // double command, §7.2.6.16
  kRco,      // regulating step command, §7.2.6.17
  kNvaQos,   // set-point command, normalised value
  kSvaQos,   // set-point command, scaled value
  kFloatQos, // set-point command, short float
  kBsi,      // 32-bit string command
  kQoi,      // qualifier of interrogation, §7.2.6.22
  kQcc,      // qualifier of counter interrogation
  kQrp,      // qualifier of reset process
  kCoi,      // cause of initialisation
  kCp56,     // a bare CP56Time2a (clock synchronisation)
};

// The time tag appended after the information element, if any.
enum class TimeTag { kNone, kCp24, kCp56 };

struct TypeInfo {
  const char* mnemonic;
  Element element;
  TimeTag time_tag;
};

// IEC 60870-5-101 §7.2.1.1 Type identification, as profiled for -104 by
// IEC 60870-5-104 §8. Only the types this product's devices actually exchange
// are named; anything else falls through to the number plus raw octets, which
// is still more useful than nothing.
const TypeInfo* FindType(int type_id) {
  struct Entry {
    int id;
    TypeInfo info;
  };
  static constexpr Entry kTypes[] = {
      {1, {"M_SP_NA_1", Element::kSiq, TimeTag::kNone}},
      {2, {"M_SP_TA_1", Element::kSiq, TimeTag::kCp24}},
      {3, {"M_DP_NA_1", Element::kDiq, TimeTag::kNone}},
      {4, {"M_DP_TA_1", Element::kDiq, TimeTag::kCp24}},
      {5, {"M_ST_NA_1", Element::kVtiQds, TimeTag::kNone}},
      {6, {"M_ST_TA_1", Element::kVtiQds, TimeTag::kCp24}},
      {7, {"M_BO_NA_1", Element::kBsiQds, TimeTag::kNone}},
      {8, {"M_BO_TA_1", Element::kBsiQds, TimeTag::kCp24}},
      {9, {"M_ME_NA_1", Element::kNvaQds, TimeTag::kNone}},
      {10, {"M_ME_TA_1", Element::kNvaQds, TimeTag::kCp24}},
      {11, {"M_ME_NB_1", Element::kSvaQds, TimeTag::kNone}},
      {12, {"M_ME_TB_1", Element::kSvaQds, TimeTag::kCp24}},
      {13, {"M_ME_NC_1", Element::kFloatQds, TimeTag::kNone}},
      {14, {"M_ME_TC_1", Element::kFloatQds, TimeTag::kCp24}},
      {15, {"M_IT_NA_1", Element::kBcr, TimeTag::kNone}},
      {16, {"M_IT_TA_1", Element::kBcr, TimeTag::kCp24}},
      {21, {"M_ME_ND_1", Element::kNva, TimeTag::kNone}},
      {30, {"M_SP_TB_1", Element::kSiq, TimeTag::kCp56}},
      {31, {"M_DP_TB_1", Element::kDiq, TimeTag::kCp56}},
      {32, {"M_ST_TB_1", Element::kVtiQds, TimeTag::kCp56}},
      {33, {"M_BO_TB_1", Element::kBsiQds, TimeTag::kCp56}},
      {34, {"M_ME_TD_1", Element::kNvaQds, TimeTag::kCp56}},
      {35, {"M_ME_TE_1", Element::kSvaQds, TimeTag::kCp56}},
      {36, {"M_ME_TF_1", Element::kFloatQds, TimeTag::kCp56}},
      {37, {"M_IT_TB_1", Element::kBcr, TimeTag::kCp56}},
      {45, {"C_SC_NA_1", Element::kSco, TimeTag::kNone}},
      {46, {"C_DC_NA_1", Element::kDco, TimeTag::kNone}},
      {47, {"C_RC_NA_1", Element::kRco, TimeTag::kNone}},
      {48, {"C_SE_NA_1", Element::kNvaQos, TimeTag::kNone}},
      {49, {"C_SE_NB_1", Element::kSvaQos, TimeTag::kNone}},
      {50, {"C_SE_NC_1", Element::kFloatQos, TimeTag::kNone}},
      {51, {"C_BO_NA_1", Element::kBsi, TimeTag::kNone}},
      {58, {"C_SC_TA_1", Element::kSco, TimeTag::kCp56}},
      {59, {"C_DC_TA_1", Element::kDco, TimeTag::kCp56}},
      {60, {"C_RC_TA_1", Element::kRco, TimeTag::kCp56}},
      {61, {"C_SE_TA_1", Element::kNvaQos, TimeTag::kCp56}},
      {62, {"C_SE_TB_1", Element::kSvaQos, TimeTag::kCp56}},
      {63, {"C_SE_TC_1", Element::kFloatQos, TimeTag::kCp56}},
      {64, {"C_BO_TA_1", Element::kBsi, TimeTag::kCp56}},
      {70, {"M_EI_NA_1", Element::kCoi, TimeTag::kNone}},
      {100, {"C_IC_NA_1", Element::kQoi, TimeTag::kNone}},
      {101, {"C_CI_NA_1", Element::kQcc, TimeTag::kNone}},
      {102, {"C_RD_NA_1", Element::kNone, TimeTag::kNone}},
      {103, {"C_CS_NA_1", Element::kCp56, TimeTag::kNone}},
      {105, {"C_RP_NA_1", Element::kQrp, TimeTag::kNone}},
  };
  for (const Entry& entry : kTypes) {
    if (entry.id == type_id)
      return &entry.info;
  }
  return nullptr;
}

// IEC 60870-5-101 §7.2.3 Cause of transmission. The abbreviations are the
// standard's own and are not translated, like the type mnemonics: an engineer
// reading a trace matches them against the protocol document.
const char* CauseName(int cause) {
  switch (cause) {
    case 1: return "per/cyc";
    case 2: return "back";
    case 3: return "spont";
    case 4: return "init";
    case 5: return "req";
    case 6: return "act";
    case 7: return "actcon";
    case 8: return "deact";
    case 9: return "deactcon";
    case 10: return "actterm";
    case 11: return "retrem";
    case 12: return "retloc";
    case 13: return "file";
    case 20: return "inrogen";
    case 37: return "reqcogcn";
    case 44: return "unknown type";
    case 45: return "unknown cause";
    case 46: return "unknown asdu address";
    case 47: return "unknown object address";
    default:
      // §7.2.3: 21..36 are the group interrogations, 38..41 the counter
      // groups. Naming each one individually earns nothing over the number.
      if (cause >= 21 && cause <= 36)
        return "inro group";
      if (cause >= 38 && cause <= 41)
        return "reqco group";
      return nullptr;
  }
}

int ElementSize(Element element) {
  switch (element) {
    case Element::kNone:
    case Element::kUnknown:
      return 0;
    case Element::kSiq:
    case Element::kDiq:
    case Element::kSco:
    case Element::kDco:
    case Element::kRco:
    case Element::kQoi:
    case Element::kQcc:
    case Element::kQrp:
    case Element::kCoi:
      return 1;
    case Element::kVtiQds:
    case Element::kNva:
      return 2;
    case Element::kNvaQds:
    case Element::kSvaQds:
    case Element::kNvaQos:
    case Element::kSvaQos:
      return 3;
    case Element::kBsi:
      return 4;
    case Element::kBsiQds:
    case Element::kFloatQds:
    case Element::kFloatQos:
    case Element::kBcr:
      return 5;
    case Element::kCp56:
      return 7;
  }
  return 0;
}

int TimeTagSize(TimeTag tag) {
  switch (tag) {
    case TimeTag::kNone: return 0;
    case TimeTag::kCp24: return 3;
    case TimeTag::kCp56: return 7;
  }
  return 0;
}

// IEC 60870-5-101 §7.2.6.3 Quality descriptor (QDS). All-zero is the healthy
// case and is by far the most common, so it gets a word rather than "0x00".
std::string QualityText(std::uint8_t qds) {
  if ((qds & 0xF1) == 0)
    return std::format("0x{:02X} · good", qds);
  std::string flags;
  auto add = [&flags](const char* name) {
    if (!flags.empty())
      flags += ' ';
    flags += name;
  };
  if (qds & 0x01) add("OV");   // overflow
  if (qds & 0x10) add("BL");   // blocked
  if (qds & 0x20) add("SB");   // substituted
  if (qds & 0x40) add("NT");   // not topical
  if (qds & 0x80) add("IV");   // invalid
  return std::format("0x{:02X} · {}", qds, flags);
}

std::uint16_t ReadU16(Octets data, size_t offset) {
  return static_cast<std::uint16_t>(data[offset] |
                                    (data[offset + 1] << 8));
}

std::uint32_t ReadU24(Octets data, size_t offset) {
  return static_cast<std::uint32_t>(data[offset]) |
         (static_cast<std::uint32_t>(data[offset + 1]) << 8) |
         (static_cast<std::uint32_t>(data[offset + 2]) << 16);
}

std::uint32_t ReadU32(Octets data, size_t offset) {
  return ReadU24(data, offset) |
         (static_cast<std::uint32_t>(data[offset + 3]) << 24);
}

float ReadFloat(Octets data, size_t offset) {
  const std::uint32_t bits = ReadU32(data, offset);
  float value = 0;
  std::memcpy(&value, &bits, sizeof(value));
  return value;
}

// IEC 60870-5-101 §7.2.6.6: a normalised value is a signed 16-bit fraction of
// full scale, i.e. F16 / 32768.
std::string NormalisedText(Octets data, size_t offset) {
  const std::int16_t raw = static_cast<std::int16_t>(ReadU16(data, offset));
  return std::format("{:.5g}", static_cast<double>(raw) / 32768.0);
}

// IEC 60870-5-101 §7.2.6.18 CP56Time2a. Rendered as the operator reads it, not
// as a field dump: the point of showing it is to compare it against the row's
// own arrival time.
std::string Cp56Text(Octets data, size_t offset) {
  const std::uint16_t millis = ReadU16(data, offset);
  const int minute = data[offset + 2] & 0x3F;
  const int hour = data[offset + 3] & 0x1F;
  const int day = data[offset + 4] & 0x1F;
  const int month = data[offset + 5] & 0x0F;
  const int year = data[offset + 6] & 0x7F;
  const bool invalid = (data[offset + 2] & 0x80) != 0;
  return std::format("{:02}-{:02}-{:02} {:02}:{:02}:{:02}.{:03}{}", day, month,
                     year, hour, minute, millis / 1000, millis % 1000,
                     invalid ? " · IV" : "");
}

// IEC 60870-5-101 §7.2.6.19 CP24Time2a: milliseconds and minute only.
std::string Cp24Text(Octets data, size_t offset) {
  const std::uint16_t millis = ReadU16(data, offset);
  const int minute = data[offset + 2] & 0x3F;
  return std::format("{:02}:{:02}.{:03}", minute, millis / 1000, millis % 1000);
}

FrameDecodeNode MakeNode(std::u16string name,
                         std::string value,
                         int offset = 0,
                         int length = 0) {
  return FrameDecodeNode{.name = std::move(name),
                         .value = U16(value),
                         .offset = offset,
                         .length = length};
}

// Appends the decoded information element (and its time tag, if the type has
// one) as children of the object node. `offset` is the first octet after the
// object address.
void AppendElement(FrameDecodeNode& object,
                   Octets data,
                   size_t offset,
                   const TypeInfo& type) {
  const int size = ElementSize(type.element);
  switch (type.element) {
    case Element::kNone:
      break;

    case Element::kSiq: {
      const std::uint8_t siq = data[offset];
      object.children.push_back(MakeNode(
          Translate("Value"), std::format("{}", (siq & 0x01) ? "ON" : "OFF"),
          static_cast<int>(offset), 1));
      object.children.push_back(
          MakeNode(Translate("Quality"), QualityText(siq & 0xF0),
                   static_cast<int>(offset), 1));
      break;
    }

    case Element::kDiq: {
      const std::uint8_t diq = data[offset];
      // §7.2.6.2: 0 and 3 are the indeterminate states, 1 is OFF, 2 is ON.
      const int dpi = diq & 0x03;
      const char* text = dpi == 1 ? "OFF" : dpi == 2 ? "ON" : "indeterminate";
      object.children.push_back(MakeNode(Translate("Value"),
                                         std::format("{} ({})", text, dpi),
                                         static_cast<int>(offset), 1));
      object.children.push_back(
          MakeNode(Translate("Quality"), QualityText(diq & 0xF0),
                   static_cast<int>(offset), 1));
      break;
    }

    case Element::kVtiQds: {
      // §7.2.6.5: a 7-bit signed step position plus a transient flag.
      const std::uint8_t vti = data[offset];
      int step = vti & 0x7F;
      if (step > 63)
        step -= 128;
      object.children.push_back(
          MakeNode(Translate("Value"),
                   std::format("{}{}", step, (vti & 0x80) ? " · transient" : ""),
                   static_cast<int>(offset), 1));
      object.children.push_back(
          MakeNode(Translate("Quality"), QualityText(data[offset + 1]),
                   static_cast<int>(offset) + 1, 1));
      break;
    }

    case Element::kBsiQds:
      object.children.push_back(
          MakeNode(Translate("Value"),
                   std::format("0x{:08X}", ReadU32(data, offset)),
                   static_cast<int>(offset), 4));
      object.children.push_back(
          MakeNode(Translate("Quality"), QualityText(data[offset + 4]),
                   static_cast<int>(offset) + 4, 1));
      break;

    case Element::kBsi:
      object.children.push_back(
          MakeNode(Translate("Value"),
                   std::format("0x{:08X}", ReadU32(data, offset)),
                   static_cast<int>(offset), 4));
      break;

    case Element::kNva:
      object.children.push_back(MakeNode(Translate("Value"),
                                         NormalisedText(data, offset),
                                         static_cast<int>(offset), 2));
      break;

    case Element::kNvaQds:
    case Element::kNvaQos:
      object.children.push_back(MakeNode(Translate("Value"),
                                         NormalisedText(data, offset),
                                         static_cast<int>(offset), 2));
      object.children.push_back(MakeNode(
          type.element == Element::kNvaQds ? Translate("Quality")
                                           : Translate("Qualifier"),
          QualityText(data[offset + 2]), static_cast<int>(offset) + 2, 1));
      break;

    case Element::kSvaQds:
    case Element::kSvaQos:
      object.children.push_back(MakeNode(
          Translate("Value"),
          std::format("{}", static_cast<std::int16_t>(ReadU16(data, offset))),
          static_cast<int>(offset), 2));
      object.children.push_back(MakeNode(
          type.element == Element::kSvaQds ? Translate("Quality")
                                           : Translate("Qualifier"),
          QualityText(data[offset + 2]), static_cast<int>(offset) + 2, 1));
      break;

    case Element::kFloatQds:
    case Element::kFloatQos:
      object.children.push_back(
          MakeNode(Translate("Value"),
                   std::format("{:.7g}", ReadFloat(data, offset)),
                   static_cast<int>(offset), 4));
      object.children.push_back(MakeNode(
          type.element == Element::kFloatQds ? Translate("Quality")
                                             : Translate("Qualifier"),
          QualityText(data[offset + 4]), static_cast<int>(offset) + 4, 1));
      break;

    case Element::kBcr:
      // §7.2.6.9: a 32-bit counter plus a sequence/flags octet.
      object.children.push_back(MakeNode(
          Translate("Value"),
          std::format("{}", static_cast<std::int32_t>(ReadU32(data, offset))),
          static_cast<int>(offset), 4));
      object.children.push_back(
          MakeNode(Translate("Sequence"),
                   std::format("0x{:02X}", data[offset + 4]),
                   static_cast<int>(offset) + 4, 1));
      break;

    case Element::kSco:
    case Element::kDco:
    case Element::kRco: {
      // §7.2.6.15-17: the command state is in the low bits, the qualifier in
      // bits 2-6, and bit 7 selects between select and execute.
      const std::uint8_t command = data[offset];
      const int state = type.element == Element::kSco ? (command & 0x01)
                                                      : (command & 0x03);
      object.children.push_back(MakeNode(
          Translate("Command"),
          std::format("{}{}", state,
                      (command & 0x80) ? " · select" : " · execute"),
          static_cast<int>(offset), 1));
      object.children.push_back(
          MakeNode(Translate("Qualifier"),
                   std::format("{}", (command >> 2) & 0x1F),
                   static_cast<int>(offset), 1));
      break;
    }

    case Element::kQoi:
    case Element::kQcc:
    case Element::kQrp:
    case Element::kCoi:
      object.children.push_back(
          MakeNode(Translate("Qualifier"), std::format("{}", data[offset]),
                   static_cast<int>(offset), 1));
      break;

    case Element::kCp56:
      object.children.push_back(MakeNode(Translate("Time"),
                                         Cp56Text(data, offset),
                                         static_cast<int>(offset), 7));
      break;

    case Element::kUnknown:
      break;
  }

  if (type.time_tag == TimeTag::kNone)
    return;

  const size_t time_offset = offset + size;
  object.children.push_back(
      MakeNode(Translate("Time"),
               type.time_tag == TimeTag::kCp56 ? Cp56Text(data, time_offset)
                                               : Cp24Text(data, time_offset),
               static_cast<int>(time_offset), TimeTagSize(type.time_tag)));
}

// The ASDU, starting at `offset` (the octet after the APCI control field).
// IEC 60870-5-101 §7.2: type identification, variable structure qualifier,
// cause of transmission, common address, then the information objects.
FrameDecodeNode DecodeAsdu(Octets data,
                           size_t offset,
                           std::vector<scada::Int32>& object_addresses) {
  FrameDecodeNode asdu{.name = Translate("ASDU")};

  const int type_id = data[offset];
  const TypeInfo* type = FindType(type_id);
  asdu.value = U16(type ? type->mnemonic : std::format("type {}", type_id));
  asdu.children.push_back(MakeNode(
      Translate("Type ID"),
      type ? std::format("{} · {}", type_id, type->mnemonic)
           : std::format("{}", type_id),
      static_cast<int>(offset), 1));

  const std::uint8_t vsq = data[offset + 1];
  const bool sequence = (vsq & 0x80) != 0;
  const int count = vsq & 0x7F;
  asdu.children.push_back(MakeNode(Translate("SQ / count"),
                                   std::format("{} / {}", sequence ? 1 : 0,
                                               count),
                                   static_cast<int>(offset) + 1, 1));

  const std::uint8_t cot = data[offset + 2];
  const int cause = cot & 0x3F;
  const char* cause_name = CauseName(cause);
  std::string cause_text =
      cause_name ? std::format("{} · {}", cause, cause_name)
                 : std::format("{}", cause);
  // §7.2.3: P/N marks a negative confirmation and T marks a test ASDU. Both
  // change what the frame means, so they are never hidden.
  if (cot & 0x40)
    cause_text += " · negative";
  if (cot & 0x80)
    cause_text += " · test";
  asdu.children.push_back(MakeNode(Translate("Cause (COT)"),
                                   std::move(cause_text),
                                   static_cast<int>(offset) + 2, 1));
  asdu.children.push_back(MakeNode(Translate("Originator"),
                                   std::format("{}", data[offset + 3]),
                                   static_cast<int>(offset) + 3, 1));
  asdu.children.push_back(MakeNode(Translate("Common address"),
                                   std::format("{}", ReadU16(data, offset + 4)),
                                   static_cast<int>(offset) + 4, 2));

  size_t pos = offset + 6;
  const int element_size =
      type ? ElementSize(type->element) + TimeTagSize(type->time_tag) : 0;

  // A type this decoder does not know has an unknown element size, so the
  // objects cannot be walked. Show the remaining octets whole rather than
  // mis-splitting them.
  if (!type) {
    if (pos < data.size()) {
      asdu.children.push_back(
          MakeNode(Translate("Information objects"),
                   std::format("{} bytes", data.size() - pos),
                   static_cast<int>(pos),
                   static_cast<int>(data.size() - pos)));
    }
    return asdu;
  }

  std::uint32_t sequence_address = 0;
  for (int i = 0; i < count; ++i) {
    // §7.2.5: with SQ set, one address is followed by `count` elements at
    // consecutive addresses; otherwise every element carries its own.
    const bool reads_address = !sequence || i == 0;
    if (reads_address) {
      if (pos + 3 > data.size())
        break;
      sequence_address = ReadU24(data, pos);
      pos += 3;
    } else {
      ++sequence_address;
    }
    if (pos + element_size > data.size())
      break;

    FrameDecodeNode object{
        .name = Translate("Object"),
        .value = U16(std::format("IOA {}", sequence_address)),
        .offset = static_cast<int>(reads_address ? pos - 3 : pos),
        .length = static_cast<int>((reads_address ? 3 : 0) + element_size)};
    AppendElement(object, data, pos, *type);
    asdu.children.push_back(std::move(object));
    object_addresses.push_back(static_cast<scada::Int32>(sequence_address));
    pos += element_size;
  }

  return asdu;
}

}  // namespace

FrameDecode DecodeFrame(const scada::DeviceFrame& frame) {
  FrameDecode decode;

  const Octets data{reinterpret_cast<const std::uint8_t*>(frame.raw_data.data()),
                    frame.raw_data.size()};

  std::string hex;
  hex.reserve(data.size() * 3);
  for (std::uint8_t byte : data) {
    if (!hex.empty())
      hex += ' ';
    hex += std::format("{:02X}", byte);
  }
  decode.hex = U16(hex);

  // Nothing to decode: an ordinary log line, or a server that reports the
  // summary fields without the octets.
  if (data.empty()) {
    decode.note = Translate("No octets were captured for this line.");
    return decode;
  }

  // IEC 60870-5-104 §5.1: an APDU is a 0x68 start octet, a length octet
  // covering everything after it, and four control-field octets. The same
  // connection layer also carries -101 FT1.2 frames, which start with 0x68 but
  // repeat the length — the length check is what tells them apart.
  if (data.size() < 6 || data[0] != 0x68 ||
      data[1] != data.size() - 2) {
    decode.note = Translate("Not an IEC 60870-5-104 APDU.");
    return decode;
  }

  FrameDecodeNode apci{.name = Translate("APCI")};
  apci.children.push_back(MakeNode(Translate("Start"), "0x68", 0, 1));
  apci.children.push_back(
      MakeNode(Translate("APDU length"), std::format("{}", data[1]), 1, 1));

  const std::uint8_t control = data[2];
  if ((control & 0x01) == 0) {
    // §5.1: I-format. Both sequence numbers are 15-bit, carried left-shifted
    // one bit so the low bit can hold the format marker.
    decode.summary = Translate("I-format");
    apci.value = Translate("I-format");
    apci.children.push_back(MakeNode(Translate("N(S) send"),
                                     std::format("{}", ReadU16(data, 2) >> 1),
                                     2, 2));
    apci.children.push_back(MakeNode(Translate("N(R) recv"),
                                     std::format("{}", ReadU16(data, 4) >> 1),
                                     4, 2));
  } else if ((control & 0x03) == 0x01) {
    // §5.1: S-format acknowledges without sending, so it carries N(R) only.
    decode.summary = Translate("S-format");
    apci.value = Translate("S-format");
    apci.children.push_back(MakeNode(Translate("N(R) recv"),
                                     std::format("{}", ReadU16(data, 4) >> 1),
                                     4, 2));
  } else {
    // §5.1: U-format is unnumbered; the function is a one-hot field.
    decode.summary = Translate("U-format");
    apci.value = Translate("U-format");
    const char* function = "unknown";
    switch (control & 0xFC) {
      case 0x04: function = "STARTDT act"; break;
      case 0x08: function = "STARTDT con"; break;
      case 0x10: function = "STOPDT act"; break;
      case 0x20: function = "STOPDT con"; break;
      case 0x40: function = "TESTFR act"; break;
      case 0x80: function = "TESTFR con"; break;
      default: break;
    }
    apci.children.push_back(MakeNode(Translate("Function"), function, 2, 1));
  }

  decode.nodes.push_back(std::move(apci));

  // Only an I-format APDU carries an ASDU, and only when there are octets left
  // after the control field. §7.2.1 fixes the ASDU header at six octets.
  const bool has_asdu = (control & 0x01) == 0 && data.size() >= 6 + 6;
  if (has_asdu) {
    FrameDecodeNode asdu = DecodeAsdu(data, 6, decode.object_addresses);
    decode.summary += u" · " + asdu.value;
    decode.nodes.push_back(std::move(asdu));
  }

  return decode;
}

void AppendMappedNodes(FrameDecode& decode,
                       std::span<const FrameObjectMapping> mappings) {
  if (decode.object_addresses.empty())
    return;

  FrameDecodeNode group{.name = Translate("Mapped node")};
  for (scada::Int32 address : decode.object_addresses) {
    const auto found = std::ranges::find(mappings, address,
                                         &FrameObjectMapping::object_address);
    if (found != mappings.end()) {
      group.children.push_back(
          {.name = found->signal, .value = found->node_id});
    } else {
      group.children.push_back(
          {.name = U16(std::format("IOA {}", address)),
           .value = Translate("Not in the device's address map")});
    }
  }
  decode.nodes.push_back(std::move(group));
}
