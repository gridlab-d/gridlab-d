include(BundleUtilities)

set(BUNDLE_LIBS ${GLD_LIBS} ${GLD_DEPS})
message(STATUS "running fixup")
fixup_bundle(${CMAKE_INSTALL_PREFIX}/bundle/gridlabd.app ${BUNDLE_LIBS} "")
verify_app(${CMAKE_INSTALL_PREFIX}/bundle/gridlabd.app)