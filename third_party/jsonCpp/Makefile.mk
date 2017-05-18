noinst_LTLIBRARIES += third_party/jsonCpp/libjsoncpp.la

third_party_jsonCpp_libjsoncpp_la_CPPFLAGS =
third_party_jsonCpp_libjsoncpp_la_CPPFLAGS += -D_PTHREAD
third_party_jsonCpp_libjsoncpp_la_CPPFLAGS += -DAdd_
third_party_jsonCpp_libjsoncpp_la_CPPFLAGS += $(AM_CPPFLAGS)

third_party_jsonCpp_libjsoncpp_la_LDFLAGS =
third_party_jsonCpp_libjsoncpp_la_LDFLAGS += $(AM_LDFLAGS)

third_party_jsonCpp_libjsoncpp_la_SOURCES =
third_party_jsonCpp_libjsoncpp_la_SOURCES += third_party/jsonCpp/jsoncpp.cpp
third_party_jsonCpp_libjsoncpp_la_SOURCES += third_party/jsonCpp/json/json.h
third_party_jsonCpp_libjsoncpp_la_SOURCES += third_party/jsonCpp/json/json-forwards.h

