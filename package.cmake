scada_register_package(
  NAME scada-client
  REPO client
  COMPONENT Client
  DISPLAY_NAME "Клиент"
  VERSION "${SCADA_PACKAGING_VERSION}"
  DESCRIPTION "Client package"
  DISTRIBUTION_KIND "end-user"
  STANDALONE_INSTALLER_DEFAULT ON
  INSTALL_DESTINATIONS "bin" "bin/translations" "examples/schemes"
  RUNTIME_DEPENDENCY_POLICY "repo-local-runtime-deps"
  ROOT_GROUPS product-client
  DEPENDS scada-common)
