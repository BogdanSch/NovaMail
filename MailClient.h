#pragma once
#include <windows.h>
#include <string>

using std::wstring, std::string;

struct MailboxMeta {
    DWORD messageCount;
    DWORD totalSize;
    DWORD maxSize;
};

struct MessageMeta {
    DWORD size;
};

class MailClient
{
private:
    wstring directoryPath;
    bool ReadMailboxMeta(const wstring& filepath, MailboxMeta& meta);
    bool WriteMailboxMeta(HANDLE hFile, const MailboxMeta& meta);
    wstring getFilePath(const wstring& mailboxName) { return directoryPath + L"\\" + mailboxName + FILE_EXTENSION; }
public:
	const wstring FILE_EXTENSION = L"dat";
    MailClient(const wstring& dir) : directoryPath(dir) { CreateDirectoryW(directoryPath.c_str(), NULL); }
    bool MailboxExists(const wstring& name);
    bool CreateMailbox(const wstring& name, DWORD maxSize);
    bool AddMessage(const wstring& mailboxName, const string& body);
    string ReadMessage(const wstring& mailboxName, DWORD targetIndex, bool deleteAfterReading);
    bool DeleteMessage(const wstring& mailboxName, DWORD targetIndex);
    bool DeleteAllMessages(const wstring& mailboxName);
    DWORD GetMessageCount(const wstring& mailboxName);
    DWORD GetTotalMailboxesCount(); 
    vector<wstring> GetAllMailboxNames();
};