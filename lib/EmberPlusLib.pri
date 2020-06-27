QT       += core network xml

DEFINES += LIBEMBER_HEADER_ONLY

INCLUDEPATH +=  lib/libformula/Headers \
                lib/libs101/Headers \
                lib/libember/Headers \

SOURCES += \
    lib/gadget/util/EntityPath.cpp \
    lib/gadget/BooleanParameter.cpp \
    lib/gadget/EnumParameter.cpp \
    lib/gadget/IntegerParameter.cpp \
    lib/gadget/Node.cpp \
    lib/gadget/NodeFactory.cpp \
    lib/gadget/Parameter.cpp \
    lib/gadget/ParameterFactory.cpp \
    lib/gadget/RealParameter.cpp \
    lib/gadget/StreamManager.cpp \
    lib/gadget/StringParameter.cpp \
    lib/gadget/Subscriber.cpp \
    lib/glow/util/NodeConverter.cpp \
    lib/glow/util/ParameterConverter.cpp \
    lib/glow/util/StreamConverter.cpp \
    lib/glow/Consumer.cpp \
    lib/glow/ConsumerProxy.cpp \
    lib/glow/ConsumerRequestProcessor.cpp \
    lib/glow/Encoder.cpp \
    lib/net/TcpClient.cpp \
    lib/net/TcpServer.cpp \
    lib/serialization/detail/GadgetTreeReader.cpp \
    lib/serialization/detail/GadgetTreeWriter.cpp \
    lib/serialization/Archive.cpp \
    lib/serialization/SettingsSerializer.cpp \
    lib/util/StreamFormatConverter.cpp

HEADERS += \
    lib/gadget/util/EntityPath.h \
    lib/gadget/util/NumberFactory.h \
    lib/gadget/Access.h \
    lib/gadget/BooleanParameter.h \
    lib/gadget/Collection.h \
    lib/gadget/DirtyState.h \
    lib/gadget/DirtyStateListener.h \
    lib/gadget/EnumParameter.h \
    lib/gadget/Formula.h \
    lib/gadget/IntegerParameter.h \
    lib/gadget/Node.h \
    lib/gadget/NodeFactory.h \
    lib/gadget/NodeField.h \
    lib/gadget/Parameter.h \
    lib/gadget/ParameterFactory.h \
    lib/gadget/ParameterField.h \
    lib/gadget/ParameterType.h \
    lib/gadget/ParameterTypeVisitor.h \
    lib/gadget/RealParameter.h \
    lib/gadget/StreamDescriptor.h \
    lib/gadget/StreamFormat.h \
    lib/gadget/StreamManager.h \
    lib/gadget/StringParameter.h \
    lib/gadget/Subscriber.h \
    lib/glow/util/NodeConverter.h \
    lib/glow/util/ParameterConverter.h \
    lib/glow/util/StreamConverter.h \
    lib/glow/Consumer.h \
    lib/glow/ConsumerProxy.h \
    lib/glow/ConsumerRequestProcessor.h \
    lib/glow/Encoder.h \
    lib/glow/ProviderInterface.h \
    lib/glow/Settings.h \
    lib/net/TcpClient.h \
    lib/net/TcpClientFactory.h \
    lib/net/TcpServer.h \
    lib/serialization/detail/GadgetTreeReader.h \
    lib/serialization/detail/GadgetTreeWriter.h \
    lib/serialization/Archive.h \
    lib/serialization/SettingsSerializer.h \
    lib/util/StreamFormatConverter.h \
    lib/util/StringConverter.h \
    lib/util/Validation.h \
    lib/Types.h
