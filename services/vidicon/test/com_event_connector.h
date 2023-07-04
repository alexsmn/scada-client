#pragma once

#include <wrl/client.h>

class ComEventConnector {
 public:
  ComEventConnector() = default;

  ComEventConnector(IUnknown& data_point, IUnknown& event_sink) {
    Connect(data_point, event_sink);
  }

  ~ComEventConnector() { Disconnect(); }

  bool connected() const noexcept { return connected_; }

  HRESULT Connect(IUnknown& data_point, IUnknown& event_sink) {
    if (connected_) {
      return E_FAIL;
    }

    Microsoft::WRL::ComPtr<IConnectionPointContainer>
        connection_point_container;
    auto hr = data_point.QueryInterface(
        connection_point_container.ReleaseAndGetAddressOf());
    if (FAILED(hr)) {
      return hr;
    }

    Microsoft::WRL::ComPtr<IConnectionPoint> connection_point;
    hr = connection_point_container->FindConnectionPoint(
        __uuidof(_IDataPointEvents3),
        connection_point.ReleaseAndGetAddressOf());
    if (FAILED(hr)) {
      return hr;
    }

    hr = connection_point->Advise(&event_sink, &advise_cookie_);
    if (FAILED(hr)) {
      return hr;
    }

    connection_point_ = std::move(connection_point);
    connected_ = true;
    return S_OK;
  }

  HRESULT Disconnect() {
    if (!connected_) {
      return S_FALSE;
    }

    auto hr = connection_point_->Unadvise(advise_cookie_);
    if (FAILED(hr)) {
      return hr;
    }

    connection_point_.Reset();
    connected_ = false;
    return S_OK;
  }

  ComEventConnector(const ComEventConnector&) = delete;
  ComEventConnector& operator=(const ComEventConnector&) = delete;

 private:
  Microsoft::WRL::ComPtr<IConnectionPoint> connection_point_;
  DWORD advise_cookie_ = 0;
  bool connected_ = false;
};
