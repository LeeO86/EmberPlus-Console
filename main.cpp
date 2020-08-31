#include <QCoreApplication>
#include <QCommandLineParser>
#include "emberquery.h"

#include <iostream>

#ifdef Q_OS_WIN32
#include <fcntl.h>
#include <io.h>
#elif
#include <unistd.h>
#endif


enum CommandLineParseResult
{
    CommandLineOk,
    CommandLineError,
    CommandLineVersionRequested,
    CommandLineHelpRequested
};

QTextStream out(stdout);
QTextStream err(stderr);

QStringList checkStdIn()
{
    QStringList input;

#ifdef Q_OS_WIN32
    _setmode(_fileno(stdin), _O_BINARY);
    if( _isatty(_fileno(stdin))) {
        qDebug() << "stdIn is a Windows Terminal! Not reading Input!";
        return input;
    }
#else
    if (isatty(fileno(stdin))){
        qDebug() << "stdIn is a Terminal! Not reading Input!";
        return input;
    }
#endif

    while(!std::cin.eof()) {
        std::string line;
        std::getline(std::cin, line);
        input.append(QString::fromStdString(line));
    }
    return input;
}

CommandLineParseResult parseCommandLine(QCommandLineParser &parser, EmberQuery *query, QString &errorMessage)
{
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    const QCommandLineOption briefOption(QStringList() << "b" << "brief", "Briefly Output only Results.");
    parser.addOption(briefOption);
//    const QCommandLineOption jsonOption(QStringList() << "j" << "json", "Print Output in JSON Format (this sets also the -b --brief Option).");
//    parser.addOption(jsonOption);
    const QCommandLineOption numberedOption(QStringList() << "n" << "numbered-path", "Use the numbered Ember+ Path instead of the Identifier-Path. The notation is separated with a dot.\n Example: 1.2.3.1/:'value'");
    parser.addOption(numberedOption);
    const QCommandLineOption quietOption(QStringList() << "q" << "quiet", "Suppress all Error or Log Messages.");
    parser.addOption(quietOption);
    const QCommandLineOption timeoutOption(QStringList() << "t" << "timeout", "Time to wait for changed Parameters. The Connection-Timeout is two times <seconds>. If not set it defaults to 1 second.", "seconds");
    parser.addOption(timeoutOption);
    const QCommandLineOption verboseOption(QStringList() << "v" << "verbose", "Prints the verbose Ember+ Output.");
    parser.addOption(verboseOption);
    const QCommandLineOption versionOption(QStringList() << "V" << "version", "Displays version information.");
    parser.addOption(versionOption);
    const QCommandLineOption writeOption(QStringList() << "w" << "write", "Write the <value> to specified <path>. It has to be entered with the path as one String in the follwoing format: \"'path'/:'value'\"\n Example: \"RootNode/ChildNode/Parameter/:YourValue\"");
    parser.addOption(writeOption);
    parser.addPositionalArgument("destination", "EmBer+ Provider Destination as <IP-Address>:<Port>.");
    parser.addPositionalArgument("\"path/:[value]\" ...", "Start-Path of EmBer-Tree to Print or Edit, if not specified we start at root. Ensure that the path is recognised as one string. To be sure encapsulate it with \"double qoutes\". Multiple \"'path'/:'value'\" pairs are possible.");
    const QCommandLineOption helpOption = parser.addHelpOption();

    if (!parser.parse(QCoreApplication::arguments())) {
        errorMessage = parser.errorText();
        return CommandLineError;
    }

    if (parser.isSet(versionOption))
        return CommandLineVersionRequested;

    if (parser.isSet(helpOption))
        return CommandLineHelpRequested;

    if (parser.isSet(briefOption))
        query->setBrief(true);

//    if (parser.isSet(jsonOption))
//        query->setJson(true);

    if (parser.isSet(numberedOption))
        query->setNumberOut(true);

    if (parser.isSet(quietOption))
        query->setQuiet(true);

    if (parser.isSet(timeoutOption)) {
        if (parser.value(timeoutOption).isEmpty()) {
            errorMessage = "Timeout Option is set, but <seconds> can not be empty. If the default TimeOut of 1s should be used, do not use the Timeout-Option.";
            return CommandLineError;
        } else {
            bool ok;
            int time = parser.value(timeoutOption).toInt(&ok);
            if (ok)
                    query->setTimeOut(time);
            else {
                errorMessage = QString("Timeout Option Value '%1' is not a valid number.").arg(parser.value(timeoutOption));
                return CommandLineError;
            }
        }
    }

    if (parser.isSet(verboseOption))
        query->setVerbose(true);

    if (parser.isSet(writeOption))
        query->setWriteString(true);

    QStringList positionalArguments = parser.positionalArguments();
    positionalArguments.append(checkStdIn());
    if (positionalArguments.isEmpty()) {
        errorMessage = "Argument 'destination' missing.";
        return CommandLineError;
    }
    if(!query->setAddress(positionalArguments.first())){
        errorMessage = QString("'destination' URL <%1> is invalid.").arg(positionalArguments.first());
        return CommandLineError;
    } else {
        positionalArguments.removeFirst();
    }

    if (positionalArguments.size() > 1  && query->isJson()) {
        errorMessage = "Several 'path' arguments specified. With the --json option only one <path> Argument is allowed and has to be in JSON-Format.";
        return CommandLineError;
    }
    if(!query->setPaths(positionalArguments, errorMessage)) {
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
    switch (parseCommandLine(parser, query, error)) {
    case CommandLineOk:
        break;
    case CommandLineError:
        if (!query->isQuiet()) {
            err << error << Qt::endl;
            out << "\n" << parser.helpText() << Qt::endl;
            query->setQuiet(true);
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
