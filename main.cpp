#include <QCoreApplication>
#include <QCommandLineParser>
#include "emberquery.h"


enum CommandLineParseResult
{
    CommandLineOk,
    CommandLineError,
    CommandLineVersionRequested,
    CommandLineHelpRequested
};

/*
 * b, brief: Briefly Output only Results
 * j, json: Print Output in JSON Format (this sets also the -b --brief Option)
 * q, quiet: Suppress all Error or Log Messages
 * w, write, <Value>: Write the <Value> to specified <Path>
*/

CommandLineParseResult parseCommandLine(QCommandLineParser &parser, EmberQuery *query, QString *errorMessage)
{
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    const QCommandLineOption briefOption(QStringList() << "b" << "brief", "Briefly Output only Results.");
    parser.addOption(briefOption);
    const QCommandLineOption jsonOption(QStringList() << "j" << "json", "Print Output in JSON Format (this sets also the -b --brief Option).");
    parser.addOption(jsonOption);
    const QCommandLineOption quietOption(QStringList() << "q" << "quiet", "Suppress all Error or Log Messages.");
    parser.addOption(quietOption);
    const QCommandLineOption writeOption(QStringList() << "w" << "write", "Write the <value> to specified <path>.", "value");
    parser.addOption(writeOption);
    parser.addPositionalArgument("destination", "EmBer+ Provider Destination as <IP-Address>:<Port>.");
    parser.addPositionalArgument("path", "Start-Path of EmBer-Tree to Print or Edit, if empty we start at root.");
    const QCommandLineOption helpOption = parser.addHelpOption();
    const QCommandLineOption versionOption = parser.addVersionOption();

    if (!parser.parse(QCoreApplication::arguments())) {
        *errorMessage = parser.errorText();
        return CommandLineError;
    }

    if (parser.isSet(versionOption))
        return CommandLineVersionRequested;

    if (parser.isSet(helpOption))
        return CommandLineHelpRequested;

    if (parser.isSet(briefOption))
        query->setBrief(true);

    if (parser.isSet(jsonOption))
        query->setJson(true);

    if (parser.isSet(quietOption))
        query->setQuiet(true);

    if (parser.isSet(writeOption)) {
        if (parser.value(writeOption).isEmpty()) {
            *errorMessage = "Write Option is set, but <value> can not be empty.";
            return CommandLineError;
        } else {
            query->setWriteString(true, parser.value(writeOption));
        }
    }

    QStringList positionalArguments = parser.positionalArguments();
    if (positionalArguments.isEmpty()) {
        *errorMessage = "Argument 'destination' missing.";
        return CommandLineError;
    }
    if(!query->setAddress(positionalArguments.first())){
        *errorMessage = QString("'destination' URL <%1> is invalid.").arg(positionalArguments.first());
        return CommandLineError;
    } else {
        positionalArguments.removeFirst();
    }

    if (positionalArguments.size() > 1) {
        *errorMessage = "Several 'path' arguments specified.";
        return CommandLineError;
    }

    return CommandLineOk;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationName("EmberPlus-Console");
    QCoreApplication::setApplicationVersion("0.1");

    QCommandLineParser parser;
    parser.setApplicationDescription("CLI-Tool to access EmBer+ Provider.");

    EmberQuery *query = new EmberQuery();
    QString error;
    switch (parseCommandLine(parser, query, &error)) {
    case CommandLineOk:
        break;
    case CommandLineError:
        if (!query->isQuiet()) {
            fputs(qPrintable(error), stderr);
            fputs("\n\n", stderr);
            fputs(qPrintable(parser.helpText()), stderr);
        }
        delete query;
        return 1;
    case CommandLineVersionRequested:
        parser.showVersion();
        Q_UNREACHABLE();
    case CommandLineHelpRequested:
        parser.showHelp();
        Q_UNREACHABLE();
    }

    QObject::connect(query, &EmberQuery::quitApp, &a, &QCoreApplication::quit);
    QObject::connect(query, &EmberQuery::error, &a, &QCoreApplication::exit);
    // Start Query after EventLoop is succesfully started
    QTimer::singleShot(0, query, SLOT(start()));

    return a.exec();
}
