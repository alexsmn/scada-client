scada_register_package(
  NAME scada-client
  REPO client
  COMPONENT Client
  DISPLAY_NAME "Клиент"
  VERSION "${SCADA_PACKAGING_VERSION}"
  DESCRIPTION "Client package"
  INSTALL_DESTINATIONS "bin" "bin/translations"
  RUNTIME_DEPENDENCY_POLICY "repo-local-runtime-deps"
  ROOT_GROUPS product-client
  DEPENDS scada-common)
