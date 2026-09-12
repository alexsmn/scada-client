set(SCADA_PACKAGE_NAME "scada-client")
set(SCADA_PACKAGE_REPO "client")
set(SCADA_PACKAGE_COMPONENT "Client")
set(SCADA_PACKAGE_DISPLAY_NAME "Клиент")
set(SCADA_PACKAGE_DESCRIPTION "Client package")
set(SCADA_PACKAGE_DISTRIBUTION_KIND "end-user")
set(SCADA_PACKAGE_STANDALONE_INSTALLER_DEFAULT ON)
set(SCADA_PACKAGE_INSTALL_DESTINATIONS "bin" "bin/translations" "examples/schemes")
set(SCADA_PACKAGE_RUNTIME_DEPENDENCY_POLICY "repo-local-runtime-deps")
set(SCADA_PACKAGE_ROOT_GROUPS "product-client")
# scada-vds-runtime was a conditional dependency here until ADR 0012 phase 4:
# the client dlopened the Designer's VDS plugin, so a client install needed it
# beside the executable. The renderer is linked into the client now, so the
# client package carries it and depends on nothing from the Designer.
set(SCADA_PACKAGE_DEPENDS "scada-common")
