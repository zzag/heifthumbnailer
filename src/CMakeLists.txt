set(heifthumbnail_SRCS heifcreator.cpp)
add_library(heifthumbnail MODULE ${heifthumbnail_SRCS})
target_link_libraries(heifthumbnail Qt5::Gui KF5::KIOWidgets libheif::libheif)

install(TARGETS heifthumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES heifthumbnail.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
install(FILES heif.xml DESTINATION ${KDE_INSTALL_MIMEDIR})
