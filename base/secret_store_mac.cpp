#include "base/secret_store.h"

#if defined(__APPLE__)

#include "base/utf_convert.h"

#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>

namespace {

// The Client's own item: kSecAttrService. What the operator sees in Keychain
// Access as the item's name.
constexpr char kClientService[] = "Telecontrol SCADA Client";

// Owns one CoreFoundation reference.
template <class T>
class ScopedCf {
 public:
  explicit ScopedCf(T ref = nullptr) : ref_{ref} {}
  ~ScopedCf() {
    if (ref_)
      ::CFRelease(ref_);
  }
  ScopedCf(ScopedCf&& other) noexcept : ref_{other.ref_} {
    other.ref_ = nullptr;
  }
  ScopedCf(const ScopedCf&) = delete;
  ScopedCf& operator=(const ScopedCf&) = delete;

  T get() const { return ref_; }
  T* receive() { return &ref_; }

 private:
  T ref_;
};

ScopedCf<CFStringRef> MakeCfString(std::string_view utf8) {
  return ScopedCf<CFStringRef>{::CFStringCreateWithBytes(
      kCFAllocatorDefault, reinterpret_cast<const UInt8*>(utf8.data()),
      static_cast<CFIndex>(utf8.size()), kCFStringEncodingUTF8, false)};
}

// A generic-password item in the user's default (login) keychain, one per
// secret name: kSecClassGenericPassword keyed by service + account.
// https://developer.apple.com/documentation/security/ksecclassgenericpassword
class KeychainSecretStore final : public SecretStore {
 public:
  explicit KeychainSecretStore(std::string service)
      : service_{std::move(service)} {}

  std::optional<std::u16string> Read(std::string_view name) const override {
    ScopedCf<CFMutableDictionaryRef> query = MakeQuery(name);
    ::CFDictionarySetValue(query.get(), kSecReturnData, kCFBooleanTrue);
    ::CFDictionarySetValue(query.get(), kSecMatchLimit, kSecMatchLimitOne);

    ScopedCf<CFTypeRef> result;
    if (::SecItemCopyMatching(query.get(), result.receive()) != errSecSuccess ||
        !result.get() || ::CFGetTypeID(result.get()) != ::CFDataGetTypeID()) {
      return std::nullopt;
    }
    auto data = static_cast<CFDataRef>(result.get());
    std::string_view utf8{
        reinterpret_cast<const char*>(::CFDataGetBytePtr(data)),
        static_cast<size_t>(::CFDataGetLength(data))};
    return UtfConvert<char16_t>(utf8);
  }

  bool Write(std::string_view name, std::u16string_view value) override {
    const std::string utf8 = UtfConvert<char>(value);
    ScopedCf<CFDataRef> data{::CFDataCreate(
        kCFAllocatorDefault, reinterpret_cast<const UInt8*>(utf8.data()),
        static_cast<CFIndex>(utf8.size()))};

    ScopedCf<CFMutableDictionaryRef> query = MakeQuery(name);
    ScopedCf<CFMutableDictionaryRef> update{::CFDictionaryCreateMutable(
        kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks)};
    ::CFDictionarySetValue(update.get(), kSecValueData, data.get());

    OSStatus status = ::SecItemUpdate(query.get(), update.get());
    if (status == errSecItemNotFound) {
      ::CFDictionarySetValue(query.get(), kSecValueData, data.get());
      status = ::SecItemAdd(query.get(), nullptr);
    }
    return status == errSecSuccess;
  }

  bool Erase(std::string_view name) override {
    ScopedCf<CFMutableDictionaryRef> query = MakeQuery(name);
    const OSStatus status = ::SecItemDelete(query.get());
    return status == errSecSuccess || status == errSecItemNotFound;
  }

 private:
  ScopedCf<CFMutableDictionaryRef> MakeQuery(std::string_view name) const {
    ScopedCf<CFMutableDictionaryRef> query{::CFDictionaryCreateMutable(
        kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks)};
    ScopedCf<CFStringRef> service = MakeCfString(service_);
    ScopedCf<CFStringRef> account = MakeCfString(name);
    ::CFDictionarySetValue(query.get(), kSecClass, kSecClassGenericPassword);
    ::CFDictionarySetValue(query.get(), kSecAttrService, service.get());
    ::CFDictionarySetValue(query.get(), kSecAttrAccount, account.get());
    return query;
  }

  const std::string service_;
};

}  // namespace

std::shared_ptr<SecretStore> CreateKeychainSecretStore(std::string service) {
  return std::make_shared<KeychainSecretStore>(std::move(service));
}

std::shared_ptr<SecretStore> CreatePlatformSecretStore(
    std::shared_ptr<SettingsStore> /*settings_store*/) {
  return CreateKeychainSecretStore(kClientService);
}

#endif  // defined(__APPLE__)
