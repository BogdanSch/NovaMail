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
    bool ReadHeader(const wstring& filepath, MailboxMeta& meta);
    
public:
    MailClient(const wstring& dir) : directoryPath(dir) {
        CreateDirectoryW(directoryPath.c_str(), NULL);
    }
    bool CreateMailbox(const wstring& name, DWORD maxSize);
    bool AddMessage(const wstring& name, const string& body);
    string ReadMessage(const wstring& name, DWORD targetIndex, bool deleteAfterReading);
    bool DeleteMessage(const wstring& name, DWORD targetIndex);
    bool DeleteAllMessages(const wstring& name); 
    DWORD GetMessageCount(const wstring& name);
    DWORD GetTotalMailboxes();
};