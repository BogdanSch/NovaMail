#include "MailClient.h"
#include <vector>

using std::vector, std::to_string;

bool MailClient::ReadMailboxMeta(const wstring& filepath, MailboxMeta& meta)
{
    HANDLE hFile = CreateFileW(filepath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) 
        return false;

    DWORD bytesRead;
    bool isSuccessful = ReadFile(hFile, &meta, sizeof(MailboxMeta), &bytesRead, NULL);

    CloseHandle(hFile);
    return isSuccessful && bytesRead == sizeof(MailboxMeta);
}

bool MailClient::WriteMailboxMeta(HANDLE hFile, const MailboxMeta& meta)
{
    DWORD bytesRead;
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    bool isSuccessful = WriteFile(hFile, &meta, sizeof(MailboxMeta), &bytesRead, NULL);

	return isSuccessful && bytesRead == sizeof(MailboxMeta);
}

bool MailClient::MailboxExists(const wstring& name)
{
    wstring filepath = getFilePath(name);
	MailboxMeta meta;

    return ReadMailboxMeta(filepath, meta);
}

bool MailClient::CreateMailbox(const wstring& name, DWORD maxSize)
{
    wstring filepath = getFilePath(name);
    HANDLE hFile = CreateFileW(filepath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) 
        return false;

    MailboxMeta meta = { 0, 0, maxSize };
    DWORD bytesWritten;
    bool isSuccessful = WriteFile(hFile, &meta, sizeof(MailboxMeta), &bytesWritten, NULL);

    CloseHandle(hFile);
    return isSuccessful;
}

bool MailClient::AddMessage(const wstring& mailboxName, const string& body)
{
    wstring filepath = getFilePath(mailboxName);

    HANDLE hFile = CreateFileW(filepath.c_str(), GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) 
        return false;

    MailboxMeta meta;
    DWORD bytesRW;
    ReadFile(hFile, &meta, sizeof(MailboxMeta), &bytesRW, NULL);

    DWORD newTotalSize = meta.totalSize + body.length();
    if (newTotalSize > meta.maxSize) {
        printf("Error: Exceeded the max storage capacity of the mailbox!\n");
        CloseHandle(hFile);
        return false;
    }

    SetFilePointer(hFile, 0, NULL, FILE_END);

    MessageMeta messageMeta = { (DWORD)body.length() };
    WriteFile(hFile, &messageMeta, sizeof(MessageMeta), &bytesRW, NULL);
    WriteFile(hFile, body.c_str(), sizeof(body), &bytesRW, NULL);

    meta.messageCount++;
    meta.totalSize = newTotalSize;
    WriteMailboxMeta(hFile, meta);

    CloseHandle(hFile);
    return true;
}

string MailClient::ReadMessage(const wstring& mailboxName, DWORD targetIndex, bool deleteAfterReading)
{

    wstring filepath = getFilePath(mailboxName);
    HANDLE hFile = CreateFileW(filepath.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return "";

    MailboxMeta meta;
    DWORD bytesRead;
    ReadFile(hFile, &meta, sizeof(MailboxMeta), &bytesRead, NULL);

    if (targetIndex >= meta.messageCount) {
        CloseHandle(hFile);
        return "Error: Message with index=" + to_string(targetIndex) + " doesn't exist.";
    }

    string result = "";
    for (DWORD i = 0; i <= targetIndex; i++) {
        MessageMeta messageMeta;
        ReadFile(hFile, &messageMeta, sizeof(MessageMeta), &bytesRead, NULL);

        if (i == targetIndex) {
			char* buffer = new char[messageMeta.size + 1];
            ReadFile(hFile, buffer, messageMeta.size, &bytesRead, NULL);
			buffer[messageMeta.size] = '\0';
            result = string(buffer);
            break;
        }
        else {
            SetFilePointer(hFile, messageMeta.size, NULL, FILE_CURRENT);
        }
    }
    CloseHandle(hFile);

    if (deleteAfterReading) {
        DeleteMessage(mailboxName, targetIndex);
    }

    return result;
}

bool MailClient::DeleteMessage(const wstring& mailboxName, DWORD targetIndex)
{
	if (targetIndex < 0) {
        printf("Error: Invalid message index=%d provided for deletion!\n", targetIndex);
        return false;
    }
    wstring filepath = getFilePath(mailboxName);

    HANDLE hFile = CreateFileW(filepath.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) 
        return false;

    MailboxMeta meta;
    DWORD bytesRW;
    ReadFile(hFile, &meta, sizeof(MailboxMeta), &bytesRW, NULL);

    if (targetIndex >= meta.messageCount) {
        printf("Error: Invalid message index=%d provided for deletion!\n", targetIndex);
        CloseHandle(hFile);
        return false;
    }

    vector<string> allMessages;
    for (DWORD i = 0; i < meta.messageCount; i++) {
        MessageMeta messageMeta;
        ReadFile(hFile, &messageMeta, sizeof(MessageMeta), &bytesRW, NULL);

		char* buffer = new char[messageMeta.size + 1];
        ReadFile(hFile, buffer, messageMeta.size, &bytesRW, NULL);
		buffer[messageMeta.size] = '\0';

        if (i != targetIndex) {
            allMessages.push_back(string(buffer));
        }
		delete[] buffer;
    }
    CloseHandle(hFile);

	meta.messageCount = 0;
    meta.totalSize = 0;
    WriteMailboxMeta(hFile, meta);
    SetEndOfFile(hFile);

    for (const string& message : allMessages) {
        AddMessage(mailboxName, message);
    }

    return true;
}

bool MailClient::DeleteAllMessages(const wstring& mailboxName)
{
    wstring filepath = getFilePath(mailboxName);

    MailboxMeta meta;
    if (!ReadMailboxMeta(filepath, meta))
        return false;

    return CreateMailbox(mailboxName, meta.maxSize);
}

DWORD MailClient::GetMessageCount(const wstring& mailboxName)
{
    wstring filepath = getFilePath(mailboxName);

    MailboxMeta meta;
    if (ReadMailboxMeta(filepath, meta)) {
        return meta.messageCount;
    }
    return 0;
}

DWORD MailClient::GetTotalMailboxesCount()
{
    WIN32_FIND_DATAW findFileData;
    wstring searchPath = directoryPath + L"\\*." + FILE_EXTENSION;

    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findFileData);
	if (hFind == INVALID_HANDLE_VALUE) {
        printf("Error: Failed to access the directory for counting mailboxes!\n");
        return 0;
    }

    DWORD count = 0;
    do {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            count++;
        }
    } while (FindNextFileW(hFind, &findFileData) != false);

    FindClose(hFind);
    return count;
}


vector<wstring> MailClient::GetAllMailboxNames()
{
    WIN32_FIND_DATAW findFileData;
    wstring searchPath = directoryPath + L"\\*." + FILE_EXTENSION;

    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        printf("Error: Failed to access the directory for counting mailboxes!\n");
        return {};
    }

    vector<wstring> mailboxNames = {};
    do {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			mailboxNames.push_back(wstring(findFileData.cFileName));
        }
    } while (FindNextFileW(hFind, &findFileData) != false);

    FindClose(hFind);
    return mailboxNames;
}