/*
 * Copyright (C) by Erik Verbruggen <erik@verbruggen.consulting>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#include <QCoreApplication>
#include <QDir>
#include <QFile>

#include <iostream>
#include <vector>

#include <libsync/filesystem.h>

using namespace std;

class Command
{
public:
    Command(const QString &fileName): _fileName(fileName) {}
    virtual ~Command() = 0;

    virtual bool execute(QDir &rootDir) const = 0;

protected:
    static QString parseFileName(QStringListIterator &it)
    {
        return it.next();
    }

    const QString _fileName;
};

Command::~Command() {}

class SetMtimeCommand: public Command
{
public:
    static const QString name;

    SetMtimeCommand(const QString &fileName, qlonglong secs): Command(fileName), _secs(secs){}
    ~SetMtimeCommand() override {}

    bool execute(QDir &rootDir) const override
    {
        cerr << qPrintable(name) << endl;
        return OCC::FileSystem::setModTime(rootDir.filePath(_fileName), _secs);
    }

    static Command *parse(QStringListIterator &it)
    {
        QString fileName = parseFileName(it);
        if (fileName.isEmpty()) {
            cerr << "Error: invalid filename for " << qPrintable(name) << " command" << endl;
            return {};
        }

        QString secsStr = it.next();
        bool ok = false;
        auto secs = secsStr.toLongLong(&ok);
        if (!ok) {
            cerr << "Error: '" << qPrintable(secsStr) << "' is not a valid number (" << qPrintable(name) << ")" << endl;
            return {};
        }

        return new SetMtimeCommand(fileName, secs);
    }

private:
    const qlonglong _secs;
};

const QString SetMtimeCommand::name = QStringLiteral("mtime");

class SetContentsCommand: public Command
{
public:
    static const QString name;

    SetContentsCommand(const QString &fileName, unsigned count, char ch): Command(fileName), _count(count), _ch(ch) {}
    ~SetContentsCommand() override {}

    bool execute(QDir &rootDir) const override
    {
        cerr << qPrintable(name) << endl;
        QFile f(rootDir.filePath(_fileName));
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            cerr << "Error: cannot open file '" << qPrintable(_fileName) << "' for " << qPrintable(name) << " command: "
                 << qPrintable(f.errorString()) << endl;
            return false;
        }
        int count = _count == -1 ? 32 : _count;
        auto written = f.write(QByteArray(count, _ch));
        if (written != count) {
            cerr << "Error: wrote " << written << " bytes to '" << qPrintable(_fileName) << "' instead of requested " << _count << " bytes" << endl;
            return false;
        }
        f.close();
        return true;
    }

    static Command *parse(QStringListIterator &it)
    {
        QString fileName = parseFileName(it);
        if (fileName.isEmpty()) {
            cerr << "Error: invalid filename for " << qPrintable(name) << " command" << endl;
            return {};
        }

        QString countStr = it.next();
        bool ok = false;
        auto count = countStr.toInt(&ok);
        if (!ok) {
            cerr << "Error: '" << qPrintable(countStr) << "' is not a valid number (" << qPrintable(name) << ")" << endl;
            return {};
        }

        QString charStr = it.next();
        if (charStr.size() != 1) {
            cerr << "Error: content for " << qPrintable(name) << " command should be 1 character in size" << endl;
        }

        return new SetContentsCommand(fileName, count, charStr.at(0).toLatin1());
    }

private:
    const int _count;
    const char _ch;
};

const QString SetContentsCommand::name = QStringLiteral("contents");

class RenameCommand: public Command
{
public:
    static const QString name;

    RenameCommand(const QString &fileName, const QString &newName): Command(fileName), _newName(newName) {}
    ~RenameCommand() override {}

    bool execute(QDir &rootDir) const override
    {
        cerr << qPrintable(name) << endl;
        if (!rootDir.exists(_fileName)) {
            cerr << "File does not exist: " << qPrintable(rootDir.absoluteFilePath(_fileName)) << endl;
            return false;
        }

        bool success = rootDir.rename(_fileName, _newName);
        if (!success) {
            cerr << "Rename of " << qPrintable(_fileName) << " failed" << endl;
        }
        return success;
    }

    static Command *parse(QStringListIterator &it)
    {
        QString fileName = parseFileName(it);
        if (fileName.isEmpty()) {
            cerr << "Error: invalid filename for " << qPrintable(name) << " command" << endl;
            return {};
        }

        QString newName = it.next();
        if (newName.isEmpty()) {
            cerr << "Error: invalid new name for " << qPrintable(name) << " command" << endl;
            return {};
        }

        return new RenameCommand(fileName, newName);
    }

private:
    const QString _newName;
};

const QString RenameCommand::name = QStringLiteral("rename");

class AppendByteCommand: public Command
{
public:
    static const QString name;

    AppendByteCommand(const QString &fileName, char ch): Command(fileName), _ch(ch) {}
    ~AppendByteCommand() override {}

    bool execute(QDir &rootDir) const override
    {
        cerr << qPrintable(name) << endl;
        QFile f(rootDir.filePath(_fileName));
        if (!f.open(QIODevice::WriteOnly | QIODevice::Append)) {
            cerr << "Error: cannot open file '" << qPrintable(_fileName) << "' for " << qPrintable(name) << " command: "
                 << qPrintable(f.errorString()) << endl;
            return false;
        }
        char ch = _ch;
        if (ch == '\0') {
            ch = f.readAll().at(0);
        }
        if (!f.seek(f.size())) {
            cerr << "Error: cannot seek to EOF in '" << qPrintable(_fileName) << "' for " << qPrintable(name) << " command";
            return false;
        }
        auto written = f.write(QByteArray(1, ch));
        if (written != 1) {
            cerr << "Error: wrote " << written << " bytes to '" << qPrintable(_fileName) << "' instead of requested 1 bytes" << endl;
            return false;
        }
        f.close();
        return true;
    }

    static Command *parse(QStringListIterator &it)
    {
        QString fileName = parseFileName(it);
        if (fileName.isEmpty()) {
            cerr << "Error: invalid filename for " << qPrintable(name) << " command" << endl;
            return {};
        }

        QString charStr = it.next();
        if (charStr.size() != 1) {
            cerr << "Error: content for " << qPrintable(name) << " command should be 1 character in size" << endl;
        }

        return new AppendByteCommand(fileName, charStr.at(0).toLatin1());
    }

private:
    const char _ch;
};

const QString AppendByteCommand::name = QStringLiteral("appendbyte");

class InsertCommand: public Command
{
public:
    static const QString name;

    InsertCommand(const QString &fileName, int count, char ch): Command(fileName), _count(count), _ch(ch) {}
    ~InsertCommand() override {}

    bool execute(QDir &rootDir) const override
    {
        cerr << qPrintable(name) << endl;
        QFile f(rootDir.filePath(_fileName));
        if (f.exists()) {
            cerr << "Error: file '" << qPrintable(_fileName) << "' for " << qPrintable(name) << " command already exists" << endl;
            return false;
        }
        if (!f.open(QIODevice::WriteOnly)) {
            cerr << "Error: cannot open file '" << qPrintable(_fileName) << "' for " << qPrintable(name) << " command: "
                 << qPrintable(f.errorString()) << endl;
            return false;
        }
        auto written = f.write(QByteArray(_count, _ch));
        if (written != _count) {
            cerr << "Error: wrote " << written << " bytes to '" << qPrintable(_fileName) << "' instead of requested " << _count << " bytes" << endl;
            return false;
        }
        f.close();
        return true;
    }

    static Command *parse(QStringListIterator &it)
    {
        QString fileName = parseFileName(it);
        if (fileName.isEmpty()) {
            cerr << "Error: invalid filename for " << qPrintable(name) << " command" << endl;
            return {};
        }

        QString countStr = it.next();
        bool ok = false;
        auto count = countStr.toInt(&ok);
        if (!ok) {
            cerr << "Error: '" << qPrintable(countStr) << "' is not a valid number (" << qPrintable(name) << ")" << endl;
            return {};
        }

        QString charStr = it.next();
        if (charStr.size() != 1) {
            cerr << "Error: content for " << qPrintable(name) << " command should be 1 character in size" << endl;
        }

        return new InsertCommand(fileName, count, charStr.at(0).toLatin1());
    }

private:
    const int _count;
    const char _ch;
};

const QString InsertCommand::name = QStringLiteral("insert");

class RemoveCommand: public Command
{
public:
    static const QString name;

    RemoveCommand(const QString &fileName): Command(fileName) {}
    ~RemoveCommand() override {}

    bool execute(QDir &rootDir) const override
    {
        cerr << qPrintable(name) << endl;
        QFileInfo fi(rootDir.filePath(_fileName));
        if (fi.isFile()) {
            return rootDir.remove(_fileName);
        } else {
            return QDir(fi.filePath()).removeRecursively();
        }
    }

    static Command *parse(QStringListIterator &it)
    {
        QString fileName = parseFileName(it);
        if (fileName.isEmpty()) {
            cerr << "Error: invalid filename for " << qPrintable(name) << " command" << endl;
            return {};
        }

        return new RemoveCommand(fileName);
    }
};

const QString RemoveCommand::name = QStringLiteral("remove");

vector<Command *> parseArguments(QStringListIterator &it)
{
    vector<Command *> commands;

    while (it.hasNext()) {
        const QString option = it.next();

        if (option == SetMtimeCommand::name) {
            if (auto cmd = SetMtimeCommand::parse(it)) {
                commands.emplace_back(cmd);
            } else {
                return {};
            }
        } else if (option == SetContentsCommand::name) {
            if (auto cmd = SetContentsCommand::parse(it)) {
                commands.emplace_back(cmd);
            } else {
                return {};
            }
        } else if (option == RenameCommand::name) {
            if (auto cmd = RenameCommand::parse(it)) {
                commands.emplace_back(cmd);
            } else {
                return {};
            }
        } else if (option == AppendByteCommand::name) {
            if (auto cmd = AppendByteCommand::parse(it)) {
                commands.emplace_back(cmd);
            } else {
                return {};
            }
        } else if (option == InsertCommand::name) {
            if (auto cmd = InsertCommand::parse(it)) {
                commands.emplace_back(cmd);
            } else {
                return {};
            }
        } else if (option == RemoveCommand::name) {
            if (auto cmd = RemoveCommand::parse(it)) {
                commands.emplace_back(cmd);
            } else {
                return {};
            }
        } else {
            cerr << "Error: unknown command '" << qPrintable(option) << "'" << endl;
        }
    }

    return commands;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QStringListIterator it(app.arguments());
    if (it.hasNext()) {
        // skip program name
        it.next();
    }

    QDir rootDir(it.next());

    const auto commands = parseArguments(it);
    if (commands.empty()) {
        return -1;
    }

    cerr << "Starting executing commands...:" << endl;

    for (const auto &cmd : commands) {
        cerr << ".. Executing command: ";
        if (!cmd->execute(rootDir)) {
            return -2;
        }
        cerr << ".. command done." << endl;
    }

    cerr << "Successfully executed all commands." << endl;

    qDeleteAll(commands);

    return 0;
}
