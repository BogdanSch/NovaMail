#include "MailClient.h"
#include <vector>

using std::vector;

bool MailClient::ReadHeader(const wstring& filepath, MailboxMeta& meta)
{
    HANDLE hFile = CreateFileW(filepath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) 
        return false;

    DWORD bytesRead;
    if(!ReadFile(hFile, &meta, sizeof(MailboxMeta), &bytesRead, NULL)) 
        return false;

    CloseHandle(hFile);
    return bytesRead == sizeof(MailboxMeta);
}

bool MailClient::CreateMailbox(const wstring& name, DWORD maxSize)
{
    wstring filepath = directoryPath + L"\\" + name + L".mbx";
    HANDLE hFile = CreateFileW(filepath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) 
        return false;

    MailboxMeta header = { 0, 0, maxSize };
    DWORD bytesWritten;
    WriteFile(hFile, &header, sizeof(MailboxMeta), &bytesWritten, NULL);
    CloseHandle(hFile);
    return true;
}

bool MailClient::AddMessage(const wstring& name, const string& body)
{
    wstring filepath = directoryPath + L"\\" + name + L".mbx";

    // Відкриваємо файл для читання і запису
    HANDLE hFile = CreateFileW(filepath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) 
        return false;

    MailboxMeta header;
    DWORD bytesRW;
    ReadFile(hFile, &header, sizeof(MailboxMeta), &bytesRW, NULL);

    // Перевірка на перевищення максимального розміру
    DWORD newTotalSize = header.totalSize + body.length();
    if (newTotalSize > header.maxSize) {
        printf("Error: Exceeded the max storage capacity of the mailbox!\n");
        CloseHandle(hFile);
        return false;
    }

    // Переміщуємо вказівник у кінець файлу, щоб записати нове повідомлення
    SetFilePointer(hFile, 0, NULL, FILE_END);

    MessageMeta msgHeader = { (DWORD)body.length() };
    WriteFile(hFile, &msgHeader, sizeof(MessageMeta), &bytesRW, NULL);
    WriteFile(hFile, body.c_str(), msgHeader.size, &bytesRW, NULL);

    // Повертаємося на початок і оновлюємо заголовок
    header.messageCount++;
    header.totalSize = newTotalSize;
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    WriteFile(hFile, &header, sizeof(MailboxMeta), &bytesRW, NULL);

    CloseHandle(hFile);
    return true;
}

string MailClient::ReadMessage(const wstring& name, DWORD targetIndex, bool deleteAfterReading)
{
    wstring filepath = directoryPath + L"\\" + name + L".mbx";
    HANDLE hFile = CreateFileW(filepath.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return "";

    MailboxMeta header;
    DWORD bytesRead;
    ReadFile(hFile, &header, sizeof(MailboxMeta), &bytesRead, NULL);

    if (targetIndex >= header.messageCount) {
        CloseHandle(hFile);
        return "Помилка: Листа з таким індексом не існує.";
    }

    string result = "";
    for (DWORD i = 0; i <= targetIndex; i++) {
        MessageMeta msgHeader;
        ReadFile(hFile, &msgHeader, sizeof(MessageMeta), &bytesRead, NULL);

        if (i == targetIndex) {
            // Ми знайшли потрібний лист, читаємо тіло
            vector<char> buffer(msgHeader.size + 1, '\0');
            ReadFile(hFile, buffer.data(), msgHeader.size, &bytesRead, NULL);
            result = string(buffer.data());
            break;
        }
        else {
            // Пропускаємо тіло непотрібного листа (зміщуємо вказівник)
            SetFilePointer(hFile, msgHeader.size, NULL, FILE_CURRENT);
        }
    }
    CloseHandle(hFile);

    if (deleteAfterReading) {
        DeleteMessage(name, targetIndex);
    }

    return result;
}

bool MailClient::DeleteMessage(const wstring& name, DWORD targetIndex)
{
    wstring filepath = directoryPath + L"\\" + name + L".mbx";

    // Крок 1: Зчитати всі повідомлення в пам'ять
    HANDLE hFile = CreateFileW(filepath.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    MailboxMeta header;
    DWORD bytesRW;
    ReadFile(hFile, &header, sizeof(MailboxMeta), &bytesRW, NULL);

    if (targetIndex >= header.messageCount) {
        CloseHandle(hFile);
        return false;
    }

    vector<string> allMessages;
    for (DWORD i = 0; i < header.messageCount; i++) {
        MessageMeta msgHeader;
        ReadFile(hFile, &msgHeader, sizeof(MessageMeta), &bytesRW, NULL);

        vector<char> buffer(msgHeader.size + 1, '\0');
        ReadFile(hFile, buffer.data(), msgHeader.size, &bytesRW, NULL);

        if (i != targetIndex) {
            allMessages.push_back(string(buffer.data()));
        }
    }
    CloseHandle(hFile);

    // Крок 2: Перестворити скриньку зі старим maxSize і записати повідомлення назад
    CreateMailbox(name, header.maxSize);
    for (const string& msg : allMessages) {
        AddMessage(name, msg);
    }

    return true;
}

bool MailClient::DeleteAllMessages(const wstring& name)
{
    MailboxMeta header;
    wstring filepath = directoryPath + L"\\" + name + L".mbx";
    if (!ReadHeader(filepath, header)) return false;

    return CreateMailbox(name, header.maxSize);
}

DWORD MailClient::GetMessageCount(const wstring& name)
{
    MailboxMeta header;
    wstring filepath = directoryPath + L"\\" + name + L".mbx";
    if (ReadHeader(filepath, header)) {
        return header.messageCount;
    }
    return 0;
}

DWORD MailClient::GetTotalMailboxes()
{
    DWORD count = 0;
    WIN32_FIND_DATAW findFileData;
    wstring searchPath = directoryPath + L"\\*.mbx";

    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findFileData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                count++;
            }
        } while (FindNextFileW(hFind, &findFileData) != 0);
        FindClose(hFind);
    }
    return count;
}
