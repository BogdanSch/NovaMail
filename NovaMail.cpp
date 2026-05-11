#include <iostream>
#include <string>
#include <vector>
#include "MailClient.h"

using std::wcout, std::wcin, std::cout, std::cin, std::endl, std::string, std::to_string, std::wstring, std::getline, std::vector, std::stoi;

const int MAX_MAILBOX_SIZE = 1024;
const wstring MAILBOX_DIRECTORY = L"nova-mail";

static int tryParseOption() {
	int option = -1;
	string input;
	getline(cin, input);

	try {
		option = stoi(input);
	}
	catch (const std::exception&) {
		option = -1;
	}

	return option;
}

static bool isIndexInRange(int index, int size)
{
	if (index < 0 || index >= size) {
		cout << "Invalid index! Please enter a number between 0 and " << size - 1 << "." << endl;
		return false;
	}
	return true;
}

static void printMessagePreviews(const vector<string>& previews)
{
	if (previews.size() == 0) {
		cout << "The mailbox is empty!" << endl;
		return;
	}
	cout << "Messages in this mailbox:" << endl;
	for (size_t i = 0; i < previews.size(); i++) {
		cout << "[" << i << "]" << ": " << previews[i] << endl;
	}
}

static void printMailboxList(const vector<wstring>& mailboxNames)
{
	if (mailboxNames.size() == 0) {
		cout << "No mailboxes found. Please create a new mailbox to get started!" << endl;
		return;
	}
	cout << "Available mailboxes:" << endl;
	for (const wstring& name : mailboxNames) {
		wcout << L"- " << name << endl;
	}
}

static void displayWelcomeMessage()
{
	cout << "Welcome to Nova Mail!" << endl;
	cout << "Your personal mailbox management system." << endl;
	cout << "Use numbers to choose an option and then press enter.\n" << endl;
	cout << "---------------------------------------" << endl;
}

static void displayMailboxMenu(MailClient& client, const wstring& mailboxName)
{
	cout << "\n---------------------------------------\n" << endl;
	wcout << L"Mailbox '" << mailboxName << L"' opened successfully!";

	while (true) {
		cout << endl;
		vector<string> previews = client.GetMessagePreviews(mailboxName);
		printMessagePreviews(previews);
		cout << endl;

		cout << "Options:\n0: Add a new message.\n1: Read a message.\n2: Read a message and delete it.\n3: Delete a message.\n4: Delete all messages.\n9: Back to main menu.\n" << endl;
		cout << "Would you like to: ";

		int option = tryParseOption();
		int index;

		if (option > 0 && option <= 4 && previews.empty()) {
			cout << "The mailbox is empty!" << endl;
			continue;
		}

		switch (option) {
			case 0: {
				cout << "Enter the message body: ";
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
				cout << "Enter the index of the message to read: ";
				index = tryParseOption();
				if (!isIndexInRange(index, previews.size())) {
					break;
				}

				string message = client.ReadMessage(mailboxName, index, false);
				if (!message.empty()) {
					cout << "Message content:\n" << message << endl;
				}
				else {
					cout << "Failed to read the message!" << endl;
				}

				cout << "\nPress Enter to continue...";
				cin.get();

				break;
			}
			case 2: {
				cout << "Enter the index of the message to read and delete thereafter: ";
				index = tryParseOption();
				if (!isIndexInRange(index, previews.size())) {
					break;
				}

				string message = client.ReadMessage(mailboxName, index, true);
				if (!message.empty()) {
					cout << "Message content:\n" << message << endl;
					cout << "Message deleted successfully!" << endl;
				}
				else {
					cout << "Failed to read and delete the message!" << endl;
				}
				cout << "\nPress Enter to continue...";
				cin.get();
				break;
			}
			case 3: {
				cout << "Enter the index of the message to delete: ";
				index = tryParseOption();
				if (!isIndexInRange(index, previews.size())) {
					break;
				}

				if (client.DeleteMessage(mailboxName, index)) {
					cout << "Message deleted successfully!" << endl;
				}
				else {
					cout << "Failed to delete message!" << endl;
				}
				break;
			}
			case 4: {
				if (client.DeleteAllMessages(mailboxName)) {
					cout << "All messages deleted successfully!" << endl;
				}
				else {
					cout << "Failed to delete all messages!" << endl;
				}
				break;
			}
			case 9: 
				return;
			default: {
				cout << "Invalid option. Please try again." << endl;
				break;
			}
		}
	}
}

static bool displayMainMenu(MailClient& client)
{
	int mailboxesCount = client.GetTotalMailboxesCount();
	vector<wstring> mailboxNames = client.GetAllMailboxNames();

	string message = "\nYou have " + to_string(mailboxesCount) + (mailboxesCount == 1 ? " mailbox" : " mailboxes") + " waiting for you.";
	cout << message << endl;

	printMailboxList(mailboxNames);
	cout << endl;

	message = "Available options: \n0: Create a new mailbox.\n";
	if (mailboxesCount > 0) {
		message += "1: Open an existing mailbox.\n";
	} 
	message += "9: Exit the application.\n";
	
	cout << message;
	cout << "Would you like to: ";

	int option = tryParseOption();
	wstring mailboxName;

	switch (option) {
		case 0: {
			cout << "Enter the name of the new mailbox: ";
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
			if (mailboxesCount == 0) 
				break;

			cout << "Enter the name of the mailbox to open: ";
			getline(wcin, mailboxName);

			if (client.MailboxExists(mailboxName)) {
				displayMailboxMenu(client, mailboxName);
			}
			else {
				wcout << L"Failed to open mailbox '" << mailboxName << L"'!" << endl;
			}
			break;
		}
		case 9: {
			return false;
		}
		default: {
			cout << "Invalid option. Please try again." << endl;
			break;
		}
	}

	return true;
}

int main()
{
	displayWelcomeMessage();
	MailClient client(MAILBOX_DIRECTORY);

	bool isRunning = true;
	while (isRunning) {
		isRunning = displayMainMenu(client);
	}

	cout << "Thank you for using Nova Mail! Take care!" << endl;
}