include(BundleUtilities)

set(BUNDLE_LIBS ${GLD_LIBS} ${GLD_DEPS})
fixup_bundle(${CMAKE_INSTALL_PREFIX}/gridlabd.app ${BUNDLE_LIBS} "")
verify_app(${CMAKE_INSTALL_PREFIX}/gridlabd.app)