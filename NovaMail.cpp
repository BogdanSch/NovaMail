#include <iostream>
#include <string>
#include <vector>
#include "MailClient.h"

using std::wcout, std::wcin, std::cout, std::cin, std::endl, std::string, std::to_string, std::wstring, std::getline, std::vector;

const int MAX_MAILBOX_SIZE = 1024;
const wstring MAILBOX_DIRECTORY = L"nova-mail";

static void displayWelcomeMessage()
{
	cout << "Welcome to Nova Mail!" << endl;
	cout << "Your personal mailbox management system." << endl;
	cout << "Use numbers to choose an option and then press enter.\n" << endl;
	cout << "---------------------------------------\n" << endl;
}

static void displayMailboxMenu(MailClient& client, const wstring& mailboxName)
{
	wcout << "Mailbox: " << mailboxName << endl;
	cout << "Options:\n0: Add a new message.\n1: Read a message.\n2: Delete a message.\n3: Delete all messages.\n4: Back to main menu.\n" << endl;

	int option;
	cin >> option;
	cin.ignore();

	switch(option) {
		case 0: {
			cout << "Enter the message body: " << endl;
			string body;
			getline(cin, body);
			if (client.AddMessage(mailboxName, body)) {
				cout << "Message added successfully!" << endl;
			}
			else {
				cout << "Failed to add the message!" << endl;
			}
			break;
		}
		case 1: {
			cout << "Enter the index of the message to read: " << endl;
			DWORD index;
			cin >> index;
			cin.ignore();
			string message = client.ReadMessage(mailboxName, index, false);
			if (!message.empty()) {
				cout << "Message content:\n" << message << endl;
			}
			else {
				cout << "Failed to read message!" << endl;
			}
			break;
		}
		case 2: {
			cout << "Enter the index of the message to delete: " << endl;
			DWORD index;
			cin >> index;

			if (client.DeleteMessage(mailboxName, index)) {
				cout << "Message deleted successfully!" << endl;
			}
			else {
				cout << "Failed to delete message!" << endl;
			}
			break;
		}
		case 3: {
			if (client.DeleteAllMessages(mailboxName)) {
				cout << "All messages deleted successfully!" << endl;
			}
			else {
				cout << "Failed to delete all messages!" << endl;
			}
			break;
		}
		case 4: {
			displayMainMenu(client);
			break;
		}
		default: {
			cout << "Invalid option. Please try again." << endl;
			break;
		}
	}
}

static void displayMainMenu(MailClient& client)
{
	int mailboxesCount = client.GetTotalMailboxesCount();
	vector<wstring> mailboxNames = client.GetAllMailboxNames();

	string message = "You have " + to_string(mailboxesCount) + (mailboxesCount == 1 ? " mailbox" : " mailboxes") + " waiting for you.";

	cout << message << endl;
	cout << "Would you like to:\n0: Create a new mailbox.\n1: Open an existing mailbox.\n" << endl;

	int option;
	wstring mailboxName;
	cin >> option;
	cin.ignore();

	switch (option) {
		case 0: {
			cout << "Available mailboxes:" << endl;
			for(const wstring& name : mailboxNames) {
				wcout << L"- " << name << endl;
			}
			cout << "\nEnter the name of the new mailbox: " << endl;
			getline(wcin, mailboxName);

			if (client.CreateMailbox(mailboxName, MAX_MAILBOX_SIZE)) {
				wcout << L"Mailbox '" << mailboxName << L"' created successfully!" << endl;
			}
			else {
				wcout << L"Failed to create mailbox '" << mailboxName << L"'!" << endl;
			}
			break;
		}
		case 1: {
			cout << "Enter the name of the mailbox to open: " << endl;
			getline(wcin, mailboxName);

			if (client.MailboxExists(mailboxName)) {
				wcout << L"Mailbox '" << mailboxName << L"' opened successfully!" << endl;
				displayMailboxMenu(client, mailboxName);
			}
			else {
				wcout << L"Failed to open mailbox '" << mailboxName << L"'!" << endl;
			}
			break;
		}
		default: {
			cout << "Invalid option. Please try again." << endl;
			break;
		}
	}
}

int main()
{
	displayWelcomeMessage();

	MailClient client(MAILBOX_DIRECTORY);
	displayMainMenu(client);
}