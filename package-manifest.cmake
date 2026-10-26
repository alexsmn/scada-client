set(SCADA_PACKAGE_NAME "scada-client")
set(SCADA_PACKAGE_REPO "client")
set(SCADA_PACKAGE_COMPONENT "Client")
set(SCADA_PACKAGE_DISPLAY_NAME "Клиент")
set(SCADA_PACKAGE_DESCRIPTION "Client package")
set(SCADA_PACKAGE_DISTRIBUTION_KIND "end-user")
set(SCADA_PACKAGE_STANDALONE_INSTALLER_DEFAULT ON)
# This package's own standalone MSI UpgradeCode. Never the product MSI's
# (install.cmake) nor another package's: MajorUpgrade removes every installed
# product sharing an UpgradeCode, so a shared one made installing this MSI
# uninstall the others (backlog 929). Constant for the life of the package.
set(SCADA_PACKAGE_WIX_UPGRADE_GUID "5A16172E-7D30-4798-890E-121EA0ED006D")
set(SCADA_PACKAGE_INSTALL_DESTINATIONS "bin" "bin/translations" "examples/schemes")
set(SCADA_PACKAGE_RUNTIME_DEPENDENCY_POLICY "repo-local-runtime-deps")
set(SCADA_PACKAGE_ROOT_GROUPS "product-client")
# scada-vds-runtime was a conditional dependency here until ADR 0012 phase 4:
# the client dlopened the Designer's VDS plugin, so a client install needed it
# beside the executable. The renderer is linked into the client now, so the
# client package carries it and depends on nothing from the Designer.
set(SCADA_PACKAGE_DEPENDS "scada-common")
